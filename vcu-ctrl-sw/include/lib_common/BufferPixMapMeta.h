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

#include "lib_common/BufferMeta.h"
#include "lib_common/FourCC.h"

/*************************************************************************//*!
   \brief Plane parameters
*****************************************************************************/
typedef struct AL_t_Plane
{
  int iChunkIdx;   /*!< Index of the chunk containing the plane */
  int iOffset;      /*!< Offset of the plane from beginning of the buffer chunk (in bytes) */
  int iPitch;       /*!< Pitch of the plane (in bytes) */
}AL_TPlane;

typedef enum AL_e_PlaneId
{
  AL_PLANE_Y,
  AL_PLANE_UV,
  AL_PLANE_MAP_Y,
  AL_PLANE_MAP_UV,
  AL_PLANE_MAX_ENUM, /* sentinel */
}AL_EPlaneId;

/*************************************************************************//*!
   \brief Useful information related to the framebuffers containing the picture
*****************************************************************************/
typedef struct AL_t_PixMapMetaData
{
  AL_TMetaData tMeta;
  AL_TDimension tDim; /*!< Dimension in pixel of the frame */
  AL_TPlane tPlanes[AL_PLANE_MAX_ENUM]; /*! < Array of color planes parameters */
  TFourCC tFourCC; /*!< FOURCC identifier */
}AL_TPixMapMetaData;

/*************************************************************************//*!
   \brief Create a pixmap metadata.
   \param[in] tDim Dimension of the the picture (width and height in pixels)
   \param[in] tYPlane Array of luma plane parameters (offset and pitch in bytes)
   \param[in] tUVPlane Array of chroma plane parameters (offset and pitch in bytes)
   \param[in] tFourCC FourCC of the framebuffer
   \return Returns NULL in case of allocation failure. Returns a pointer
   to the metadata in case of success.
*****************************************************************************/
AL_TPixMapMetaData* AL_PixMapMetaData_Create(AL_TDimension tDim, AL_TPlane tYPlane, AL_TPlane tUVPlane, TFourCC tFourCC);
AL_TPixMapMetaData* AL_PixMapMetaData_CreateEmpty(TFourCC tFourCC);
void AL_PixMapMetaData_AddPlane(AL_TPixMapMetaData* pMeta, AL_TPlane tPlane, AL_EPlaneId ePlaneId);
AL_TPixMapMetaData* AL_PixMapMetaData_Clone(AL_TPixMapMetaData* pMeta);
int AL_PixMapMetaData_GetOffsetY(AL_TPixMapMetaData* pMeta);
int AL_PixMapMetaData_GetOffsetUV(AL_TPixMapMetaData* pMeta);

/*************************************************************************//*!
   \brief Get the size of the luma inside the picture
   \param[in] pMeta A pointer the the pixmap metadata
   \return Returns size of the luma region
*****************************************************************************/
int AL_PixMapMetaData_GetLumaSize(AL_TPixMapMetaData* pMeta);

/*************************************************************************//*!
   \brief Get the size of the chroma inside the picture
   \param[in] pMeta A pointer the the pixmap metadata
   \return Returns size of the chroma region
*****************************************************************************/
int AL_PixMapMetaData_GetChromaSize(AL_TPixMapMetaData* pMeta);

AL_DEPRECATED("Renamed. Use AL_TPixMapMetaData.")
typedef AL_TPixMapMetaData AL_TSrcMetaData;

/*@*/
