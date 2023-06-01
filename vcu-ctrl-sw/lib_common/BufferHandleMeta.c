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

#include "lib_common/BufferHandleMeta.h"
#include "BufferHandleMetaInternal.h"
#include "lib_rtos/lib_rtos.h"

static void freeInternal(AL_TInternalHandleMetaData* pInternal)
{
  Rtos_Free(pInternal->pHandles);
  Rtos_DeleteMutex(pInternal->mutex);
  Rtos_Free(pInternal);
}

static bool destroy(AL_TMetaData* pMeta)
{
  AL_THandleMetaData* pHandleMeta = (AL_THandleMetaData*)pMeta;
  freeInternal(pHandleMeta->pInternal);
  Rtos_Free(pHandleMeta);
  return true;
}

AL_THandleMetaData* AL_HandleMetaData_Clone(AL_THandleMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_THandleMetaData* pHandleMeta = AL_HandleMetaData_Create(pMeta->pInternal->maxHandles, pMeta->pInternal->handleSizeInBytes);

  if(!pHandleMeta)
    return NULL;

  Rtos_GetMutex(pMeta->pInternal->mutex);
  Rtos_GetMutex(pHandleMeta->pInternal->mutex);
  Rtos_Memcpy(pHandleMeta->pInternal->pHandles, pMeta->pInternal->pHandles, (pHandleMeta->pInternal->maxHandles * pHandleMeta->pInternal->handleSizeInBytes));
  Rtos_ReleaseMutex(pHandleMeta->pInternal->mutex);
  Rtos_ReleaseMutex(pMeta->pInternal->mutex);

  return pHandleMeta;
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_HandleMetaData_Clone((AL_THandleMetaData*)pMeta);
}

static AL_TInternalHandleMetaData* allocInternal(int iMaxHandles, int iHandleSize)
{
  AL_TInternalHandleMetaData* pInternal = (AL_TInternalHandleMetaData*)Rtos_Malloc(sizeof(*pInternal));

  if(!pInternal)
    return NULL;

  pInternal->mutex = Rtos_CreateMutex();

  if(!pInternal->mutex)
  {
    Rtos_Free(pInternal);
    return NULL;
  }

  pInternal->pHandles = Rtos_Malloc(iMaxHandles * iHandleSize);

  if(!pInternal->pHandles)
  {
    Rtos_DeleteMutex(pInternal->mutex);
    Rtos_Free(pInternal);
    return NULL;
  }

  pInternal->numHandles = 0;
  pInternal->handleSizeInBytes = iHandleSize;
  pInternal->maxHandles = iMaxHandles;

  return pInternal;
}

AL_THandleMetaData* AL_HandleMetaData_Create(int iMaxHandles, int iHandleSize)
{
  AL_THandleMetaData* pMeta = (AL_THandleMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->pInternal = allocInternal(iMaxHandles, iHandleSize);

  if(!pMeta->pInternal)
  {
    Rtos_Free(pMeta);
    return NULL;
  }

  pMeta->tMeta.eType = AL_META_TYPE_HANDLE;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;
  return pMeta;
}

static AL_HANDLE getHandlePtr(AL_THandleMetaData* pMeta, int iNumHandle)
{
  Rtos_GetMutex(pMeta->pInternal->mutex);
  AL_HANDLE pHandle = (AL_HANDLE)(((uintptr_t)pMeta->pInternal->pHandles) + (iNumHandle * pMeta->pInternal->handleSizeInBytes));
  Rtos_ReleaseMutex(pMeta->pInternal->mutex);
  return pHandle;
}

bool AL_HandleMetaData_AddHandle(AL_THandleMetaData* pMeta, AL_HANDLE pHandle)
{
  Rtos_GetMutex(pMeta->pInternal->mutex);

  if(pMeta->pInternal->numHandles + 1 > pMeta->pInternal->maxHandles)
  {
    Rtos_ReleaseMutex(pMeta->pInternal->mutex);
    return false;
  }

  Rtos_Memcpy(getHandlePtr(pMeta, pMeta->pInternal->numHandles), pHandle, pMeta->pInternal->handleSizeInBytes);
  ++pMeta->pInternal->numHandles;
  Rtos_ReleaseMutex(pMeta->pInternal->mutex);
  return true;
}

AL_HANDLE AL_HandleMetaData_GetHandle(AL_THandleMetaData* pMeta, int iNumHandle)
{
  return getHandlePtr(pMeta, iNumHandle);
}

int AL_HandleMetaData_GetNumHandles(AL_THandleMetaData* pMeta)
{
  Rtos_GetMutex(pMeta->pInternal->mutex);
  int numHandles = pMeta->pInternal->numHandles;
  Rtos_ReleaseMutex(pMeta->pInternal->mutex);
  return numHandles;
}
