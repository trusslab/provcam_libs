/******************************************************************************
*
* Copyright (C) 2016-2020 Allegro DVT2.  All rights reserved.
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

#pragma once

#include <list>
#include <iostream>
#include <string>
#include <vector>

#include "ICommandsSender.h"

struct CEncCmdMngr
{
  CEncCmdMngr(std::istream& CmdInput, int iLookAhead, int iFreqLT);

  void Process(ICommandsSender* sender, int iFrame);

private:
  std::istream& m_CmdInput;
  int const m_iLookAhead;
  int const m_iFreqLT;
  bool m_bHasLT;
  std::string m_sBufferedLine;

  struct TFrmCmd
  {
    int iFrame = 0;
    bool bSceneChange = false;
    bool bIsLongTerm = false;
    bool bUseLongTerm = false;
    bool bKeyFrame = false;
    bool bChangeGopLength = false;
    int iGopLength = 0;
    bool bChangeGopNumB = false;
    int iGopNumB = 0;
    bool bChangeFreqIDR = false;
    int iFreqIDR = 0;
    bool bChangeBitRate = false;
    int iBitRate = 0;
    bool bChangeFrameRate = false;
    int iFrameRate = 0;
    int iClkRatio = 0;
    bool bChangeQP = false;
    int iQP = 0;
    bool bChangeQPBounds = false;
    int iMinQP = 0;
    int iMaxQP = 0;
    bool bChangeIPDelta = false;
    int iIPDelta = 0;
    bool bChangePBDelta = false;
    int iPBDelta = 0;
    bool bChangeResolution = false;
    int iInputIdx;
    bool bSetLFBetaOffset = false;
    int iLFBetaOffset;
    bool bSetLFTcOffset = false;
    int iLFTcOffset;
  };

  std::list<TFrmCmd> m_Cmds;

  void Refill(int iCurFrame);
  bool ReadNextCmd(TFrmCmd& Cmd);
  bool ParseCmd(std::string sLine, TFrmCmd& Cmd, bool bSameFrame);
  bool GetNextLine(std::string& sNextLine);
};

