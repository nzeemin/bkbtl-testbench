/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// Emulator.h

#pragma once

#include "emubase\Board.h"
//#include "util\BitmapFile.h"

//////////////////////////////////////////////////////////////////////


extern CMotherboard* g_pBoard;

extern BOOL g_okEmulatorRunning;


//////////////////////////////////////////////////////////////////////


BOOL Emulator_Init();
BOOL Emulator_InitConfiguration(BKConfiguration configuration);
void Emulator_Done();
void Emulator_SetCPUBreakpoint(WORD address);
BOOL Emulator_IsBreakpoint();
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();
int  Emulator_SystemFrame();
DWORD Emulator_GetUptime();  // BK uptime, in seconds

BOOL Emulator_AttachFloppyImage(int slot, LPCTSTR sFilePath);

BOOL Emulator_Run(int frames);
BOOL Emulator_SaveScreenshot(LPCTSTR sFileName, int screenMode);
int  Emulator_CheckScreenshot(LPCTSTR sFileName, int screenMode);

void Emulator_KeyboardEvent(BYTE bkscan, BOOL okPressed);
void Emulator_KeyboardPressRelease(BYTE bkscan, int timeout = 3);
void Emulator_KeyboardPressReleaseChar(char ch, int timeout = 3);
void Emulator_KeyboardSequence(const char * str);

BOOL Emulator_LoadBin(LPCTSTR strFileName);

void Emulator_AttachTeletypeBuffer(int bufferSize = 1024);
void Emulator_DetachTeletypeBuffer();
const char * Emulator_GetTeletypeBuffer();


//////////////////////////////////////////////////////////////////////
