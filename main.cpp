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

    Test_SaveScreenshot(_T("test01_01.bmp"));

    Test_Done();
}

int _tmain(int argc, _TCHAR* argv[])
{
    SYSTEMTIME timeFrom;  ::GetLocalTime(&timeFrom);
    Test_LogInfo(_T("Initialization..."));

    Test_Basic10();

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
