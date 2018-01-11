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


#include "PipeVideoPlugin.h"

void PipeVideo::Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY)
{
    DWORD size = lineSize;
    DWORD dwDWSize = size>>2;

    if(bLeadingY)
    {
        for(UINT y=0; y<renderCY; y++)
        {
            LPDWORD output = (LPDWORD)(convertBuffer+(y*pitch));
            LPDWORD inputDW = (LPDWORD)(lp422+(y*linePitch)+lineShift);
            LPDWORD inputDWEnd = inputDW+dwDWSize;

            while(inputDW < inputDWEnd)
            {
                register DWORD dw = *inputDW;

                output[0] = dw;
                dw &= 0xFFFFFF00;
                dw |= BYTE(dw>>16);
                output[1] = dw;

                output += 2;
                inputDW++;
            }
        }
    }
    else
    {
        for(UINT y=0; y<renderCY; y++)
        {
            LPDWORD output = (LPDWORD)(convertBuffer+(y*pitch));
            LPDWORD inputDW = (LPDWORD)(lp422+(y*linePitch)+lineShift);
            LPDWORD inputDWEnd = inputDW+dwDWSize;

            while(inputDW < inputDWEnd)
            {
                register DWORD dw = *inputDW;

                output[0] = dw;
                dw &= 0xFFFF00FF;
                dw |= (dw>>16) & 0xFF00;
                output[1] = dw;

                output += 2;
                inputDW++;
            }
        }
    }
}
