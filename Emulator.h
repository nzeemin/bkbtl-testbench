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

extern bool g_okEmulatorRunning;
extern bool g_okEmulatorAutoTapeReading;
extern TCHAR * g_pEmulatorAutoTapeReadingFilename;


//////////////////////////////////////////////////////////////////////


bool Emulator_Init();
bool Emulator_InitConfiguration(BKConfiguration configuration);
void Emulator_Done();
void Emulator_SetCPUBreakpoint(WORD address);
bool Emulator_IsBreakpoint();
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();
int  Emulator_SystemFrame();
uint32_t Emulator_GetUptime();  // BK uptime, in seconds

bool Emulator_AttachFloppyImage(int slot, LPCTSTR sFilePath);

bool Emulator_OpenTape(LPCTSTR sFilePath);
bool Emulator_CreateTape(LPCTSTR sFilePath);
void Emulator_CloseTape();

bool Emulator_Run(int frames);
bool Emulator_SaveScreenshot(LPCTSTR sFileName, int screenMode);
int  Emulator_CheckScreenshot(LPCTSTR sFileName, int screenMode);

void Emulator_KeyboardEvent(BYTE bkscan, bool okPressed);
void Emulator_KeyboardPressRelease(BYTE bkscan, int timeout = 3);
void Emulator_KeyboardPressReleaseChar(char ch, int timeout = 3);
void Emulator_KeyboardSequence(const char * str);

bool Emulator_LoadBin(LPCTSTR strFileName);

void Emulator_AttachTeletypeBuffer(int bufferSize = 1024);
void Emulator_DetachTeletypeBuffer();
const char * Emulator_GetTeletypeBuffer();

bool Emulator_SaveImage(LPCTSTR sFilePath);
bool Emulator_LoadImage(LPCTSTR sFilePath);


//////////////////////////////////////////////////////////////////////
