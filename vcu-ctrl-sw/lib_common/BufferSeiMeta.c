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

#include "lib_common/BufferSeiMeta.h"
#include "lib_rtos/lib_rtos.h"

static bool destroy(AL_TMetaData* pMeta)
{
  AL_TSeiMetaData* pSeiMeta = (AL_TSeiMetaData*)pMeta;
  Rtos_Free(pSeiMeta->payload);
  Rtos_Free(pSeiMeta->pBuf);
  Rtos_Free(pSeiMeta);
  return true;
}

AL_TSeiMetaData* AL_SeiMetaData_Clone(AL_TSeiMetaData* pMeta)
{
  if(!pMeta)
    return NULL;

  AL_TSeiMetaData* pSeiMeta = AL_SeiMetaData_Create(pMeta->maxPayload, pMeta->maxBufSize);

  if(!pSeiMeta)
    return NULL;

  return pSeiMeta;
}

static AL_TMetaData* clone(AL_TMetaData* pMeta)
{
  return (AL_TMetaData*)AL_SeiMetaData_Clone((AL_TSeiMetaData*)pMeta);
}

AL_TSeiMetaData* AL_SeiMetaData_Create(uint8_t uMaxPayload, uint32_t uMaxBufSize)
{
  AL_TSeiMetaData* pMeta = (AL_TSeiMetaData*)Rtos_Malloc(sizeof(*pMeta));

  if(!pMeta)
    return NULL;

  pMeta->pBuf = (uint8_t*)Rtos_Malloc(uMaxBufSize * sizeof(*(pMeta->pBuf)));

  if(!pMeta->pBuf)
  {
    Rtos_Free(pMeta);
    return NULL;
  }

  pMeta->payload = (AL_TSeiMessage*)Rtos_Malloc(uMaxPayload * sizeof(*(pMeta->payload)));

  if(!pMeta->payload)
  {
    Rtos_Free(pMeta->pBuf);
    Rtos_Free(pMeta);
    return NULL;
  }

  pMeta->tMeta.eType = AL_META_TYPE_SEI;
  pMeta->tMeta.MetaDestroy = destroy;
  pMeta->tMeta.MetaClone = clone;
  pMeta->maxPayload = uMaxPayload;
  pMeta->maxBufSize = uMaxBufSize;
  AL_SeiMetaData_Reset(pMeta);

  return pMeta;
}

bool AL_SeiMetaData_AddPayload(AL_TSeiMetaData* pMeta, AL_TSeiMessage payload)
{
  if(pMeta->numPayload + 1 > pMeta->maxPayload)
    return false;

  AL_TSeiMessage* pCur = pMeta->payload + pMeta->numPayload;
  *pCur = payload;
  ++pMeta->numPayload;
  return true;
}

uint8_t* AL_SeiMetaData_GetBuffer(AL_TSeiMetaData* pMeta)
{
  if(pMeta->numPayload == 0)
    return pMeta->pBuf;

  AL_TSeiMessage* pLast = pMeta->payload + pMeta->numPayload - 1;
  return pLast->pData + pLast->size;
}

void AL_SeiMetaData_Reset(AL_TSeiMetaData* pMeta)
{
  pMeta->numPayload = 0;
  Rtos_Memset(pMeta->pBuf, 0, pMeta->maxBufSize);
}

