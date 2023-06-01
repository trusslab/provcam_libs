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

#include "DecHwScalingList.h"
#include "lib_assert/al_assert.h"

/******************************************************************************/
static void AL_sWriteWord(const uint8_t* pSrc, int iSize, uint32_t* pBuf, const int* pScan)
{
  for(int scl = 0; scl < iSize; ++scl)
  {
    int iOffset = scl << 2;
    *pBuf++ = pSrc[pScan ? pScan[iOffset] : iOffset] | (pSrc[pScan ? pScan[iOffset + 1] : iOffset + 1] << 8) |
              (pSrc[pScan ? pScan[iOffset + 2] : iOffset + 2] << 16) | ((uint32_t)pSrc[pScan ? pScan[iOffset + 3] : iOffset + 3] << 24);
  }
}

/******************************************************************************/
void AL_AVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  AL_Assert((1 & (size_t)pBuf) == 0);

  for(int m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
  {
    // 8x8
    uint8_t const* pSrc = (*pSclLst)[m].t8x8Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_AVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 4x4 Luma
    pSrc = (*pSclLst)[m].t4x4Y;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;

    // 4x4 Cb
    pSrc = (*pSclLst)[m].t4x4Cb;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;

    // 4x4 Cr
    pSrc = (*pSclLst)[m].t4x4Cr;
    AL_sWriteWord(pSrc, 4, pBuf32, NULL);
    pBuf32 += 4;
  }
}

/******************************************************************************/
void AL_HEVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf)
{
  uint32_t* pBuf32 = (uint32_t*)pBuf;

  for(int m = 0; m < 2; m++) // Mode : 0 = Intra; 1 = Inter
  {
    // 32x32
    uint8_t const* pSrc = (*pSclLst)[m].t32x32;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 luma
    pSrc = (*pSclLst)[m].t16x16Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 Cb
    pSrc = (*pSclLst)[m].t16x16Cb;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 16x16 Cr
    pSrc = (*pSclLst)[m].t16x16Cr;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 luma
    pSrc = (*pSclLst)[m].t8x8Y;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 Cb
    pSrc = (*pSclLst)[m].t8x8Cb;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 8x8 Cr
    pSrc = (*pSclLst)[m].t8x8Cr;
    AL_sWriteWord(pSrc, 16, pBuf32, AL_HEVC_DEC_SCL_ORDER_8x8);
    pBuf32 += 16;

    // 4x4 Luma
    pSrc = (*pSclLst)[m].t4x4Y;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;

    // 4x4 Cb
    pSrc = (*pSclLst)[m].t4x4Cb;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;

    // 4x4 Cr
    pSrc = (*pSclLst)[m].t4x4Cr;
    AL_sWriteWord(pSrc, 4, pBuf32, AL_HEVC_DEC_SCL_ORDER_4x4);
    pBuf32 += 4;
  }

  *pBuf32++ = (*pSclLst)[0].tDC[0] | ((*pSclLst)[0].tDC[1] << 8) | ((*pSclLst)[0].tDC[2] << 16);
  *pBuf32++ = (*pSclLst)[0].tDC[3];
  *pBuf32++ = (*pSclLst)[1].tDC[0] | ((*pSclLst)[1].tDC[1] << 8) | ((*pSclLst)[1].tDC[2] << 16);
  *pBuf32++ = (*pSclLst)[1].tDC[3];
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
  *pBuf32++ = 0;
}

