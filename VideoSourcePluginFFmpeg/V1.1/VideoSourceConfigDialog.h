/**
* John Bradley (jrb@turrettech.com)
*/
#pragma once

#include "MediaProcessSDK.h"
#include "BaseAfx.h"
#include "WindowsHelper.h"
#include "DropTarget.h"
#include <comdef.h>
#include <shlobj.h>
#include <gdiplus.h>
using namespace Gdiplus;
class VideoSourceConfig;

class VideoSourceConfigDialog
{
private:
    VideoSourceConfig *config;

public:
    HWND hwndPathOrUrl;
    HWND hwndWidth;
    HWND hwndHeight;
    HWND hwndVolume;
    HWND hwndStretch;
    HWND hwndIsAudioOutputToStream;
    HWND hwndIsAudioOutputToDevice;
    HWND hwndAudioOutputType;
    HWND hwndAudioOutputDevice;
    HWND hwndMediaFileOrUrl;
    HWND hwndPlaylist;
    HWND hwndPlaylistLooping;
    HWND hwndAddMedia;
    HWND hwndRemoveMedia;
    HWND hwndDeinterlacing;
    HWND hwndVideoFilter;
    HWND hwndApplyVideoFilter;
    HWND hwndVideoGamma;
    HWND hwndVideoContrast;
    HWND hwndVideoBrightness;
    HWND hwndResetVideoFilter;
	HWND hwndSenceChangeStartPlay;
	HWND hwndFileLoop;
	HWND hwndPlayListLoopRadio;
	HWND hwndHardwareDecode;
	HWND hwndScanProgressive;
	HWND hwndScanInterlace;


    DropTarget *playlistDropTarget;
    DropTargetListener *playlistDropListener;

    // dragging

    HIMAGELIST hDragImageList;
    bool isDragging;
    
public:
    VideoSourceConfigDialog(VideoSourceConfig *config);
    ~VideoSourceConfigDialog();

public:
    bool Show();

public:
    VideoSourceConfig *GetConfig() { return config; }
    void PlaylistFilesDropped(StringList &files);

};

class PlaylistDropListener : public DropTargetListener
{
public:
private:
    VideoSourceConfigDialog *config;
    
public:
    PlaylistDropListener(VideoSourceConfigDialog *config) { this->config = config; }
    
public:
    virtual void FilesDropped(StringList &files)
    {
        config->PlaylistFilesDropped(files);
    }
};

class CSeniorVideoSourceConfigDialog
{
private:
	VideoSourceConfig *config;

public:
	
	HWND hwndVideoGamma;
	HWND hwndVideoContrast;
	HWND hwndVideoBrightness;
	HWND hwndVolume;
public:
	CSeniorVideoSourceConfigDialog(VideoSourceConfig *config);
	~CSeniorVideoSourceConfigDialog();

public:
	bool Show();

public:
	VideoSourceConfig *GetConfig() { return config; }
};


class DirectPlayConfigDialog
{
private:
	VideoSourceConfig *config;
public:
	HWND hwndPlaylist;
public:
	DirectPlayConfigDialog(VideoSourceConfig *config);
	~DirectPlayConfigDialog();

public:
	bool Show();

public:
	VideoSourceConfig *GetConfig() { return config; }
	void PlaylistFilesDropped(StringList &files);
};

class CFileNotSupportHintDialog
{
public:
	HWND m_hwnd;
	String FileName;
	int index = 0;
public:
	CFileNotSupportHintDialog();
	~CFileNotSupportHintDialog();

public:
	bool Show();
};