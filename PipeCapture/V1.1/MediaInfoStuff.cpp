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


#include"MediaInfoStuff.h"


void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if(mt.cbFormat != 0)
    {
        CoTaskMemFree((LPVOID)mt.pbFormat);
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }

    SafeRelease(mt.pUnk);
}

HRESULT WINAPI CopyMediaType(AM_MEDIA_TYPE *pmtTarget, const AM_MEDIA_TYPE *pmtSource)
{
    if(!pmtSource || !pmtTarget) return S_FALSE;

    *pmtTarget = *pmtSource;

    if(pmtSource->cbFormat && pmtSource->pbFormat)
    {
        pmtTarget->pbFormat = (PBYTE)CoTaskMemAlloc(pmtSource->cbFormat);
        if(pmtTarget->pbFormat == NULL)
        {
            pmtTarget->cbFormat = 0;
            return E_OUTOFMEMORY;
        }
        else
            mcpy(pmtTarget->pbFormat, pmtSource->pbFormat, pmtTarget->cbFormat);
    }

    if(pmtTarget->pUnk != NULL)
        pmtTarget->pUnk->AddRef();

    return S_OK;
}

