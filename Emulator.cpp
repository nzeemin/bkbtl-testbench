/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// Emulator.cpp

#include "stdafx.h"
#include <stdio.h>
#include <Share.h>
#include "Emulator.h"
#include "emubase\Emubase.h"
#include "util/WavPcmFile.h"

//NOTE: I know, we use unsafe string functions
#pragma warning( disable: 4996 )


//////////////////////////////////////////////////////////////////////


CMotherboard* g_pBoard = NULL;
BKConfiguration g_nEmulatorConfiguration;  // Current configuration

bool g_okEmulatorInitialized = false;
bool g_okEmulatorRunning = false;
bool g_okEmulatorAutoTapeReading = false;
TCHAR * g_pEmulatorAutoTapeReadingFilename = NULL;

uint16_t m_wEmulatorCPUBreakpoint = 0177777;

uint32_t m_dwTickCount = 0;
uint32_t m_dwEmulatorUptime = 0;  // BK uptime, seconds, from turn on or reset, increments every 25 frames
long m_nUptimeFrameCount = 0;

char * m_pEmulatorTeletypeBuffer = NULL;
int m_nEmulatorTeletypeBufferSize = 0;
int m_nEmulatorTeletypeBufferIndex = 0;

HWAVPCMFILE m_hTapeWavPcmFile = (HWAVPCMFILE) INVALID_HANDLE_VALUE;
#define TAPE_BUFFER_SIZE 624
uint8_t m_TapeBuffer[TAPE_BUFFER_SIZE];
bool CALLBACK Emulator_TapeReadCallback(UINT samples);
bool CALLBACK Emulator_TapeWriteCallback(UINT samples);

void Emulator_FakeTape_StartReadFile();


//////////////////////////////////////////////////////////////////////


//Прототип функции преобразования экрана
// Input:
//   pVideoBuffer   Исходные данные, биты экрана БК
//   okSmallScreen  Признак "малого" экрана
//   pPalette       Палитра
//   scroll         Текущее значение скроллинга
//   pImageBits     Результат, 32-битный цвет, размер для каждой функции свой
typedef void (CALLBACK* PREPARE_SCREEN_CALLBACK)(const uint8_t* pVideoBuffer, int okSmallScreen, const uint32_t* pPalette, int scroll, void* pImageBits);

void CALLBACK Emulator_PrepareScreenBW512x256(const uint8_t* pVideoBuffer, int okSmallScreen, const uint32_t* pPalette, int scroll, void* pImageBits);
void CALLBACK Emulator_PrepareScreenColor512x256(const uint8_t* pVideoBuffer, int okSmallScreen, const uint32_t* pPalette, int scroll, void* pImageBits);

struct ScreenModeStruct
{
    int width;
    int height;
    PREPARE_SCREEN_CALLBACK callback;
}
static ScreenModeReference[] =
{
    { 512, 256, Emulator_PrepareScreenBW512x256 },
    { 512, 256, Emulator_PrepareScreenColor512x256 },
};

//////////////////////////////////////////////////////////////////////


const LPCTSTR FILENAME_BKROM_MONIT10    = _T("monit10.rom");
const LPCTSTR FILENAME_BKROM_FOCAL      = _T("focal.rom");
const LPCTSTR FILENAME_BKROM_TESTS      = _T("tests.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_1  = _T("basic10_1.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_2  = _T("basic10_2.rom");
const LPCTSTR FILENAME_BKROM_BASIC10_3  = _T("basic10_3.rom");
const LPCTSTR FILENAME_BKROM_DISK_326   = _T("disk_326.rom");
const LPCTSTR FILENAME_BKROM_BK11M_BOS  = _T("b11m_bos.rom");
const LPCTSTR FILENAME_BKROM_BK11M_EXT  = _T("b11m_ext.rom");
const LPCTSTR FILENAME_BKROM_BASIC11M_0 = _T("basic11m_0.rom");
const LPCTSTR FILENAME_BKROM_BASIC11M_1 = _T("basic11m_1.rom");
const LPCTSTR FILENAME_BKROM_BK11M_MSTD = _T("b11m_mstd.rom");


//////////////////////////////////////////////////////////////////////
// Colors

const uint32_t ScreenView_BWPalette[4] =
{
    0x000000, 0xFFFFFF, 0x000000, 0xFFFFFF
};

const uint32_t ScreenView_ColorPalette[4] =
{
    0x000000, 0x0000FF, 0x00FF00, 0xFF0000
};

const uint32_t ScreenView_ColorPalettes[16][4] =
{
    //                                     Palette#     01           10          11
    0x000000, 0x0000FF, 0x00FF00, 0xFF0000,  // 00    синий   |   зеленый  |  красный
    0x000000, 0xFFFF00, 0xFF00FF, 0xFF0000,  // 01   желтый   |  сиреневый |  красный
    0x000000, 0x00FFFF, 0x0000FF, 0xFF00FF,  // 02   голубой  |    синий   | сиреневый
    0x000000, 0x00FF00, 0x00FFFF, 0xFFFF00,  // 03   зеленый  |   голубой  |  желтый
    0x000000, 0xFF00FF, 0x00FFFF, 0xFFFFFF,  // 04  сиреневый |   голубой  |   белый
    0x000000, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF,  // 05    белый   |    белый   |   белый
    0x000000, 0x7F0000, 0x7F0000, 0xFF0000,  // 06  темн-красн| красн-корич|  красный
    0x000000, 0x00FF7F, 0x00FF7F, 0xFFFF00,  // 07  салатовый | светл-зелен|  желтый
    0x000000, 0xFF00FF, 0x7F00FF, 0x7F007F,  // 08  фиолетовый| фиол-синий | сиреневый
    0x000000, 0x00FF7F, 0x7F00FF, 0x7F0000,  // 09 светл-зелен| фиол-синий |красн-корич
    0x000000, 0x00FF7F, 0x7F007F, 0x7F0000,  // 10  салатовый | фиолетовый |темн-красный
    0x000000, 0x00FFFF, 0xFFFF00, 0xFF0000,  // 11   голубой  |   желтый   |  красный
    0x000000, 0xFF0000, 0x00FF00, 0x00FFFF,  // 12   красный  |   зеленый  |  голубой
    0x000000, 0x00FFFF, 0xFFFF00, 0xFFFFFF,  // 13   голубой  |   желтый   |   белый
    0x000000, 0xFFFF00, 0x00FF00, 0xFFFFFF,  // 14   желтый   |   зеленый  |   белый
    0x000000, 0x00FFFF, 0x00FF00, 0xFFFFFF,  // 15   голубой  |   зеленый  |   белый
};


//////////////////////////////////////////////////////////////////////

bool Emulator_LoadRomFile(LPCTSTR strFileName, uint8_t* buffer, uint32_t fileOffset, uint32_t bytesToRead)
{
    FILE* fpRomFile = ::_tfsopen(strFileName, _T("rb"), _SH_DENYWR);
    if (fpRomFile == NULL)
        return false;

    ASSERT(bytesToRead <= 8192);
    ::memset(buffer, 0, 8192);

    if (fileOffset > 0)
    {
        ::fseek(fpRomFile, fileOffset, SEEK_SET);
    }

    size_t dwBytesRead = ::fread(buffer, 1, bytesToRead, fpRomFile);
    if (dwBytesRead != bytesToRead)
    {
        ::fclose(fpRomFile);
        return false;
    }

    ::fclose(fpRomFile);

    return true;
}

bool Emulator_Init()
{
    ASSERT(g_pBoard == NULL);

    CProcessor::Init();

    g_pBoard = new CMotherboard();

    g_pBoard->Reset();

    g_okEmulatorAutoTapeReading = false;
    g_pEmulatorAutoTapeReadingFilename = NULL;

    g_okEmulatorInitialized = true;
    return true;
}

void Emulator_Done()
{
    ASSERT(g_pBoard != NULL);

    CProcessor::Done();

    delete g_pBoard;
    g_pBoard = NULL;
}

bool Emulator_InitConfiguration(BKConfiguration configuration)
{
    g_pBoard->SetConfiguration((uint16_t)configuration);

    uint8_t buffer[8192];

    if ((configuration & BK_COPT_BK0011) == 0)
    {
        // Load Monitor ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_MONIT10, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Monitor ROM file."));
            return false;
        }
        g_pBoard->LoadROM(0, buffer);
    }

    if ((configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_ROM_BASIC) != 0 ||
        (configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_FDD) != 0)  // BK 0010 BASIC ROM 1-2
    {
        // Load BASIC ROM 1 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_1, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 1 file."));
            return false;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BASIC ROM 2 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_2, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 2 file."));
            return false;
        }
        g_pBoard->LoadROM(2, buffer);
    }
    if ((configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_ROM_BASIC) != 0)  // BK 0010 BASIC ROM 3
    {
        // Load BASIC ROM 3 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_3, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load BASIC ROM 3 file."));
            return false;
        }
        g_pBoard->LoadROM(3, buffer);
    }
    if ((configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_ROM_FOCAL) != 0)  // BK 0010 FOCAL
    {
        // Load Focal ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_FOCAL, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Focal ROM file."));
            return false;
        }
        g_pBoard->LoadROM(1, buffer);
        // Unused 8KB
        ::memset(buffer, 0, 8192);
        g_pBoard->LoadROM(2, buffer);
        // Load Tests ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_TESTS, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load Tests ROM file."));
            return false;
        }
        g_pBoard->LoadROM(3, buffer);
    }

    if (configuration & BK_COPT_BK0011)
    {
        // Load BK0011M BASIC 0, part 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return false;
        }
        g_pBoard->LoadROM(0, buffer);
        // Load BK0011M BASIC 0, part 2
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 8192, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return false;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BK0011M BASIC 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_1, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 1 ROM file."));
            return false;
        }
        g_pBoard->LoadROM(2, buffer);

        // Load BK0011M EXT
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_EXT, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M EXT ROM file."));
            return false;
        }
        g_pBoard->LoadROM(3, buffer);
        // Load BK0011M BOS
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_BOS, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BOS ROM file."));
            return false;
        }
        g_pBoard->LoadROM(4, buffer);
    }

    if (configuration & BK_COPT_FDD)
    {
        // Load disk driver ROM file
        ::memset(buffer, 0, 8192);
        if (!Emulator_LoadRomFile(FILENAME_BKROM_DISK_326, buffer, 0, 4096))
        {
            AlertWarning(_T("Failed to load DISK ROM file."));
            return false;
        }
        g_pBoard->LoadROM((configuration & BK_COPT_BK0011) ? 5 : 3, buffer);
    }

    if ((configuration & BK_COPT_BK0011) && (configuration & BK_COPT_FDD) == 0)
    {
        // Load BK0011M MSTD
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_MSTD, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M MSTD ROM file."));
            return false;
        }
        g_pBoard->LoadROM(5, buffer);
    }


    g_nEmulatorConfiguration = configuration;

    g_pBoard->Reset();

#if 0  //DEBUG: CPU and memory tests
    //Emulator_LoadRomFile(_T("791401"), buffer, 8192);
    //g_pBoard->LoadRAM(0, buffer, 8192);
    //Emulator_LoadRomFile(_T("791404"), buffer, 6144);
    //g_pBoard->LoadRAM(0, buffer, 6144);
    Emulator_LoadRomFile(_T("791323"), buffer, 4096);
    g_pBoard->LoadRAM(0, buffer, 4096);

    g_pBoard->GetCPU()->SetPC(0200);  //DEBUG
    g_pBoard->GetCPU()->SetPSW(0000);  //DEBUG
#endif

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    return true;
}

void Emulator_Start()
{
    g_okEmulatorRunning = true;

    //m_nFrameCount = 0;
    //m_dwTickCount = GetTickCount();
}
void Emulator_Stop()
{
    g_okEmulatorRunning = false;
    m_wEmulatorCPUBreakpoint = 0177777;
}

void Emulator_Reset()
{
    ASSERT(g_pBoard != NULL);

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;

    g_okEmulatorAutoTapeReading = false;
    g_pEmulatorAutoTapeReadingFilename = NULL;
}

void Emulator_SetCPUBreakpoint(uint16_t address)
{
    m_wEmulatorCPUBreakpoint = address;
}

bool Emulator_IsBreakpoint()
{
    uint16_t wCPUAddr = g_pBoard->GetCPU()->GetPC();
    if (wCPUAddr == m_wEmulatorCPUBreakpoint)
        return true;
    return false;
}
int Emulator_SystemFrame()
{
    g_pBoard->SetCPUBreakpoint(m_wEmulatorCPUBreakpoint);

    if (!g_pBoard->SystemFrame())
        return 0;

    // Calculate emulator uptime (25 frames per second)
    m_nUptimeFrameCount++;
    if (m_nUptimeFrameCount >= 25)
    {
        m_dwEmulatorUptime++;
        m_nUptimeFrameCount = 0;
    }

    if (g_okEmulatorAutoTapeReading)
    {
        uint16_t pc = g_pBoard->GetCPU()->GetPC();
        // Check if BK-0010 and PC=116722,116724 for tape reading
        if ((g_nEmulatorConfiguration & 1) == BK_COPT_BK0010 &&
            (pc == 0116722 || pc == 0116724))
        {
            Emulator_FakeTape_StartReadFile();
        }
    }

    return 1;
}

void Emulator_FakeTape_StartReadFile()
{
    // Retrieve EMT 36 file name
    TCHAR filename[24];
    uint16_t nameaddr = 0326; //g_pBoard->GetRAMWord(0306) + 6;
    for (uint16_t i = 0; i < 16; i++)
    {
        uint8_t ch = g_pBoard->GetRAMByte(nameaddr + i);
        filename[i] = (ch < 32) ? 0 : Translate_BK_Unicode(ch);
    }
    filename[16] = 0;
    // Trim trailing spaces
    for (int i = 15; i >= 0 && filename[i] == _T(' '); i--)
        filename[i] = 0;
    TCHAR* pdot = NULL;
    if (*filename != 0)
    {
        // Check if we have filename extension
        pdot = _tcsrchr(filename, _T('.'));
        if (pdot == NULL)  // Have no dot so append default '.BIN' extension
            _tcsncat(filename, _T(".BIN"), 4);
        else
        {
            // We have dot in string so cut off spaces before the dot
            if (pdot != filename)
            {
                TCHAR* pspace = pdot;
                while (pspace > filename && *(pspace - 1) == _T(' '))
                    pspace--;
                if (pspace < pdot)
                    _tcscpy(pspace, pdot);
            }
        }
    }

    FILE* fpFile = NULL;
    TCHAR filepath[MAX_PATH];
    *filepath = 0;
    // First, if the filename specified, try to find it
    if (g_pEmulatorAutoTapeReadingFilename != NULL)
        _tcscpy(filepath, g_pEmulatorAutoTapeReadingFilename);
    else if (*filename != 0)
        _sntprintf(filepath, 36, _T("data\\%s"), filename);
    if (*filepath != 0)
        fpFile = ::_tfsopen(filepath, _T("rb"), _SH_DENYWR);
    // If file not found then report
    if (fpFile == NULL)
    {
        Test_LogFormat('E', _T("Tape reading failed, file \'%s\' not found."), filepath);
    }

    uint8_t result = 2;  // EMT36 result = checksum error
    uint8_t* pData = NULL;
    if (fpFile != NULL)
    {
        for (;;)  // For breaks only
        {
            // Read the file header
            uint16_t header[2];
            if (::fread(header, 1, 4, fpFile) != 4)
            {
                Test_LogFormat('E', _T("Tape reading error, file %s"), filepath);
                break;  // Reading error
            }
            uint16_t filestart = header[0];
            uint16_t filesize = header[1];

            g_pBoard->SetRAMWord(0350, filesize);
            g_pBoard->SetRAMWord(0346, filestart);
            //TODO: Copy 16-char file name from 0326..0345 to 0352..0371

            if (filesize == 0)
            {
                Test_LogFormat('E', _T("Tape reading error, wrong file size %s"), filepath);
                break;  // Wrong Length
            }

            // Read the file
            pData = (uint8_t*)malloc(filesize);
            if (::fread(pData, 1, filesize, fpFile) != filesize)
            {
                Test_LogFormat('E', _T("Tape reading error, file %s"), filepath);
                break;  // Reading error
            }

            // Copy to memory
            uint16_t start = g_pBoard->GetRAMWord(0322);
            if (start == 0)
                start = filestart;
            for (uint16_t i = 0; i < filesize; i++)
            {
                g_pBoard->SetRAMByte(start + i, pData[i]);
            }

            result = 0;  // EMT36 result = OK
            Test_LogFormat('i', _T("Tape reading: loaded file %s"), filepath);
            break;
        }
    }

    if (pData != NULL)
        free(pData);

    // Report EMT36 result
    g_pBoard->SetRAMByte(0321, result);

    // Execute RTS twice -- return from EMT36
    CProcessor* pCPU = g_pBoard->GetCPU();
    pCPU->SetPC(g_pBoard->GetRAMWord(pCPU->GetSP()));
    pCPU->SetSP(pCPU->GetSP() + 2);
    pCPU->SetPC(g_pBoard->GetRAMWord(pCPU->GetSP()));
    pCPU->SetSP(pCPU->GetSP() + 2);
    //TODO: Set flags
}

bool Emulator_AttachFloppyImage(int slot, LPCTSTR sFilePath)
{
    return g_pBoard->AttachFloppyImage(slot, sFilePath);
}

// Tape emulator callback used to read a tape recorded data.
// Input:
//   samples    Number of samples to play.
// Output:
//   result     Bit to put in tape input port.
bool CALLBACK Emulator_TapeReadCallback(unsigned int samples)
{
    if (samples == 0) return 0;

    // Scroll buffer
    memmove(m_TapeBuffer, m_TapeBuffer + samples, TAPE_BUFFER_SIZE - samples);

    UINT value = 0;
    for (UINT i = 0; i < samples; i++)
    {
        value = WavPcmFile_ReadOne(m_hTapeWavPcmFile);
        *(m_TapeBuffer + TAPE_BUFFER_SIZE - samples + i) = (uint8_t)((value >> 24) & 0xff);
    }
    bool result = (value >= UINT_MAX / 2);
    return result;
}

void CALLBACK Emulator_TapeWriteCallback(int value, UINT samples)
{
    if (samples == 0) return;

    // Scroll buffer
    memmove(m_TapeBuffer, m_TapeBuffer + samples, TAPE_BUFFER_SIZE - samples);

    // Write samples to the file
    for (UINT i = 0; i < samples; i++)
    {
        WavPcmFile_WriteOne(m_hTapeWavPcmFile, value);
        //TODO: Check WavPcmFile_WriteOne result
        *(m_TapeBuffer + TAPE_BUFFER_SIZE - samples + i) = (uint8_t)((value >> 24) & 0xff);
    }
}

bool Emulator_OpenTape(LPCTSTR sFilePath)
{
    m_hTapeWavPcmFile = WavPcmFile_Open(sFilePath);
    if (m_hTapeWavPcmFile == INVALID_HANDLE_VALUE)
        return false;

    int sampleRate = WavPcmFile_GetFrequency(m_hTapeWavPcmFile);
    g_pBoard->SetTapeReadCallback(Emulator_TapeReadCallback, sampleRate);

    return true;
}

bool Emulator_CreateTape(LPCTSTR sFilePath)
{
    m_hTapeWavPcmFile = WavPcmFile_Create(sFilePath, 44100);
    if (m_hTapeWavPcmFile == INVALID_HANDLE_VALUE)
        return false;

    int sampleRate = WavPcmFile_GetFrequency(m_hTapeWavPcmFile);
    g_pBoard->SetTapeWriteCallback(Emulator_TapeWriteCallback, sampleRate);

    return true;
}

void Emulator_CloseTape()
{
    g_pBoard->SetTapeReadCallback(NULL, 0);
    g_pBoard->SetTapeWriteCallback(NULL, 0);

    WavPcmFile_Close(m_hTapeWavPcmFile);
    m_hTapeWavPcmFile = (HWAVPCMFILE) INVALID_HANDLE_VALUE;
}


//////////////////////////////////////////////////////////////////////

void CALLBACK Emulator_TeletypeCallback(uint8_t symbol)
{
    if (m_pEmulatorTeletypeBuffer == NULL)
        return;
    if (m_nEmulatorTeletypeBufferIndex >= m_nEmulatorTeletypeBufferSize)
        return;

    m_pEmulatorTeletypeBuffer[m_nEmulatorTeletypeBufferIndex] = symbol;
    m_nEmulatorTeletypeBufferIndex++;
    m_pEmulatorTeletypeBuffer[m_nEmulatorTeletypeBufferIndex] = 0;
}

const char * Emulator_GetTeletypeBuffer()
{
    return m_pEmulatorTeletypeBuffer;
}
void Emulator_AttachTeletypeBuffer(int bufferSize)
{
    ASSERT(bufferSize > 0);

    m_pEmulatorTeletypeBuffer = (char *) ::malloc(bufferSize + 1);
    m_nEmulatorTeletypeBufferIndex = 0;
    m_nEmulatorTeletypeBufferSize = bufferSize;
    m_pEmulatorTeletypeBuffer[0] = 0;

    g_pBoard->SetTeletypeCallback(Emulator_TeletypeCallback);
}
void Emulator_DetachTeletypeBuffer()
{
    g_pBoard->SetTeletypeCallback(NULL);

    if (m_pEmulatorTeletypeBuffer != NULL)
    {
        ::free(m_pEmulatorTeletypeBuffer);
        m_pEmulatorTeletypeBuffer = NULL;
    }
}


//////////////////////////////////////////////////////////////////////

const uint32_t * Emulator_GetPalette(int screenMode)
{
    if ((screenMode & 1) == 0)
        return (const uint32_t *)ScreenView_BWPalette;
    if ((g_nEmulatorConfiguration & BK_COPT_BK0011) == 0)
        return (const uint32_t *)ScreenView_ColorPalette;
    else
        return (const uint32_t *)ScreenView_ColorPalettes[g_pBoard->GetPalette()];
}

void Emulator_PrepareScreenRGB32(void* pImageBits, int screenMode)
{
    if (pImageBits == NULL) return;

    // Get scroll value
    uint16_t scroll = g_pBoard->GetPortView(0177664);
    bool okSmallScreen = ((scroll & 01000) == 0);
    scroll &= 0377;
    scroll = (scroll >= 0330) ? scroll - 0330 : 050 + scroll;

    const uint32_t * pPalette = Emulator_GetPalette(screenMode);

    const uint8_t* pVideoBuffer = g_pBoard->GetVideoBuffer();
    ASSERT(pVideoBuffer != NULL);

    // Render to bitmap
    PREPARE_SCREEN_CALLBACK callback = ScreenModeReference[screenMode].callback;
    callback(pVideoBuffer, okSmallScreen, pPalette, scroll, pImageBits);
}

void CALLBACK Emulator_PrepareScreenBW512x256(const uint8_t* pVideoBuffer, int okSmallScreen, const uint32_t* /*pPalette*/, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const uint16_t* pVideo = (uint16_t*)(pVideoBuffer + yy * 0100);
        uint32_t* pBits = (uint32_t*)pImageBits + (255 - y) * 512;
        for (int x = 0; x < 512 / 16; x++)
        {
            uint16_t src = *pVideo;

            for (int bit = 0; bit < 16; bit++)
            {
                uint32_t color = (src & 1) ? 0x0ffffff : 0;
                *pBits = color;
                pBits++;
                src = src >> 1;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((uint32_t*)pImageBits, 0, (256 - 64) * 512 * sizeof(uint32_t));
    }
}

void CALLBACK Emulator_PrepareScreenColor512x256(const uint8_t* pVideoBuffer, int okSmallScreen, const uint32_t* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const uint16_t* pVideo = (uint16_t*)(pVideoBuffer + yy * 0100);
        uint32_t* pBits = (uint32_t*)pImageBits + (255 - y) * 512;
        for (int x = 0; x < 512 / 16; x++)
        {
            uint16_t src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                uint32_t color = pPalette[src & 3];
                *pBits = color;
                pBits++;
                *pBits = color;
                pBits++;
                src = src >> 2;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((uint32_t*)pImageBits, 0, (256 - 64) * 512 * sizeof(uint32_t));
    }
}


//////////////////////////////////////////////////////////////////////

uint32_t Emulator_GetUptime()
{
    return m_dwEmulatorUptime;
}

bool Emulator_Run(int frames)
{
    for (int i = 0; i < frames; i++)
    {
        int res = Emulator_SystemFrame();
        if (!res)
            return false;
    }

    return true;
}

bool Emulator_SaveScreenshot(LPCTSTR sFileName, const uint32_t * bits, const uint32_t * palette)
{
    ASSERT(sFileName != NULL);
    ASSERT(bits != NULL);

    // Create file
    HANDLE hFile = ::CreateFile(sFileName,
            GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    BITMAPFILEHEADER hdr;
    ::ZeroMemory(&hdr, sizeof(hdr));
    hdr.bfType = 0x4d42;  // "BM"
    BITMAPINFOHEADER bih;
    ::ZeroMemory(&bih, sizeof(bih));
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = 512;
    bih.biHeight = 256;
    bih.biSizeImage = bih.biWidth * bih.biHeight / 2;
    bih.biPlanes = 1;
    bih.biBitCount = 4;
    bih.biCompression = BI_RGB;
    bih.biXPelsPerMeter = bih.biXPelsPerMeter = 2000;
    hdr.bfSize = (uint32_t) sizeof(BITMAPFILEHEADER) + bih.biSize + bih.biSizeImage;
    hdr.bfOffBits = (uint32_t) sizeof(BITMAPFILEHEADER) + bih.biSize + sizeof(RGBQUAD) * 16;

    DWORD dwBytesWritten = 0;

    uint8_t * pData = (uint8_t *) ::malloc(bih.biSizeImage);

    // Prepare the image data
    const uint32_t * psrc = bits;
    uint8_t * pdst = pData;
    for (int i = 0; i < 512 * 256; i++)
    {
        uint32_t rgb = *psrc;
        psrc++;
        uint8_t color = 0;
        for (uint8_t c = 0; c < 4; c++)
        {
            if (palette[c] == rgb)
            {
                color = c;
                break;
            }
        }
        if ((i & 1) == 0)
            *pdst = (color << 4);
        else
        {
            *pdst = (*pdst) & 0xf0 | color;
            pdst++;
        }
    }

    WriteFile(hFile, &hdr, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(BITMAPFILEHEADER))
    {
        ::free(pData);
        return false;
    }
    WriteFile(hFile, &bih, bih.biSize, &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(BITMAPINFOHEADER))
    {
        ::free(pData);
        return false;
    }
    uint32_t palette16[16];
    ::memcpy(palette16, palette, 4 * 16);
    WriteFile(hFile, palette16, sizeof(RGBQUAD) * 16, &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(RGBQUAD) * 16)
    {
        ::free(pData);
        return false;
    }
    WriteFile(hFile, pData, bih.biSizeImage, &dwBytesWritten, NULL);
    ::free(pData);
    if (dwBytesWritten != bih.biSizeImage)
        return false;

    // Close file
    CloseHandle(hFile);

    return true;
}

bool Emulator_SaveScreenshot(LPCTSTR sFileName, int screenMode)
{
    uint32_t * bits = (uint32_t *) ::malloc(512 * 256 * 4);

    Emulator_PrepareScreenRGB32(bits, screenMode);

    const uint32_t * palette = Emulator_GetPalette(screenMode);
    bool result = Emulator_SaveScreenshot(sFileName, bits, palette);

    ::free(bits);

    return result;
}
// Returns: amount of different pixels
int Emulator_CompareScreens(const uint32_t * scr1, const uint32_t * scr2)
{
    const uint32_t * p1 = scr1;
    const uint32_t * p2 = scr2;

    int result = 0;
    for (int i = 512 * 256; i > 0; i--)
    {
        if (*p1 != *p2)
            result++;
        p1++;  p2++;
    }

    return result;
}

// Returns: amount of different pixels
int Emulator_CheckScreenshot(LPCTSTR sFileName, const uint32_t * bits, const uint32_t * palette, uint32_t * tempbits)
{
    ASSERT(sFileName != NULL);
    ASSERT(bits != NULL);

    // Open file for reading
    HANDLE hFile = ::CreateFile(sFileName,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return -1;

    BITMAPFILEHEADER hdr;
    BITMAPINFOHEADER bih;
    DWORD dwBytesRead = 0;

    ReadFile(hFile, &hdr, sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
    if (dwBytesRead != sizeof(BITMAPFILEHEADER))
        return -1;
    //TODO: Check the header
    ReadFile(hFile, &bih, sizeof(BITMAPINFOHEADER), &dwBytesRead, NULL);
    if (dwBytesRead != sizeof(BITMAPINFOHEADER))
        return -1;
    //TODO: Check the header
    if (bih.biSizeImage != 512 * 256 / 2)
        return -1;
    // Skip the palette
    SetFilePointer(hFile, sizeof(RGBQUAD) * 16, 0, FILE_CURRENT);

    uint8_t * pData = (uint8_t *) ::malloc(bih.biSizeImage);

    ReadFile(hFile, pData, bih.biSizeImage, &dwBytesRead, NULL);
    if (dwBytesRead != bih.biSizeImage)
    {
        ::free(pData);
        return -1;
    }

    // Decode the image data
    uint8_t * psrc = pData;
    uint32_t * pdst = tempbits;
    for (int i = 0; i < 512 * 256; i++)
    {
        uint8_t color = 0;
        if ((i & 1) == 0)
            color = (*psrc) >> 4;
        else
        {
            color = (*psrc) & 15;
            psrc++;
        }
        *pdst = palette[color];
        pdst++;
    }

    ::free(pData);

    // Compare the screenshots
    int result = Emulator_CompareScreens(bits, tempbits);

    // Close file
    CloseHandle(hFile);

    return result;
}

int Emulator_CheckScreenshot(LPCTSTR sFileName, int screenMode)
{
    uint32_t * bits = (uint32_t *) ::malloc(512 * 256 * 4);
    uint32_t * tempbits = (uint32_t *) ::malloc(512 * 256 * 4);

    Emulator_PrepareScreenRGB32(bits, screenMode);

    const uint32_t * palette = Emulator_GetPalette(screenMode);
    int result = Emulator_CheckScreenshot(sFileName, bits, palette, tempbits);

    ::free(tempbits);
    ::free(bits);

    return result;
}

void Emulator_KeyboardEvent(uint8_t bkscan, bool okPressed)
{
    g_pBoard->KeyboardEvent(bkscan, okPressed, false);
    Emulator_Run(1);
}

void Emulator_KeyboardPressRelease(uint8_t bkscan, int timeout)
{
    g_pBoard->KeyboardEvent(bkscan, true, false);
    Emulator_Run(timeout);
    g_pBoard->KeyboardEvent(bkscan, false, false);
    Emulator_Run(3);
}

const uint8_t arrChar2BkScan[256] =
{
    /*       0     1     2     3     4     5     6     7     8     9     a     b     c     d     e     f  */
    /*0*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0030, 0000, 0012, 0000, 0000, 0000, 0000, 0000,
    /*1*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*2*/    0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047, 0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
    /*3*/    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067, 0070, 0071, 0072, 0073, 0074, 0275, 0076, 0077,
    /*4*/    0100, 0101, 0102, 0103, 0104, 0105, 0106, 0107, 0110, 0111, 0112, 0113, 0114, 0115, 0116, 0117,
    /*5*/    0120, 0121, 0122, 0123, 0124, 0125, 0126, 0127, 0130, 0131, 0132, 0133, 0134, 0135, 0136, 0137,
    /*6*/    0140, 0141, 0142, 0143, 0144, 0145, 0146, 0147, 0150, 0151, 0152, 0153, 0154, 0155, 0156, 0157,
    /*7*/    0160, 0161, 0162, 0163, 0164, 0165, 0166, 0167, 0170, 0171, 0172, 0173, 0174, 0175, 0176, 0000,
    /*8*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*9*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*a*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*b*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*c*/    0007, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*d*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*e*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
    /*f*/    0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
};

void Emulator_KeyboardPressReleaseChar(char ch, int timeout)
{
    uint8_t scan = arrChar2BkScan[(uint8_t)ch];
    if (scan == 0)
        return;
    Emulator_KeyboardPressRelease(scan, timeout);
}

void Emulator_KeyboardSequence(const char * str)
{
    const char * p = str;
    while (*p != 0)
    {
        Emulator_KeyboardPressReleaseChar(*p);
        p++;
    }
}

bool Emulator_LoadBin(LPCTSTR strFileName)
{
    // Open file for reading
    HANDLE hFile = CreateFile(strFileName,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        //AlertWarning(_T("Failed to load binary file."));
        return false;
    }

    // Load BIN header
    uint8_t bufHeader[20];
    DWORD bytesRead;
    ::ReadFile(hFile, bufHeader, 4, &bytesRead, NULL);
    if (bytesRead != 4)
    {
        ::CloseHandle(hFile);
        //AlertWarning(_T("Failed to load binary file."));
        return false;
    }

    uint16_t baseAddress = *((uint16_t*)bufHeader);
    uint16_t dataSize = *(((uint16_t*)bufHeader) + 1);

    // Get file size
    uint32_t bytesToRead = dataSize;
    uint16_t memoryBytes = (dataSize + 1) & 0xfffe;

    // Allocate memory
    uint8_t* pBuffer = (uint8_t*)::LocalAlloc(LPTR, memoryBytes);

    // Load file data
    ::ReadFile(hFile, pBuffer, dataSize, &bytesRead, NULL);
    if (bytesRead != bytesToRead)
    {
        ::LocalFree(pBuffer);
        ::CloseHandle(hFile);
        //AlertWarning(_T("Failed to load binary file."));
        return false;
    }

    // Copy data to BK memory
    uint16_t address = baseAddress;
    uint16_t* pData = (uint16_t*)pBuffer;
    while (address < baseAddress + memoryBytes)
    {
        uint16_t value = *pData++;
        g_pBoard->SetRAMWord(address, value);
        address += 2;
    }

    ::LocalFree(pBuffer);
    ::CloseHandle(hFile);

    return true;
}


//////////////////////////////////////////////////////////////////////
