/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <BLive.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#include "common.h"

extern LARGE_INTEGER clockFreq;
__declspec(thread) LONGLONG lastQPCTime = 0;

QWORD GetQPCTimeNS()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    if (currentTime.QuadPart < lastQPCTime)
        Log (TEXT("GetQPCTimeNS: WTF, clock went backwards! %I64d < %I64d"), currentTime.QuadPart, lastQPCTime);

    lastQPCTime = currentTime.QuadPart;

    double timeVal = double(currentTime.QuadPart);
    timeVal *= 1000000000.0;
    timeVal /= double(clockFreq.QuadPart);

    return QWORD(timeVal);
}

QWORD GetQPCTime100NS()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    if (currentTime.QuadPart < lastQPCTime)
        Log (TEXT("GetQPCTime100NS: WTF, clock went backwards! %I64d < %I64d"), currentTime.QuadPart, lastQPCTime);

    lastQPCTime = currentTime.QuadPart;

    double timeVal = double(currentTime.QuadPart);
    timeVal *= 10000000.0;
    timeVal /= double(clockFreq.QuadPart);

    return QWORD(timeVal);
}

QWORD GetQPCTimeMS()
{
    LARGE_INTEGER currentTime;
    QueryPerformanceCounter(&currentTime);

    if (currentTime.QuadPart < lastQPCTime)
        Log (TEXT("GetQPCTimeMS: WTF, clock went backwards! %I64d < %I64d"), currentTime.QuadPart, lastQPCTime);

    lastQPCTime = currentTime.QuadPart;

    QWORD timeVal = currentTime.QuadPart;
    timeVal *= 1000;
    timeVal /= clockFreq.QuadPart;

    return timeVal;
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
#if defined _M_X64 && _MSC_VER == 1800
        //workaround AVX2 bug in VS2013, http://connect.microsoft.com/VisualStudio/feedback/details/811093
        _set_FMA3_enable(0);
#endif
    }

    return TRUE;
}
