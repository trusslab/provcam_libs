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

/**************************************************************************//*!
   \addtogroup Buffers
   @{
   \file
 *****************************************************************************/

#pragma once

#include "lib_common/HDR.h"
#include "lib_common/BufferMeta.h"

/*************************************************************************//*!
   \brief Metadata containing HDR related settings
*****************************************************************************/
typedef struct AL_t_HDRMeta
{
  AL_TMetaData tMeta;
  AL_EColourDescription eColourDescription;
  AL_ETransferCharacteristics eTransferCharacteristics;
  AL_EColourMatrixCoefficients eColourMatrixCoeffs;
  AL_THDRSEIs tHDRSEIs;
}AL_THDRMetaData;

/*************************************************************************//*!
   \brief Creates a HDR Metadata
   \return Pointer to an HDR Metadata if success, NULL otherwise
*****************************************************************************/
AL_THDRMetaData* AL_HDRMetaData_Create(void);

/*************************************************************************//*!
   \brief Reset an HDR MetaData
   \param[in] pMeta Pointer to the HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Reset(AL_THDRMetaData* pMeta);

/*************************************************************************//*!
   \brief Copy HDR Info from one HDRMetaData to another
   \param[in] pMetaSrc Pointer to the source HDR Metadata
   \param[in] pMetaDst Pointer to the destination HDR Metadata
*****************************************************************************/
void AL_HDRMetaData_Copy(AL_THDRMetaData* pMetaSrc, AL_THDRMetaData* pMetaDst);

/*@}*/

