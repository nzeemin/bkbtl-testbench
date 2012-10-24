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

    //Test_SaveScreenshotSeria(_T("video\\test01_%04u.bmp"), 20, 10);

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
    ////NOTE: Программа вылетает с ошибкой переполнения при J=26 -- видимо, не хватает точности

    Test_Done();
}

void Test03_Tmos()
{
    Test_Init(_T("TEST 3: TMOS tests"), BK_CONF_BK0010_FDD);

    Emulator_Run(50);
    //Test_SaveScreenshot(_T("test03_01.bmp"));
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

void Test04_MSTD11()
{
    Test_Init(_T("TEST 4: BK0011M MSTD"), BK_CONF_BK0011);

    Emulator_KeyboardEvent(0040, TRUE);  // Hold Spacebar to boot to monitor
    Emulator_Run(75);
    Emulator_KeyboardEvent(0040, FALSE);
    Emulator_KeyboardSequence("160100G");
    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test04_01.bmp"), 1);  // Menu

    // ROM test
    Emulator_KeyboardPressRelease(033);  // Down
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
    //Emulator_KeyboardEvent(BK_KEY_BACKSHIFT, TRUE);
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
    Emulator_KeyboardPressRelease(0112);  // Й J
    Emulator_KeyboardPressRelease(0103);  // Ц C
    Emulator_KeyboardPressRelease(0125);  // У U
    Emulator_KeyboardPressRelease(0113);  // К K
    Emulator_KeyboardPressRelease(0105);  // Е E
    Emulator_KeyboardPressRelease(0116);  // Н N
    Emulator_KeyboardPressRelease(0107);  // Г G
    Emulator_KeyboardPressRelease(0133);  // Ш [
    Emulator_KeyboardPressRelease(0135);  // Щ ]
    Emulator_KeyboardPressRelease(0132);  // З Z
    Emulator_KeyboardPressRelease(0110);  // Х H
    Emulator_KeyboardPressRelease(0137);  // Ъ }
    Emulator_KeyboardPressRelease(0057);  // / ?
    Emulator_KeyboardPressRelease(0023);  // ВС
    Emulator_KeyboardPressRelease(0006);  // SU + F
    Emulator_KeyboardPressRelease(0131);  // Ы Y
    Emulator_KeyboardPressRelease(0127);  // В W
    Emulator_KeyboardPressRelease(0101);  // А A
    Emulator_KeyboardPressRelease(0120);  // П P
    Emulator_KeyboardPressRelease(0122);  // Р R
    Emulator_KeyboardPressRelease(0117);  // О O
    Emulator_KeyboardPressRelease(0114);  // Л L
    Emulator_KeyboardPressRelease(0104);  // Д D
    Emulator_KeyboardPressRelease(0126);  // Ж V
    Emulator_KeyboardPressRelease(0134);  // Э Backslash
    Emulator_KeyboardPressRelease(0076);  // . >  //TODO
    Emulator_KeyboardPressRelease(0012);  // ENTER
    //Emulator_KeyboardEvent(BK_KEY_LOWER, TRUE);
    Emulator_KeyboardPressRelease(0121);  // Я Q  //TODO: with ???
    Emulator_KeyboardPressRelease(0136);  // Ч ^  //TODO
    Emulator_KeyboardPressRelease(0123);  // С S  //TODO
    Emulator_KeyboardPressRelease(0115);  // М M  //TODO
    Emulator_KeyboardPressRelease(0111);  // И I  //TODO
    Emulator_KeyboardPressRelease(0124);  // Т T  //TODO
    Emulator_KeyboardPressRelease(0130);  // Ь X  //TODO
    Emulator_KeyboardPressRelease(0102);  // Б B  //TODO
    Emulator_KeyboardPressRelease(0100);  // Ю @  //TODO
    Emulator_KeyboardPressRelease(0074);  // , <  //TODO
    Emulator_KeyboardPressRelease(0010);  // Left
    Emulator_KeyboardPressRelease(0032);  // Up
    Emulator_KeyboardPressRelease(0031);  // Right
    Emulator_KeyboardPressRelease(0033);  // Down
    Emulator_KeyboardPressRelease(0040);  //TODO: AP2? + Space
    Emulator_KeyboardPressRelease(0017);  // LAT
    Test_SaveScreenshot(_T("test04_04_2.bmp"), 1);

    Emulator_KeyboardPressRelease(012);  // Enter -- exit the test

    // Palette test
    Emulator_KeyboardPressRelease(033);  // Down
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

int _tmain(int argc, _TCHAR* argv[])
{
    SYSTEMTIME timeFrom;  ::GetLocalTime(&timeFrom);
    Test_LogInfo(_T("Initialization..."));

    Test01_Basic10();
    Test02_Focal10();
    Test03_Tmos();
    Test04_MSTD11();

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
