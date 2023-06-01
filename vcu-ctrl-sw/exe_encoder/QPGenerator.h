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
 *****************************************************************************/
#pragma once

extern "C"
{
#include "lib_common_enc/Settings.h"
}

#include <string>
#include "ROIMngr.h"

typedef enum e_GenerateQpMode
{
  // exclusive modes
  AL_GENERATE_UNIFORM_QP = 0x00, /*!< default behaviour */
  AL_GENERATE_RAMP_QP = 0x02, /*!< used for test purpose */
  AL_GENERATE_RANDOM_QP = 0x03, /*!< used for test purpose */
  AL_GENERATE_LOAD_QP = 0x04, /*!< used for test purpose */
  AL_GENERATE_BORDER_QP = 0x05, /*!< used for test purpose */
  AL_GENERATE_ROI_QP = 0x06,
  AL_GENERATE_MASK_QP_TABLE = 0x07,

  // additional modes
  AL_GENERATE_RANDOM_SKIP = 0x20, /*!< used for test purpose */
  AL_GENERATE_RANDOM_I_ONLY = 0x40, /*!< used for test purpose */

  AL_GENERATE_BORDER_SKIP = 0x100,
  AL_GENERATE_FULL_SKIP = 0x200,

  AL_GENERATE_MASK_QP_TABLE_EXT = 0x367,

  // Auto QP
  AL_GENERATE_AUTO_QP = 0x400, /*!< compute Qp by MB on the fly */
  AL_GENERATE_ADAPTIVE_AUTO_QP = 0x800, /*!< Dynamically compute Qp by MB on the fly */
  AL_GENERATE_MASK_AUTO_QP = 0xC00,

  // QP table mode
  AL_GENERATE_RELATIVE_QP = 0x8000,
  AL_GENERATE_QP_MAX_ENUM, /* sentinel */
}AL_EGenerateQpMode;

static AL_INLINE bool AL_IsAutoQP(AL_EGenerateQpMode eMode)
{
  return eMode & AL_GENERATE_MASK_AUTO_QP;
}

static AL_INLINE bool AL_HasQpTable(AL_EGenerateQpMode eMode)
{
  return eMode & AL_GENERATE_MASK_QP_TABLE_EXT;
}

/*************************************************************************//*!
   \brief Fill QP part of the buffer pointed to by pQP with a QP for each
        Macroblock of the slice.
   \param[in]  eMode      Specifies the way QP values are computed. see AL_EGenerateQpMode
   \param[in]  iSliceQP   Slice QP value (in range [0..51])
   \param[in]  iMinQP     Minimum allowed QP value (in range [0..50])
   \param[in]  iMaxQP     Maximum allowed QP value (in range [1..51]).
   \param[in]  iLCUWidth  Width in Lcu Unit of the picture
   \param[in]  iLCUHeight Height in Lcu Unit of the picture
   \param[in]  uLcuSize   Ctb maximum size
   \param[in]  eProf      Profile used for the encoding
   \param[in]  sQPTablesFolder In case QP are loaded from files, path to the folder
               containing the QP table files
   \param[in]  iFrameID   Frame identifier
   \param[out] pQPs       Pointer to the buffer that receives the computed QPs
   \param[out] pSegs      Pointer to the buffer that receives the computed Segments
   \note iMinQp <= iMaxQP
   \return true on success, false on error
*****************************************************************************/
bool GenerateQPBuffer(AL_EGenerateQpMode eMode, int16_t iSliceQP, int16_t iMinQP, int16_t iMaxQP, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, const std::string& sQPTablesFolder, int iFrameID, uint8_t* pQPs, uint8_t* pSegs);

/*************************************************************************//*!
   \brief Fill QP part of the buffer pointed to by pQP with a QP for each
        Macroblock of the slice with roi information
   \param[in]  pRoiCtx    Pointer to the roi object holding roi information
   \param[in]  sRoiFileName path and file name of the ROI description
   \param[in]  eMode      Specifies the way QP values are computed. see EQpCtrlMode
   \param[in]  iLCUWidth  Width in Lcu Unit of the picture
   \param[in]  iLCUHeight Height in Lcu Unit of the picture
   \param[in]  eProf      Profile used for the encoding
   \param[in]  iFrameID   Frame identifier
   \param[out] pQPs       Pointer to the buffer that receives the computed QPs
   \return true on success, false on error
*****************************************************************************/
bool GenerateROIBuffer(AL_TRoiMngrCtx* pRoiCtx, std::string const& sRoiFileName, int iLCUWidth, int iLCUHeight, AL_EProfile eProf, int iFrameID, uint8_t* pQPs);

