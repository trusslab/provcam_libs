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
   \addtogroup lib_common
   @{
   \file
 *****************************************************************************/
#pragma once

#include "lib_rtos/types.h"
#include "lib_common/ScalingList.h"

/****************************************************************************/
static const int AL_AVC_DEC_SCL_ORDER_8x8[64] =
{
  0, 1, 2, 3,
  8, 9, 10, 11,
  16, 17, 18, 19,
  24, 25, 26, 27,
  4, 5, 6, 7,
  12, 13, 14, 15,
  20, 21, 22, 23,
  28, 29, 30, 31,
  32, 33, 34, 35,
  40, 41, 42, 43,
  48, 49, 50, 51,
  56, 57, 58, 59,
  36, 37, 38, 39,
  44, 45, 46, 47,
  52, 53, 54, 55,
  60, 61, 62, 63
};

/****************************************************************************/
static const int AL_HEVC_DEC_SCL_ORDER_8x8[64] =
{
  0, 8, 16, 24,
  1, 9, 17, 25,
  2, 10, 18, 26,
  3, 11, 19, 27,
  4, 12, 20, 28,
  5, 13, 21, 29,
  6, 14, 22, 30,
  7, 15, 23, 31,
  32, 40, 48, 56,
  33, 41, 49, 57,
  34, 42, 50, 58,
  35, 43, 51, 59,
  36, 44, 52, 60,
  37, 45, 53, 61,
  38, 46, 54, 62,
  39, 47, 55, 63
};

/****************************************************************************/
static const int AL_HEVC_DEC_SCL_ORDER_4x4[16] =
{
  0, 4, 8, 12,
  1, 5, 9, 13,
  2, 6, 10, 14,
  3, 7, 11, 15
};

/*************************************************************************//*!
   \brief Dumps Hardware formated decoder scaling list into buffer of bytes
   \param[in]  pSclLst Pointer to  Scaling list to dump
   \param[out] pBuf    Pointer to buffer that receives the scaling list
                     matrices data
*****************************************************************************/
extern void AL_AVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf);

extern void AL_HEVC_WriteDecHwScalingList(AL_TScl const* pSclLst, uint8_t* pBuf);

/*@}*/

