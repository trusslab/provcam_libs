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

#include <fstream>
#include <stdexcept>
#include <cstdlib>
#include <cstdarg>
#include "utils.h"

using namespace std;

int g_Verbosity = 10;

void Message(EConColor Color, const char* sMsg, va_list args)
{
  SetConsoleColor(Color);
  vprintf(sMsg, args);
  fflush(stdout);
  SetConsoleColor(CC_DEFAULT);
}

void LogError(const char* sMsg, ...)
{
  if(g_Verbosity < 1)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_RED, sMsg, args);
  va_end(args);
}

void LogWarning(const char* sMsg, ...)
{
  if(g_Verbosity < 3)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_YELLOW, sMsg, args);
  va_end(args);
}

void LogDimmedWarning(const char* sMsg, ...)
{
  if(g_Verbosity < 4)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_GREY, sMsg, args);
  va_end(args);
}

void LogInfo(const char* sMsg, ...)
{
  if(g_Verbosity < 5)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_DEFAULT, sMsg, args);
  va_end(args);
}

void LogInfo(EConColor Color, const char* sMsg, ...)
{
  if(g_Verbosity < 5)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(Color, sMsg, args);
  va_end(args);
}

void LogVerbose(const char* sMsg, ...)
{
  if(g_Verbosity < 7)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(CC_DEFAULT, sMsg, args);
  va_end(args);
}

void LogVerbose(EConColor Color, const char* sMsg, ...)
{
  if(g_Verbosity < 7)
    return;

  va_list args;
  va_start(args, sMsg);
  Message(Color, sMsg, args);
  va_end(args);
}

void OpenInput(std::ifstream& fp, std::string filename, bool binary)
{
  fp.open(filename, binary ? std::ios::binary : std::ios::in);
  fp.exceptions(ifstream::badbit);

  if(!fp.is_open())
    throw std::runtime_error("Can't open file for reading: '" + filename + "'");
}

void OpenOutput(std::ofstream& fp, std::string filename, bool binary)
{
  auto open_mode = binary ? std::ios::out | std::ios::binary : std::ios::out;

  fp.open(filename, open_mode);
  fp.exceptions(ofstream::badbit);

  if(!fp.is_open())
    throw std::runtime_error("Can't open file for writing: '" + filename + "'");
}

