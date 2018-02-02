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


#pragma once

#include "BaseAfx.h"

#include <dshow.h>
#include <Amaudio.h>
#include <Dvdmedia.h>

#include "MediaInfoStuff.h"
#include "CaptureFilter.h"
#include "DeviceSource.h"
#include "resource.h"

//×Ô»æ»¬¿é
#define DENOISE_SLIDER_CLASS TEXT("DenoiseSlider")
enum {
	ID_DENOISESLIDER = 9999,
	ID_CHECKDENOISE,
	ID_UPPOSDENOISE,
	ID_DOWNPOSDENOISE
};

bool  CheckDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName);
IPin* GetOutputPin(IBaseFilter *filter, const GUID *majorType);
IBaseFilter* GetDeviceByValue(const IID &enumType, WSTR lpType, CTSTR lpName, WSTR lpType2 = NULL, CTSTR lpName2 = NULL);
//-----------------------------------------------------------
extern HINSTANCE hinstMain;

