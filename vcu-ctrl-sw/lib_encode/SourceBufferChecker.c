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

#include "SourceBufferChecker.h"
#include "lib_common/PixMapBufferInternal.h"
#include "lib_common_enc/EncBuffers.h"
#include "lib_common_enc/IpEncFourCC.h"
#include "lib_common/Utils.h"

void AL_SrcBuffersChecker_Init(AL_TSrcBufferChecker* pCtx, AL_TEncChanParam const* pChParam)
{
  pCtx->maxDim.iWidth = AL_GetSrcWidth(*pChParam);
  pCtx->maxDim.iHeight = AL_GetSrcHeight(*pChParam);

  pCtx->currentDim = pCtx->maxDim;

  pCtx->picFmt = AL_EncGetSrcPicFormat(AL_GET_CHROMA_MODE(pChParam->ePicFormat), pChParam->uSrcBitDepth, AL_GetSrcStorageMode(pChParam->eSrcMode), AL_IsSrcCompressed(pChParam->eSrcMode));
  pCtx->fourCC = AL_GetFourCC(pCtx->picFmt);
  pCtx->srcMode = pChParam->eSrcMode;
}

bool AL_SrcBuffersChecker_UpdateResolution(AL_TSrcBufferChecker* pCtx, AL_TDimension tNewDim)
{
  if((pCtx->maxDim.iWidth < tNewDim.iWidth) || (pCtx->maxDim.iHeight < tNewDim.iHeight))
    return false;
  pCtx->currentDim = tNewDim;
  return true;
}

static bool CheckMetaData(AL_TBuffer* pBuf)
{
  return AL_Buffer_GetMetaData(pBuf, AL_META_TYPE_PIXMAP) != NULL;
}

static bool CheckPlanes(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_EChromaMode eChromaMode = AL_GetChromaMode(tFourCC);

  int iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);

  if(tDim.iWidth != pCtx->currentDim.iWidth)
    return false;

  if(tDim.iHeight != pCtx->currentDim.iHeight)
    return false;

  if(tFourCC != pCtx->fourCC)
    return false;

  int iMinPitchY = AL_EncGetMinPitch(tDim.iWidth, AL_GetBitDepth(tFourCC), AL_GetStorageMode(tFourCC));

  if(iPitchY < iMinPitchY)
    return false;

  if(iPitchY % HW_IP_BURST_ALIGNMENT)
    return false;

  if(eChromaMode != AL_CHROMA_MONO)
  {
    int iPitchUV = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_UV);

    if(iPitchUV != iPitchY)
      return false;
  }

  return true;
}

static uint32_t GetSrcPlaneSize(AL_TDimension tDim, AL_EChromaMode eChromaMode, AL_ESrcMode eSrcFmt, int iPitchY, int iStrideHeight, AL_EPlaneId ePlaneId)
{
  (void)tDim;
  switch(ePlaneId)
  {
  case AL_PLANE_Y:
    return AL_GetAllocSizeSrc_Y(eSrcFmt, iPitchY, iStrideHeight);
  case AL_PLANE_UV:
    return AL_GetAllocSizeSrc_UV(eSrcFmt, iPitchY, iStrideHeight, eChromaMode);
  default:
    assert(0);
  }

  return 0;
}

static bool CheckSize(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pBuf);
  int const iPitchY = AL_PixMapBuffer_GetPlanePitch(pBuf, AL_PLANE_Y);
  int const iMinStrideHeight = RoundUp(tDim.iHeight, 8);

  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pBuf);
  AL_EChromaMode eChromaMode = AL_GetChromaMode(tFourCC);

  uint32_t uChunkSizes[AL_BUFFER_MAX_CHUNK] = { 0 };

  for(int iPlane = 0; iPlane < (int)AL_PLANE_MAX_ENUM; iPlane++)
  {
    AL_EPlaneId ePlaneId = (AL_EPlaneId)iPlane;
    int iChunkIdx = AL_PixMapBuffer_GetPlaneChunkIdx(pBuf, ePlaneId);

    if(iChunkIdx != AL_BUFFER_BAD_CHUNK)
      uChunkSizes[iChunkIdx] += GetSrcPlaneSize(tDim, eChromaMode, pCtx->srcMode, iPitchY, iMinStrideHeight, ePlaneId);
  }

  for(int i = 0; i < AL_BUFFER_MAX_CHUNK; i++)
  {
    if(uChunkSizes[i] != 0 && (AL_Buffer_GetSizeChunk(pBuf, i) < uChunkSizes[i]))
      return false;
  }

  return true;
}

bool AL_SrcBuffersChecker_CanBeUsed(AL_TSrcBufferChecker* pCtx, AL_TBuffer* pBuf)
{
  if(pBuf == NULL)
    return false;

  if(!CheckMetaData(pBuf))
    return false;

  if(!CheckPlanes(pCtx, pBuf))
    return false;

  return CheckSize(pCtx, pBuf);
}

