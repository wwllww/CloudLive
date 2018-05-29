#pragma once

#include "BaseAfx.h"
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <propsys.h>
#include <Functiondiscoverykeys_devpkey.h>

#include <tuple>
#include <vector>
#include "resource.h"
struct AudioOutputDevice
{

private:
	String name;
	String longName;
	int index;

public:
	AudioOutputDevice(String name, String longName)
	{
		this->name = name;
		this->longName = longName;
	}

public:
	String &GetName() { return name; }
	String &GetLongName() { return longName; }
};

class AudioOutputType
{

private:
	String name;
	String deviceSetName;
	String description;
	std::vector<AudioOutputDevice> audioOutputDevices;
	bool isEnabled;

public:
	AudioOutputType(String name, String deviceSetName, String description)
	{
		this->name = name;
		this->deviceSetName = deviceSetName;
		this->description = description;
	}

public:
	void AddAudioOutputDevice(AudioOutputDevice &audioOutputDevice)
	{
		audioOutputDevices.push_back(audioOutputDevice);
	}

public:
	void SetName(String &name) { this->name = name; }
	String &GetName() { return name; }
	void SetDeviceSetName(String &deviceSetName) { this->deviceSetName = deviceSetName; }
	String &GetDeviceSetName() { return deviceSetName; }
	String &GetDescription() { return description; }

	std::vector<AudioOutputDevice> &GetAudioOutputDevices() { return audioOutputDevices; }
};

class VideoSourceConfig
{

private:
	Value& element;

	std::vector<AudioOutputType> audioOutputTypes;

public:
	unsigned int width;
	unsigned int height;
	bool isStretching;
	unsigned int volume;
	bool isAudioOutputToStream;
	String audioOutputType;
	String audioOutputTypeDevice;
	String audioOutputDevice;
	bool isPlaylistLooping;
	bool isFileLoop;
	bool isListLoop;
	bool isSenceChangeStartPlay;
	bool isHardwareDecode;
	StringList playlist;
	StringList playlistm3u8;
	StringList playlistName;
	StringList playlistFlv;
	String deinterlacing;
	bool isApplyingVideoFilter;
	unsigned int videoGamma;
	unsigned int videoContrast;
	unsigned int videoBrightness;
	unsigned int CurrentIndex = 0;
	bool m_bisDirectPlay = false;
	bool isHaveSelect = false;
	int bufferTime = 5;
	int WarnTime = 2;
	String LastPlayFile;
	int LastbufferTime = 0;
	bool bIsScanInterlace;// «∑Ò «∏Ù––…®√Ë

public:
	VideoSourceConfig(Value &element) :element(element)
	{
		Reload();
	}

	~VideoSourceConfig()
	{
	}

public:


	void Populate()
	{
		height = 1280;//element["BaseHeight"].asUInt();
		width = 720;//element["BaseWidth"].asUInt();
		volume = 100;
		isStretching = false;
		isAudioOutputToStream = true;
		audioOutputType = TEXT("wavemapper");
		audioOutputTypeDevice = TEXT("wavemapper");
		audioOutputDevice = TEXT("");
		isPlaylistLooping = true;
		isListLoop = true;
		isFileLoop = false;
		deinterlacing = TEXT("none");
		isApplyingVideoFilter = false;
		videoGamma = 100;
		videoContrast = 100;
		videoBrightness = 100;
		isHaveSelect = false;
		isHardwareDecode = false;
		int bufferTime = 5;
		int WarnTime = 2;
		String LastPlayFile;
		int LastbufferTime = 0;
		bIsScanInterlace = false;
#if YangfanLive
		isSenceChangeStartPlay = true;
#else
		isSenceChangeStartPlay = false;
#endif
	}

	void Reload()
	{
		height = 1280;
		width = 720;
		volume = element["volume"].asDouble();
		isStretching = element["isStretching"].asUInt() == 1;
		isAudioOutputToStream = element["isAudioOutputToStream"].asUInt() == 1;
		if (element["bufferTime"].isNull())
		{
			bufferTime = 5;
		}
		else
		{
			bufferTime = element["bufferTime"].asInt();
		}
		LastbufferTime = element["LastbufferTime"].asInt();
		
		if (element["WarnTime"].isNull())
		{
			WarnTime = 2;
		}
		else
		{
			WarnTime = element["WarnTime"].asInt();
		}
		isPlaylistLooping = element["isPlaylistLooping"].asUInt() == 1;
		isFileLoop = element["isFileLoop"].asUInt() == 1;
		isListLoop = element["isListLoop"].asUInt() == 1;
		isSenceChangeStartPlay = element["isSenceChangeStartPlay"].asUInt() == 1;
		isHardwareDecode = element["isHardwareDecode"].asUInt() == 1;
		playlist.Clear();

		Json::Value &ArryList = element["PlayList"];
		playlistm3u8.SetSize(ArryList.size());
		playlistName.SetSize(ArryList.size());
		playlistFlv.SetSize(ArryList.size());
		for (int i = 0; i < ArryList.size(); ++i)
		{
			playlistm3u8[i] = Asic2WChar(ArryList[i]["m3u8"].asString().c_str()).c_str();
			playlistName[i] = Asic2WChar(ArryList[i]["name"].asString().c_str()).c_str();
			playlistFlv[i] = Asic2WChar(ArryList[i]["flv"].asString().c_str()).c_str();
		}

		const char *JList = element.toStyledString().c_str();
		Json::Value &ArryPlayList = element["playlist"];

		playlist.SetSize(ArryPlayList.size());
		for (int i = 0; i < ArryPlayList.size(); ++i)
		{
			playlist[i] = Asic2WChar(ArryPlayList[i].asString().c_str()).c_str();
		}
		//deinterlacing = Asic2WChar(element["deinterlacing"].asCString().c_str()).c_str();
		isApplyingVideoFilter = element["isApplyingVideoFilter"].asUInt() == 1;
		videoGamma = element["videoGamma"].asUInt();
		videoContrast = element["videoContrast"].asUInt();
		videoBrightness = element["videoBrightness"].asUInt();
		CurrentIndex = element["CurrentIndex"].asUInt();
		m_bisDirectPlay = element["m_bisDirectPlay"].asUInt() == 1;
		isHaveSelect = element["isHaveSelect"].asUInt() == 1;
		bIsScanInterlace = element["ScanInterlace"].asUInt() == 1;
	}

	void Save()
	{
		element["width"] = width;
		element["height"] = height;
		element["volume"] = volume;
		element["bufferTime"] = bufferTime;
		element["LastbufferTime"] = LastbufferTime;
		element["WarnTime"] = WarnTime;
		element["isStretching"] = isStretching ? 1 : 0;
		element["isAudioOutputToStream"] = isAudioOutputToStream ? 1 : 0;
		//element["audioOutputType"] = WcharToAnsi(audioOutputType.Array()).c_str();
		//element["audioOutputTypeDevice"] = WcharToAnsi(audioOutputTypeDevice.Array()).c_str();
		//element["audioOutputDevice"] = WcharToAnsi(audioOutputDevice.Array()).c_str();
		element["isPlaylistLooping"] = isPlaylistLooping ? 1 : 0;
		element["isFileLoop"] = isFileLoop ? 1 : 0;
		element["isListLoop"] = isListLoop ? 1 : 0;
		element["isSenceChangeStartPlay"] = isSenceChangeStartPlay ? 1 : 0;
		element["isHardwareDecode"] = isHardwareDecode ? 1 : 0;

		Json::Value &ArryList = element["playlist"];
		ArryList.resize(playlist.Num());

		for (int i = 0; i < playlist.Num(); ++i)
		{
			ArryList[i] = WcharToAnsi(playlist[i].Array()).c_str();
		}
		//element["deinterlacing"] =  WcharToAnsi(deinterlacing.Array()).c_str();
		element["isApplyingVideoFilter"] = isApplyingVideoFilter ? 1 : 0;
		element["videoGamma"] = videoGamma;
		element["videoContrast"] = videoContrast;
		element["videoBrightness"] = videoBrightness;
		element["CurrentIndex"] = CurrentIndex;
		element["m_bisDirectPlay"] = m_bisDirectPlay ? 1 : 0;
		element["isHaveSelect"] = isHaveSelect ? 1 : 0;
		element["ScanInterlace"] = bIsScanInterlace ? 1 : 0;
	}

	std::vector<AudioOutputType> &GetAudioOutputTypes()
	{
		return audioOutputTypes;
	}

	AudioOutputType &GetAudioOutputType(String name)
	{
		for (auto i = audioOutputTypes.begin(); i < audioOutputTypes.end(); i++)
		{
			AudioOutputType &type = *i;
			if ((*i).GetName() == name) {
				return type;
			}
		}

		// device not found, return wavemapper
		// how could this happen? device disappeared/unplugged
		assert(audioOutputTypes.size());
		return audioOutputTypes[0];
	}
};