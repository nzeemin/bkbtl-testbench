/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

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

BOOL g_okEmulatorInitialized = FALSE;
BOOL g_okEmulatorRunning = FALSE;

WORD m_wEmulatorCPUBreakpoint = 0177777;

DWORD m_dwTickCount = 0;
DWORD m_dwEmulatorUptime = 0;  // BK uptime, seconds, from turn on or reset, increments every 25 frames
long m_nUptimeFrameCount = 0;

char * m_pEmulatorTeletypeBuffer = NULL;
int m_nEmulatorTeletypeBufferSize = 0;
int m_nEmulatorTeletypeBufferIndex = 0;

HWAVPCMFILE m_hTapeWavPcmFile = (HWAVPCMFILE) INVALID_HANDLE_VALUE;
#define TAPE_BUFFER_SIZE 624
BYTE m_TapeBuffer[TAPE_BUFFER_SIZE];
BOOL CALLBACK Emulator_TapeReadCallback(UINT samples);
BOOL CALLBACK Emulator_TapeWriteCallback(UINT samples);


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


//Прототип функции преобразования экрана
// Input:
//   pVideoBuffer   Исходные данные, биты экрана БК
//   okSmallScreen  Признак "малого" экрана
//   pPalette       Палитра
//   scroll         Текущее значение скроллинга
//   pImageBits     Результат, 32-битный цвет, размер для каждой функции свой
typedef void (CALLBACK* PREPARE_SCREEN_CALLBACK)(const BYTE* pVideoBuffer, int okSmallScreen, const DWORD* pPalette, int scroll, void* pImageBits);

void CALLBACK Emulator_PrepareScreenBW512x256(const BYTE* pVideoBuffer, int okSmallScreen, const DWORD* pPalette, int scroll, void* pImageBits);
void CALLBACK Emulator_PrepareScreenColor512x256(const BYTE* pVideoBuffer, int okSmallScreen, const DWORD* pPalette, int scroll, void* pImageBits);

struct ScreenModeStruct
{
    int width;
    int height;
    PREPARE_SCREEN_CALLBACK callback;
}
static ScreenModeReference[] = {
    { 512, 256, Emulator_PrepareScreenBW512x256 },
    { 512, 256, Emulator_PrepareScreenColor512x256 },
};

const DWORD ScreenView_BWPalette[4] = {
    0x000000, 0xFFFFFF, 0x000000, 0xFFFFFF
};

const DWORD ScreenView_ColorPalette[4] = {
    0x000000, 0x0000FF, 0x00FF00, 0xFF0000
};

const DWORD ScreenView_ColorPalettes[16][4] = {
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


BOOL Emulator_LoadRomFile(LPCTSTR strFileName, BYTE* buffer, DWORD fileOffset, DWORD bytesToRead)
{
    FILE* fpRomFile = ::_tfsopen(strFileName, _T("rb"), _SH_DENYWR);
    if (fpRomFile == NULL)
        return FALSE;

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
        return FALSE;
    }

    ::fclose(fpRomFile);

    return TRUE;
}

BOOL Emulator_Init()
{
    ASSERT(g_pBoard == NULL);

    CProcessor::Init();

    g_pBoard = new CMotherboard();

    g_pBoard->Reset();

    g_okEmulatorInitialized = TRUE;
    return TRUE;
}

BOOL Emulator_InitConfiguration(BKConfiguration configuration)
{
    g_pBoard->SetConfiguration(configuration);

    BYTE buffer[8192];

    if ((configuration & BK_COPT_BK0011) == 0)
    {
        // Load Monitor ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_MONIT10, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Monitor ROM file."));
            return FALSE;
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
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BASIC ROM 2 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_2, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BASIC ROM 2 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(2, buffer);
    }
    if ((configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_ROM_BASIC) != 0)  // BK 0010 BASIC ROM 3
    {
        // Load BASIC ROM 3 file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC10_3, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load BASIC ROM 3 file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }
    if ((configuration & BK_COPT_BK0011) == 0 && (configuration & BK_COPT_ROM_FOCAL) != 0)  // BK 0010 FOCAL
    {
        // Load Focal ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_FOCAL, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load Focal ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Unused 8KB
        ::memset(buffer, 0, 8192);
        g_pBoard->LoadROM(2, buffer);
        // Load Tests ROM file
        if (!Emulator_LoadRomFile(FILENAME_BKROM_TESTS, buffer, 0, 8064))
        {
            AlertWarning(_T("Failed to load Tests ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
    }

    if (configuration & BK_COPT_BK0011)
    {
        // Load BK0011M BASIC 0, part 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(0, buffer);
        // Load BK0011M BASIC 0, part 2
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_0, buffer, 8192, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 0 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(1, buffer);
        // Load BK0011M BASIC 1
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BASIC11M_1, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BASIC 1 ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(2, buffer);

        // Load BK0011M EXT
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_EXT, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M EXT ROM file."));
            return FALSE;
        }
        g_pBoard->LoadROM(3, buffer);
        // Load BK0011M BOS
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_BOS, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M BOS ROM file."));
            return FALSE;
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
            return FALSE;
        }
        g_pBoard->LoadROM((configuration & BK_COPT_BK0011) ? 5 : 3, buffer);
    }

    if ((configuration & BK_COPT_BK0011) && (configuration & BK_COPT_FDD) == 0)
    {
        // Load BK0011M MSTD
        if (!Emulator_LoadRomFile(FILENAME_BKROM_BK11M_MSTD, buffer, 0, 8192))
        {
            AlertWarning(_T("Failed to load BK11M MSTD ROM file."));
            return FALSE;
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

    return TRUE;
}

void Emulator_Done()
{
    ASSERT(g_pBoard != NULL);

    CProcessor::Done();

    delete g_pBoard;
    g_pBoard = NULL;
}

void Emulator_Start()
{
    g_okEmulatorRunning = TRUE;

    //m_nFrameCount = 0;
    //m_dwTickCount = GetTickCount();
}
void Emulator_Stop()
{
    g_okEmulatorRunning = FALSE;
    m_wEmulatorCPUBreakpoint = 0177777;
}

void Emulator_Reset()
{
    ASSERT(g_pBoard != NULL);

    g_pBoard->Reset();

    m_nUptimeFrameCount = 0;
    m_dwEmulatorUptime = 0;
}

void Emulator_SetCPUBreakpoint(WORD address)
{
    m_wEmulatorCPUBreakpoint = address;
}
BOOL Emulator_IsBreakpoint()
{
    WORD wCPUAddr = g_pBoard->GetCPU()->GetPC();
    if (wCPUAddr == m_wEmulatorCPUBreakpoint)
        return TRUE;
    return FALSE;
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

    return 1;
}

BOOL Emulator_AttachFloppyImage(int slot, LPCTSTR sFilePath)
{
    return g_pBoard->AttachFloppyImage(slot, sFilePath);
}

// Tape emulator callback used to read a tape recorded data.
// Input:
//   samples    Number of samples to play.
// Output:
//   result     Bit to put in tape input port.
BOOL CALLBACK Emulator_TapeReadCallback(unsigned int samples)
{
	if (samples == 0) return 0;

    // Scroll buffer
    memmove(m_TapeBuffer, m_TapeBuffer + samples, TAPE_BUFFER_SIZE - samples);

	UINT value = 0;
	for (UINT i = 0; i < samples; i++)
	{
		value = WavPcmFile_ReadOne(m_hTapeWavPcmFile);
        *(m_TapeBuffer + TAPE_BUFFER_SIZE - samples + i) = (BYTE)((value >> 24) & 0xff);
	}
	BOOL result = (value >= UINT_MAX / 2);
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
        *(m_TapeBuffer + TAPE_BUFFER_SIZE - samples + i) = (BYTE)((value >> 24) & 0xff);
    }
}

BOOL Emulator_OpenTape(LPCTSTR sFilePath)
{
	m_hTapeWavPcmFile = WavPcmFile_Open(sFilePath);
	if (m_hTapeWavPcmFile == INVALID_HANDLE_VALUE)
		return FALSE;

    int sampleRate = WavPcmFile_GetFrequency(m_hTapeWavPcmFile);
    g_pBoard->SetTapeReadCallback(Emulator_TapeReadCallback, sampleRate);

    return TRUE;
}

BOOL Emulator_CreateTape(LPCTSTR sFilePath)
{
	m_hTapeWavPcmFile = WavPcmFile_Create(sFilePath, 44100);
	if (m_hTapeWavPcmFile == INVALID_HANDLE_VALUE)
		return FALSE;

    int sampleRate = WavPcmFile_GetFrequency(m_hTapeWavPcmFile);
    g_pBoard->SetTapeWriteCallback(Emulator_TapeWriteCallback, sampleRate);

    return TRUE;
}

void Emulator_CloseTape()
{
    g_pBoard->SetTapeReadCallback(NULL, 0);
    g_pBoard->SetTapeWriteCallback(NULL, 0);

    WavPcmFile_Close(m_hTapeWavPcmFile);
	m_hTapeWavPcmFile = (HWAVPCMFILE) INVALID_HANDLE_VALUE;
}

void CALLBACK Emulator_PrepareScreenBW512x256(const BYTE* pVideoBuffer, int okSmallScreen, const DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + (255 - y) * 512;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit++)
            {
                DWORD color = (src & 1) ? 0x0ffffff : 0;
                *pBits = color;
                pBits++;
                src = src >> 1;
            }

            pVideo++;
        }
    }
    if (okSmallScreen)
    {
        memset((DWORD*)pImageBits, 0, (256 - 64) * 512 * sizeof(DWORD));
    }
}

void CALLBACK Emulator_PrepareScreenColor512x256(const BYTE* pVideoBuffer, int okSmallScreen, const DWORD* pPalette, int scroll, void* pImageBits)
{
    int linesToShow = okSmallScreen ? 64 : 256;
    for (int y = 0; y < linesToShow; y++)
    {
        int yy = (y + scroll) & 0377;
        const WORD* pVideo = (WORD*)(pVideoBuffer + yy * 0100);
        DWORD* pBits = (DWORD*)pImageBits + (255 - y) * 512;
        for (int x = 0; x < 512 / 16; x++)
        {
            WORD src = *pVideo;

            for (int bit = 0; bit < 16; bit += 2)
            {
                DWORD color = pPalette[src & 3];
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
        memset((DWORD*)pImageBits, 0, (256 - 64) * 512 * sizeof(DWORD));
    }
}

const DWORD * Emulator_GetPalette(int screenMode)
{
    const DWORD * pPalette = ScreenView_BWPalette;
    if ((screenMode & 1) != 0)
    {
        if ((g_nEmulatorConfiguration & BK_COPT_BK0011) == 0)
            pPalette = (DWORD*)ScreenView_ColorPalette;
        else
            pPalette = (DWORD*)ScreenView_ColorPalettes[g_pBoard->GetPalette()];
    }
    return pPalette;
}

void Emulator_PrepareScreenRGB32(void* pImageBits, int screenMode)
{
    if (pImageBits == NULL) return;

    // Get scroll value
    WORD scroll = g_pBoard->GetPortView(0177664);
    BOOL okSmallScreen = ((scroll & 01000) == 0);
    scroll &= 0377;
    scroll = (scroll >= 0330) ? scroll - 0330 : 050 + scroll;

    const DWORD * pPalette = Emulator_GetPalette(screenMode);

    const BYTE* pVideoBuffer = g_pBoard->GetVideoBuffer();
    ASSERT(pVideoBuffer != NULL);

    // Render to bitmap
    PREPARE_SCREEN_CALLBACK callback = ScreenModeReference[screenMode].callback;
    callback(pVideoBuffer, okSmallScreen, pPalette, scroll, pImageBits);
}

DWORD Emulator_GetUptime()
{
    return m_dwEmulatorUptime;
}

BOOL Emulator_Run(int frames)
{
    for (int i = 0; i < frames; i++)
    {
        int res = Emulator_SystemFrame();
        if (!res)
            return FALSE;
    }

    return TRUE;
}

BOOL Emulator_SaveScreenshot(LPCTSTR sFileName, const DWORD * bits, const DWORD * palette)
{
    ASSERT(sFileName != NULL);
    ASSERT(bits != NULL);

    // Create file
    HANDLE hFile = ::CreateFile(sFileName,
            GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

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
    hdr.bfSize = (DWORD) sizeof(BITMAPFILEHEADER) + bih.biSize + bih.biSizeImage;
    hdr.bfOffBits = (DWORD) sizeof(BITMAPFILEHEADER) + bih.biSize + sizeof(RGBQUAD) * 16;

    DWORD dwBytesWritten = 0;

    BYTE * pData = (BYTE *) ::malloc(bih.biSizeImage);

    // Prepare the image data
    const DWORD * psrc = bits;
    BYTE * pdst = pData;
    for (int i = 0; i < 512 * 256; i++)
    {
        DWORD rgb = *psrc;
        psrc++;
        BYTE color = 0;
        for (BYTE c = 0; c < 4; c++)
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
        return FALSE;
    }
    WriteFile(hFile, &bih, bih.biSize, &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(BITMAPINFOHEADER))
    {
        ::free(pData);
        return FALSE;
    }
    DWORD palette16[16];
    ::memcpy(palette16, palette, 4 * 16);
    WriteFile(hFile, palette16, sizeof(RGBQUAD) * 16, &dwBytesWritten, NULL);
    if (dwBytesWritten != sizeof(RGBQUAD) * 16)
    {
        ::free(pData);
        return FALSE;
    }
    WriteFile(hFile, pData, bih.biSizeImage, &dwBytesWritten, NULL);
    ::free(pData);
    if (dwBytesWritten != bih.biSizeImage)
        return FALSE;

    // Close file
    CloseHandle(hFile);

    return TRUE;
}

BOOL Emulator_SaveScreenshot(LPCTSTR sFileName, int screenMode)
{
    DWORD * bits = (DWORD *) ::malloc(512 * 256 * 4);

    Emulator_PrepareScreenRGB32(bits, screenMode);

    const DWORD * palette = Emulator_GetPalette(screenMode);
    BOOL result = Emulator_SaveScreenshot(sFileName, bits, palette);

    ::free(bits);

    return result;
}
// Returns: amount of different pixels
int Emulator_CompareScreens(const DWORD * scr1, const DWORD * scr2)
{
    const DWORD * p1 = scr1;
    const DWORD * p2 = scr2;

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
int Emulator_CheckScreenshot(LPCTSTR sFileName, const DWORD * bits, const DWORD * palette, DWORD * tempbits)
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

    BYTE * pData = (BYTE *) ::malloc(bih.biSizeImage);

    ReadFile(hFile, pData, bih.biSizeImage, &dwBytesRead, NULL);
    if (dwBytesRead != bih.biSizeImage)
    {
        ::free(pData);
        return -1;
    }

    // Decode the image data
    BYTE * psrc = pData;
    DWORD * pdst = tempbits;
    for (int i = 0; i < 512 * 256; i++)
    {
        BYTE color = 0;
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
    DWORD * bits = (DWORD *) ::malloc(512 * 256 * 4);
    DWORD * tempbits = (DWORD *) ::malloc(512 * 256 * 4);

    Emulator_PrepareScreenRGB32(bits, screenMode);

    const DWORD * palette = Emulator_GetPalette(screenMode);
    int result = Emulator_CheckScreenshot(sFileName, bits, palette, tempbits);

    ::free(tempbits);
    ::free(bits);

    return result;
}

void Emulator_KeyboardEvent(BYTE bkscan, BOOL okPressed)
{
    g_pBoard->KeyboardEvent(bkscan, okPressed, FALSE);
    Emulator_Run(1);
}

void Emulator_KeyboardPressRelease(BYTE bkscan, int timeout)
{
    g_pBoard->KeyboardEvent(bkscan, TRUE, FALSE);
    Emulator_Run(timeout);
    g_pBoard->KeyboardEvent(bkscan, FALSE, FALSE);
    Emulator_Run(3);
}

const BYTE arrChar2BkScan[256] = {
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
    BYTE scan = arrChar2BkScan[(BYTE)ch];
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

BOOL Emulator_LoadBin(LPCTSTR strFileName)
{
    // Open file for reading
    HANDLE hFile = CreateFile(strFileName,
            GENERIC_READ, FILE_SHARE_READ, NULL,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        //AlertWarning(_T("Failed to load binary file."));
        return FALSE;
    }

    // Load BIN header
    BYTE bufHeader[20];
	DWORD bytesRead;
	::ReadFile(hFile, bufHeader, 4, &bytesRead, NULL);
    if (bytesRead != 4)
    {
        ::CloseHandle(hFile);
        //AlertWarning(_T("Failed to load binary file."));
        return FALSE;
    }

    WORD baseAddress = *((WORD*)bufHeader);
    WORD dataSize = *(((WORD*)bufHeader) + 1);

    // Get file size
    DWORD bytesToRead = dataSize;
    WORD memoryBytes = (dataSize + 1) & 0xfffe;

    // Allocate memory
    BYTE* pBuffer = (BYTE*)::LocalAlloc(LPTR, memoryBytes);

    // Load file data
	::ReadFile(hFile, pBuffer, dataSize, &bytesRead, NULL);
    if (bytesRead != bytesToRead)
    {
        ::LocalFree(pBuffer);
        ::CloseHandle(hFile);
        //AlertWarning(_T("Failed to load binary file."));
        return FALSE;
    }

    // Copy data to BK memory
    WORD address = baseAddress;
    WORD* pData = (WORD*)pBuffer;
    while (address < baseAddress + memoryBytes)
    {
        WORD value = *pData++;
        g_pBoard->SetRAMWord(address, value);
        address += 2;
    }

    ::LocalFree(pBuffer);
    ::CloseHandle(hFile);

    return TRUE;
}

void CALLBACK Emulator_TeletypeCallback(BYTE symbol)
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
