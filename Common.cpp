/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// Common.cpp

#include "stdafx.h"
#include "Emulator.h"
//#include "util\BitmapFile.h"

//////////////////////////////////////////////////////////////////////

DWORD m_dwTotalEmulatorUptime = 0;  // Total BK uptime, seconds
int m_nCommon_TestsStarted = 0;
bool m_okCommon_CurrentTestFailed = false;
int m_nCommon_TestsFailed = 0;


//////////////////////////////////////////////////////////////////////

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    //TODO: Implement in this environment

    return FALSE;
}

void AlertWarning(LPCTSTR sMessage)
{
    //TODO: Implement in this environment
}
void AlertWarningFormat(LPCTSTR sFormat, ...)
{
    //TODO: Implement in this environment
}


//////////////////////////////////////////////////////////////////////
// DebugPrint and DebugLog

#if !defined(PRODUCT)

void DebugPrint(LPCTSTR message)
{
    Test_Log('d', message);
}

void DebugPrintFormat(LPCTSTR format, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, format);
    _vsntprintf_s(buffer, 512, 512 - 1, format, ptr);
    va_end(ptr);

    Test_Log('d', buffer);
}

const LPCTSTR TRACELOG_FILE_NAME = _T("trace.log");
const LPCTSTR TRACELOG_NEWLINE = _T("\r\n");

HANDLE Common_LogFile = NULL;

void DebugLogClear()
{
    if (Common_LogFile != NULL)
    {
        CloseHandle(Common_LogFile);
        Common_LogFile = NULL;
    }

    ::DeleteFile(TRACELOG_FILE_NAME);
}

void DebugLog(LPCTSTR message)
{
    if (Common_LogFile == NULL)
    {
        // Create file
        Common_LogFile = CreateFile(TRACELOG_FILE_NAME,
                GENERIC_WRITE, FILE_SHARE_READ, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
    SetFilePointer(Common_LogFile, 0, NULL, FILE_END);

    DWORD dwLength = lstrlen(message) * sizeof(TCHAR);

    char ascii[256];  *ascii = 0;
    WideCharToMultiByte(CP_ACP, 0, message, dwLength, ascii, 256, NULL, NULL);

    DWORD dwBytesWritten = 0;
    //WriteFile(Common_LogFile, message, dwLength, &dwBytesWritten, NULL);
    WriteFile(Common_LogFile, ascii, (DWORD)strlen(ascii), &dwBytesWritten, NULL);

    //dwLength = lstrlen(TRACELOG_NEWLINE) * sizeof(TCHAR);
    //WriteFile(Common_LogFile, TRACELOG_NEWLINE, dwLength, &dwBytesWritten, NULL);
}

void DebugLogFormat(LPCTSTR pszFormat, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, pszFormat);
    _vsntprintf_s(buffer, 512, 512 - 1, pszFormat, ptr);
    va_end(ptr);

    DebugLog(buffer);
}


#endif // !defined(PRODUCT)


//////////////////////////////////////////////////////////////////////


// Названия регистров процессора
const TCHAR* REGISTER_NAME[] = { _T("R0"), _T("R1"), _T("R2"), _T("R3"), _T("R4"), _T("R5"), _T("SP"), _T("PC") };


// Print octal 16-bit value to buffer
// buffer size at least 7 characters
void PrintOctalValue(TCHAR* buffer, WORD value)
{
    for (int p = 0; p < 6; p++) {
        int digit = value & 7;
        buffer[5 - p] = _T('0') + digit;
        value = (value >> 3);
    }
    buffer[6] = 0;
}
// Print hex 16-bit value to buffer
// buffer size at least 5 characters
void PrintHexValue(TCHAR* buffer, WORD value)
{
    for (int p = 0; p < 4; p++) {
        int digit = value & 15;
        buffer[3 - p] = (digit < 10) ? _T('0') + digit : _T('a') + digit - 10;
        value = (value >> 4);
    }
    buffer[4] = 0;
}
// Print binary 16-bit value to buffer
// buffer size at least 17 characters
void PrintBinaryValue(TCHAR* buffer, WORD value)
{
    for (int b = 0; b < 16; b++) {
        int bit = (value >> b) & 1;
        buffer[15 - b] = bit ? _T('1') : _T('0');
    }
    buffer[16] = 0;
}


// BK to Unicode conversion table
const TCHAR BK_CHAR_CODES[] =
{
    0x3C0,  0x2534, 0x2665, 0x2510, 0x2561, 0x251C, 0x2514, 0x2550, 0x2564, 0x2660, 0x250C, 0x252C, 0x2568, 0x2193, 0x253C, 0x2551,
    0x2524, 0x2190, 0x256C, 0x2191, 0x2663, 0x2500, 0x256B, 0x2502, 0x2666, 0x2518, 0x256A, 0x2565, 0x2567, 0x255E, 0x2192, 0x2593,
    0x44E,  0x430,  0x431,  0x446,  0x434,  0x435,  0x444,  0x433,  0x445,  0x438,  0x439,  0x43A,  0x43B,  0x43C,  0x43D,  0x43E,
    0x43F,  0x44F,  0x440,  0x441,  0x442,  0x443,  0x436,  0x432,  0x44C,  0x44B,  0x437,  0x448,  0x44D,  0x449,  0x447,  0x44A,
    0x42E,  0x410,  0x411,  0x426,  0x414,  0x415,  0x424,  0x413,  0x425,  0x418,  0x419,  0x41A,  0x41B,  0x41C,  0x41D,  0x41E,
    0x41F,  0x42F,  0x420,  0x421,  0x422,  0x423,  0x416,  0x412,  0x42C,  0x42B,  0x417,  0x428,  0x42D,  0x429,  0x427,  0x42A,
};
// Translate one KOI8-R character to Unicode character
TCHAR Translate_BK_Unicode(BYTE ch)
{
    if (ch < 32) return _T('·');
    if (ch < 127) return (TCHAR) ch;
    if (ch == 127) return (TCHAR) 0x25A0;
    if (ch >= 128 && ch < 160) return _T('·');
    return BK_CHAR_CODES[ch - 160];
}


//////////////////////////////////////////////////////////////////////


void Test_Log(char eventtype, LPCTSTR message)
{
    HANDLE hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
    WORD fgcolor = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
    if (eventtype == 'E')
    {
        fgcolor = FOREGROUND_RED|FOREGROUND_INTENSITY;
        m_okCommon_CurrentTestFailed = true;
    }
    else if (eventtype == '!')
        fgcolor = FOREGROUND_GREEN|FOREGROUND_INTENSITY;
    //TODO: Show BK uptime
    SYSTEMTIME stm;
    ::GetLocalTime(&stm);
    ::SetConsoleTextAttribute(hStdOut, fgcolor);
    printf("%02d:%02d:%02d.%03d %c %S\n",
        stm.wHour, stm.wMinute, stm.wSecond, stm.wMilliseconds,
        eventtype, message);
    ::SetConsoleTextAttribute(hStdOut, FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);
}

void Test_LogFormat(char eventtype, LPCTSTR format, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, format);
    _vsntprintf_s(buffer, 512, 512 - 1, format, ptr);
    va_end(ptr);

    Test_Log(eventtype, buffer);
}

void Test_Init(LPCTSTR sTestTitle, int configuration)
{
    Test_Log('!', sTestTitle);

    m_nCommon_TestsStarted++;
    m_okCommon_CurrentTestFailed = false;

    if (! Emulator_Init())
        Test_Log('E', _T("FAILED to initialize the emulator."));

    if (! Emulator_InitConfiguration((BKConfiguration)configuration))
        Test_Log('E', _T("FAILED to initialize the emulator configuration."));
}

void Test_Done()
{
    m_dwTotalEmulatorUptime += Emulator_GetUptime();
    Emulator_Done();

    if (m_okCommon_CurrentTestFailed)
        m_nCommon_TestsFailed++;
}

void Test_LogSummary()
{
    Test_LogFormat('i', _T("Emulator time spent: %u seconds"), m_dwTotalEmulatorUptime);
    char evtype = (m_nCommon_TestsFailed == 0) ? '!' : 'E';
    Test_LogFormat(evtype, _T("TOTAL tests started: %u, failed: %u"), m_nCommon_TestsStarted, m_nCommon_TestsFailed);
}

void Test_AttachFloppyImage(int slot, LPCTSTR sFilePath)
{
    BOOL res = Emulator_AttachFloppyImage(slot, sFilePath);
    if (!res)
        Test_LogFormat('E', _T("FAILED to attach floppy image %s"), sFilePath);
}

void Test_OpenTape(LPCTSTR sFilePath)
{
    BOOL res = Emulator_OpenTape(sFilePath);
    if (!res)
        Test_LogFormat('E', _T("FAILED to open tape image %s"), sFilePath);
}
void Test_CreateTape(LPCTSTR sFilePath)
{
    BOOL res = Emulator_CreateTape(sFilePath);
    if (!res)
        Test_LogFormat('E', _T("FAILED to create tape image %s"), sFilePath);
}
void Test_CloseTape()
{
    Emulator_CloseTape();
}

void Test_SaveScreenshot(LPCTSTR sFileName, int screenMode)
{
    if (Emulator_SaveScreenshot(sFileName, screenMode))
        Test_LogFormat('i', _T("Saved screenshot %s"), sFileName);
    else
        Test_LogFormat('E', _T("FAILED to save screenshot %s"), sFileName);
}

void Test_SaveScreenshotSeria(LPCTSTR sFileNameTemplate, int count, int frameStep)
{
    TCHAR buffer[255];
    for (int i = 0; i < count; i++)
    {
        swprintf(buffer, 255, sFileNameTemplate, i);
        Test_SaveScreenshot(buffer);
        Emulator_Run(frameStep);
    }
}

void Test_CheckScreenshot(LPCTSTR sFileName, int screenMode)
{
    int diff = Emulator_CheckScreenshot(sFileName, screenMode);
    if (diff == 0)
    {
        Test_LogFormat('i', _T("Checked screenshot %s"), sFileName);
        return;
    }
    if (diff < 0)
    {
        Test_LogFormat('E', _T("ERROR checking screenshot %s"), sFileName);
        exit(1);
    }

    Test_LogFormat('E', _T("TEST FAILED checking screenshot %s, diff %d"), sFileName, diff);
}

void Test_CopyFile(LPCTSTR sFileNameFrom, LPCTSTR sFileNameTo)
{
    if (!CopyFile(sFileNameFrom, sFileNameTo, FALSE))
    {
        Test_LogFormat('E', _T("ERROR copying file %s to %s"), sFileNameFrom, sFileNameTo);
        exit(1);
    }

    Test_LogFormat('i', _T("Copyed file %s to %s"), sFileNameFrom, sFileNameTo);
}

void Test_CreateDiskImage(LPCTSTR sFileName, int tracks)
{
    LONG fileSize = tracks * 10240;
	HANDLE hFile = ::CreateFile(sFileName,
		GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
    {
        Test_LogFormat('E', _T("FAILED to create disk image %s"), sFileName);
        return;
    }

    // Zero-fill the file
    ::SetFilePointer(hFile, fileSize, NULL, FILE_BEGIN);
    ::SetEndOfFile(hFile);
    ::CloseHandle(hFile);
}

void Test_LoadBin(LPCTSTR strFileName)
{
    if (! Emulator_LoadBin(strFileName))
    {
        Test_LogFormat('E', _T("FAILED to load BIN file %s"), strFileName);
        exit(1);
    }
}

//void Test_SaveStateImage(LPCTSTR sFileName)
//{
//    if (Emulator_SaveImage(sFileName))
//        Test_LogFormat('i', _T("Saved state image %s"), sFileName);
//    else
//        Test_LogFormat('E', _T("FAILED to save state image %s"), sFileName);
//}
//void Test_LoadStateImage(LPCTSTR sFileName)
//{
//    if (Emulator_LoadImage(sFileName))
//        Test_LogFormat('i', _T("Loaded state image %s"), sFileName);
//    else
//        Test_LogFormat('E', _T("FAILED to load state image %s"), sFileName);
//}


//////////////////////////////////////////////////////////////////////
