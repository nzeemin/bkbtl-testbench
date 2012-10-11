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


void Test_Basic10()
{
    Test_Init(_T("TEST 1: BASIC BK0010"), BK_CONF_BK0010_BASIC);

    Emulator_Run(50);
    Test_CheckScreenshot(_T("data\\test01_01.bmp"));

    Emulator_KeyboardSequence("PRINT PI\n");

    Emulator_KeyboardSequence("10 FOR I=32 TO 127\n");
    Emulator_KeyboardSequence("20 PRINT CHR$(I);\n");
    Emulator_KeyboardSequence("30 IF I MOD 16 = 15 THEN PRINT\n");
    Emulator_KeyboardSequence("50 NEXT I\n");
    Emulator_KeyboardSequence("RUN\n");
    Emulator_Run(25);  // Wait 1 second
    Test_CheckScreenshot(_T("data\\test01_02.bmp"));

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
    //Test_SaveScreenshotSeria(_T("video\\test01_%04u.bmp"), 12, 1);
    Test_CheckScreenshot(_T("data\\test01_04.bmp"));

    Test_Done();
}

void Test_Focal10()
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

int _tmain(int argc, _TCHAR* argv[])
{
    SYSTEMTIME timeFrom;  ::GetLocalTime(&timeFrom);
    Test_LogInfo(_T("Initialization..."));

    Test_Basic10();
    Test_Focal10();

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
