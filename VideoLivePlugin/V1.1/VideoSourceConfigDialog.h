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
#include<string>
using namespace Gdiplus;
class VideoSourceConfig;
class TProgramListItem;
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
	HWND hwndBufferTime;
	HWND hwndWarnTime;
	HWND hwndScanProgressive;
	HWND hwndScanInterlace;

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

//---------------------------------------------------

class TCanvas
{
	friend LRESULT CALLBACK ScreenProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	friend LRESULT CALLBACK CanvasProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);
	friend LRESULT CALLBACK BtnSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	static TCanvas* CreateCanvas(int left, int top, int width, int height, HWND hwndParent);
	void Fresh();
	void SetWindowPos(int left, int top, int width, int height);
	void SetWindowCanvasPos(int left, int top, int width, int height);
	HWND GetWinID() { return m_hHwnd; }
	HWND GetCanvasWinID() { return hwnd_canvas; }
	void Destroy() { ::DestroyWindow(m_hHwnd); }
	void SetVisiable(bool v);
	void SetScroll(int delta);

private:
	TCanvas();
	~TCanvas();
	void MouseMove(int x, int y);

private:
	HWND Parent = NULL;
	HWND m_hHwnd = NULL;
	HWND hwnd_canvas = NULL;
	HWND m_btnSlider = NULL;

	int  canvas_width = 0;//画布长度  
	int  canvas_height = 0;//画布宽度  

	int  screen_width = 0;//屏幕长度  
	int  screen_height = 0;//屏幕宽度  

	int  position_height = 0;//当前位置的纵坐标  
	int  vbPos = 0;//竖直方向滚动条当前位置  

	//==============
	int m_vPos = 0; //自会控件位置
	int m_vHeight = 0; //自会控件高度
	bool m_bTrace = false;

	//==========
	int ScrollPos;
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

class CProgramVideoSourceConfigDialog
{
private:
	VideoSourceConfig *config;
	

public:

	TCanvas* m_canvas = NULL;
	HWND hwndProgramPlaylist;
	HWND m_hwndParent;
public:
	CProgramVideoSourceConfigDialog(VideoSourceConfig *config);
	~CProgramVideoSourceConfigDialog();

public:
	bool Show();

public:
	VideoSourceConfig *GetConfig() { return config; }
	std::vector<TProgramListItem*>  *m_TProgramListItemVector = NULL;
};
enum{
	ID_DLG_SLIDER
};
#define PROGRAM_CLASS TEXT("ProgramEx")
#define ITEMSCROLL (WM_USER + 43321)
#define  ITEMCHECK (WM_USER + 43322)
#define  UPDATAURL (WM_USER + 43323)
void InitProgramListEx(HINSTANCE hInst);
LRESULT CALLBACK ProgramProcEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

class TProgramListItem
{
	friend LRESULT CALLBACK ProgramProcEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
public:
	static TProgramListItem* CreateProgramItem(int left, int top, int width, int height, String title, String URL,HWND hwndParent, HMENU menu, int tag, VideoSourceConfig *config);
	void Desory();
	void Fresh();
	HWND m_hWinID = NULL;      //控件句
	std::vector <TProgramListItem*> *m_TProgramListItemVector;
	void SetCheckFlag(bool flag)
	{
		m_check = flag;
	}
private:
	TProgramListItem();
	~TProgramListItem();
	int OnFrameCreate();
	void OnSelected(HWND hwnd);
	void UnSelectedOthers();

private:
	int m_iWidth;
	int m_iHeight;
	bool m_check = false;

	
	HWND m_hParent = NULL;     //父句柄

	Gdiplus::Image* pRadioImage = NULL;
	Gdiplus::Image* pRadioSelectImage = NULL;
	String Title;
	String m_URL;
	int mTag;
	VideoSourceConfig *m_config;
};