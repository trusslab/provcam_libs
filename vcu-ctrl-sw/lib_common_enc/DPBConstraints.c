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

#include "DPBConstraints.h"
#include "lib_common/Utils.h"

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_DefaultGopMngr(const AL_TGopParam* pGopParam, AL_ECodec eCodec)
{
  int iNumRef = 0;

  if(pGopParam->eMode == AL_GOP_MODE_LOW_DELAY_B)
  {
    iNumRef = 2; /* the refs of a B picture */

    /*
     * Add 1 in AVC for the current frame. If GOP length is 1, the current frame ref can be spared as
     * the ref to remove is always the oldest ref
     */
    if(eCodec == AL_CODEC_AVC && pGopParam->uGopLength > 1)
      iNumRef++;
  }
  /*
   * The structure of the default gop makes it that:
   *
   * When there is no reordering (no B frames) We only need one reference buffer
   * at a time
   *  ,-,,-,,-,
   * v  |v |v |
   * I  P  P  P ....
   *
   * When there are B frames, we need two reference buffer at a time
   *
   *   -
   * ,   `--
   * v   | v
   * I B B P B B ...
   * 0 2 3 1
   */
  else
  {
    iNumRef = 1;

    if(pGopParam->uNumB > 0 || (pGopParam->eMode & AL_GOP_FLAG_B_ONLY))
      ++iNumRef;
  }

  /*
   * We need an extra buffer to keep the long term picture while we deal
   * with the other pictures as normal
   */
  if(pGopParam->bEnableLT)
    ++iNumRef;

  return iNumRef;
}

#define NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel) (((uNumBForPyrLevel) << 1) + 1)
/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxRef_GopMngrCustom(const AL_TGopParam* pGopParam, AL_ECodec eCodec)
{
  uint8_t uNumRef;

  uint8_t uPyrLevel = 0;
  uint8_t uNumBForPyrLevel = 1;
  uint8_t uNumBForNextPyrLevel = NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel);

  while(pGopParam->uNumB >= uNumBForNextPyrLevel)
  {
    uNumBForPyrLevel = uNumBForNextPyrLevel;
    uNumBForNextPyrLevel = NEXT_PYR_LEVEL_NUMB(uNumBForPyrLevel);
    uPyrLevel++;
  }

  uNumRef = uPyrLevel + 2; // Right B frames + 2 P frames

  /* Add 1 in AVC for the current frame */
  if(eCodec == AL_CODEC_AVC)
    uNumRef++;

  if(pGopParam->bEnableLT)
    uNumRef++;

  return uNumRef;
}

/****************************************************************************/
uint8_t AL_DPBConstraint_GetMaxDPBSize(const AL_TEncChanParam* pChParam)
{
  bool bIsAOM = false;
  bool bIsLookAhead = false;
  AL_EGopMngrType eGopMngrType = AL_GetGopMngrType(pChParam->tGopParam.eMode, bIsAOM, bIsLookAhead);
  AL_ECodec eCodec = AL_GET_CODEC(pChParam->eProfile);
  uint8_t uDPBSize = 0;
  switch(eGopMngrType)
  {
  case AL_GOP_MNGR_DEFAULT:
    uDPBSize = AL_DPBConstraint_GetMaxRef_DefaultGopMngr(&pChParam->tGopParam, eCodec);
    break;
  case AL_GOP_MNGR_CUSTOM:
    uDPBSize = AL_DPBConstraint_GetMaxRef_GopMngrCustom(&pChParam->tGopParam, eCodec);
    break;
  case AL_GOP_MNGR_MAX_ENUM:
    break;
  }

  /* reconstructed buffer is an actual part of the dpb algorithm in hevc */
  if(eCodec == AL_CODEC_HEVC)
    uDPBSize++;

  return uDPBSize;
}

/****************************************************************************/
AL_EGopMngrType AL_GetGopMngrType(AL_EGopCtrlMode eMode, bool bIsAom, bool bIsLookAhead)
{
  (void)bIsAom;
  (void)bIsLookAhead;

  if(eMode == AL_GOP_MODE_ADAPTIVE || (eMode & AL_GOP_FLAG_LOW_DELAY))
    return AL_GOP_MNGR_DEFAULT;
  else if(eMode & AL_GOP_FLAG_PYRAMIDAL)
    return AL_GOP_MNGR_CUSTOM;
  else if(eMode & AL_GOP_FLAG_DEFAULT)
  {
    return AL_GOP_MNGR_DEFAULT;
  }

  return AL_GOP_MNGR_MAX_ENUM;
}

