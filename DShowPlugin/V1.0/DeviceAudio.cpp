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


#include <tchar.h>
#include "DShowPlugin.h"
#include "AudioWaveOut.h"


#include <Audioclient.h>


IMPLEMENT_DYNIC(DeviceAudioSource, "DeviceAudioSource", "1.0.0.1")

DeviceAudioSource::DeviceAudioSource()
{
	m_pAudioWaveOut = NULL;
	hAudioMutex = NULL;
	m_pSecWaveOut = NULL;
	sampleSegmentSize = -1;
	bLiveInstance = false;
}

bool DeviceAudioSource::GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp)
{
    if(sampleBuffer.Num() >= sampleSegmentSize)
    {
        OSEnterMutex(hAudioMutex);

		if (!sampleBuffer.Num())
		{
			OSLeaveMutex(hAudioMutex);
			return false;
		}

        mcpy(outputBuffer.Array(), sampleBuffer.Array(), sampleSegmentSize);
        sampleBuffer.RemoveRange(0, sampleSegmentSize);

        OSLeaveMutex(hAudioMutex);

        *buffer = outputBuffer.Array();
        *numFrames = sampleFrameCount;

		*timestamp = 0;//赋值时间戳
		if (!lastTimestamp || LastTimeTimeStamp > lastTimestamp + 10) {
			*timestamp = LastTimeTimeStamp;
		}
		else {
			*timestamp = lastTimestamp + 10;
		}
		//Log(TEXT("hhhhhhhBLiveGetAudioTime() = %llu,*timestamp = %llu"), BLiveGetAudioTime(), *timestamp);
		lastTimestamp = *timestamp;
        return true;
    }

    return false;
}

void DeviceAudioSource::ReleaseBuffer()
{
}


CTSTR DeviceAudioSource::GetDeviceName() const
{
    if(device)
        return device->strDeviceName.Array();

    return NULL;
}


bool DeviceAudioSource::Initialize(DeviceSource *parent)
{
	m_timeStart = 0;	
    device = parent;
	if (!hAudioMutex)
		hAudioMutex = OSCreateMutex();
	m_bErrorLog = false;
	lastTimestamp = 0;
    //---------------------------------

    bool  bFloat = false;
    UINT  inputChannels;
    UINT  inputSamplesPerSec;
    UINT  inputBitsPerSample;
    //UINT  inputBlockSize;
    DWORD inputChannelMask;

    //---------------------------------

    if(device->audioFormat.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        WAVEFORMATEXTENSIBLE *wfext = (WAVEFORMATEXTENSIBLE*)&device->audioFormat;
        if(wfext->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)
            bFloat = true;
    }
    else if(device->audioFormat.wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
        bFloat = true;

    inputBitsPerSample = device->audioFormat.wBitsPerSample;
    inputBlockSize     = device->audioFormat.nBlockAlign;
    inputChannelMask   = 0;
    inputChannels      = device->audioFormat.nChannels;
    inputSamplesPerSec = device->audioFormat.nSamplesPerSec;

    sampleFrameCount   = inputSamplesPerSec/100;
    sampleSegmentSize  = inputBlockSize*sampleFrameCount;

    outputBuffer.SetSize(sampleSegmentSize);

    InitAudioData(bFloat, inputChannels, inputSamplesPerSec, inputBitsPerSample, inputBlockSize, inputChannelMask);

	if (m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Uninitialize();
	}

	String strReanderName = GetDirectorMonitorDevices();
	if (NULL == m_pAudioWaveOut)
	{
		if (!strReanderName.Compare(TEXT("禁用")) && !m_pAudioWaveOut)
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	else
	{
		if ((strReanderName.Compare(TEXT("Disable")) || strReanderName.Compare(TEXT("禁用"))))
		{
			delete m_pAudioWaveOut;
			m_pAudioWaveOut = NULL;
		}
	}
	if (NULL != m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Initialize(strReanderName.Array(), inputChannels, inputSamplesPerSec, inputBitsPerSample);
	}

	String SecRenderName = GetSecMonitorDevices();


	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Uninitialize();
	}

	if (SecRenderName.CompareI(strReanderName.Array()) && (!SecRenderName.Compare(TEXT("Disable")) || !SecRenderName.Compare(TEXT("禁用"))))
	{
		bSameDevice = true;
	}
	else if (!SecRenderName.Compare(TEXT("禁用")) && !m_pSecWaveOut)
	{
		m_pSecWaveOut = new AudioWaveOut;
	}
	else if (m_pSecWaveOut)
	{
		if ((SecRenderName.Compare(TEXT("Disable")) || SecRenderName.Compare(TEXT("禁用"))))
		{
			delete m_pSecWaveOut;
			m_pSecWaveOut = NULL;
		}
	}

	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Initialize(SecRenderName.Array(), inputChannels, inputSamplesPerSec, inputBitsPerSample);
	}

    return true;
}

DeviceAudioSource::~DeviceAudioSource()
{
	if (m_pAudioWaveOut)
		delete m_pAudioWaveOut;

    if(hAudioMutex)
        OSCloseMutex(hAudioMutex);
//	fclose(audiosource);
	if (NULL != m_pSecWaveOut)
	{
		delete m_pSecWaveOut;
		m_pSecWaveOut = NULL;
	}
}


void DeviceAudioSource::ReceiveAudio(LPBYTE lpData, UINT dataLength, float volume)
{
    if(lpData)
    {
		if (volume != 1.0f)
		{
			short *Tem = (short*)lpData;
			for (int i = 0; i < dataLength; i += 2)
			{
				long sVolume = Tem[i / 2];

				sVolume *= volume;

				if (sVolume > 0x7fff)
				{
					sVolume = 0x7fff;
				}
				else if (sVolume < -0x8000)
				{
					sVolume = -0x8000;
				}

				Tem[i / 2] = (short)sVolume;
			}
		}
		bool bPlayLive = false;
		if (bLiveInstance)
		{
			OSEnterMutex(hAudioMutex);
			sampleBuffer.AppendArray(lpData, dataLength);
			OSLeaveMutex(hAudioMutex);
			bPlayLive = m_bPlayPcmLive;
		}
		else
		{
			if (sampleBuffer.Num())
			{
				sampleBuffer.RemoveRange(0, sampleBuffer.Num());
			}
		}
		QWORD tmNow  = GetQPCMS();
		if (tmNow - m_timeStart > 200 && m_timeStart != 0)
		{
			if (!m_bErrorLog)
			{
				Log(L"111111 dshow送入时间间隔过大<%d> 缓冲大小%d,送入数据%d", tmNow - m_timeStart, sampleBuffer.Num(), dataLength);
				m_bErrorLog = true;
			}			
		}
		else
		{
			m_bErrorLog = false;
		}

		m_timeStart = tmNow;

		OSEnterMutex(hAudioMutex);
		int Len = dataLength;
		if (m_pAudioWaveOut && (m_bPlayPcmLocal || bPlayLive))
		{
			char *OutBuffer;
			CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer);

			m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);

			if (!bSameDevice && bProjector && m_pSecWaveOut)
				m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);

		}
		else if (bProjector)
		{
			char *OutBuffer;
			CaculateVolume((LPVOID)lpData, Len, (void**)&OutBuffer);

			if (bSameDevice && m_pAudioWaveOut)
			{
				m_pAudioWaveOut->push_pcm_data((char*)OutBuffer, Len);
			}
			else if (m_pSecWaveOut)
			{
				m_pSecWaveOut->push_pcm_data((char*)OutBuffer, Len);
			}
		}
		OSLeaveMutex(hAudioMutex);
    }
}

void DeviceAudioSource::FlushSamples()
{
    OSEnterMutex(hAudioMutex);
    sampleBuffer.Clear();
    OSLeaveMutex(hAudioMutex);
}


void DeviceAudioSource::SetLiveInstance(bool bLiveInstance)
{
	this->bLiveInstance = bLiveInstance;

	if (!bLiveInstance && hAudioMutex)
	{
		OSEnterMutex(hAudioMutex);
		sampleBuffer.RemoveRange(0, sampleBuffer.Num());
		OSLeaveMutex(hAudioMutex);
	}
}

bool DeviceAudioSource::IsNeedRemove() const
{
	return !bLiveInstance && !audioSegments.Num();
}

void DeviceAudioSource::OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor)
{
	if (this->MonitorDevices.Compare(MonitorDevices.Array()) && this->SecMonitor.Compare(SecMonitor.Array()))
	{
		return;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
	OSEnterMutex(hAudioMutex);

	bSameDevice = false;

	if (m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Uninitialize();
	}

	if (NULL == m_pAudioWaveOut)
	{
		if (!MonitorDevices.Compare(TEXT("停用")) && !m_pAudioWaveOut)
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	else
	{
		if ((MonitorDevices.Compare(TEXT("Disable")) || MonitorDevices.Compare(TEXT("停用"))))
		{
			delete m_pAudioWaveOut;
			m_pAudioWaveOut = NULL;
		}
	}

	if (NULL != m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Initialize(MonitorDevices.Array(), 2, device->audioFormat.nSamplesPerSec, device->audioFormat.wBitsPerSample);
	}

	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Uninitialize();
	}

	if (SecMonitor.CompareI(MonitorDevices.Array()) && (!SecMonitor.Compare(TEXT("Disable")) || !SecMonitor.Compare(TEXT("停用"))))
	{
		bSameDevice = true;
	}
	else if (!SecMonitor.Compare(TEXT("停用")) && !m_pSecWaveOut)
	{
		m_pSecWaveOut = new AudioWaveOut;
	}
	else if (m_pSecWaveOut)
	{
		if ((SecMonitor.Compare(TEXT("Disable")) || SecMonitor.Compare(TEXT("停用"))))
		{
			delete m_pSecWaveOut;
			m_pSecWaveOut = NULL;
		}
	}

	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Initialize(SecMonitor.Array(), 2, device->audioFormat.nSamplesPerSec, device->audioFormat.wBitsPerSample);
	}

	OSLeaveMutex(hAudioMutex);

	this->MonitorDevices = MonitorDevices;
	this->SecMonitor = SecMonitor;

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}