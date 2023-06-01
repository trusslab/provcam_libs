/******************************************************************************
*
* Copyright (C) 2008-2020 Allegro DVT2.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX OR ALLEGRO DVT2 BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of  Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
*
* Except as contained in this notice, the name of Allegro DVT2 shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Allegro DVT2.
*
******************************************************************************/

/****************************************************************************
   -----------------------------------------------------------------------------
 **************************************************************************//*!
   \addtogroup lib_decode_hls
   @{
   \file
 *****************************************************************************/

#include <string.h>
#include <stdio.h>

#include "DefaultDecoder.h"
#include "NalUnitParser.h"
#include "UnsplitBufferFeeder.h"
#include "SplitBufferFeeder.h"

#include "lib_common/Error.h"
#include "lib_common/StreamBuffer.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferStreamMeta.h"
#include "lib_common/BufferHandleMeta.h"
#include "lib_common/BufferAPIInternal.h"
#include "lib_common/SEI.h"

#include "lib_common/AvcLevelsLimit.h"
#include "lib_common/AvcUtils.h"
#include "lib_common/HevcLevelsLimit.h"
#include "lib_common/HevcUtils.h"

#include "lib_parsing/I_PictMngr.h"
#include "lib_decode/I_DecScheduler.h"
#include "lib_common_dec/DecHook.h"

#include "lib_assert/al_assert.h"

#define TraceRecordEnd(pTraceHooks, unit) \
  do { \
    if(pTraceHooks && pTraceHooks->RecordEnd) \
      pTraceHooks->RecordEnd(pTraceHooks->pUserParam, unit); \
  } while(0) \

#define TraceRecordStart(pTraceHooks, unit) \
  do { \
    if(pTraceHooks && pTraceHooks->RecordStart) \
      pTraceHooks->RecordStart(pTraceHooks->pUserParam, unit); \
  } while(0) \


static int const AVC_NAL_HDR_SIZE = 4;
static int const HEVC_NAL_HDR_SIZE = 5;

/*****************************************************************************/
static bool isAVC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_AVC;
}

/*****************************************************************************/
static bool isHEVC(AL_ECodec eCodec)
{
  return eCodec == AL_CODEC_HEVC;
}

/******************************************************************************/
static bool IsAtLeastOneStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) || (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsAllStreamDimSet(AL_TDimension tDim)
{
  return (tDim.iWidth > 0) && (tDim.iHeight > 0);
}

/******************************************************************************/
static bool IsStreamChromaSet(AL_EChromaMode eChroma)
{
  return eChroma != AL_CHROMA_MAX_ENUM;
}

/******************************************************************************/
static bool IsStreamBitDepthSet(int iBitDepth)
{
  return iBitDepth > 0;
}

/******************************************************************************/
static bool IsStreamLevelSet(int iLevel)
{
  return iLevel > 0;
}

/******************************************************************************/
static bool IsStreamProfileSet(int iProfileIdc)
{
  return iProfileIdc > 0;
}

static bool IsStreamSequenceModeSet(AL_ESequenceMode eSequenceMode)
{
  return eSequenceMode != AL_SM_MAX_ENUM;
}

/******************************************************************************/
static bool IsAllStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAllStreamDimSet(tStreamSettings.tDim) && IsStreamChromaSet(tStreamSettings.eChroma) && IsStreamBitDepthSet(tStreamSettings.iBitDepth) && IsStreamLevelSet(tStreamSettings.iLevel) && IsStreamProfileSet(tStreamSettings.iProfileIdc) && IsStreamSequenceModeSet(tStreamSettings.eSequenceMode);
}

/*****************************************************************************/
static bool IsAtLeastOneStreamSettingsSet(AL_TStreamSettings tStreamSettings)
{
  return IsAtLeastOneStreamDimSet(tStreamSettings.tDim) || IsStreamChromaSet(tStreamSettings.eChroma) || IsStreamBitDepthSet(tStreamSettings.iBitDepth) || IsStreamLevelSet(tStreamSettings.iLevel) || IsStreamProfileSet(tStreamSettings.iProfileIdc) || IsStreamSequenceModeSet(tStreamSettings.eSequenceMode);
}

/* Buffer size must be aligned with hardware requests, which are 2048 or 4096 bytes for dec1 units (for old or new decoder respectively). */
static int const bitstreamRequestSize = 4096;

/*****************************************************************************/
static int GetCircularBufferSize(AL_ECodec eCodec, int iStack, AL_TStreamSettings tStreamSettings)
{
  int const zMaxCPBSize = (isAVC(eCodec) ? 120 : 50) * 1024 * 1024; /* CPB default worst case */
  int const zWorstCaseNalSize = 2 << (isAVC(eCodec) ? 24 : 23); /* Single frame worst case */

  int circularBufferSize = 0;

  if(IsAllStreamSettingsSet(tStreamSettings))
  {
    /* Circular buffer always should be able to hold one frame, therefore compute the worst case and use it as a lower bound.  */
    int const zMaxNalSize = AL_GetMaxNalSize(eCodec, tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth, tStreamSettings.iLevel, tStreamSettings.iProfileIdc); /* Worst case: (5/3)*PCM + Worst case slice Headers */
    int const zRealworstcaseNalSize = AL_GetMitigatedMaxNalSize(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth); /* Reasonnable: PCM + Slice Headers */
    circularBufferSize = UnsignedMax(zMaxNalSize, iStack * zRealworstcaseNalSize);
  }
  else
    circularBufferSize = zWorstCaseNalSize * iStack;

  /* Get minimum between absolute CPB worst case and computed value. */
  int const bufferSize = UnsignedMin(circularBufferSize, zMaxCPBSize);

  /* Round up for hardware. */
  return RoundUp(bufferSize, bitstreamRequestSize);
}

/*****************************************************************************/
static AL_TDecCtx* AL_sGetContext(AL_TDecoder* pDec)
{
  return &(pDec->ctx);
}

/*****************************************************************************/
bool AL_Decoder_Alloc(AL_TDecCtx* pCtx, TMemDesc* pMD, uint32_t uSize, char const* name)
{
  if(!MemDesc_AllocNamed(pMD, pCtx->pAllocator, uSize, name))
  {
    AL_Default_Decoder_SetError(pCtx, AL_ERR_NO_MEMORY, -1);
    return false;
  }

  return true;
}

/*****************************************************************************/
static void AL_Decoder_Free(TMemDesc* pMD)
{
  MemDesc_Free(pMD);
}

/*****************************************************************************/
static bool isSubframeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit == AL_VCL_NAL_UNIT;
}

/*****************************************************************************/
static void AL_sDecoder_CallEndParsing(AL_TDecCtx* pCtx, AL_TBuffer* pParsedFrame)
{
  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pParsedFrame, AL_META_TYPE_HANDLE);

  if(!pHandlesMeta)
  {
    if(pCtx->parseCB.func)
      pCtx->parseCB.func(pParsedFrame, pCtx->parseCB.userParam);

    return;
  }

  int numHandles = AL_HandleMetaData_GetNumHandles(pHandlesMeta);

  for(int handle = 0; handle < numHandles; handle++)
  {
    AL_TDecMetaHandle* pDecMetaHandle = (AL_TDecMetaHandle*)AL_HandleMetaData_GetHandle(pHandlesMeta, handle);

    if(pDecMetaHandle->eState == AL_DEC_HANDLE_STATE_PROCESSING)
    {
      AL_TBuffer* pStream = pDecMetaHandle->pHandle;
      pDecMetaHandle->eState = AL_DEC_HANDLE_STATE_PROCESSED;

      if(pCtx->parseCB.func)
        pCtx->parseCB.func(pParsedFrame, pCtx->parseCB.userParam);
      AL_Feeder_FreeBuf(pCtx->Feeder, pStream);

      if(isSubframeUnit(pCtx->pChanParam->eDecUnit))
        return;
    }
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallDecode(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
  AL_Assert(pDecodedFrame);

  AL_THandleMetaData* pHandlesMeta = (AL_THandleMetaData*)AL_Buffer_GetMetaData(pDecodedFrame, AL_META_TYPE_HANDLE);

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    AL_sDecoder_CallEndParsing(pCtx, pDecodedFrame);

  if(!pCtx->pChanParam->bUseEarlyCallback)
    pCtx->decodeCB.func(pDecodedFrame, pCtx->decodeCB.userParam);

  if(!pHandlesMeta)
    return;

  AL_Buffer_RemoveMetaData(pDecodedFrame, (AL_TMetaData*)pHandlesMeta);
  AL_MetaData_Destroy((AL_TMetaData*)pHandlesMeta);
}

/*****************************************************************************/
static void AL_sDecoder_CallDisplay(AL_TDecCtx* pCtx)
{
  while(1)
  {
    AL_TInfoDecode pInfo = { 0 };
    AL_TBuffer* pFrameToDisplay = AL_PictMngr_GetDisplayBuffer(&pCtx->PictMngr, &pInfo);

    if(!pFrameToDisplay)
      break;

    AL_Assert(AL_Buffer_GetData(pFrameToDisplay));

    if(!pCtx->pChanParam->bUseEarlyCallback)
      pCtx->displayCB.func(pFrameToDisplay, &pInfo, pCtx->displayCB.userParam);
    AL_PictMngr_SignalCallbackDisplayIsDone(&pCtx->PictMngr);
  }
}

/*****************************************************************************/
static void AL_sDecoder_CallBacks(AL_TDecCtx* pCtx, int iFrameID)
{
  AL_sDecoder_CallDecode(pCtx, iFrameID);
  AL_sDecoder_CallDisplay(pCtx);
}

/*****************************************************************************/
void AL_Default_Decoder_EndParsing(void* pUserParam, int iFrameID)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;

  if(!isSubframeUnit(pCtx->pChanParam->eDecUnit))
    return;

  AL_TBuffer* pParsedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
  AL_Assert(pParsedFrame);

  AL_sDecoder_CallEndParsing(pCtx, pParsedFrame);
}

/*****************************************************************************/
void AL_Default_Decoder_EndDecoding(void* pUserParam, AL_TDecPicStatus* pStatus)
{
  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_CMD_INVALID))
  {
    printf("\n***** /!\\ Error trying to conceal bitstream - ending decoding /!\\ *****\n");
    AL_Assert(0);
  }

  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  int iFrameID = pStatus->tBufIDs.FrmID;

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_NOT_FINISHED))
  {
    /* we want to notify the user, but we don't want to update the decoder state */
    AL_TBuffer* pDecodedFrame = AL_PictMngr_GetDisplayBufferFromID(&pCtx->PictMngr, iFrameID);
    AL_Assert(pDecodedFrame);
    pCtx->decodeCB.func(pDecodedFrame, pCtx->decodeCB.userParam);
    AL_TInfoDecode info = { 0 };
    AL_TBuffer* pFrameToDisplay = AL_PictMngr_ForceDisplayBuffer(&pCtx->PictMngr, &info, iFrameID);
    AL_Assert(pFrameToDisplay);
    pCtx->displayCB.func(pFrameToDisplay, &info, pCtx->displayCB.userParam);
    AL_PictMngr_SignalCallbackDisplayIsDone(&pCtx->PictMngr);
    return;
  }

  AL_PictMngr_UpdateDisplayBufferCRC(&pCtx->PictMngr, iFrameID, pStatus->uCRC);
  AL_PictMngr_EndDecoding(&pCtx->PictMngr, iFrameID);
  int iOffset = pCtx->iNumFrmBlk2 % MAX_STACK_SIZE;
  AL_PictMngr_UnlockRefID(&pCtx->PictMngr, pCtx->uNumRef[iOffset], pCtx->uFrameIDRefList[iOffset], pCtx->uMvIDRefList[iOffset]);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = pCtx->iStreamOffset[pCtx->iNumFrmBlk2 % pCtx->iStackSize];
  ++pCtx->iNumFrmBlk2;

  if(AL_DEC_IS_PIC_STATE_ENABLED(pStatus->tDecPicState, AL_DEC_PIC_STATE_HANGED))
    printf("\n***** /!\\ Timeout - resetting the decoder /!\\ *****\n");

  Rtos_ReleaseMutex(pCtx->DecMutex);

  AL_Feeder_Signal(pCtx->Feeder);
  AL_sDecoder_CallBacks(pCtx, iFrameID);

  Rtos_GetMutex(pCtx->DecMutex);
  int iMotionVectorID = pStatus->tBufIDs.MvID;
  AL_PictMngr_UnlockID(&pCtx->PictMngr, iFrameID, iMotionVectorID);
  Rtos_ReleaseMutex(pCtx->DecMutex);

  Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*************************************************************************//*!
   \brief This function performs DPB operations after frames decoding
   \param[in] pUserParam filled with the decoder context
   \param[in] pStatus Current start code searching status
*****************************************************************************/
static void AL_Decoder_EndScd(void* pUserParam, AL_TScStatus* pStatus)
{
  AL_TDecCtx* pCtx = (AL_TDecCtx*)pUserParam;
  pCtx->ScdStatus.uNumBytes = pStatus->uNumBytes;
  pCtx->ScdStatus.uNumSC = pStatus->uNumSC;

  Rtos_SetEvent(pCtx->ScDetectionComplete);
}

/***************************************************************************/
/*                           Lib functions                                 */
/***************************************************************************/

/*****************************************************************************/
static void InitInternalBuffers(AL_TDecCtx* pCtx)
{
  MemDesc_Init(&pCtx->BufNoAE.tMD);
  MemDesc_Init(&pCtx->BufSCD.tMD);
  MemDesc_Init(&pCtx->SCTable.tMD);

  for(int i = 0; i < MAX_STACK_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PoolSclLst[i].tMD);
    MemDesc_Init(&pCtx->PoolCompData[i].tMD);
    MemDesc_Init(&pCtx->PoolCompMap[i].tMD);
    MemDesc_Init(&pCtx->PoolSP[i].tMD);
    MemDesc_Init(&pCtx->PoolWP[i].tMD);
    MemDesc_Init(&pCtx->PoolListRefAddr[i].tMD);
  }

  for(int i = 0; i < MAX_DPB_SIZE; ++i)
  {
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
    MemDesc_Init(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
  }

  MemDesc_Init(&pCtx->tMDChanParam);
  pCtx->pChanParam = NULL;
}

/*****************************************************************************/
static void DeinitBuffers(AL_TDecCtx* pCtx)
{
  for(int i = 0; i < MAX_DPB_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD);
    AL_Decoder_Free(&pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD);
  }

  for(int i = 0; i < MAX_STACK_SIZE; i++)
  {
    AL_Decoder_Free(&pCtx->PoolCompData[i].tMD);
    AL_Decoder_Free(&pCtx->PoolCompMap[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolWP[i].tMD);
    AL_Decoder_Free(&pCtx->PoolListRefAddr[i].tMD);
    AL_Decoder_Free(&pCtx->PoolSclLst[i].tMD);
  }

  AL_Decoder_Free(&pCtx->BufSCD.tMD);
  AL_Decoder_Free(&pCtx->SCTable.tMD);
  AL_Decoder_Free(&pCtx->BufNoAE.tMD);
  AL_Decoder_Free(&pCtx->tMDChanParam);
}

/*****************************************************************************/
static void ReleaseFramePictureUnused(AL_TDecCtx* pCtx)
{
  while(true)
  {
    AL_TBuffer* pFrameToRelease = AL_PictMngr_GetUnusedDisplayBuffer(&pCtx->PictMngr);

    if(!pFrameToRelease)
      break;

    AL_Assert(AL_Buffer_GetData(pFrameToRelease));

    pCtx->displayCB.func(pFrameToRelease, NULL, pCtx->displayCB.userParam);
    AL_PictMngr_SignalCallbackReleaseIsDone(&pCtx->PictMngr, pFrameToRelease);
  }
}

/*****************************************************************************/
static void DeinitPictureManager(AL_TDecCtx* pCtx)
{
  ReleaseFramePictureUnused(pCtx);
  AL_PictMngr_Deinit(&pCtx->PictMngr);
}

/*****************************************************************************/
void AL_Default_Decoder_Destroy(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_Assert(pCtx);

  AL_PictMngr_DecommitPool(&pCtx->PictMngr);

  if(pCtx->Feeder)
    AL_Feeder_Destroy(pCtx->Feeder);

  if(pCtx->eosBuffer)
    AL_Buffer_Unref(pCtx->eosBuffer);

  AL_IDecScheduler_DestroyStartCodeChannel(pCtx->pScheduler, pCtx->hStartCodeChannel);
  AL_IDecScheduler_DestroyChannel(pCtx->pScheduler, pCtx->hChannel);
  DeinitPictureManager(pCtx);
  Rtos_Free(pCtx->BufNoAE.tMD.pVirtualAddr);
  DeinitBuffers(pCtx);

  Rtos_DeleteSemaphore(pCtx->Sem);
  Rtos_DeleteEvent(pCtx->ScDetectionComplete);
  Rtos_DeleteMutex(pCtx->DecMutex);

  Rtos_Free(pDec);
}

/*****************************************************************************/
void AL_Default_Decoder_SetParam(AL_TDecoder* pAbsDec, bool bConceal, bool bUseBoard, int iFrmID, int iNumFrm, bool bForceCleanBuffers, bool shouldPrintFrameDelimiter)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  pCtx->bConceal = bConceal;
  pCtx->bUseBoard = bUseBoard;

  pCtx->iTracePreviousFrmNum = -1;
  pCtx->iTraceFirstFrame = iFrmID;
  pCtx->iTraceLastFrame = iFrmID + iNumFrm;
  pCtx->shouldPrintFrameDelimiter = shouldPrintFrameDelimiter;

  if(iNumFrm > 0 || bForceCleanBuffers)
    AL_CLEAN_BUFFERS = 1;
}

/*****************************************************************************/
static bool isAud(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_AUD;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_AUD;
  return false;
}

/*****************************************************************************/
static bool isEosOrEob(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return (eNut == AL_AVC_NUT_EOS) || (eNut == AL_AVC_NUT_EOB);

  if(isHEVC(eCodec))
    return (eNut == AL_HEVC_NUT_EOS) || (eNut == AL_HEVC_NUT_EOB);
  return false;
}

/*****************************************************************************/
static bool isFd(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_FD;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_FD;
  return false;
}

/*****************************************************************************/
static bool isPrefixSei(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_PREFIX_SEI;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_PREFIX_SEI;
  return false;
}

/*****************************************************************************/
static bool isIRAP(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return eNut == AL_AVC_NUT_VCL_IDR;

  if(isHEVC(eCodec))
    return eNut == AL_HEVC_NUT_BLA_W_LP || eNut == AL_HEVC_NUT_BLA_W_RADL || eNut == AL_HEVC_NUT_BLA_N_LP || eNut == AL_HEVC_NUT_IDR_W_RADL || eNut == AL_HEVC_NUT_IDR_N_LP || eNut == AL_HEVC_NUT_CRA;
  return false;
}

/*****************************************************************************/
static bool checkSeiUUID(uint8_t* pBufs, AL_TNal* pNal, AL_ECodec codec, int iTotalSize)
{
  int const iTotalUUIDSize = isAVC(codec) ? 25 : 26;

  if((int)pNal->uSize != iTotalUUIDSize)
    return false;

  int const iStart = isAVC(codec) ? 6 : 7;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);

  for(int i = 0; i < iSize; i++)
  {
    int iPosition = (pNal->tStartCode.uPosition + iStart + i) % iTotalSize;

    if(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID[i] != pBufs[iPosition])
      return false;
  }

  return true;
}

/*****************************************************************************/
static int getNumSliceInSei(uint8_t* pBufs, AL_TNal* pNal, AL_ECodec eCodec, int iTotalSize)
{
  AL_Assert(checkSeiUUID(pBufs, pNal, eCodec, iTotalSize));
  int const iStart = isAVC(eCodec) ? 6 : 7;
  int const iSize = sizeof(SEI_PREFIX_USER_DATA_UNREGISTERED_UUID) / sizeof(*SEI_PREFIX_USER_DATA_UNREGISTERED_UUID);
  int iPosition = (pNal->tStartCode.uPosition + iStart + iSize) % iTotalSize;
  return pBufs[iPosition];
}

/*****************************************************************************/
static bool enoughStartCode(int iNumStartCode)
{
  return iNumStartCode > 1;
}

/*****************************************************************************/
static bool isStartCode(uint8_t* pBuf, uint32_t uSize, uint32_t uPos)
{
  return (pBuf[uPos % uSize] == 0x00) &&
         (pBuf[(uPos + 1) % uSize] == 0x00) &&
         (pBuf[(uPos + 2) % uSize] == 0x01);
}

/*****************************************************************************/
static uint32_t skipNalHeader(uint32_t uPos, AL_ECodec eCodec, uint32_t uSize)
{
  int iNalHdrSize = 0;

  if(isAVC(eCodec))
    iNalHdrSize = AVC_NAL_HDR_SIZE;

  if(isHEVC(eCodec))
    iNalHdrSize = HEVC_NAL_HDR_SIZE;
  return (uPos + iNalHdrSize) % uSize; // skip start code + nal header
}

/*****************************************************************************/
static bool isVcl(AL_ECodec eCodec, AL_ENut eNut)
{

  if(isAVC(eCodec))
    return AL_AVC_IsVcl(eNut);

  if(isHEVC(eCodec))
    return AL_HEVC_IsVcl(eNut);
  return false;
}

/* should only be used when the position is right after the nal header */
/*****************************************************************************/
static bool isFirstSlice(uint8_t* pBuf, uint32_t uPos)
{
  // in AVC, the first bit of the slice data is 1. (first_mb_in_slice = 0 encoded in ue)
  // in HEVC, the first bit is 1 too. (first_slice_segment_in_pic_flag = 1 if true))
  return (pBuf[uPos] & 0x80) != 0;
}

/*****************************************************************************/
static bool isFirstSliceStatusAvailable(int iSize, int iNalHdrSize)
{
  return iSize > iNalHdrSize;
}

/*****************************************************************************/
static bool isFirstSliceNAL(AL_TNal* pNal, AL_TBuffer* pStream, AL_ECodec eCodec)
{
  uint8_t* pBuf = AL_Buffer_GetData(pStream);
  uint32_t uPos = pNal->tStartCode.uPosition;
  uint32_t uSize = AL_Buffer_GetSize(pStream);
  AL_Assert(isStartCode(pBuf, uSize, uPos));
  uPos = skipNalHeader(uPos, eCodec, uSize);
  return isFirstSlice(pBuf, uPos);
}

/*****************************************************************************/
static bool SearchNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* pLastStartCodeInDecodingUnit, int* iLastVclNalInDecodingUnit, bool* pNewCVSStarts)
{
  if(!enoughStartCode(pCtx->uNumSC))
    return false;

  AL_TNal* pTable = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
  uint8_t* pBuf = AL_Buffer_GetData(pStream);
  bool bVCLNalSeen = false;
  int iNalFound = 0;
  AL_ECodec const eCodec = pCtx->pChanParam->eCodec;
  int const iNalCount = (int)pCtx->uNumSC;
  *pNewCVSStarts = false;

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_TNal* pNal = &pTable[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(iNal > 0)
      iNalFound++;

    if(!isVcl(eCodec, eNUT))
    {
      if(isAud(eCodec, eNUT) || isEosOrEob(eCodec, eNUT))
      {
        if(bVCLNalSeen)
        {
          iNalFound--;
          *pLastStartCodeInDecodingUnit = iNalFound;
          return true;
        }
      }

      if(isPrefixSei(eCodec, eNUT) && checkSeiUUID(pBuf, pNal, eCodec, AL_Buffer_GetSize(pStream)))
      {
        pCtx->iNumSlicesRemaining = getNumSliceInSei(pBuf, pNal, eCodec, AL_Buffer_GetSize(pStream));
        AL_Assert(pCtx->iNumSlicesRemaining > 0);
      }

      if(isFd(eCodec, eNUT) && isSubframeUnit(pCtx->pChanParam->eDecUnit))
      {
        bool bIsLastSlice = pCtx->iNumSlicesRemaining == 1;

        if(bIsLastSlice && bVCLNalSeen)
        {
          *pLastStartCodeInDecodingUnit = iNalFound;
          pCtx->iNumSlicesRemaining = 0;
          return true;
        }
      }
    }

    if(isVcl(eCodec, eNUT))
    {
      int iNalHdrSize = isAVC(eCodec) ? AVC_NAL_HDR_SIZE : HEVC_NAL_HDR_SIZE;

      if(isFirstSliceStatusAvailable(pNal->uSize, iNalHdrSize))
      {
        bool bIsFirstSlice = isFirstSliceNAL(pNal, pStream, eCodec);

        if(bVCLNalSeen)
        {
          if(bIsFirstSlice)
          {
            iNalFound--;
            *pLastStartCodeInDecodingUnit = iNalFound;
            return true;
          }

          if(isSubframeUnit(pCtx->pChanParam->eDecUnit))
          {
            pCtx->iNumSlicesRemaining--;
            iNalFound--;
            *pLastStartCodeInDecodingUnit = iNalFound;
            int const iIsNotLastSlice = -1;
            *iLastVclNalInDecodingUnit = iIsNotLastSlice;
            return true;
          }
        }

        if(bIsFirstSlice && isIRAP(eCodec, eNUT))
          *pNewCVSStarts = true;
      }

      bVCLNalSeen = true;
      *iLastVclNalInDecodingUnit = iNal;
    }
  }

  return false;
}

/*****************************************************************************/
static void DecodeOneAVCNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(ScTable.tStartCode.uNUT >= AL_AVC_NUT_ERR)
    return;

  AL_AVC_DecodeOneNAL(&pCtx->aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
}

/*****************************************************************************/
static void DecodeOneHEVCNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(ScTable.tStartCode.uNUT >= AL_HEVC_NUT_ERR)
    return;

  AL_HEVC_DecodeOneNAL(&pCtx->aup, pCtx, ScTable.tStartCode.uNUT, bIsLastVclNal, pNumSlice);
}

/*****************************************************************************/
static void DecodeOneNAL(AL_TDecCtx* pCtx, AL_TNal ScTable, int* pNumSlice, bool bIsLastVclNal)
{
  if(*pNumSlice > 0 && *pNumSlice > pCtx->pChanParam->iMaxSlices)
    return;

  if(isAVC(pCtx->pChanParam->eCodec))
    DecodeOneAVCNAL(pCtx, ScTable, pNumSlice, bIsLastVclNal);

  if(isHEVC(pCtx->pChanParam->eCodec))
    DecodeOneHEVCNAL(pCtx, ScTable, pNumSlice, bIsLastVclNal);
}

/*****************************************************************************/
static void StartNewCVS(AL_TDecCtx* pCtx)
{
  AL_HDRSEIs_Reset(&pCtx->aup.tHDRSEIs);
}

/*****************************************************************************/
static void GenerateScdIpTraces(AL_TDecCtx* pCtx, AL_TScParam ScP, AL_TScBufferAddrs ScdBuffer, AL_TBuffer* pStream, TMemDesc scBuffer)
{
  (void)ScP;
  (void)ScdBuffer;
  (void)pStream;
  (void)scBuffer;

}

/*****************************************************************************/
static bool canStoreMoreStartCodes(AL_TDecCtx* pCtx)
{
  return (pCtx->uNumSC + 1) * sizeof(AL_TNal) <= pCtx->SCTable.tMD.uSize - pCtx->BufSCD.tMD.uSize;
}

/*****************************************************************************/
static void ResetStartCodes(AL_TDecCtx* pCtx)
{
  pCtx->uNumSC = 0;
}

/*****************************************************************************/
static size_t DeltaPosition(uint32_t uFirstPos, uint32_t uSecondPos, uint32_t uSize)
{
  if(uFirstPos < uSecondPos)
    return uSecondPos - uFirstPos;
  return uSize + uSecondPos - uFirstPos;
}

/*****************************************************************************/
static bool RefillStartCodes(AL_TDecCtx* pCtx, AL_TBuffer* pStream)
{
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);
  int const SCDHardwareConstraintMinSize = 5;

  if(pMeta->iAvailSize < SCDHardwareConstraintMinSize)
    return false;

  AL_TScBufferAddrs ScdBuffer = { 0 };
  TMemDesc startCodeOutputArray = pCtx->BufSCD.tMD;
  ScdBuffer.pBufOut = startCodeOutputArray.uPhysicalAddr;
  ScdBuffer.pStream = AL_Buffer_GetPhysicalAddress(pStream);
  ScdBuffer.uMaxSize = AL_Buffer_GetSize(pStream);
  ScdBuffer.uOffset = pMeta->iOffset;
  ScdBuffer.uAvailSize = pMeta->iAvailSize;

  AL_CleanupMemory(startCodeOutputArray.pVirtualAddr, startCodeOutputArray.uSize);

  AL_TDecScheduler_CB_EndStartCode callback = { AL_Decoder_EndScd, pCtx };

  AL_TScParam ScParam = { 0 };
  ScParam.MaxSize = startCodeOutputArray.uSize >> 3; // en bits ?
  ScParam.AVC = isAVC(pCtx->pChanParam->eCodec);

  /* if the start code couldn't be launched because the start code queue is full,
   * we have to retry as failing to do so will make us return ERR_UNIT_NOT_FOUND which
   * means that the whole chunk of data doesn't have a startcode in it and
   * for example that we can stop sending new decoding request if there isn't
   * more input data. */

  do
  {
    AL_IDecScheduler_SearchSC(pCtx->pScheduler, pCtx->hStartCodeChannel, &ScParam, &ScdBuffer, callback);
    Rtos_WaitEvent(pCtx->ScDetectionComplete, AL_WAIT_FOREVER);

    if(pCtx->ScdStatus.uNumBytes == 0)
    {
      printf("***** /!\\ Warning: Start code queue was full, degraded mode, retrying. /!\\ *****\n");
      Rtos_Sleep(1);
    }
  }
  while(pCtx->ScdStatus.uNumBytes == 0);

  GenerateScdIpTraces(pCtx, ScParam, ScdBuffer, pStream, startCodeOutputArray);
  pMeta->iOffset = (pMeta->iOffset + pCtx->ScdStatus.uNumBytes) % AL_Buffer_GetSize(pStream);
  pMeta->iAvailSize -= pCtx->ScdStatus.uNumBytes;

  AL_TStartCode const* src = (AL_TStartCode const*)startCodeOutputArray.pVirtualAddr;
  AL_TNal* dst = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  if(pCtx->uNumSC && pCtx->ScdStatus.uNumSC)
    dst[pCtx->uNumSC - 1].uSize = DeltaPosition(dst[pCtx->uNumSC - 1].tStartCode.uPosition, src[0].uPosition, ScdBuffer.uMaxSize);

  for(int i = 0; i < pCtx->ScdStatus.uNumSC; i++)
  {
    dst[pCtx->uNumSC].tStartCode = src[i];

    if(i + 1 == pCtx->ScdStatus.uNumSC)
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, pMeta->iOffset, ScdBuffer.uMaxSize);
    else
      dst[pCtx->uNumSC].uSize = DeltaPosition(src[i].uPosition, src[i + 1].uPosition, ScdBuffer.uMaxSize);

    pCtx->uNumSC++;
  }

  return pCtx->ScdStatus.uNumSC > 0;
}

/*****************************************************************************/
static int FindNextDecodingUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* iLastVclNalInAU, bool* pNewCVSStarts)
{
  int iLastStartCodeIdx = 0;

  while(!SearchNextDecodingUnit(pCtx, pStream, &iLastStartCodeIdx, iLastVclNalInAU, pNewCVSStarts))
  {
    if(!canStoreMoreStartCodes(pCtx))
    {
      // The start code table is full and doesn't contain any AU.
      // Clear the start code table to avoid a stall
      ResetStartCodes(pCtx);
    }

    if(!RefillStartCodes(pCtx, pStream))
      return 0;
  }

  return iLastStartCodeIdx + 1;
}

/*****************************************************************************/
static int FillNalInfo(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int* iLastVclNalInAU, bool* pNewCVSStarts)
{
  pCtx->pInputBuffer = pStream;

  *pNewCVSStarts = false;

  while(RefillStartCodes(pCtx, pStream) != false)
    ;

  int iNalCount = pCtx->uNumSC;
  AL_TStreamMetaData* pStreamMeta = (AL_TStreamMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_STREAM);
  bool bSearchLastVCLNal = false;

  for(int curSection = 0; curSection < pStreamMeta->uNumSection; ++curSection)
  {
    if(pStreamMeta->pSections[curSection].eFlags & AL_SECTION_END_FRAME_FLAG)
    {
      bSearchLastVCLNal = true;
      break;
    }
  }

  AL_TNal* pTable = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;
  AL_ECodec const eCodec = pCtx->pChanParam->eCodec;

  for(int iNal = iNalCount - 1; iNal >= 0; --iNal)
  {
    AL_TNal* pNal = &pTable[iNal];
    AL_ENut eNUT = pNal->tStartCode.uNUT;

    if(isVcl(eCodec, eNUT))
    {
      if(bSearchLastVCLNal)
      {
        *iLastVclNalInAU = iNal;
        bSearchLastVCLNal = false;
      }

      if(isFirstSliceNAL(pNal, pStream, eCodec))
      {
        if(isIRAP(eCodec, eNUT))
          *pNewCVSStarts = true;
        break;
      }
    }
  }

  return iNalCount;
}

/*****************************************************************************/
static void ConsumeNals(AL_TDecCtx* pCtx, int iNumNal)
{
  AL_TNal* nals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  pCtx->uNumSC -= iNumNal;
  Rtos_Memmove(nals, nals + iNumNal, pCtx->uNumSC * sizeof(AL_TNal));
}

/*****************************************************************************/
static void ResetValidFlags(AL_TDecCtx* pCtx)
{
  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
}

/*****************************************************************************/
static UNIT_ERROR DecodeOneUnit(AL_TDecCtx* pCtx, AL_TBuffer* pStream, int iNalCount, int iLastVclNalInAU, bool bNewCVSStarts)
{
  AL_TNal* nals = (AL_TNal*)pCtx->SCTable.tMD.pVirtualAddr;

  /* copy start code buffer stream information into decoder stream buffer */
  pCtx->Stream.tMD.uSize = AL_Buffer_GetSize(pStream);
  pCtx->Stream.tMD.pAllocator = pStream->pAllocator;
  pCtx->Stream.tMD.hAllocBuf = pStream->hBufs[0];
  pCtx->Stream.tMD.pVirtualAddr = AL_Buffer_GetData(pStream);
  pCtx->Stream.tMD.uPhysicalAddr = AL_Buffer_GetPhysicalAddress(pStream);
  AL_TCircMetaData* pMeta = (AL_TCircMetaData*)AL_Buffer_GetMetaData(pStream, AL_META_TYPE_CIRCULAR);

  int iNumSlice = 0;
  bool bIsEndOfFrame = false;

  if(bNewCVSStarts)
    StartNewCVS(pCtx);

  for(int iNal = 0; iNal < iNalCount; ++iNal)
  {
    AL_TNal CurrentNal = nals[iNal];
    AL_TStartCode CurrentStartCode = CurrentNal.tStartCode;
    AL_TStartCode NextStartCode;

    if(iNal + 1 < pCtx->uNumSC)
    {
      NextStartCode = nals[iNal + 1].tStartCode;
    }
    else /* if we didn't wait for the next start code to arrive to decode the current NAL */
    {
      /* If there isn't a next start code, we take the end of the data processed
       * by the start code detector */
      NextStartCode.uPosition = pMeta->iOffset;
    }

    pCtx->Stream.iOffset = CurrentStartCode.uPosition;
    pCtx->Stream.iAvailSize = DeltaPosition(CurrentStartCode.uPosition, NextStartCode.uPosition, AL_Buffer_GetSize(pStream));

    bool bIsLastVclNal = (iNal == iLastVclNalInAU);

    if(bIsLastVclNal)
      bIsEndOfFrame = true;
    DecodeOneNAL(pCtx, CurrentNal, &iNumSlice, bIsLastVclNal);

    if(pCtx->eChanState == CHAN_INVALID)
      return ERR_UNIT_INVALID_CHANNEL;

    if(pCtx->error == AL_ERR_NO_MEMORY)
      return ERR_UNIT_DYNAMIC_ALLOC;

    if(bIsLastVclNal)
      ResetValidFlags(pCtx);

    if(pCtx->eChanState == CHAN_DESTROYING)
    {
      ConsumeNals(pCtx, iNalCount);
      ResetValidFlags(pCtx);
      return ERR_UNIT_FAILED;
    }
  }

  ConsumeNals(pCtx, iNalCount);

  return bIsEndOfFrame ? SUCCESS_ACCESS_UNIT : SUCCESS_NAL_UNIT;
}

/*****************************************************************************/
UNIT_ERROR AL_Default_Decoder_TryDecodeOneUnit(AL_TDecoder* pAbsDec, AL_TBuffer* pStream)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  bool bNewCVSStarts = false;
  int iLastVclNalInAU = -1;
  int iNalCount = pCtx->bSplitInput ? FillNalInfo(pCtx, pStream, &iLastVclNalInAU, &bNewCVSStarts)
                  : FindNextDecodingUnit(pCtx, pStream, &iLastVclNalInAU, &bNewCVSStarts);

  if(iNalCount == 0)
    return ERR_UNIT_NOT_FOUND;

  return DecodeOneUnit(pCtx, pStream, iNalCount, iLastVclNalInAU, bNewCVSStarts);
}

/*****************************************************************************/
bool AL_Default_Decoder_PushBuffer(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf, size_t uSize)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  return AL_Feeder_PushBuffer(pCtx->Feeder, pBuf, uSize, false);
}

/*****************************************************************************/
void AL_Default_Decoder_Flush(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_Feeder_Flush(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_WaitFrameSent(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
    Rtos_GetSemaphore(pCtx->Sem, AL_WAIT_FOREVER);
}

/*****************************************************************************/
void AL_Default_Decoder_ReleaseFrames(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  for(int iSem = 0; iSem < pCtx->iStackSize; ++iSem)
    Rtos_ReleaseSemaphore(pCtx->Sem);
}

/*****************************************************************************/
void AL_Default_Decoder_FlushInput(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  ResetStartCodes(pCtx);
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->iCurOffset = 0;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  AL_Feeder_Reset(pCtx->Feeder);
}

/*****************************************************************************/
void AL_Default_Decoder_InternalFlush(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  AL_Default_Decoder_WaitFrameSent(pAbsDec);

  AL_PictMngr_Flush(&pCtx->PictMngr);

  AL_Default_Decoder_FlushInput(pAbsDec);

  // Send eos & get last frames in dpb if any
  AL_sDecoder_CallDisplay(pCtx);

  AL_Default_Decoder_ReleaseFrames(pAbsDec);

  pCtx->displayCB.func(NULL, NULL, pCtx->displayCB.userParam);

  AL_PictMngr_Terminate(&pCtx->PictMngr);
}

/*****************************************************************************/
static void CheckDisplayBufferCanBeUsed(AL_TDecCtx* pCtx, AL_TBuffer* pBuf)
{
  AL_Assert(pBuf);

  int iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);
  int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);

  /* decoder only output semiplanar */
  AL_Assert((iPitchY != 0) && (iPitchY == iPitchUV));

  AL_TStreamSettings* pSettings = &pCtx->tStreamSettings;

  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);

  AL_Assert(iPitchY >= (int)AL_Decoder_GetMinPitch(pSettings->tDim.iWidth, pSettings->iBitDepth, eDisplayStorageMode));

  AL_Assert(iPitchY % 64 == 0);
}

/*****************************************************************************/
void AL_Default_Decoder_PutDecPict(AL_TDecoder* pAbsDec, AL_TBuffer* pDecPict)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  CheckDisplayBufferCanBeUsed(pCtx, pDecPict);

  AL_PictMngr_PutDisplayBuffer(&pCtx->PictMngr, pDecPict);

  AL_Feeder_Signal(pCtx->Feeder);
}

/*****************************************************************************/
int AL_Default_Decoder_GetMaxBD(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);

  return pCtx->tStreamSettings.iBitDepth;
}

/*****************************************************************************/
static int Secure_GetStrOffset(AL_TDecCtx* pCtx)
{
  int iCurOffset;
  Rtos_GetMutex(pCtx->DecMutex);
  iCurOffset = pCtx->iCurOffset;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return iCurOffset;
}

/*****************************************************************************/
int AL_Default_Decoder_GetStrOffset(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = AL_sGetContext(pDec);
  return Secure_GetStrOffset(pCtx);
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_GetLastError(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  Rtos_GetMutex(pCtx->DecMutex);
  AL_ERR ret = pCtx->error;
  Rtos_ReleaseMutex(pCtx->DecMutex);
  return ret;
}

/*****************************************************************************/
AL_ERR AL_Default_Decoder_GetFrameError(AL_TDecoder* pAbsDec, AL_TBuffer* pBuf)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;

  AL_ERR error;
  bool bUseLastError = (pBuf == NULL) || (!AL_PictMngr_GetFrameEncodingError(&pCtx->PictMngr, pBuf, &error));

  if(bUseLastError)
    error = AL_Default_Decoder_GetLastError(pAbsDec);

  return error;
}

/*****************************************************************************/
bool AL_Default_Decoder_PreallocateBuffers(AL_TDecoder* pAbsDec)
{
  AL_TDecoder* pDec = (AL_TDecoder*)pAbsDec;
  AL_TDecCtx* pCtx = &pDec->ctx;
  AL_Assert(!pCtx->bIsBuffersAllocated);
  AL_Assert(IsAllStreamSettingsSet(pCtx->tStreamSettings));

  AL_ERR error = AL_ERR_NO_MEMORY;
  AL_TStreamSettings tStreamSettings = pCtx->tStreamSettings;

  if(pCtx->tStreamSettings.iBitDepth > HW_IP_BIT_DEPTH)
  {
    AL_Default_Decoder_SetError(pCtx, AL_ERR_REQUEST_MALFORMED, -1);
    return false;
  }

  if(pCtx->tStreamSettings.eSequenceMode == AL_SM_MAX_ENUM)
  {
    AL_Default_Decoder_SetError(pCtx, AL_ERR_REQUEST_MALFORMED, -1);
    return false;
  }

  int iSPSMaxSlices = RoundUp(pCtx->pChanParam->iHeight, 16) / 16;

  if(isAVC(pCtx->pChanParam->eCodec))
    iSPSMaxSlices = AL_AVC_GetMaxNumberOfSlices(AVC_PROFILE_IDC_HIGH_422, 52, 1, 60, INT32_MAX); // FIXME : use real level

  if(isHEVC(pCtx->pChanParam->eCodec))
    iSPSMaxSlices = AL_HEVC_GetMaxNumberOfSlices(62); // FIXME : use real level
  int iSizeWP = iSPSMaxSlices * WP_SLICE_SIZE;
  int iSizeSP = iSPSMaxSlices * sizeof(AL_TDecSliceParam);
  int iSizeCompData = isAVC(pCtx->pChanParam->eCodec) ? AL_GetAllocSize_AvcCompData(tStreamSettings.tDim, tStreamSettings.eChroma) : AL_GetAllocSize_HevcCompData(tStreamSettings.tDim, tStreamSettings.eChroma);
  int iSizeCompMap = AL_GetAllocSize_DecCompMap(tStreamSettings.tDim);

  if(!AL_Default_Decoder_AllocPool(pCtx, iSizeWP, iSizeSP, iSizeCompData, iSizeCompMap))
    goto fail_alloc;

  int iDpbMaxBuf = 0, iRecBuf = 0, iSizeMV = 0;

  if(isAVC(pCtx->pChanParam->eCodec))
  {
    iDpbMaxBuf = AL_AVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight);
    iRecBuf = REC_BUF;
    iSizeMV = AL_GetAllocSize_AvcMV(tStreamSettings.tDim);
  }

  if(isHEVC(pCtx->pChanParam->eCodec))
  {
    iDpbMaxBuf = AL_HEVC_GetMaxDPBSize(tStreamSettings.iLevel, tStreamSettings.tDim.iWidth, tStreamSettings.tDim.iHeight);
    iRecBuf = 0;
    iSizeMV = AL_GetAllocSize_HevcMV(tStreamSettings.tDim);
  }
  int iConcealBuf = CONCEAL_BUF;
  int iMaxBuf = iDpbMaxBuf + pCtx->iStackSize + iRecBuf + iConcealBuf;
  int iSizePOC = POCBUFF_PL_SIZE;

  if(!AL_Default_Decoder_AllocMv(pCtx, iSizeMV, iSizePOC, iMaxBuf))
    goto fail_alloc;

  AL_TPictMngrParam tPictMngrParam =
  {
    iDpbMaxBuf, pCtx->eDpbMode, pCtx->pChanParam->eFBStorageMode, tStreamSettings.iBitDepth,
    iMaxBuf, iSizeMV,
    pCtx->pChanParam->bUseEarlyCallback,

  };

  AL_PictMngr_Init(&pCtx->PictMngr, pCtx->pAllocator, &tPictMngrParam);

  bool bEnableDisplayCompression;
  AL_EFbStorageMode eDisplayStorageMode = AL_Default_Decoder_GetDisplayStorageMode(pCtx, &bEnableDisplayCompression);

  int iSizeYuv = AL_GetAllocSize_Frame(tStreamSettings.tDim, tStreamSettings.eChroma, tStreamSettings.iBitDepth, bEnableDisplayCompression, eDisplayStorageMode);
  AL_TCropInfo tCropInfo = { false, 0, 0, 0, 0 };

  error = pCtx->resolutionFoundCB.func(iMaxBuf, iSizeYuv, &tStreamSettings, &tCropInfo, pCtx->resolutionFoundCB.userParam);

  if(!AL_IS_SUCCESS_CODE(error))
    goto fail_alloc;

  pCtx->bIsBuffersAllocated = true;

  return true;
  fail_alloc:
  AL_Default_Decoder_SetError(pCtx, error, -1);
  return false;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferHEVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x4A, 0
  }; // simulate a end_of_bitstream
  int iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iSize, &AL_Buffer_Destroy, "eos-buffer");
  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);
  return pEOS;
}

/*****************************************************************************/
static AL_TBuffer* AllocEosBufferAVC(bool bSplitInput, AL_TAllocator* pAllocator)
{
  static const uint8_t EOSNal[] =
  {
    0, 0, 1, 0x0B, 0
  }; // simulate end_of_stream
  int iSize = sizeof EOSNal;

  if(!bSplitInput)
    return AL_Buffer_WrapData((uint8_t*)EOSNal, iSize, &AL_Buffer_Destroy);
  AL_TBuffer* pEOS = AL_Buffer_Create_And_AllocateNamed(pAllocator, iSize, &AL_Buffer_Destroy, "eos-buffer");
  Rtos_Memcpy(AL_Buffer_GetData(pEOS), EOSNal, iSize);
  return pEOS;
}

/*****************************************************************************/
static bool CheckStreamSettings(AL_TStreamSettings tStreamSettings)
{
  if(!IsAtLeastOneStreamSettingsSet(tStreamSettings))
    return true;

  if(!IsAllStreamSettingsSet(tStreamSettings))
    return false;

  if(tStreamSettings.iBitDepth > HW_IP_BIT_DEPTH)
    return false;

  return true;
}

/*****************************************************************************/
static bool CheckDecodeUnit(AL_EDecUnit eDecUnit)
{
  return eDecUnit != AL_DEC_UNIT_MAX_ENUM;
}

/*****************************************************************************/
static bool CheckAVCSettings(AL_TDecSettings const* pSettings)
{
  AL_Assert(isAVC(pSettings->eCodec));

  if(pSettings->bParallelWPP)
    return false;

  if((pSettings->tStream.eSequenceMode != AL_SM_UNKNOWN) && (pSettings->tStream.eSequenceMode != AL_SM_PROGRESSIVE) && (pSettings->tStream.eSequenceMode != AL_SM_MAX_ENUM))
    return false;

  return true;
}

/*****************************************************************************/
static bool CheckRecStorageMode(AL_TDecSettings const* pSettings)
{
  if(pSettings->eFBStorageMode == AL_FB_RASTER)
    return true;

  return false;
}

/*****************************************************************************/
static bool CheckSettings(AL_TDecSettings const* pSettings)
{
  if(!CheckStreamSettings(pSettings->tStream))
    return false;

  int const iStack = pSettings->iStackSize;

  if((iStack < 1) || (iStack > MAX_STACK_SIZE))
    return false;

  if((pSettings->uDDRWidth != 16) && (pSettings->uDDRWidth != 32) && (pSettings->uDDRWidth != 64))
    return false;

  if((pSettings->uFrameRate == 0) && pSettings->bForceFrameRate)
    return false;

  if(!CheckDecodeUnit(pSettings->eDecUnit))
    return false;

  if(isSubframeUnit(pSettings->eDecUnit) && (pSettings->bParallelWPP || (pSettings->eDpbMode != AL_DPB_NO_REORDERING)))
    return false;

  if(isAVC(pSettings->eCodec))
  {
    if(!CheckAVCSettings(pSettings))
      return false;
  }

  if(!CheckRecStorageMode(pSettings))
    return false;

  return true;
}

/*****************************************************************************/
static void AssignSettings(AL_TDecCtx* const pCtx, AL_TDecSettings const* const pSettings)
{
  pCtx->bSplitInput = pSettings->bSplitInput;
  pCtx->iStackSize = pSettings->iStackSize;
  pCtx->bForceFrameRate = pSettings->bForceFrameRate;
  pCtx->eDpbMode = pSettings->eDpbMode;
  pCtx->tStreamSettings = pSettings->tStream;
  pCtx->bUseIFramesAsSyncPoint = pSettings->bUseIFramesAsSyncPoint;

  AL_TDecChanParam* pChan = pCtx->pChanParam;
  pChan->uMaxLatency = pSettings->iStackSize;
  pChan->uNumCore = pSettings->uNumCore;
  pChan->uClkRatio = pSettings->uClkRatio;
  pChan->uFrameRate = pSettings->uFrameRate == 0 ? pSettings->uClkRatio : pSettings->uFrameRate;
  pChan->bDisableCache = pSettings->bDisableCache;
  pChan->bFrameBufferCompression = pSettings->bFrameBufferCompression;
  pChan->eFBStorageMode = pSettings->eFBStorageMode;
  pChan->uDDRWidth = pSettings->uDDRWidth == 16 ? 0 : pSettings->uDDRWidth == 32 ? 1 : 2;
  pChan->bParallelWPP = pSettings->bParallelWPP;
  pChan->bLowLat = pSettings->bLowLat;
  pChan->bUseEarlyCallback = pSettings->bUseEarlyCallback;
  pChan->eCodec = pSettings->eCodec;
  pChan->eDecUnit = pSettings->eDecUnit;

  pChan->eBufferOutputMode = pSettings->eBufferOutputMode;

}

/*****************************************************************************/
static bool CheckCallBacks(AL_TDecCallBacks* pCallbacks)
{
  if(pCallbacks->endDecodingCB.func == NULL)
    return false;

  if(pCallbacks->displayCB.func == NULL)
    return false;

  if(pCallbacks->resolutionFoundCB.func == NULL)
    return false;

  return true;
}

/*****************************************************************************/
static void AssignCallBacks(AL_TDecCtx* const pCtx, AL_TDecCallBacks* pCB)
{
  pCtx->parseCB = pCB->endParsingCB;
  pCtx->decodeCB = pCB->endDecodingCB;
  pCtx->displayCB = pCB->displayCB;
  pCtx->resolutionFoundCB = pCB->resolutionFoundCB;
  pCtx->parsedSeiCB = pCB->parsedSeiCB;
}

/*****************************************************************************/
static void errorHandler(void* pUserParam)
{
  AL_TDecoder* const pDec = (AL_TDecoder*)pUserParam;
  AL_TDecCtx* const pCtx = &pDec->ctx;
  pCtx->decodeCB.func(NULL, pCtx->decodeCB.userParam);
}

/*****************************************************************************/
bool AL_Default_Decoder_AllocPool(AL_TDecCtx* pCtx, int iWPSize, int iSPSize, int iCompDataSize, int iCompMapSize)
{
#define SAFE_POOL_ALLOC(pCtx, pMD, iSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, iSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < pCtx->iStackSize; ++i)
  {
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolWP[i].tMD, iWPSize, "wp");
    AL_CleanupMemory(pCtx->PoolWP[i].tMD.pVirtualAddr, pCtx->PoolWP[i].tMD.uSize);
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolSP[i].tMD, iSPSize, "sp");
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompData[i].tMD, iCompDataSize, "comp data");
    SAFE_POOL_ALLOC(pCtx, &pCtx->PoolCompMap[i].tMD, iCompMapSize, "comp map");
  }

  return true;
}

/*****************************************************************************/
bool AL_Default_Decoder_AllocMv(AL_TDecCtx* pCtx, int iMVSize, int iPOCSize, int iNum)
{
#define SAFE_MV_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
      return false; \
  } while(0)

  for(int i = 0; i < iNum; ++i)
  {
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pMvBufs[i].tMD, iMVSize, "mv");
    SAFE_MV_ALLOC(pCtx, &pCtx->PictMngr.MvBufPool.pPocBufs[i].tMD, iPOCSize, "poc");
  }

  return true;
}

/*****************************************************************************/
void AL_Default_Decoder_SetError(AL_TDecCtx* pCtx, AL_ERR eError, int iFrameID)
{
  Rtos_GetMutex(pCtx->DecMutex);
  pCtx->error = eError;
  Rtos_ReleaseMutex(pCtx->DecMutex);

  if(iFrameID != -1)
    AL_PictMngr_UpdateDisplayBufferError(&pCtx->PictMngr, iFrameID, eError);
}

/*****************************************************************************/
static void InitAUP(AL_TDecCtx* pCtx)
{

  if(isAVC(pCtx->pChanParam->eCodec))
    AL_AVC_InitAUP(&pCtx->aup.avcAup);

  if(isHEVC(pCtx->pChanParam->eCodec))
    AL_HEVC_InitAUP(&pCtx->aup.hevcAup);
}

/*****************************************************************************/
AL_ERR AL_CreateDefaultDecoder(AL_TDecoder** hDec, AL_IDecScheduler* pScheduler, AL_TAllocator* pAllocator, AL_TDecSettings* pSettings, AL_TDecCallBacks* pCB)
{
  *hDec = NULL;

  if(!CheckSettings(pSettings))
    return AL_ERR_REQUEST_MALFORMED;

  if(!CheckCallBacks(pCB))
    return AL_ERR_REQUEST_MALFORMED;

  AL_TDecoder* const pDec = (AL_TDecoder*)Rtos_Malloc(sizeof(AL_TDecoder));
  AL_ERR errorCode = AL_ERROR;

  if(!pDec)
    return AL_ERR_NO_MEMORY;

  Rtos_Memset(pDec, 0, sizeof(*pDec));

  AL_TDecCtx* const pCtx = &pDec->ctx;

  pCtx->pScheduler = pScheduler;
  pCtx->hChannel = NULL;
  pCtx->hStartCodeChannel = NULL;
  pCtx->pAllocator = pAllocator;

  InitInternalBuffers(pCtx);

#define SAFE_ALLOC(pCtx, pMD, uSize, name) \
  do { \
    if(!AL_Decoder_Alloc(pCtx, pMD, uSize, name)) \
    { \
      errorCode = AL_ERR_NO_MEMORY; \
      goto cleanup; \
    } \
  } while(0)

  SAFE_ALLOC(pCtx, &pCtx->tMDChanParam, sizeof(AL_TDecChanParam), "chp");
  pCtx->pChanParam = (AL_TDecChanParam*)pCtx->tMDChanParam.pVirtualAddr;
  Rtos_Memset(pCtx->pChanParam, 0, sizeof(*pCtx->pChanParam));

  AssignSettings(pCtx, pSettings);
  AssignCallBacks(pCtx, pCB);

  pCtx->Sem = Rtos_CreateSemaphore(pCtx->iStackSize);
  pCtx->ScDetectionComplete = Rtos_CreateEvent(0);
  pCtx->DecMutex = Rtos_CreateMutex();

  AL_Default_Decoder_SetParam((AL_TDecoder*)pDec, false, false, 0, 0, false, false);

  // initialize decoder context
  pCtx->bFirstIsValid = false;
  pCtx->PictMngr.bFirstInit = false;
  pCtx->uNoRaslOutputFlag = 1;
  pCtx->bIsFirstPicture = true;
  pCtx->bFirstSliceInFrameIsValid = false;
  pCtx->bBeginFrameIsValid = false;
  pCtx->bIsFirstSPSChecked = false;
  pCtx->bIsBuffersAllocated = false;
  pCtx->uNumSC = 0;
  pCtx->iNumSlicesRemaining = 0;

  AL_Conceal_Init(&pCtx->tConceal);
  // initialize decoder counters
  pCtx->uCurPocLsb = 0xFFFFFFFF;
  pCtx->uToggle = 0;
  pCtx->iNumFrmBlk1 = 0;
  pCtx->iNumFrmBlk2 = 0;
  pCtx->iCurOffset = 0;
  pCtx->iTraceCounter = 0;
  pCtx->eChanState = CHAN_UNINITIALIZED;

  // initialize tile information
  pCtx->uCurTileID = 0;
  pCtx->bTileSupToSlice = false;

  // initialize slice toggle information
  pCtx->uCurID = 0;

  InitAUP(pCtx);

  if((isAVC(pSettings->eCodec)) || (isHEVC(pSettings->eCodec)))
  {
    // Alloc Start Code Detector buffer
    SAFE_ALLOC(pCtx, &pCtx->BufSCD.tMD, SCD_SIZE, "scd");
    AL_CleanupMemory(pCtx->BufSCD.tMD.pVirtualAddr, pCtx->BufSCD.tMD.uSize);

    SAFE_ALLOC(pCtx, &pCtx->SCTable.tMD, pCtx->iStackSize * MAX_NAL_UNIT * sizeof(AL_TNal), "sctable");
    AL_CleanupMemory(pCtx->SCTable.tMD.pVirtualAddr, pCtx->SCTable.tMD.uSize);

    // Alloc Decoder buffers
    for(int i = 0; i < pCtx->iStackSize; ++i)
    {
      SAFE_ALLOC(pCtx, &pCtx->PoolListRefAddr[i].tMD, REF_LIST_SIZE * sizeof(AL_PADDR), "reflist");
      SAFE_ALLOC(pCtx, &pCtx->PoolSclLst[i].tMD, SCLST_SIZE_DEC, "scllst");
      AL_CleanupMemory(pCtx->PoolSclLst[i].tMD.pVirtualAddr, pCtx->PoolSclLst[i].tMD.uSize);

      Rtos_Memset(&pCtx->PoolPP[i], 0, sizeof(pCtx->PoolPP[0]));
      Rtos_Memset(&pCtx->PoolPB[i], 0, sizeof(pCtx->PoolPB[0]));
      AL_SET_DEC_OPT(&pCtx->PoolPP[i], IntraOnly, 1);
    }

  }

  // Alloc Decoder Deanti-emulated buffer for high level syntax parsing
  pCtx->BufNoAE.tMD.pVirtualAddr = Rtos_Malloc(NON_VCL_NAL_SIZE);

  if(!pCtx->BufNoAE.tMD.pVirtualAddr)
    goto cleanup;

  pCtx->BufNoAE.tMD.uSize = NON_VCL_NAL_SIZE;

  if(isAVC(pCtx->pChanParam->eCodec))
    pCtx->eosBuffer = AllocEosBufferAVC(pCtx->bSplitInput, pAllocator);

  if(isHEVC(pCtx->pChanParam->eCodec))
    pCtx->eosBuffer = AllocEosBufferHEVC(pCtx->bSplitInput, pAllocator);

  AL_Assert(pCtx->eosBuffer);

  if(!pCtx->eosBuffer)
    goto cleanup;

  AL_Buffer_Ref(pCtx->eosBuffer);

  int iInputFifoSize = 256;

  if(pCtx->bSplitInput)
    pCtx->Feeder = AL_SplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pCtx->eosBuffer);
  else
  {
    AL_CB_Error errorCallback =
    {
      errorHandler,
      (void*)pDec
    };

    int iBufferStreamSize = GetCircularBufferSize(pCtx->pChanParam->eCodec, pCtx->iStackSize, pCtx->tStreamSettings);
    pCtx->Feeder = AL_UnsplitBufferFeeder_Create((AL_HDecoder)pDec, iInputFifoSize, pAllocator, iBufferStreamSize, pCtx->eosBuffer, &errorCallback);
  }

  if(!pCtx->Feeder)
    goto cleanup;

  AL_IDecScheduler_CreateStartCodeChannel(&pCtx->hStartCodeChannel, pCtx->pScheduler);

  AL_Default_Decoder_SetError(pCtx, AL_SUCCESS, -1);

  *hDec = (AL_TDecoder*)pDec;

  return AL_SUCCESS;

  cleanup:
  AL_Decoder_Destroy((AL_HDecoder)pDec);
  return errorCode;
}

/*****************************************************************************/
AL_EFbStorageMode AL_Default_Decoder_GetDisplayStorageMode(AL_TDecCtx* pCtx, bool* pEnableCompression)
{
  AL_EFbStorageMode eDisplayStorageMode = pCtx->pChanParam->eFBStorageMode;
  *pEnableCompression = pCtx->pChanParam->bFrameBufferCompression;

  return eDisplayStorageMode;
}

/*@}*/

