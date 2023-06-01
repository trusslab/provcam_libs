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
   \addtogroup lib_base
   @{
   \file
 *****************************************************************************/

#include <cstring>
#include <cassert>
#include <iostream>

extern "C" {
#include "lib_rtos/lib_rtos.h"
#include "lib_common/BufCommon.h"
#include "lib_common/PixMapBuffer.h"
#include "lib_common/BufferAPI.h"
}

#include "convert.h"

static inline uint8_t RND_10B_TO_8B(uint16_t val)
{
  return (uint8_t)(((val) >= 0x3FC) ? 0xFF : (((val) + 2) >> 2));
}

/****************************************************************************/
void I420_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
static void SwapChromaComponents(AL_TBuffer const* pSrc, AL_TBuffer* pDst, TFourCC tDestFourCC)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, tDestFourCC);

  // Luma
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; iH++)
  {
    memcpy(pDstData, pSrcData, tDim.iWidth);
    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }

  // Chroma
  iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iChromaWidth = tDim.iWidth >> 1;
  int iChromaHeight = tDim.iHeight >> 1;
  int iChromaOffsetSrc = iPitchSrc * iChromaHeight;
  int iChromaOffsetDst = iPitchDst * iChromaHeight;

  for(int iH = 0; iH < iChromaHeight; iH++)
  {
    memcpy(pDstData, pSrcData + iChromaOffsetSrc, iChromaWidth); // Chroma U
    memcpy(pDstData + iChromaOffsetDst, pSrcData, iChromaWidth); // Chroma V
    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }
}

/****************************************************************************/
void I420_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SwapChromaComponents(pSrc, pDst, FOURCC(YV12));
}

/****************************************************************************/
void I420_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  // Luma
  AL_VADDR pSrcY = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  AL_VADDR pDstY = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, tDim.iWidth);
    pSrcY += iPitchSrc;
    pDstY += iPitchDst;
  }
}

/****************************************************************************/
void I420_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));

  // Luma
  uint8_t* pSrcY = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pDstY = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    for(int iW = 0; iW < tDim.iWidth; ++iW)
      *pDstY++ = ((uint16_t)*pSrcY++) << 2;

    pSrcY += iPitchSrc - tDim.iWidth;
    pDstY += iPitchDst - tDim.iWidth;
  }
}

/****************************************************************************/
static void I4XX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));

  // Luma
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; iH++)
  {
    for(int iW = 0; iW < tDim.iWidth; iW++)
      *pDstData++ = ((uint16_t)*pSrcData++) << 2;

    pSrcData += iPitchSrc - tDim.iWidth;
    pDstData += iPitchDst - tDim.iWidth;
  }

  // Chroma
  iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);
  pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iWidthChroma = tDim.iWidth / iHScale;
  int iHeightChroma = (tDim.iHeight * 2) / iVScale;

  for(int iH = 0; iH < iHeightChroma /* U then V component */; iH++)
  {
    for(int iW = 0; iW < iWidthChroma; iW++)
      *pDstData++ = ((uint16_t)*pSrcData++) << 2;

    pSrcData += iPitchSrc - iWidthChroma;
    pDstData += iPitchDst - iWidthChroma;
  }
}

/****************************************************************************/
void I420_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
  I4XX_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I444_To_I4AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I4AL));
  I4XX_To_IXAL(pSrc, pDst, 1, 1);
}

/****************************************************************************/
void IYUV_To_420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  CopyPixMapBuffer(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_YV12(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void IYUV_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I420_To_I0AL(pSrc, pDst);
}

/****************************************************************************/
void YV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SwapChromaComponents(pSrc, pDst, FOURCC(I420));
}

/****************************************************************************/
void YV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  YV12_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void YV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  int iSize = tDim.iWidth * tDim.iHeight;

  // Luma
  memcpy(AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y), AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y), iSize);
}

/****************************************************************************/
void YV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));

  const int iLumaSize = tDim.iWidth * tDim.iHeight;
  const int iChromaSize = iLumaSize >> 2;

  // Luma
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  int iSize = iLumaSize;

  while(iSize--)
    *pBufOut++ = ((uint16_t)*pBufIn++) << 2;

  // Chroma
  uint8_t* pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pBufInU = pBufInV + iChromaSize;
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pBufOutV = pBufOutU + iChromaSize;
  iSize = iChromaSize;

  while(iSize--)
  {
    *pBufOutU++ = ((uint16_t)*pBufInU++) << 2;
    *pBufOutV++ = ((uint16_t)*pBufInV++) << 2;
  }
}

/****************************************************************************/
void NV12_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  int iSize = tDim.iWidth * tDim.iHeight;

  // Luma
  memcpy(AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y), AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y), iSize);
}

/****************************************************************************/
void Y800_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));

  int iSize = tDim.iWidth * tDim.iHeight;

  // Luma
  memcpy(AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y), AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y), iSize);

  // Chroma
  Rtos_Memset(AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV), 0x80, iSize >> 1);
}

/****************************************************************************/
void Y800_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void Y800_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void Y800_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void Y800_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));

  const int iLumaSize = tDim.iWidth * tDim.iHeight;

  // Luma
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  int iSize = iLumaSize;

  while(iSize--)
    *pBufOut++ = ((uint16_t)*pBufIn++) << 2;

  // Chroma
  pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  iSize = iLumaSize >> 1;

  while(iSize--)
    *pBufOut++ = 0x0200;
}

/****************************************************************************/
void Y800_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Y800_To_P010(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void Y800_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));

  int iPitchSrcY = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDstY = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  assert(iPitchDstY % 4 == 0);
  assert(iPitchDstY >= (tDim.iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * iPitchSrcY);
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iPitchDstY);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
      *pDst32 |= ((uint32_t)*pSrcY++) << 22;
      ++pDst32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
      *pDst32 |= ((uint32_t)*pSrcY++) << 12;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcY++) << 2;
    }
  }
}

/****************************************************************************/
void Y800_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));

  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  int iSrcPitchLuma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iDstPitchLuma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    for(int iW = 0; iW < tDim.iWidth; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += iSrcPitchLuma;
    pBufOut += iDstPitchLuma;
  }
}

/****************************************************************************/
static uint32_t toTen(uint8_t sample)
{
  return (((uint32_t)sample) << 2) & 0x3FF;
}

/****************************************************************************/
void Y800_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV10));

  int iPitchSrcY = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDstY = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  assert(iPitchDstY % 4 == 0);
  assert(iPitchDstY >= (tDim.iWidth + 2) / 3 * 4);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint8_t* pSrcY = (uint8_t*)(pSrcData + h * iPitchSrcY);
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iPitchDstY);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
      *pDst32 |= toTen(*pSrcY++) << 20;
      ++pDst32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDst32 = toTen(*pSrcY++);
      *pDst32 |= toTen(*pSrcY++) << 10;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDst32 = toTen(*pSrcY++);
    }
  }
}

/****************************************************************************/
static void SemiPlanar_To_XV_OneComponent(AL_TBuffer const* pSrcBuf, AL_TBuffer* pDstBuf, bool bProcessY, int iVrtScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrcBuf);

  int iDstHeight = tDim.iHeight / iVrtScale;

  AL_EPlaneId ePlaneID = bProcessY ? AL_PLANE_Y : AL_PLANE_UV;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrcBuf, ePlaneID);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, ePlaneID);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrcBuf, ePlaneID);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, ePlaneID);

  assert(iPitchDst % 4 == 0);

  if(bProcessY)
  {
    assert(iPitchDst >= (tDim.iWidth + 2) / 3 * 4);
  }

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + h * iPitchSrc);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 20;
      ++pDst;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
      *pDst |= ((uint32_t)(*pSrc++) & 0x3FF) << 10;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDst = ((uint32_t)(*pSrc++) & 0x3FF);
    }
  }
}

/****************************************************************************/
void Y010_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV10));
}

/****************************************************************************/
void Y010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
static void PX10_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
    uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
    uint32_t uSrcPitchLuma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
    uint32_t uDstPitchLuma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

    for(int iH = 0; iH < tDim.iHeight; ++iH)
    {
      for(int iW = 0; iW < tDim.iWidth; ++iW)
        pBufOut[iW] = (uint8_t)((2 + pBufIn[iW]) >> 2);

      pBufIn += uSrcPitchLuma;
      pBufOut += uDstPitchLuma;
    }
  }
  // Chroma
  {
    uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
    uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

    int iWidth = tDim.iWidth / uHrzCScale;
    int iHeight = tDim.iHeight / uVrtCScale;

    uint16_t* pBufInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    uint8_t* pBufOutU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
    uint8_t* pBufOutV = pBufOutU + uPitchDst * iHeight;

    for(int iH = 0; iH < iHeight; ++iH)
    {
      for(int iW = 0; iW < iWidth; ++iW)
      {
        pBufOutU[iW] = (uint8_t)((2 + pBufInC[(iW << 1)]) >> 2);
        pBufOutV[iW] = (uint8_t)((2 + pBufInC[(iW << 1) + 1]) >> 2);
      }

      pBufInC += uPitchSrc;
      pBufOutU += uPitchDst;
      pBufOutV += uPitchDst;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(I422) : FOURCC(I420));
}

/****************************************************************************/
void P010_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_I42X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void P210_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_I42X(pSrc, pDst, 2, 1);
}

void P210_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 1);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
void P010_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_I42X(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
static void PX10_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  P010_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iSizeDst = tDim.iWidth * tDim.iHeight;
  int iCScale = uHrzCScale * uVrtCScale;

  uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pBufOutV = pBufOutU + (iSizeDst / iCScale);

  uint32_t uSrcPitchChroma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidth = tDim.iWidth / uHrzCScale;
  int iHeight = tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = pBufIn[(iW << 1)];
      pBufOutV[iW] = pBufIn[(iW << 1) + 1];
    }

    pBufIn += uSrcPitchChroma;
    pBufOutU += uDstPitchChroma;
    pBufOutV += uDstPitchChroma;
  }

  AL_PixMapBuffer_SetFourCC(pDst, iCScale == 2 ? FOURCC(I2AL) : FOURCC(I0AL));
}

/****************************************************************************/
void P010_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void P210_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  PX10_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void P010_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));

  const int iLumaSize = tDim.iWidth * tDim.iHeight;
  const int iChromaSize = iLumaSize >> 2;

  // Luma
  {
    uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
    uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
    int iSize = iLumaSize;

    while(iSize--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);
  }

  // Chroma
  {
    uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    uint8_t* pBufOutV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
    uint8_t* pBufOutU = pBufOutV + iChromaSize;
    int iSize = iChromaSize;

    while(iSize--)
    {
      *pBufOutU++ = (uint8_t)((2 + *pBufIn++) >> 2);
      *pBufOutV++ = (uint8_t)((2 + *pBufIn++) >> 2);
    }
  }
}

/****************************************************************************/
void P010_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  {
    int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
    int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

    uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
    uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

    for(int iH = 0; iH < tDim.iHeight; ++iH)
    {
      for(int iW = 0; iW < tDim.iWidth; ++iW)
        *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

      pBufIn += iPitchSrc - tDim.iWidth;
      pBufOut += iPitchDst - tDim.iWidth;
    }
  }

  // Chroma
  {
    int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
    int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

    uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
    int iHeightChroma = tDim.iHeight >> 1;

    for(int iH = 0; iH < iHeightChroma; ++iH)
    {
      for(int iW = 0; iW < tDim.iWidth; ++iW)
        *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

      pBufIn += iPitchSrc - tDim.iWidth;
      pBufOut += iPitchDst - tDim.iWidth;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void P010_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void P010_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void P010_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, true, 1);
  SemiPlanar_To_XV_OneComponent(pSrc, pDst, false, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
static void IXAL_To_I4XX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHScale, int iVScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint32_t uSrcPitchChroma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  int iH = (tDim.iHeight * 2) / iVScale;

  int iCWidth = tDim.iWidth / iHScale;

  while(iH--)
  {
    int iW = iCWidth;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufOut += uDstPitchChroma - iCWidth;
    pBufIn += uSrcPitchChroma - iCWidth;
  }
}

/****************************************************************************/
void I0AL_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_I4XX(pSrc, pDst, 2, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void I4AL_To_I444(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_I4XX(pSrc, pDst, 1, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I444));
}

/****************************************************************************/
void I0AL_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I0AL_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void I0AL_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  uint32_t uSrcPitchLuma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
  uint32_t uDstPitchLuma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  int iH = tDim.iHeight;

  while(iH--)
  {
    int iW = tDim.iWidth;

    while(iW--)
      *pBufOut++ = (uint8_t)((2 + *pBufIn++) >> 2);

    pBufIn += uSrcPitchLuma - tDim.iWidth;
    pBufOut += uDstPitchLuma - tDim.iWidth;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void I0AL_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // luma
  uint16_t* pBufIn = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  uint32_t uSrcPitchLuma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
  uint32_t uDstPitchLuma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  int iH = tDim.iHeight;

  while(iH--)
  {
    memcpy(pBufOut, pBufIn, tDim.iWidth * sizeof(uint16_t));
    pBufOut += uDstPitchLuma;
    pBufIn += uSrcPitchLuma;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
static void I42X_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  AL_VADDR pSrcY = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  AL_VADDR pDstY = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    Rtos_Memcpy(pDstY, pSrcY, tDim.iWidth);
    pDstY += iPitchDst;
    pSrcY += iPitchSrc;
  }

  // Chroma
  iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iCScale = uHrzCScale * uVrtCScale;
  int iChromaSecondCompOffset = iPitchSrc * tDim.iHeight / uVrtCScale;
  AL_VADDR pBufInU = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV) + (bIsUFirst ? 0 : iChromaSecondCompOffset);
  AL_VADDR pBufInV = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV) + (bIsUFirst ? iChromaSecondCompOffset : 0);
  AL_VADDR pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iHeightC = tDim.iHeight / uVrtCScale;
  int iWidthC = tDim.iWidth / uHrzCScale;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[iW * 2] = pBufInU[iW];
      pBufOut[iW * 2 + 1] = pBufInV[iW];
    }

    pBufOut += iPitchDst;
    pBufInU += iPitchSrc;
    pBufInV += iPitchSrc;
  }

  AL_PixMapBuffer_SetFourCC(pDst, iCScale == 2 ? FOURCC(NV16) : FOURCC(NV12));
}

/****************************************************************************/
void YV12_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void I420_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void I42X_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidthC = tDim.iWidth / uHrzCScale;
  int iHeightC = tDim.iHeight / uVrtCScale;
  int iChromaSizeSrc = iPitchSrc * iHeightC;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pBufInU = pSrcData + (bIsUFirst ? 0 : iChromaSizeSrc);
  uint8_t* pBufInV = pSrcData + (bIsUFirst ? iChromaSizeSrc : 0);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = ((uint16_t)*pBufInU++) << 2;
      *pBufOut++ = ((uint16_t)*pBufInV++) << 2;
    }

    pBufInU += iPitchSrc - iWidthC;
    pBufInV += iPitchSrc - iWidthC;
    pBufOut += iPitchDst - (2 * iWidthC);
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(P210) : FOURCC(P010));
}

/****************************************************************************/
void YV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void I420_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void I42X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, bool bIsUFirst = true)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iSrcPitch = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iDstPitch = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  assert(iDstPitch % 4 == 0);
  assert(iDstPitch >= (tDim.iWidth + 2) / 3 * 4);

  // Chroma
  int iHeightC = tDim.iHeight / uVrtCScale;
  int iSrcSizeC = iHeightC * iSrcPitch;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iDstPitch);
    uint8_t* pSrcFirstCComp = pSrcData + h * iSrcPitch;
    uint8_t* pSrcU = (uint8_t*)(pSrcFirstCComp + (bIsUFirst ? 0 : iSrcSizeC));
    uint8_t* pSrcV = (uint8_t*)(pSrcFirstCComp + (bIsUFirst ? iSrcSizeC : 0));

    int w = tDim.iWidth / 6;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
      *pDst32 |= ((uint32_t)*pSrcU++) << 12;
      *pDst32 |= ((uint32_t)*pSrcV++) << 22;
      ++pDst32;
    }

    if(tDim.iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
      *pDst32 |= ((uint32_t)*pSrcU++) << 22;
      ++pDst32;
      *pDst32 = ((uint32_t)*pSrcV++) << 2;
    }
    else if(tDim.iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcU++) << 2;
      *pDst32 |= ((uint32_t)*pSrcV++) << 12;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void YV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2, false);
}

/****************************************************************************/
void I420_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I422_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void IYUV_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  I42X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
static void IXAL_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // luma
  I0AL_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iHeightC = tDim.iHeight / uVrtCScale;
  int iWidthC = tDim.iWidth / uHrzCScale;
  uint32_t uSrcPitchChroma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint16_t* pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufInV = pBufInU + uSrcPitchChroma * iHeightC;
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      *pBufOut++ = (uint8_t)((2 + *pBufInU++) >> 2);
      *pBufOut++ = (uint8_t)((2 + *pBufInV++) >> 2);
    }

    pBufOut += uDstPitchChroma - tDim.iWidth;
    pBufInU += uSrcPitchChroma - iWidthC;
    pBufInV += uSrcPitchChroma - iWidthC;
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(NV16) : FOURCC(NV12));
}

/****************************************************************************/
void I0AL_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void IXAL_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  I0AL_To_Y010(pSrc, pDst);

  // Chroma
  int iHeightC = tDim.iHeight / uVrtCScale;
  int iWidthC = tDim.iWidth / uHrzCScale;
  uint32_t uSrcPitchChroma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pBufInU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufInV = pBufInU + uSrcPitchChroma * iHeightC;

  for(int iH = 0; iH < iHeightC; ++iH)
  {
    for(int iW = 0; iW < iWidthC; ++iW)
    {
      pBufOut[2 * iW] = pBufInU[iW];
      pBufOut[2 * iW + 1] = pBufInV[iW];
    }

    pBufInU += uSrcPitchChroma;
    pBufInV += uSrcPitchChroma;
    pBufOut += uDstPitchChroma;
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(P210) : FOURCC(P010));
}

/****************************************************************************/
void I0AL_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void IXAL_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y010_To_XV10(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  uint32_t uSrcPitchChroma = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  uint32_t uDstPitchChroma = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  assert(uDstPitchChroma % 4 == 0);
  assert(uDstPitchChroma >= (uint32_t)(tDim.iWidth + 2) / 3 * 4);

  int iHeightC = tDim.iHeight / uVrtCScale;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * uDstPitchChroma);
    uint16_t* pSrcU = (uint16_t*)(pSrcData + h * uSrcPitchChroma);
    uint16_t* pSrcV = pSrcU + (uSrcPitchChroma / sizeof(uint16_t)) * iHeightC;

    int w = tDim.iWidth / 6;

    while(w--)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 20;
      ++pDst32;
    }

    if(tDim.iWidth % 6 > 2)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
      *pDst32 |= ((uint32_t)(*pSrcU++) & 0x3FF) << 20;
      ++pDst32;
      *pDst32 = ((uint32_t)(*pSrcV++) & 0x3FF);
    }
    else if(tDim.iWidth % 6 > 0)
    {
      *pDst32 = ((uint32_t)(*pSrcU++) & 0x3FF);
      *pDst32 |= ((uint32_t)(*pSrcV++) & 0x3FF) << 10;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void I0AL_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void I2AL_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  IXAL_To_XVXX(pSrc, pDst, 2, 1);
}

void ALX8_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, int iHeightC, bool bIsUFirst = true)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  const int iTileW = 64;
  const int iTileH = 4;

  uint32_t uPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);

  uint32_t uPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iOffsetSecondChromaComp = uPitchDst * iHeightC;
  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bIsUFirst ? 0 : iOffsetSecondChromaComp);
  uint8_t* pDstDataV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bIsUFirst ? iOffsetSecondChromaComp : 0);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * uPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 4)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 4)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstDataU + (H + h) * uPitchDst + (W + w) / 2;
          uint8_t* pOutV = pDstDataV + (H + h) * uPitchDst + (W + w) / 2;

          pOutU[0] = pInC[0];
          pOutV[0] = pInC[1];
          pOutU[1] = pInC[2];
          pOutV[1] = pInC[3];
          pOutU += uPitchDst;
          pOutV += uPitchDst;
          pOutU[0] = pInC[4];
          pOutV[0] = pInC[5];
          pOutU[1] = pInC[6];
          pOutV[1] = pInC[7];

          if(H + h + 4 <= iHeightC)
          {
            pOutU += uPitchDst;
            pOutV += uPitchDst;
            pOutU[0] = pInC[8];
            pOutV[0] = pInC[9];
            pOutU[1] = pInC[10];
            pOutV[1] = pInC[11];
            pOutU += uPitchDst;
            pOutV += uPitchDst;
            pOutU[0] = pInC[12];
            pOutV[0] = pInC[13];
            pOutU[1] = pInC[14];
            pOutV[1] = pInC[15];
          }
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }
}

/****************************************************************************/
void T608_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iHeightC = tDim.iHeight >> 1;
  ALX8_To_I42X(pSrc, pDst, iHeightC);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
void T608_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T608_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void T608_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iHeightC = tDim.iHeight >> 1;
  ALX8_To_I42X(pSrc, pDst, iHeightC, false);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void T608_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;

  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iHeightC = tDim.iHeight >> 1;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);

          pOutC[0] = pInC[0];
          pOutC[1] = pInC[1];
          pOutC[2] = pInC[2];
          pOutC[3] = pInC[3];
          pOutC += iPitchDst;
          pOutC[0] = pInC[4];
          pOutC[1] = pInC[5];
          pOutC[2] = pInC[6];
          pOutC[3] = pInC[7];
          pOutC += iPitchDst;
          pOutC[0] = pInC[8];
          pOutC[1] = pInC[9];
          pOutC[2] = pInC[10];
          pOutC[3] = pInC[11];
          pOutC += iPitchDst;
          pOutC[0] = pInC[12];
          pOutC[1] = pInC[13];
          pOutC[2] = pInC[14];
          pOutC[3] = pInC[15];
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T608_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutY = pDstData + (H + h) * iPitchDst + (W + w);

          pOutY[0] = pInY[0];
          pOutY[1] = pInY[1];
          pOutY[2] = pInY[2];
          pOutY[3] = pInY[3];
          pOutY += iPitchDst;
          pOutY[0] = pInY[4];
          pOutY[1] = pInY[5];
          pOutY[2] = pInY[6];
          pOutY[3] = pInY[7];
          pOutY += iPitchDst;
          pOutY[0] = pInY[8];
          pOutY[1] = pInY[9];
          pOutY[2] = pInY[10];
          pOutY[3] = pInY[11];
          pOutY += iPitchDst;
          pOutY[0] = pInY[12];
          pOutY[1] = pInY[13];
          pOutY[2] = pInY[14];
          pOutY[3] = pInY[15];
          pInY += 16;
        }

        pInY += 4 * iCropW;
      }

      pInY += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T608_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  // Luma
  const int iTileW = 64;
  const int iTileH = 4;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint8_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutY = ((uint16_t*)pDstData) + (H + h) * iPitchDst + (W + w);

          pOutY[0] = ((uint16_t)pInY[0]) << 2;
          pOutY[1] = ((uint16_t)pInY[1]) << 2;
          pOutY[2] = ((uint16_t)pInY[2]) << 2;
          pOutY[3] = ((uint16_t)pInY[3]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[4]) << 2;
          pOutY[1] = ((uint16_t)pInY[5]) << 2;
          pOutY[2] = ((uint16_t)pInY[6]) << 2;
          pOutY[3] = ((uint16_t)pInY[7]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[8]) << 2;
          pOutY[1] = ((uint16_t)pInY[9]) << 2;
          pOutY[2] = ((uint16_t)pInY[10]) << 2;
          pOutY[3] = ((uint16_t)pInY[11]) << 2;
          pOutY += iPitchDst;
          pOutY[0] = ((uint16_t)pInY[12]) << 2;
          pOutY[1] = ((uint16_t)pInY[13]) << 2;
          pOutY[2] = ((uint16_t)pInY[14]) << 2;
          pOutY[3] = ((uint16_t)pInY[15]) << 2;
          pInY += 16;
        }

        pInY += 4 * iCropW;
      }

      pInY += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T608_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iHeightC = tDim.iHeight >> 1;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = ((uint16_t*)pDstData) + (H + h) * iPitchDst + (W + w);

          pOutC[0] = ((uint16_t)pInC[0]) << 2;
          pOutC[1] = ((uint16_t)pInC[1]) << 2;
          pOutC[2] = ((uint16_t)pInC[2]) << 2;
          pOutC[3] = ((uint16_t)pInC[3]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[4]) << 2;
          pOutC[1] = ((uint16_t)pInC[5]) << 2;
          pOutC[2] = ((uint16_t)pInC[6]) << 2;
          pOutC[3] = ((uint16_t)pInC[7]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[8]) << 2;
          pOutC[1] = ((uint16_t)pInC[9]) << 2;
          pOutC[2] = ((uint16_t)pInC[10]) << 2;
          pOutC[3] = ((uint16_t)pInC[11]) << 2;
          pOutC += iPitchDst;
          pOutC[0] = ((uint16_t)pInC[12]) << 2;
          pOutC[1] = ((uint16_t)pInC[13]) << 2;
          pOutC[2] = ((uint16_t)pInC[14]) << 2;
          pOutC[3] = ((uint16_t)pInC[15]) << 2;
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void T608_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iHeightC = tDim.iHeight >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pDstDataV = pDstDataU + (iPitchDst * iHeightC);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint8_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = pDstDataU + (H + h) * iPitchDst + (W + w) / 2;
          uint16_t* pOutV = pDstDataV + (H + h) * iPitchDst + (W + w) / 2;

          pOutU[0] = ((uint16_t)pInC[0]) << 2;
          pOutV[0] = ((uint16_t)pInC[1]) << 2;
          pOutU[1] = ((uint16_t)pInC[2]) << 2;
          pOutV[1] = ((uint16_t)pInC[3]) << 2;
          pOutU += iPitchDst;
          pOutV += iPitchDst;
          pOutU[0] = ((uint16_t)pInC[4]) << 2;
          pOutV[0] = ((uint16_t)pInC[5]) << 2;
          pOutU[1] = ((uint16_t)pInC[6]) << 2;
          pOutV[1] = ((uint16_t)pInC[7]) << 2;
          pOutU += iPitchDst;
          pOutV += iPitchDst;
          pOutU[0] = ((uint16_t)pInC[8]) << 2;
          pOutV[0] = ((uint16_t)pInC[9]) << 2;
          pOutU[1] = ((uint16_t)pInC[10]) << 2;
          pOutV[1] = ((uint16_t)pInC[11]) << 2;
          pOutU += iPitchDst;
          pOutV += iPitchDst;
          pOutU[0] = ((uint16_t)pInC[12]) << 2;
          pOutV[0] = ((uint16_t)pInC[13]) << 2;
          pOutU[1] = ((uint16_t)pInC[14]) << 2;
          pOutV[1] = ((uint16_t)pInC[15]) << 2;
          pInC += 16;
        }

        pInC += 4 * iCropW;
      }

      pInC += iCropH * iTileW;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T6m8_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  int iSizeC = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_MAP_UV) * tDim.iHeight;

  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  Rtos_Memset(AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_MAP_UV), 0x80, iSizeC);

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I420));
}

/****************************************************************************/
static uint16_t getTile10BitVal(int w, int hInsideTile, uint16_t* pSrc)
{
  int offset = ((w >> 2) << 4) + ((hInsideTile & 0x3) << 2) + (w & 0x3); // (w / 4) * 16 + (hInsideTile % 4) * 4 + w % 4;
  int u16offset = (offset * 5) >> 3; // offset * 10 / (8 * sizeof(uint16_t));
  int bitOffset = (offset * 10) & 0xF; // (offset * 10) % (8 * sizeof(uint16_t));
  int remainingBit = bitOffset - 6; // 10 + bitOffset - (8 * sizeof(uint16_t));

  pSrc += u16offset;
  uint16_t result = ((*pSrc) >> bitOffset);

  if(remainingBit > 0)
  {
    result |= (*(pSrc + 1)) << (10 - remainingBit);
  }

  result &= 0x3FF;

  return result;
}

/****************************************************************************/
static void Tile_To_XV_OneComponent(AL_TBuffer const* pSrcBuf, AL_TBuffer* pDstBuf, bool bProcessY, int iVrtScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrcBuf);

  int iDstHeight = tDim.iHeight / iVrtScale;

  AL_EPlaneId ePlaneID = bProcessY ? AL_PLANE_Y : AL_PLANE_UV;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrcBuf, ePlaneID);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDstBuf, ePlaneID);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrcBuf, ePlaneID);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDstBuf, ePlaneID);

  assert(iPitchDst % 4 == 0);

  if(bProcessY)
  {
    assert(iPitchDst >= (tDim.iWidth + 2) / 3 * 4);
  }

  for(int h = 0; h < iDstHeight; h++)
  {
    uint32_t* pDst = (uint32_t*)(pDstData + h * iPitchDst);
    uint16_t* pSrc = (uint16_t*)(pSrcData + (h >> 2) * iPitchSrc);

    int hInsideTile = h & 0x3;

    int w = 0;
    int wStop = tDim.iWidth - 2;

    while(w < wStop)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 20;
      ++pDst;
    }

    if(w < tDim.iWidth)
    {
      *pDst = getTile10BitVal(w++, hInsideTile, pSrc);

      if(w < tDim.iWidth)
      {
        *pDst |= getTile10BitVal(w++, hInsideTile, pSrc) << 10;
      }
    }
  }
}

/****************************************************************************/
void T628_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T628_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T628_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);
  ALX8_To_I42X(pSrc, pDst, tDim.iHeight);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void T628_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pDst);

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pOutC = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 4);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      pOutC[0] = pInC[0];
      pOutC[1] = pInC[1];
      pOutC[2] = pInC[2];
      pOutC[3] = pInC[3];
      pOutC += iPitchDst;
      pOutC[0] = pInC[4];
      pOutC[1] = pInC[5];
      pOutC[2] = pInC[6];
      pOutC[3] = pInC[7];
      pOutC += iPitchDst;
      pOutC[0] = pInC[8];
      pOutC[1] = pInC[9];
      pOutC[2] = pInC[10];
      pOutC[3] = pInC[11];
      pOutC += iPitchDst;
      pOutC[0] = pInC[12];
      pOutC[1] = pInC[13];
      pOutC[2] = pInC[14];
      pOutC[3] = pInC[15];
      pOutC -= 3 * iPitchDst - 4;
      pInC += 16;
    }

    pOutC += iPitchDst * 4 - tDim.iWidth;
    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void T628_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 4);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pDstDataV = pDstDataU + iPitchDst * tDim.iHeight;

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pOutU = pDstDataU + h * iPitchDst + w / 2;
      uint16_t* pOutV = pDstDataV + h * iPitchDst + w / 2;

      pOutU[0] = ((uint16_t)pInC[0]) << 2;
      pOutV[0] = ((uint16_t)pInC[1]) << 2;
      pOutU[1] = ((uint16_t)pInC[2]) << 2;
      pOutV[1] = ((uint16_t)pInC[3]) << 2;
      pOutU += iPitchDst;
      pOutV += iPitchDst;
      pOutU[0] = ((uint16_t)pInC[4]) << 2;
      pOutV[0] = ((uint16_t)pInC[5]) << 2;
      pOutU[1] = ((uint16_t)pInC[6]) << 2;
      pOutV[1] = ((uint16_t)pInC[7]) << 2;
      pOutU += iPitchDst;
      pOutV += iPitchDst;
      pOutU[0] = ((uint16_t)pInC[8]) << 2;
      pOutV[0] = ((uint16_t)pInC[9]) << 2;
      pOutU[1] = ((uint16_t)pInC[10]) << 2;
      pOutV[1] = ((uint16_t)pInC[11]) << 2;
      pOutU += iPitchDst;
      pOutV += iPitchDst;
      pOutU[0] = ((uint16_t)pInC[12]) << 2;
      pOutV[0] = ((uint16_t)pInC[13]) << 2;
      pOutU[1] = ((uint16_t)pInC[14]) << 2;
      pOutV[1] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T628_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T608_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iJump = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 4);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint8_t* pInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pOutC = pDstData + h * iPitchDst + w;

      pOutC[0] = ((uint16_t)pInC[0]) << 2;
      pOutC[1] = ((uint16_t)pInC[1]) << 2;
      pOutC[2] = ((uint16_t)pInC[2]) << 2;
      pOutC[3] = ((uint16_t)pInC[3]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[4]) << 2;
      pOutC[1] = ((uint16_t)pInC[5]) << 2;
      pOutC[2] = ((uint16_t)pInC[6]) << 2;
      pOutC[3] = ((uint16_t)pInC[7]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[8]) << 2;
      pOutC[1] = ((uint16_t)pInC[9]) << 2;
      pOutC[2] = ((uint16_t)pInC[10]) << 2;
      pOutC[3] = ((uint16_t)pInC[11]) << 2;
      pOutC += iPitchDst;
      pOutC[0] = ((uint16_t)pInC[12]) << 2;
      pOutC[1] = ((uint16_t)pInC[13]) << 2;
      pOutC[2] = ((uint16_t)pInC[14]) << 2;
      pOutC[3] = ((uint16_t)pInC[15]) << 2;
      pInC += 16;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4Block10b(uint16_t* pTiled4x4, TUntiled* pUntiled, int iUntiledPitch, FConvert convert)
{
  pUntiled[0] = convert(pTiled4x4[0] & 0x3FF);
  pUntiled[1] = convert(((pTiled4x4[0] >> 10) | (pTiled4x4[1] << 6)) & 0x3FF);
  pUntiled[2] = convert((pTiled4x4[1] >> 4) & 0x3FF);
  pUntiled[3] = convert(((pTiled4x4[1] >> 14) | (pTiled4x4[2] << 2)) & 0x3FF);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(((pTiled4x4[2] >> 8) | (pTiled4x4[3] << 8)) & 0x3FF);
  pUntiled[1] = convert((pTiled4x4[3] >> 2) & 0x3FF);
  pUntiled[2] = convert(((pTiled4x4[3] >> 12) | (pTiled4x4[4] << 4)) & 0x3FF);
  pUntiled[3] = convert(pTiled4x4[4] >> 6);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(pTiled4x4[5] & 0x3FF);
  pUntiled[1] = convert(((pTiled4x4[5] >> 10) | (pTiled4x4[6] << 6)) & 0x3FF);
  pUntiled[2] = convert((pTiled4x4[6] >> 4) & 0x3FF);
  pUntiled[3] = convert(((pTiled4x4[6] >> 14) | (pTiled4x4[7] << 2)) & 0x3FF);
  pUntiled += iUntiledPitch;
  pUntiled[0] = convert(((pTiled4x4[7] >> 8) | (pTiled4x4[8] << 8)) & 0x3FF);
  pUntiled[1] = convert((pTiled4x4[8] >> 2) & 0x3FF);
  pUntiled[2] = convert(((pTiled4x4[8] >> 12) | (pTiled4x4[9] << 4)) & 0x3FF);
  pUntiled[3] = convert(pTiled4x4[9] >> 6);
}

/****************************************************************************/
static void Untile4x4Block10bTo8b(uint16_t* pTiled4x4, uint8_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint8_t>(pTiled4x4, pUntiled, iUntiledPitch, RND_10B_TO_8B);
}

/****************************************************************************/
static void Untile4x4Block10bTo10b(uint16_t* pTiled4x4, uint16_t* pUntiled, int iUntiledPitch)
{
  Untile4x4Block10b<uint16_t>(pTiled4x4, pUntiled, iUntiledPitch, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
template<typename TUntiled, typename FConvert>
static void Untile4x4ChromaBlock10bToPlanar(uint16_t* pTiled4x4, TUntiled* pUntiledU, TUntiled* pUntiledV, int iUntiledPitch, FConvert convert)
{
  pUntiledU[0] = convert(pTiled4x4[0] & 0x3FF);
  pUntiledV[0] = convert(((pTiled4x4[0] >> 10) | (pTiled4x4[1] << 6)) & 0x3FF);
  pUntiledU[1] = convert((pTiled4x4[1] >> 4) & 0x3FF);
  pUntiledV[1] = convert(((pTiled4x4[1] >> 14) | (pTiled4x4[2] << 2)) & 0x3FF);
  pUntiledU += iUntiledPitch;
  pUntiledV += iUntiledPitch;
  pUntiledU[0] = convert(((pTiled4x4[2] >> 8) | (pTiled4x4[3] << 8)) & 0x3FF);
  pUntiledV[0] = convert((pTiled4x4[3] >> 2) & 0x3FF);
  pUntiledU[1] = convert(((pTiled4x4[3] >> 12) | (pTiled4x4[4] << 4)) & 0x3FF);
  pUntiledV[1] = convert(pTiled4x4[4] >> 6);
  pUntiledU += iUntiledPitch;
  pUntiledV += iUntiledPitch;
  pUntiledU[0] = convert(pTiled4x4[5] & 0x3FF);
  pUntiledV[0] = convert(((pTiled4x4[5] >> 10) | (pTiled4x4[6] << 6)) & 0x3FF);
  pUntiledU[1] = convert((pTiled4x4[6] >> 4) & 0x3FF);
  pUntiledV[1] = convert(((pTiled4x4[6] >> 14) | (pTiled4x4[7] << 2)) & 0x3FF);
  pUntiledU += iUntiledPitch;
  pUntiledV += iUntiledPitch;
  pUntiledU[0] = convert(((pTiled4x4[7] >> 8) | (pTiled4x4[8] << 8)) & 0x3FF);
  pUntiledV[0] = convert((pTiled4x4[8] >> 2) & 0x3FF);
  pUntiledU[1] = convert(((pTiled4x4[8] >> 12) | (pTiled4x4[9] << 4)) & 0x3FF);
  pUntiledV[1] = convert(pTiled4x4[9] >> 6);
}

/****************************************************************************/
static void Untile4x4ChromaBlock10bToPlanar8b(uint16_t* pTiled4x4, uint8_t* pUntiledU, uint8_t* pUntiledV, int iUntiledPitch)
{
  Untile4x4ChromaBlock10bToPlanar<uint8_t>(pTiled4x4, pUntiledU, pUntiledV, iUntiledPitch, RND_10B_TO_8B);
}

/****************************************************************************/
static void Untile4x4ChromaBlock10bToPlanar10b(uint16_t* pTiled4x4, uint16_t* pUntiledU, uint16_t* pUntiledV, int iUntiledPitch)
{
  Untile4x4ChromaBlock10bToPlanar<uint16_t>(pTiled4x4, pUntiledU, pUntiledV, iUntiledPitch, [](uint16_t u16) { return u16; });
}

/****************************************************************************/
static void T60A_To_Planar8b(AL_TBuffer const* pSrc, AL_TBuffer* pDst, bool bDstUFirst, TFourCC tDstFourCC)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iOffsetC = iPitchDst * iHeightC;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bDstUFirst ? 0 : iOffsetC);
  uint8_t* pDstDataV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bDstUFirst ? iOffsetC : 0);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = (uint16_t*)(pSrcData + (H / iTileH) * iPitchSrc);

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutU = pDstDataU + (H + h) * iPitchDst + (W + w) / 2;
          uint8_t* pOutV = pDstDataV + (H + h) * iPitchDst + (W + w) / 2;
          Untile4x4ChromaBlock10bToPlanar8b(pInC, pOutU, pOutV, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, tDstFourCC);
}

/****************************************************************************/
void T60A_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T60A_To_Planar8b(pSrc, pDst, true, FOURCC(I420));
}

/****************************************************************************/
void T60A_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T60A_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void T60A_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  T60A_To_Planar8b(pSrc, pDst, false, FOURCC(YV12));
}

/****************************************************************************/
void T60A_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block10bTo8b(pInC, pOutC, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV12));
}

/****************************************************************************/
void T60A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint16_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint8_t* pOutY = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block10bTo8b(pInY, pOutY, iPitchDst);
          pInY += 10;
        }

        pInY += 5 * iCropW / sizeof(uint16_t);
      }

      pInY += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T60A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y) / sizeof(uint16_t);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int H = 0; H < tDim.iHeight; H += iTileH)
  {
    uint16_t* pInY = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - tDim.iHeight;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutY = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block10bTo10b(pInY, pOutY, iPitchDst);
          pInY += 10;
        }

        pInY += 5 * iCropW / sizeof(uint16_t);
      }

      pInY += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T60A_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutC = pDstData + (H + h) * iPitchDst + (W + w);
          Untile4x4Block10bTo10b(pInC, pOutC, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));
}

/****************************************************************************/
void T60A_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  const int iTileW = 64;
  const int iTileH = 4;
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight >> 1;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pSrcData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pDstDataV = pDstDataU + iPitchDst * iHeightC;

  for(int H = 0; H < iHeightC; H += iTileH)
  {
    uint16_t* pInC = pSrcData + (H / iTileH) * iPitchSrc;

    int iCropH = (H + iTileH) - iHeightC;

    if(iCropH < 0)
      iCropH = 0;

    for(int W = 0; W < tDim.iWidth; W += iTileW)
    {
      int iCropW = (W + iTileW) - tDim.iWidth;

      if(iCropW < 0)
        iCropW = 0;

      for(int h = 0; h < iTileH - iCropH; h += 4)
      {
        for(int w = 0; w < iTileW - iCropW; w += 4)
        {
          uint16_t* pOutU = pDstDataU + (H + h) * iPitchDst + (W + w) / 2;
          uint16_t* pOutV = pDstDataV + (H + h) * iPitchDst + (W + w) / 2;
          Untile4x4ChromaBlock10bToPlanar10b(pInC, pOutU, pOutV, iPitchDst);
          pInC += 10;
        }

        pInC += 5 * iCropW / sizeof(uint16_t);
      }

      pInC += iCropH * iTileW * 5 / 4 / sizeof(uint16_t);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I0AL));
}

/****************************************************************************/
void T60A_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 2);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV15));
}

/****************************************************************************/
void T60A_To_XV10(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV10));
}

/****************************************************************************/
void T62A_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void T62A_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void T62A_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint8_t* pDstDataU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint8_t* pDstDataV = pDstDataU + iPitchDst * tDim.iHeight;

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint8_t* pOutU = pDstDataU + h * iPitchDst + w / 2;
      uint8_t* pOutV = pDstDataV + h * iPitchDst + w / 2;
      Untile4x4ChromaBlock10bToPlanar8b(pInC, pOutU, pOutV, iPitchDst);
      pInC += 10;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I422));
}

/****************************************************************************/
void T62A_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pOutC = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      Untile4x4Block10bTo8b(pInC, pOutC, iPitchDst);
      pOutC -= 3 * iPitchDst - 4;
      pInC += 10;
    }

    pOutC += iPitchDst * 4 - tDim.iWidth;
    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(NV16));
}

/****************************************************************************/
void T62A_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstDataU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pDstDataV = pDstDataU + (iPitchDst * tDim.iHeight);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pOutU = pDstDataU + h * iPitchDst + w / 2;
      uint16_t* pOutV = pDstDataV + h * iPitchDst + w / 2;
      Untile4x4ChromaBlock10bToPlanar10b(pInC, pOutU, pOutV, iPitchDst);
      pInC += 10;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(I2AL));
}

/****************************************************************************/
void T62A_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  T60A_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iJump = (AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV) - (tDim.iWidth * 5)) / sizeof(uint16_t);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint16_t* pInC = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pDstData = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < tDim.iHeight; h += 4)
  {
    for(int w = 0; w < tDim.iWidth; w += 4)
    {
      uint16_t* pOutC = pDstData + h * iPitchDst + w;
      Untile4x4Block10bTo10b(pInC, pOutC, iPitchDst);
      pInC += 10;
    }

    pInC += iJump;
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P210));
}

/****************************************************************************/
void T62A_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  Tile_To_XV_OneComponent(pSrc, pDst, true, 1);
  Tile_To_XV_OneComponent(pSrc, pDst, false, 1);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(XV20));
}

/****************************************************************************/
static void Read24BitsOn32Bits(uint32_t* pIn, uint8_t** pOut1, uint8_t** pOut2)
{
  *((*pOut1)++) = (*pIn >> 2) & 0xFF;
  *((*pOut2)++) = (*pIn >> 12) & 0xFF;
  *((*pOut1)++) = (*pIn >> 22) & 0xFF;
}

/****************************************************************************/
static void XVXX_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  (void)uHrzCScale;
  // Luma
  XV15_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iOffsetC = iHeightC * iPitchDst;

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstU = pDstData + h * iPitchDst;
    uint8_t* pDstV = pDstU + iOffsetC;

    int w = tDim.iWidth / 6;

    while(w--)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read24BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(tDim.iWidth % 6 > 2)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
    }
    else if(tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (uint8_t)((*pSrc32 >> 2) & 0xFF);
      *pDstV++ = (uint8_t)((*pSrc32 >> 12) & 0xFF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(I422) : FOURCC(I420));
}

/****************************************************************************/
void XV15_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_I42X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_I42X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void XV15_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstY = (uint8_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
      *pDstY++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
      *pDstY++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));
}

/****************************************************************************/
void XV15_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int h = 0; h < tDim.iHeight; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstY = (uint16_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstY++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstY++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y010));
}

/****************************************************************************/
void XV15_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  // Luma
  XV15_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / 2;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iOffsetC = iHeightC * iPitchDst;

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstV = (uint8_t*)(pDstData + h * iPitchDst);
    uint8_t* pDstU = pDstV + iOffsetC;

    int w = tDim.iWidth / 6;

    while(w--)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read24BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(tDim.iWidth % 6 > 2)
    {
      Read24BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (*pSrc32 >> 2) & 0xFF;
    }
    else if(tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (*pSrc32 >> 2) & 0xFF;
      *pDstV++ = (*pSrc32 >> 12) & 0xFF;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(YV12));
}

/****************************************************************************/
void XV15_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_I420(pSrc, pDst);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(IYUV));
}

/****************************************************************************/
void XV20_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_Y800(pSrc, pDst);
}

/****************************************************************************/
void XV20_To_Y010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XV15_To_Y010(pSrc, pDst);
}

/****************************************************************************/
void XVXX_To_NV1X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV15_To_Y800(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint8_t* pDstC = (uint8_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
      *pDstC++ = (*pSrc32 >> 22) & 0xFF;
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
      *pDstC++ = (*pSrc32 >> 12) & 0xFF;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstC++ = (*pSrc32 >> 2) & 0xFF;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(NV16) : FOURCC(NV12));
}

/****************************************************************************/
void XV15_To_NV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_NV1X(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_NV16(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_NV1X(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void XVXX_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV15_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstC = (uint16_t*)(pDstData + h * iPitchDst);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 20) & 0x3FF);
      ++pSrc32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstC++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDstC++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(P210) : FOURCC(P010));
}

/****************************************************************************/
void XV15_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void Read30BitsOn32Bits(uint32_t* pIn, uint16_t** pOut1, uint16_t** pOut2)
{
  *((*pOut1)++) = (uint16_t)((*pIn) & 0x3FF);
  *((*pOut2)++) = (uint16_t)((*pIn >> 10) & 0x3FF);
  *((*pOut1)++) = (uint16_t)((*pIn >> 20) & 0x3FF);
}

/****************************************************************************/
static void XVXX_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  XV15_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iOffsetC = iHeightC * iPitchDst / sizeof(uint16_t);

  assert(iPitchSrc % 4 == 0);

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pSrc32 = (uint32_t*)(pSrcData + h * iPitchSrc);
    uint16_t* pDstU = (uint16_t*)(pDstData + h * iPitchDst);
    uint16_t* pDstV = pDstU + iOffsetC;

    int w = tDim.iWidth / 6;

    while(w--)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      Read30BitsOn32Bits(pSrc32, &pDstV, &pDstU);
      ++pSrc32;
    }

    if(tDim.iWidth % 6 > 2)
    {
      Read30BitsOn32Bits(pSrc32, &pDstU, &pDstV);
      ++pSrc32;
      *pDstV++ = (uint16_t)((*pSrc32) & 0x3FF);
    }
    else if(tDim.iWidth % 6 > 0)
    {
      *pDstU++ = (uint16_t)((*pSrc32) & 0x3FF);
      *pDstV++ = (uint16_t)((*pSrc32 >> 10) & 0x3FF);
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(I2AL) : FOURCC(I0AL));
}

/****************************************************************************/
void XV15_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void XV20_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  XVXX_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NV1X_To_I42X(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale, TFourCC tDestFourCC, bool bIsUFirst = true)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  // Luma
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, tDim.iWidth);

    pBufIn += iPitchSrc;
    pBufOut += iPitchDst;
  }

  // Chroma
  iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
  int iChromaCompSize = iPitchDst * (tDim.iHeight / uVrtCScale);
  uint8_t* pBufInC = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pBufOutU = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bIsUFirst ? 0 : iChromaCompSize);
  uint8_t* pBufOutV = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV) + (bIsUFirst ? iChromaCompSize : 0);

  int iWidth = tDim.iWidth / uHrzCScale;
  int iHeight = tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = pBufInC[(iW << 1)];
      pBufOutV[iW] = pBufInC[(iW << 1) + 1];
    }

    pBufInC += iPitchSrc;
    pBufOutU += iPitchDst;
    pBufOutV += iPitchDst;
  }

  AL_PixMapBuffer_SetFourCC(pDst, tDestFourCC);
}

/****************************************************************************/
void NV12_To_YV12(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(YV12), false);
}

/****************************************************************************/
void NV12_To_I420(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(I420));
}

/****************************************************************************/
void NV16_To_I422(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 1, FOURCC(I422));
}

/****************************************************************************/
void NV12_To_IYUV(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_I42X(pSrc, pDst, 2, 2, FOURCC(IYUV));
}

/****************************************************************************/
static void NV1X_To_IXAL(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y800_To_Y010(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iCScale = uHrzCScale * uVrtCScale;
  int iWidth = tDim.iWidth / uHrzCScale;
  int iHeight = tDim.iHeight / uVrtCScale;

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOutU = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);
  uint16_t* pBufOutV = pBufOutU + (iPitchDst * iHeight);

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
    {
      pBufOutU[iW] = ((uint16_t)pBufIn[(iW << 1)]) << 2;
      pBufOutV[iW] = ((uint16_t)pBufIn[(iW << 1) + 1]) << 2;
    }

    pBufIn += iPitchSrc;
    pBufOutU += iPitchDst;
    pBufOutV += iPitchDst;
  }

  AL_PixMapBuffer_SetFourCC(pDst, iCScale == 2 ? FOURCC(I2AL) : FOURCC(I0AL));
}

/****************************************************************************/
void NV12_To_I0AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_IXAL(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_I2AL(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_IXAL(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NV1X_To_PX10(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(P010));

  // Luma
  I420_To_Y010(pSrc, pDst);

  // Chroma
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint16_t* pBufOut = (uint16_t*)AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV) / sizeof(uint16_t);

  int iWidth = 2 * tDim.iWidth / uHrzCScale;
  int iHeight = tDim.iHeight / uVrtCScale;

  for(int iH = 0; iH < iHeight; ++iH)
  {
    for(int iW = 0; iW < iWidth; ++iW)
      pBufOut[iW] = ((uint16_t)pBufIn[iW]) << 2;

    pBufIn += iPitchSrc;
    pBufOut += iPitchDst;
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(P210) : FOURCC(P010));
}

/****************************************************************************/
void NV12_To_P010(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_PX10(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_P210(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_PX10(pSrc, pDst, 2, 1);
}

/****************************************************************************/
static void NV1X_To_XVXX(AL_TBuffer const* pSrc, AL_TBuffer* pDst, uint8_t uHrzCScale, uint8_t uVrtCScale)
{
  // Luma
  Y800_To_XV10(pSrc, pDst);

  // Chroma
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  int iHeightC = tDim.iHeight / uVrtCScale;

  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);

  assert(iPitchSrc % 4 == 0);

  for(int h = 0; h < iHeightC; h++)
  {
    uint32_t* pDst32 = (uint32_t*)(pDstData + h * iPitchDst);
    uint8_t* pSrcC = (uint8_t*)(pSrcData + h * iPitchSrc);

    int w = tDim.iWidth / 3;

    while(w--)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
      *pDst32 |= ((uint32_t)*pSrcC++) << 22;
      ++pDst32;
    }

    if(tDim.iWidth % 3 > 1)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
      *pDst32 |= ((uint32_t)*pSrcC++) << 12;
    }
    else if(tDim.iWidth % 3 > 0)
    {
      *pDst32 = ((uint32_t)*pSrcC++) << 2;
    }
  }

  AL_PixMapBuffer_SetFourCC(pDst, (uHrzCScale * uVrtCScale) == 2 ? FOURCC(XV20) : FOURCC(XV15));
}

/****************************************************************************/
void NV12_To_XV15(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_XVXX(pSrc, pDst, 2, 2);
}

/****************************************************************************/
void NV16_To_XV20(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  NV1X_To_XVXX(pSrc, pDst, 2, 1);
}

/****************************************************************************/
void Y800_To_Y800(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);
  AL_PixMapBuffer_SetDimension(pDst, tDim);
  AL_PixMapBuffer_SetFourCC(pDst, FOURCC(Y800));

  // Luma
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);
  uint8_t* pBufIn = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pBufOut = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; ++iH)
  {
    memcpy(pBufOut, pBufIn, tDim.iWidth);
    pBufIn += iPitchSrc;
    pBufOut += iPitchDst;
  }
}

/****************************************************************************/
void CopyPixMapBuffer(AL_TBuffer const* pSrc, AL_TBuffer* pDst)
{
  TFourCC tFourCC = AL_PixMapBuffer_GetFourCC(pSrc);
  AL_TDimension tDim = AL_PixMapBuffer_GetDimension(pSrc);

  AL_TPicFormat tPicFormat;
  assert(AL_GetPicFormat(tFourCC, &tPicFormat) && !tPicFormat.b10bPacked && !tPicFormat.bCompressed);

  AL_PixMapBuffer_SetFourCC(pDst, tFourCC);
  AL_PixMapBuffer_SetDimension(pDst, tDim);

  // Luma
  int iWidthInByte = tDim.iWidth * (tPicFormat.uBitDepth == 10 ? 2 : 1);
  int iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_Y);
  int iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_Y);
  uint8_t* pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_Y);
  uint8_t* pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_Y);

  for(int iH = 0; iH < tDim.iHeight; iH++)
  {
    memcpy(pDstData, pSrcData, iWidthInByte);
    pSrcData += iPitchSrc;
    pDstData += iPitchDst;
  }

  // Chroma
  if(tPicFormat.eChromaMode != AL_CHROMA_MONO)
  {
    int iHScale = tPicFormat.eChromaOrder == AL_C_ORDER_SEMIPLANAR || tPicFormat.eChromaMode == AL_CHROMA_4_4_4 ? 1 : 2;
    iWidthInByte = iWidthInByte / iHScale;

    int iVScale = tPicFormat.eChromaMode == AL_CHROMA_4_2_0 ? 2 : 1;
    int iChromaHeight = (tDim.iHeight * 2) / iVScale;

    iPitchSrc = AL_PixMapBuffer_GetPlanePitch(pSrc, AL_PLANE_UV);
    iPitchDst = AL_PixMapBuffer_GetPlanePitch(pDst, AL_PLANE_UV);
    pSrcData = AL_PixMapBuffer_GetPlaneAddress(pSrc, AL_PLANE_UV);
    pDstData = AL_PixMapBuffer_GetPlaneAddress(pDst, AL_PLANE_UV);

    for(int iH = 0; iH < iChromaHeight; iH++)
    {
      memcpy(pDstData, pSrcData, iWidthInByte);
      pSrcData += iPitchSrc;
      pDstData += iPitchDst;
    }
  }
}

/*@}*/

