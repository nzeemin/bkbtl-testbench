/*  This file is part of BKBTL.
    BKBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    BKBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
BKBTL. If not, see <http://www.gnu.org/licenses/>. */

// main.cpp

#include "stdafx.h"
#include "Emulator.h"
#include "emubase\\Emubase.h"


void Test01_Basic10()
{
    Test_Init(_T("TEST 1: BASIC BK0010"), BK_CONF_BK0010_BASIC);

    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test01_01.bmp"), 1);

    Emulator_KeyboardSequence("PRINT PI\n");

    Emulator_KeyboardSequence("05 COLOR 2\n");
    Emulator_KeyboardSequence("10 FOR I=32 TO 127\n");
    Emulator_KeyboardSequence("20 PRINT CHR$(I);\n");
    Emulator_KeyboardSequence("30 IF I MOD 16 = 15 THEN PRINT\n");
    Emulator_KeyboardSequence("50 NEXT I\n");
    Emulator_KeyboardSequence("55 COLOR 3\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(25);  // Wait 1 second
    Test_CheckScreenshot(_T("data\\test01_02.bmp"), 1);

    Emulator_KeyboardSequence("COLOR 1\n");
    Emulator_KeyboardSequence("CLS\n");
    Emulator_KeyboardSequence("1  !\"#$%&'()*+,-./\n");
    Emulator_KeyboardSequence("2 0123456789:;<=>?\n");
    Emulator_KeyboardSequence("3 @[\\]^_ `{|}~\n");
    Emulator_KeyboardSequence("4 ABCDEFGHIJKLMNOPQRSTUVWXYZ\n");
    Emulator_KeyboardSequence("5 abcdefghijklmnopqrstuvwxyz\n");
    Test_SaveScreenshot(_T("test01_03.bmp"));

    // BASIC speed test by Sergey Frolov, see http://www.leningrad.su/calc/speed.php
    Emulator_Reset();
    Emulator_Run(50);
    Emulator_KeyboardSequence("4 FOR I = 1 TO 10\n");
    Emulator_KeyboardSequence("5 A = 1.0000001\n");
    Emulator_KeyboardSequence("10 B = A\n");
    Emulator_KeyboardSequence("15 FOR J = 1 TO 27\n");
    Emulator_KeyboardSequence("20 A = A * A\n");
    Emulator_KeyboardSequence("25 B = B ^ 2.01\n");
    Emulator_KeyboardSequence("30 NEXT J\n");
    Emulator_KeyboardSequence("35 NEXT I\n");
    Emulator_KeyboardSequence("40 PRINT A, B\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(1084);
    Test_CheckScreenshot(_T("data\\test01_04.bmp"));

    // Random number generator check by Leonid Broukhis http://www.mailcom.com/bk0010/
    Emulator_Reset();
    Emulator_Run(50);
    Emulator_KeyboardSequence("10 CLS\n");
    Emulator_KeyboardSequence("20 FOR I=0 TO 1000\n");
    Emulator_KeyboardSequence("30 PSET (RND(1)*512, RND(1)*240)\n");
    Emulator_KeyboardSequence("40 NEXT\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(230);
    Test_CheckScreenshot(_T("data\\test01_05.bmp"));
    Emulator_Reset();
    Emulator_Run(50);
    Emulator_KeyboardSequence("10 CLS\n");
    Emulator_KeyboardSequence("20 FOR I%=0% TO 32766%\n");
    Emulator_KeyboardSequence("30 PSET (RND(1)*256%, RND(1)*240%), RND(1)*3%+1%\n");
    Emulator_KeyboardSequence("40 NEXT\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(10950);
    Test_CheckScreenshot(_T("data\\test01_06.bmp"), 1);

    // Run infinite loop and press STOP key
    Emulator_Reset();
    Emulator_Run(50);
    Emulator_KeyboardSequence("10 GOTO 10\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(100);
    Emulator_KeyboardPressRelease(BK_KEY_STOP);
    Test_CheckScreenshot(_T("data\\test01_07.bmp"));

    // Load and run COOL.COD program
    Emulator_Reset();
    Emulator_Run(50);
    g_okEmulatorAutoTapeReading = true;
    Emulator_KeyboardSequence("CLOAD \"COOL\"\n");
    Emulator_Run(5);
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(100);
    Test_CheckScreenshot(_T("data\\test01_08.bmp"), 1);

    //Test_SaveScreenshotSeria(_T("video\\test01_%04u.bmp"), 20, 10);

    Test_Done();
}

void Test011_Basic10_Cassette()
{
    Test_Init(_T("TEST 11: BASIC BK0010 CASSETTE"), BK_CONF_BK0010_BASIC);

    Emulator_Run(50);

    Emulator_KeyboardSequence("10 PRINT PI\n");

    Emulator_KeyboardSequence("CSAVE \"PI\"\n");
    Test_CreateTape(_T("temp\\test011_01.wav"));
    Emulator_Run(25 * 12);
    Test_CloseTape();
    Test_SaveScreenshot(_T("data\\test011_01.bmp"));

    Emulator_KeyboardSequence("NEW\n");
    Emulator_Run(20);
    Emulator_KeyboardSequence("CLOAD \"PI\"\n");
    Test_OpenTape(_T("temp\\test011_01.wav"));
    Emulator_Run(25 * 12);
    Test_CloseTape();
    Emulator_KeyboardSequence("LIST\n");
    Emulator_Run(5);
    Test_SaveScreenshot(_T("data\\test011_02.bmp"));

    Test_Done();
}

void Test02_Focal10()
{
    Test_Init(_T("TEST 2: Focal BK0010"), BK_CONF_BK0010_FOCAL);

    Emulator_Run(50);
    Emulator_KeyboardSequence("V\n");
    Test_CheckScreenshot(_T("data\\test02_01.bmp"));

    //// BASIC speed test by Sergey Frolov, see http://www.leningrad.su/calc/speed.php
    //Emulator_KeyboardSequence("01.04 F I=1,10; DO 2\n");
    //Emulator_KeyboardSequence("01.40 T A,B,!\n");
    //Emulator_KeyboardSequence("01.50 Q\n");
    //Emulator_KeyboardSequence("02.05 S A=1.0000001\n");
    //Emulator_KeyboardSequence("02.10 S B=A\n");
    //Emulator_KeyboardSequence("02.15 F J=1,27; DO 3\n");
    //Emulator_KeyboardSequence("03.20 S A=A*A\n");
    //Emulator_KeyboardSequence("03.25 S B=B^2.01\n");
    ////Emulator_KeyboardSequence("03.40 T J,A,B,!\n");
    //Emulator_KeyboardSequence("GO\n");
    //Emulator_Run(200);
    //Test_SaveScreenshot(_T("test02_02.bmp"));
    ////NOTE: ��������� �������� � ������� ������������ ��� J=26 -- ������, �� ������� ��������

    // PREZIDENT.FOC
    Emulator_Reset();
    Emulator_Run(50);
    g_okEmulatorAutoTapeReading = true;
    g_pEmulatorAutoTapeReadingFilename = _T("data\\PREZIDENT.FOC");
    Emulator_KeyboardSequence("L G PREZIDENT\n");
    Emulator_Run(50);
    Emulator_KeyboardSequence("GO\n");  // Run the program
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test02_03.bmp"));
    Emulator_KeyboardSequence("1\n");
    Emulator_Run(175);
    Test_CheckScreenshot(_T("data\\test02_04.bmp"));
    Emulator_KeyboardSequence("30\n");
    Emulator_Run(25);
    Emulator_KeyboardSequence("10000\n");
    Emulator_Run(125);
    Test_CheckScreenshot(_T("data\\test02_05.bmp"));

    Test_Done();
}

void Test03_Tmos()
{
    Test_Init(_T("TEST 3: TMOS tests"), BK_CONF_BK0010_FOCAL);

    Emulator_Run(50);
    Emulator_KeyboardSequence("P M\n");
    Emulator_Run(50);
    Test_SaveScreenshot(_T("test03_01.bmp"));
    Test_LoadBin(_T("data\\791401.bin"));
    Emulator_AttachTeletypeBuffer();
    Emulator_KeyboardSequence("S1000\n");
    //Test_SaveScreenshot(_T("test03_02.bmp"));
    Emulator_Run(300);  // Wait while the test runs 3 times
    const char * teletype = Emulator_GetTeletypeBuffer();
    if (0 == strcmp(teletype, "\r\n\x0ek prohod\r\n\x0ek prohod\r\n\x0ek prohod"))
        Test_LogInfo(_T("Teletype check passed"));
    else
        Test_LogError(_T("Teletype check FAILED"));
    Emulator_DetachTeletypeBuffer();
    //Test_SaveScreenshot(_T("test03_03.bmp"));

    Test_Done();
}

void Test031_Tricks()
{
    Test_Init(_T("TEST 31: Tricks"), BK_CONF_BK0010_BASIC);

    // Double loop using INC PC, see http://zx.pk.ru/showpost.php?p=283480&postcount=29
    Emulator_Run(50);
    Emulator_KeyboardSequence("P M\n");
    g_pBoard->SetRAMWord(001010, 000240);  // 001010: 0240    NOP
    g_pBoard->SetRAMWord(001012, 000240);  // 001012: 0240    NOP
    g_pBoard->SetRAMWord(001014, 000240);  // 001014: 0240    NOP
    g_pBoard->SetRAMWord(001016, 000240);  // 001016: 0240    NOP
    g_pBoard->SetRAMWord(001020, 005207);  // 001020: 005207  INC PC
    g_pBoard->SetRAMWord(001022, 000772);  // 001022: 000772  BR  1010
    g_pBoard->SetRAMWord(001024, 000240);  // 001024: 0240    NOP
    g_pBoard->GetCPU()->SetPC(001020);
    g_pBoard->DebugTicks();  // INC PC
    Test_Assert(01023 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // BR 1010
    Test_Assert(01011 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // NOP
    Test_Assert(01013 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // NOP
    Test_Assert(01015 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // NOP
    Test_Assert(01017 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // NOP
    Test_Assert(01021 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // INC PC
    Test_Assert(01024 == g_pBoard->GetCPU()->GetPC());
    g_pBoard->DebugTicks();  // NOP
    Test_Assert(01026 == g_pBoard->GetCPU()->GetPC());

    Test_Done();
}

void Test04_MSTD11()
{
    Test_Init(_T("TEST 4: BK0011M MSTD"), BK_CONF_BK0011);

    Emulator_KeyboardEvent(0040, true);  // Hold Spacebar to boot to monitor
    Emulator_Run(75);
    Emulator_KeyboardEvent(0040, false);
    Emulator_KeyboardSequence("160100G");
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_01.bmp"), 1);  // Menu

    // RAM test
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start RAM test
    Emulator_Run(80 * 25);
    Test_CheckScreenshot(_T("data\\test04_02.bmp"), 1);  // RAM test results
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test
    Emulator_Run(50);

    // ROM test
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start ROM test
    Emulator_Run(16 * 25);
    Test_CheckScreenshot(_T("data\\test04_03.bmp"), 1);  // ROM test results
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test

    // Keyboard test
    Emulator_Run(25);
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start keyboard test
    Emulator_Run(75);
    Test_CheckScreenshot(_T("data\\test04_04_1.bmp"), 1);
    Emulator_KeyboardPressRelease(0016);  // RUS
    Emulator_KeyboardPressRelease(BK_KEY_REPEAT);
    Emulator_KeyboardPressRelease(0003);  // KT
    Emulator_KeyboardPressRelease(0231);  // =|=>|  //TODO
    Emulator_KeyboardPressRelease(0026);  // |<===
    Emulator_KeyboardPressRelease(0027);  // |===>
    Emulator_KeyboardPressRelease(0202);  // IND SU
    Emulator_KeyboardPressRelease(0204);  // BLOK RED
    Emulator_KeyboardPressRelease(0220);  // STEP
    Emulator_KeyboardPressRelease(0014);  // SBR
    Emulator_KeyboardPressRelease(BK_KEY_STOP);
    //Emulator_KeyboardEvent(BK_KEY_BACKSHIFT, true);
    Emulator_KeyboardPressRelease(0073);  // ; +  //TODO: with BackShift
    Emulator_KeyboardPressRelease(0061);  // 1
    Emulator_KeyboardPressRelease(0062);
    Emulator_KeyboardPressRelease(0063);
    Emulator_KeyboardPressRelease(0064);
    Emulator_KeyboardPressRelease(0065);
    Emulator_KeyboardPressRelease(0066);
    Emulator_KeyboardPressRelease(0067);
    Emulator_KeyboardPressRelease(0070);
    Emulator_KeyboardPressRelease(0071);
    Emulator_KeyboardPressRelease(0060);  // 0
    Emulator_KeyboardPressRelease(0055);  // - =
    Emulator_KeyboardPressRelease(0072);  // : *
    Emulator_KeyboardPressRelease(0030);  // Backspace
    Emulator_KeyboardPressRelease(0015);  // TAB
    Emulator_KeyboardPressRelease(0112);  // � J
    Emulator_KeyboardPressRelease(0103);  // � C
    Emulator_KeyboardPressRelease(0125);  // � U
    Emulator_KeyboardPressRelease(0113);  // � K
    Emulator_KeyboardPressRelease(0105);  // � E
    Emulator_KeyboardPressRelease(0116);  // � N
    Emulator_KeyboardPressRelease(0107);  // � G
    Emulator_KeyboardPressRelease(0133);  // � [
    Emulator_KeyboardPressRelease(0135);  // � ]
    Emulator_KeyboardPressRelease(0132);  // � Z
    Emulator_KeyboardPressRelease(0110);  // � H
    Emulator_KeyboardPressRelease(0137);  // � }
    Emulator_KeyboardPressRelease(0057);  // / ?
    Emulator_KeyboardPressRelease(0023);  // ��
    Emulator_KeyboardPressRelease(0006);  // SU + F
    Emulator_KeyboardPressRelease(0131);  // � Y
    Emulator_KeyboardPressRelease(0127);  // � W
    Emulator_KeyboardPressRelease(0101);  // � A
    Emulator_KeyboardPressRelease(0120);  // � P
    Emulator_KeyboardPressRelease(0122);  // � R
    Emulator_KeyboardPressRelease(0117);  // � O
    Emulator_KeyboardPressRelease(0114);  // � L
    Emulator_KeyboardPressRelease(0104);  // � D
    Emulator_KeyboardPressRelease(0126);  // � V
    Emulator_KeyboardPressRelease(0134);  // � Backslash
    Emulator_KeyboardPressRelease(0076);  // . >  //TODO
    Emulator_KeyboardPressRelease(0012);  // ENTER
    //Emulator_KeyboardEvent(BK_KEY_LOWER, true);
    Emulator_KeyboardPressRelease(0121);  // � Q  //TODO: with ???
    Emulator_KeyboardPressRelease(0136);  // � ^  //TODO
    Emulator_KeyboardPressRelease(0123);  // � S  //TODO
    Emulator_KeyboardPressRelease(0115);  // � M  //TODO
    Emulator_KeyboardPressRelease(0111);  // � I  //TODO
    Emulator_KeyboardPressRelease(0124);  // � T  //TODO
    Emulator_KeyboardPressRelease(0130);  // � X  //TODO
    Emulator_KeyboardPressRelease(0102);  // � B  //TODO
    Emulator_KeyboardPressRelease(0100);  // � @  //TODO
    Emulator_KeyboardPressRelease(0074);  // , <  //TODO
    Emulator_KeyboardPressRelease(0010);  // Left
    Emulator_KeyboardPressRelease(0032);  // Up
    Emulator_KeyboardPressRelease(0031);  // Right
    Emulator_KeyboardPressRelease(0033);  // Down
    Emulator_KeyboardPressRelease(0040);  //TODO: AP2? + Space
    Emulator_KeyboardPressRelease(0017);  // LAT
    Test_SaveScreenshot(_T("test04_04_2.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test

    // Tape port test
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start tape port test
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_05_1.bmp"), 1);
    Test_CreateTape(_T("temp\\test04_05.wav"));
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(10 * 25);
    Test_CloseTape();
    Test_CheckScreenshot(_T("data\\test04_05_2.bmp"), 1);
    Test_OpenTape(_T("temp\\test04_05.wav"));
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(10 * 25);
    Test_CloseTape();
    Test_CheckScreenshot(_T("data\\test04_05_3.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test

    // Palette test
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- start palette test
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_07_00.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_01.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_02.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_03.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_04.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_05.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_06.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_07.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_08.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_09.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_10.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_11.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_12.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_13.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_14.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Test_CheckScreenshot(_T("data\\test04_07_15.bmp"), 1);
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test
    Emulator_Run(25);

    // Help page
    Emulator_KeyboardPressRelease(033);  // Down
    Emulator_KeyboardPressRelease(012);  // Enter -- open help page
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_08.bmp"), 1);

    Test_Done();
}

void Test05_Games10()
{
    Test_Init(_T("TEST 5: BK0010 Games"), BK_CONF_BK0010_BASIC);

    // KLAD
    Emulator_Run(50);
    Emulator_KeyboardSequence("MO\n");
    Emulator_Run(50);
    Test_LoadBin(_T("data\\klad.bin"));
    Emulator_KeyboardSequence("S1000\n");
    Emulator_Run(20);
    Test_CheckScreenshot(_T("data\\test05_01.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(40);
    Emulator_KeyboardPressRelease(0061);  // 1
    Emulator_Run(20);
    Test_CheckScreenshot(_T("data\\test05_02.bmp"), 1);

    // BALLY
    Emulator_Reset();
    Emulator_Run(50);
    g_okEmulatorAutoTapeReading = true;
    Emulator_KeyboardSequence("MO\n");
    Emulator_Run(5);
    Emulator_KeyboardSequence("M\nBALLY\n");
    Emulator_Run(50);
    Emulator_KeyboardSequence("S1000\n");
    Emulator_Run(100);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(100);
    Test_CheckScreenshot(_T("data\\test05_03.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(120);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(390);
    Test_CheckScreenshot(_T("data\\test05_04.bmp"), 1);

    // Kings Valley
    Emulator_Reset();
    Emulator_Run(50);
    g_okEmulatorAutoTapeReading = true;
    Emulator_KeyboardSequence("MO\n");
    Emulator_Run(5);
    Emulator_KeyboardSequence("M\nVALLEY\n");
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test05_05.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test05_06.bmp"), 1);
    Emulator_KeyboardPressRelease(012);  // Enter
    Emulator_Run(100);
    Emulator_KeyboardPressRelease(012,10);  // Enter
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test05_07.bmp"), 1);

    //TODO

    Test_Done();
}

void Test06_RT11()
{
    Test_Init(_T("TEST 6: RT11 on BK0011M"), BK_CONF_BK0011_FDD);

    Test_CopyFile(_T("data\\rt11v4_1.img"), _T("temp\\rt11v4_1.img"));
    Test_AttachFloppyImage(0, _T("temp\\rt11v4_1.img"));
    Emulator_Run(650);
    Test_CheckScreenshot(_T("data\\test06_01.bmp"));
    Emulator_Run(14 * 25);
    Test_CheckScreenshot(_T("data\\test06_02.bmp"));

    Emulator_KeyboardSequence("SH CONF\n");
    Emulator_Run(500);
    Test_CheckScreenshot(_T("data\\test06_03.bmp"));

    Emulator_KeyboardSequence("IC\n");
    Emulator_Run(34 * 25);
    Test_CheckScreenshot(_T("data\\test06_04.bmp"));
    //TODO: Press AP2+1 Help

    Test_Done();
}

int _tmain(int argc, _TCHAR* argv[])
{
    SYSTEMTIME timeFrom;  ::GetLocalTime(&timeFrom);
    DebugLogClear();
    Test_LogInfo(_T("Initialization..."));

    Test01_Basic10();
    Test011_Basic10_Cassette();
    Test02_Focal10();
    //Test03_Tmos();
    Test031_Tricks();
    Test04_MSTD11();
    Test05_Games10();
    Test06_RT11();

    Test_LogInfo(_T("Finalization..."));
    SYSTEMTIME timeTo;  ::GetLocalTime(&timeTo);
    FILETIME fileTimeFrom;
    SystemTimeToFileTime(&timeFrom, &fileTimeFrom);
    FILETIME fileTimeTo;
    SystemTimeToFileTime(&timeTo, &fileTimeTo);

    DWORD diff = fileTimeTo.dwLowDateTime - fileTimeFrom.dwLowDateTime;  // number of 100-nanosecond intervals
    Test_LogFormat('i', _T("Time spent: %.3f seconds"), (float)diff / 10000000.0);

    Test_LogSummary();

    return 0;
}
