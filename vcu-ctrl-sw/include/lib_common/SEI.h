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

/****************************************************************************/
typedef enum e_SeiFlag
{
  AL_SEI_NONE = 0x00000000, // no SEI
  // prefix (16 LSBs)
  AL_SEI_BP = 0x00000001, // Buffering period
  AL_SEI_PT = 0x00000002, // Picture Timing
  AL_SEI_RP = 0x00000004, // Recovery Point
  AL_SEI_MDCV = 0x0000008, // Mastering Display Colour Volume (HDR)
  AL_SEI_CLL = 0x00000010,  // Content Light Level (HDR)
  // suffix (16 MSBs)

  AL_SEI_ALL = 0xFFFFFFFF, // All supported SEI
}AL_ESeiFlag;

/****************************************************************************/
static AL_INLINE bool AL_HAS_SEI_SUFFIX(AL_ESeiFlag seiFlag)
{
  return seiFlag & 0xFFFF0000;
}

/****************************************************************************/
static AL_INLINE bool AL_HAS_SEI_PREFIX(AL_ESeiFlag seiFlag)
{
  return seiFlag & 0x0000FFFF;
}

/****************************************************************************/
static uint8_t const SEI_PREFIX_USER_DATA_UNREGISTERED_UUID[16] =
{
  0xb1, 0xe1, 0x67, 0xa4, 0xd9, 0xca, 0x11, 0xe7,
  0xb1, 0x9b, 0x00, 0x50, 0xc2, 0x49, 0x00, 0x48
};
