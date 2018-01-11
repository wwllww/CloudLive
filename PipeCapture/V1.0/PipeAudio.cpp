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
#include "AudioWaveOut.h"
#include <Audioclient.h>

IMPLEMENT_DYNIC(PipeAudioSource, "互动连接源", "1.0.0.1")

PipeAudioSource::PipeAudioSource()
{
	bLiveIntance = false;
	m_pAudioWaveOut = NULL;
	hAudioMutex = NULL;
	m_pSecWaveOut = NULL;
	lastTimestamp = 0;
}

bool PipeAudioSource::GetNextBuffer(void **buffer, UINT *numFrames, QWORD *timestamp)
{
    if(sampleBuffer.Num() >= sampleSegmentSize)
    {
        OSEnterMutex(hAudioMutex);

		if (!sampleBuffer.Num())
		{
			OSLeaveMutex(hAudioMutex);
			return false;
		}

// 		int64_t pts;
// 		int samplesProcessed = 0;
// 		while (samplesProcessed != sampleFrameCount) {
// 			int remaining = sampleFrameCount - samplesProcessed;
// 			AudioTimestamp &ts = sampleBufferPts[0];
// 			if (m_isFirstDiffTimeWithAPI && (LastTimeTimeStamp > 0))
// 			{
// 				m_lTimeDiffWithAPI = 0;
// 				m_isFirstDiffTimeWithAPI = false;
// 				m_lTimeDiffWithAPI = LastTimeTimeStamp - ts.pts;
// 				Log::writeMessage(LOG_RTSPSERV,1,"LINE:%d,FUNC:%s 第一次初始化:API->GetAudioTime() :%lld.  ts.pts:%lld. m_lTimeDiffWithAPI :%lld.",
// 					__LINE__, __FUNCTION__, LastTimeTimeStamp, ts.pts, m_lTimeDiffWithAPI);
// 			}
// 			if (ts.pts - m_iLastTimeStamp < 0)     //互动重现连接,重现初始化参数
// 			{
// 				m_iLastTimeStamp = 0;
// 				m_lTimeDiffWithAPI = 0;
// 				m_isFirstDiffTimeWithAPI = true;
// 			}
// 			m_iLastTimeStamp = ts.pts;
// 			ts.count -= remaining;
// 			samplesProcessed += remaining;
// 			if (ts.count < 0) {
// 				samplesProcessed += ts.count;
// 				sampleBufferPts.pop_front();
// 			}
// 			if (sampleBufferPts.size() <= 0)
// 			{
// 				return false;
// 			}
// 			pts = ts.pts;
// 		}

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
		
		/*Log(TEXT("LINE:%d,FUNC:%s 底层传入时间戳: %lld. pts = %lld."),
			__LINE__, String(__FUNCTION__).Array(), *timestamp,pts);*/
        return true;
    }
	
    return false;
}

void PipeAudioSource::ReleaseBuffer()
{

}


CTSTR PipeAudioSource::GetDeviceName() const
{
    return L"PipeAudioDevice";
}

const AudioParam& PipeAudioSource::GetAudioParam()
{
	return Param;
}

bool PipeAudioSource::Initialize(PipeVideo *parent, const AudioParam& param)
{
	m_iLastPts = 0;
	m_iBlockSize = 0;
	Times = 0;
	LimitGetData = 0;
	m_PipeVideo = parent;

	if (!hAudioMutex)
	{
		hAudioMutex = OSCreateMutex();
	}

    bool bFloat = false;
    UINT  inputChannels;
    UINT  inputSamplesPerSec;
    UINT  inputBitsPerSample;
    DWORD inputChannelMask;

	inputBitsPerSample = param.bitsPerSample; // 16;
    inputChannelMask   = 0;
	inputChannels = param.channels; // 1;
	inputSamplesPerSec = param.samplesPerSec; // 8000;

	m_iBlockSize = inputChannels * inputBitsPerSample / 8;

    sampleFrameCount   = inputSamplesPerSec / 100;
	sampleSegmentSize = m_iBlockSize * sampleFrameCount;

	Param = param;
	m_isFirstDiffTimeWithAPI = true;
	m_lTimeDiffWithAPI = 0;
	m_iLastTimeStamp = 0;
    outputBuffer.SetSize(sampleSegmentSize);
	InitAudioData(bFloat, inputChannels, inputSamplesPerSec, inputBitsPerSample, m_iBlockSize, inputChannelMask);

	if (m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Uninitialize();
	}

	String strReanderName = GetDirectorMonitorDevices();
	if (NULL == m_pAudioWaveOut)
	{
		if (!strReanderName.Compare(TEXT("停用")) && !m_pAudioWaveOut)
		{
			m_pAudioWaveOut = new AudioWaveOut;
		}
	}
	else
	{
		if ((strReanderName.Compare(TEXT("Disable")) || strReanderName.Compare(TEXT("停用"))))
		{
			delete m_pAudioWaveOut;
			m_pAudioWaveOut = NULL;
		}
	}

	if (NULL != m_pAudioWaveOut)
	{
		m_pAudioWaveOut->Initialize(strReanderName.Array(), 2, inputSamplesPerSec, inputBitsPerSample);
	}

	String SecRenderName = GetSecMonitorDevices();


	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Uninitialize();
	}

	if (SecRenderName.CompareI(strReanderName.Array()) && (!SecRenderName.Compare(TEXT("Disable")) || !SecRenderName.Compare(TEXT("停用"))))
	{
		bSameDevice = true;
	}
	else if (!SecRenderName.Compare(TEXT("停用")) && !m_pSecWaveOut)
	{
		m_pSecWaveOut = new AudioWaveOut;
	}
	else if (m_pSecWaveOut)
	{
		if ((SecRenderName.Compare(TEXT("Disable")) || SecRenderName.Compare(TEXT("停用"))))
		{
			delete m_pSecWaveOut;
			m_pSecWaveOut = NULL;
		}
	}

	if (m_pSecWaveOut)
	{
		m_pSecWaveOut->Initialize(SecRenderName.Array(), 2, inputSamplesPerSec, inputBitsPerSample);
	}

    return true;
}

PipeAudioSource::~PipeAudioSource()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LINE:%d,FUNC:%s delete PipeAudioSource", __LINE__, __FUNCTION__);
    if(hAudioMutex)
        OSCloseMutex(hAudioMutex);

	if (m_pAudioWaveOut)
	{
		delete m_pAudioWaveOut;
		m_pAudioWaveOut = NULL;
	}
	if (NULL != m_pSecWaveOut)
	{
		delete m_pSecWaveOut;
		m_pSecWaveOut = NULL;
	}
}

void PipeAudioSource::SetLiveInstance(bool bLiveIntance)
{
	this->bLiveIntance = bLiveIntance;

// 	if (!bLiveIntance)
// 	{
// 		OSEnterMutex(hAudioMutex);
// 		sampleBuffer.RemoveRange(0, sampleBuffer.Num());
// 		OSLeaveMutex(hAudioMutex);
// 	}
}

bool PipeAudioSource::IsNeedRemove() const
{
	return !bLiveIntance && !audioSegments.Num();
}

void PipeAudioSource::ReceiveAudio(LPBYTE lpData, UINT dataLength,long long pts)
{
	if (lpData)
    {
		bool bPlayLive = false;
		if (bLiveIntance)
		{
			OSEnterMutex(hAudioMutex);
			sampleBuffer.AppendArray(lpData, dataLength);
// 			AudioTimestamp audioTimestamp;
// 			audioTimestamp.count = dataLength / m_iBlockSize;
// 			audioTimestamp.pts = pts;
// 			sampleBufferPts.push_back(audioTimestamp);
// 			if (pts - m_iLastPts > 50 || m_iLastPts - pts > 50)
// 			{
// 				Log::writeMessage(LOG_RTSPSERV, 1, "LINE:%d,FUNC:%s 音频数据有抖动: LastPts : %lld. CurrntPts : %lld. diff = %lld.",
// 					__LINE__, __FUNCTION__, m_lTimeDiffWithAPI + m_iLastPts, m_lTimeDiffWithAPI + pts, pts - m_iLastPts);
// 			}
// 			m_iLastPts = pts;
			OSLeaveMutex(hAudioMutex);
			bPlayLive = m_bPlayPcmLive;
		}
		else
		{
			OSEnterMutex(hAudioMutex);
			sampleBuffer.RemoveRange(0, sampleBuffer.Num());
			OSLeaveMutex(hAudioMutex);
		}

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

void PipeAudioSource::FlushSamples()
{
    OSEnterMutex(hAudioMutex);
    sampleBuffer.Clear();
    OSLeaveMutex(hAudioMutex);
}

void PipeAudioSource::OnAudioDeviceChanged(const String &MonitorDevices, const String &SecMonitor)
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
		m_pAudioWaveOut->Initialize(MonitorDevices.Array(), 2, Param.samplesPerSec, Param.bitsPerSample);
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
		m_pSecWaveOut->Initialize(SecMonitor.Array(), 2, Param.samplesPerSec, Param.bitsPerSample);
	}

	OSLeaveMutex(hAudioMutex);

	this->MonitorDevices = MonitorDevices;
	this->SecMonitor = SecMonitor;

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}