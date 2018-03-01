/**
* John Bradley (jrb@turrettech.com)
*/
#include "VideoSourceConfigDialog.h"
#include "WindowsHelper.h"
#include "VideoSourceConfig.h"
#include "resource.h"
#include <array>
#include "CGdiPlusImage.h"
#define HANDLE_DEFAULT default: return false
#define RGB102 RGB(102, 102, 102)
#define RGB255 RGB(255, 255, 255)

CGdiPlusImage* m_pCancleBtnGrayNor = NULL;
CGdiPlusImage* m_pCancleBtnGrayHover = NULL;
CGdiPlusImage* m_pOKBtnGrayNor = NULL;
CGdiPlusImage* m_pOKBtnGrayHover = NULL;
WNDPROC buttonproc = NULL;
WNDPROC editproc = NULL;
WNDPROC resetvideofilterproc = NULL;
extern HINSTANCE HinstanceDLL;
extern LocaleStringLookup *pluginLocale;

HBRUSH HBrush;
HFONT  HFont20;
HFONT  HOldFont;

static INT_PTR CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static INT_PTR CALLBACK ResetVideoFilterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);


INT_PTR CALLBACK Config_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SeniorConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ProgramConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL Config_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL SeniorConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
BOOL ProgramConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);
void SeniorConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void Config_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void ProgramConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
INT_PTR CALLBACK Config_OnNotify(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void Config_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
void Config_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
void Config_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized);
void SeniorConfig_OnPaint(HWND hwnd);
void Config_OnPaint(HWND hwnd);
void ProgramConfig_OnPaint(HWND hwnd);
HBRUSH Config_OnEditChangeColor(HWND hwnd, HDC hDc, HWND lParam, UINT ID);
HBRUSH Config_OnStaticChangeColor(HWND hwnd, HDC hDc, HWND lParam, UINT ID);
std::array<String, 9> deinterlacing_modes = {TEXT("none"), TEXT("blend"), TEXT("bob"), TEXT("discard"), TEXT("linear"), TEXT("mean"), TEXT("x"), TEXT("yadif"), TEXT("yadif2x")};

bool DrawImagePng(HDC DC, Gdiplus::Image* pImage, INT nXPos, INT nYPos, INT Width, INT Height)
{
	//加载判断
	if (pImage == NULL) return false;

	//创建屏幕
	Graphics graphics(DC);

	//获取属性
	INT nImageWidth = pImage->GetWidth();
	INT nImageHeight = pImage->GetHeight();

	//构造位置
	RectF rcDrawRect;
	rcDrawRect.X = (REAL)nXPos;
	rcDrawRect.Y = (REAL)nYPos;
	rcDrawRect.Width = (REAL)Width;
	rcDrawRect.Height = (REAL)Height;

	//绘画图像
	graphics.DrawImage(pImage, rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
	return true;
}

VideoSourceConfigDialog::VideoSourceConfigDialog(VideoSourceConfig *config)
{
    this->config = config;

    playlistDropListener = new PlaylistDropListener(this);
    isDragging = false;
}

VideoSourceConfigDialog::~VideoSourceConfigDialog()
{
    delete playlistDropListener;
}

bool VideoSourceConfigDialog::Show()
{
	return DialogBoxParam(HinstanceDLL, MAKEINTRESOURCE(IDD_VIDEOCONFIG), GetMainWindow(), Config_DlgProc, (LPARAM)this) == IDOK;
}

void VideoSourceConfigDialog::PlaylistFilesDropped(StringList &files)
{
    LVITEM item;
    item.mask       = LVIF_TEXT | LVIF_STATE;
    item.stateMask  = 0;
    item.iSubItem   = 0;
    item.state      = 0;

    int insertIndex = ListView_GetItemCount(hwndPlaylist);
    for(unsigned int i = 0; i < files.Num(); i++) {
        item.pszText = files[i];
        item.iItem = insertIndex++;
        ListView_InsertItem(hwndPlaylist, &item);
    }
}

INT_PTR CALLBACK Config_DlgProc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
    switch(msg)
    {
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
		case WM_NCPAINT:
		{
		DrawFrame(hwndDlg, -1, true);
		}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg, -1, true);
	}
	 case UPDATAURL:
	{
					       VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwndDlg, DWLP_USER);
						   VideoSourceConfig *config = _this->GetConfig();
						   if (config->playlist.Num())
							   SetWindowText(_this->hwndMediaFileOrUrl, config->playlist[0]);
						   break;
	}
		break;
        HANDLE_MSG      (hwndDlg,   WM_INITDIALOG,  Config_OnInitDialog);
        HANDLE_MSG      (hwndDlg,   WM_COMMAND,     Config_OnCommand);
        HANDLE_MSG      (hwndDlg,   WM_LBUTTONUP,   Config_OnLButtonUp);
        HANDLE_MSG      (hwndDlg,   WM_MOUSEMOVE,   Config_OnMouseMove);
        HANDLE_MSG      (hwndDlg,   WM_ACTIVATE,    Config_OnActivate);
		HANDLE_MSG      (hwndDlg,   WM_PAINT,       Config_OnPaint);
		HANDLE_MSG      (hwndDlg,   WM_CTLCOLOREDIT, Config_OnEditChangeColor);
		HANDLE_MSG		(hwndDlg,   WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);
		
	
    case WM_NOTIFY: return Config_OnNotify(hwndDlg, msg, wParam, lParam);

        HANDLE_DEFAULT;	
    }
}

INT_PTR CALLBACK SeniorConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	static bool _bMouseTrack = true;
	switch (msg)
	{
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwndDlg,  -1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg, -1, true);
	}
		break;
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, SeniorConfig_OnInitDialog);
		HANDLE_MSG(hwndDlg, WM_COMMAND, SeniorConfig_OnCommand);
		HANDLE_MSG(hwndDlg, WM_PAINT, SeniorConfig_OnPaint);
		HANDLE_MSG(hwndDlg, WM_LBUTTONUP, Config_OnLButtonUp);
		HANDLE_MSG(hwndDlg, WM_MOUSEMOVE, Config_OnMouseMove);
		HANDLE_MSG(hwndDlg, WM_ACTIVATE, Config_OnActivate);
		HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);

	case WM_NOTIFY: return Config_OnNotify(hwndDlg, msg, wParam, lParam);
	case WM_CLOSE:
	{
					 EndDialog(hwndDlg, IDOK);
					 break;
	}

		HANDLE_DEFAULT;
	}
}

BOOL Config_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
    VideoSourceConfigDialog *_this = reinterpret_cast<VideoSourceConfigDialog *>(lParam);
    VideoSourceConfig *config = _this->GetConfig();

	LocalizeWindow(hwnd, pluginLocale);

    _this->hwndWidth                    = GetDlgItem(hwnd, IDC_WIDTH);
    _this->hwndHeight                   = GetDlgItem(hwnd, IDC_HEIGHT);
    _this->hwndVolume                   = GetDlgItem(hwnd, IDC_VOLUME);
    _this->hwndStretch                  = GetDlgItem(hwnd, IDC_STRETCH);
    _this->hwndIsAudioOutputToStream    = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TO_STREAM);
    _this->hwndIsAudioOutputToDevice    = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TO_DEVICE);
    _this->hwndAudioOutputType          = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_TYPE);
    _this->hwndAudioOutputDevice        = GetDlgItem(hwnd, IDC_AUDIO_OUTPUT_DEVICE);
    _this->hwndMediaFileOrUrl           = GetDlgItem(hwnd, IDC_MEDIA_FILE_OR_URL);
    _this->hwndPlaylist                 = GetDlgItem(hwnd, IDC_PLAYLIST); 
    _this->hwndAddMedia                 = GetDlgItem(hwnd, IDC_ADD_MEDIA);
    _this->hwndRemoveMedia              = GetDlgItem(hwnd, IDC_REMOVE_MEDIA);
    _this->hwndPlaylistLooping          = GetDlgItem(hwnd, IDC_PLAYLIST_LOOP);
    _this->hwndDeinterlacing            = GetDlgItem(hwnd, IDC_DEINTERLACING);
    _this->hwndVideoFilter              = GetDlgItem(hwnd, IDC_VIDEOFILTER);
    _this->hwndApplyVideoFilter         = GetDlgItem(hwnd, IDC_APPLYVIDEOFILTER);
    _this->hwndVideoGamma               = GetDlgItem(hwnd, IDC_VIDEOGAMMA); 
    _this->hwndVideoContrast            = GetDlgItem(hwnd, IDC_VIDEOCONTRAST);
    _this->hwndVideoBrightness          = GetDlgItem(hwnd, IDC_VIDEOBRIGHTNESS);
    _this->hwndResetVideoFilter         = GetDlgItem(hwnd, IDC_RESETVIDEOFILTER);
	_this->hwndBufferTime				= GetDlgItem(hwnd, ID_BUFFERTIME);
	_this->hwndWarnTime                 = GetDlgItem(hwnd, ID_BUFFERTIMEWARN);
	_this->hwndScanProgressive          = GetDlgItem(hwnd, IDC_PROGRESSIVE);
	_this->hwndScanInterlace            = GetDlgItem(hwnd, IDC_INTERLACE);

	SetWindowText(_this->hwndBufferTime, FormattedString(TEXT("%d"), config->bufferTime));
	SetWindowText(_this->hwndWarnTime, FormattedString(TEXT("%d"), config->WarnTime));

	HFONT TimehFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	::SendMessage(_this->hwndBufferTime, WM_SETFONT, (WPARAM)TimehFont, TRUE);
	::SendMessage(_this->hwndWarnTime, WM_SETFONT, (WPARAM)TimehFont, TRUE);
	TEXTMETRIC Timetm;
	GetTextMetrics(GetDC(_this->hwndBufferTime), &Timetm);
	int TimenFontHeight = Timetm.tmHeight + Timetm.tmExternalLeading;
	Rect Timerect;
	GetClientRect(_this->hwndBufferTime, (LPRECT)&Timerect);
	OffsetRect((LPRECT)&Timerect, 0, (Timerect.Height - TimenFontHeight) / 2 - 1);
	SendMessage(_this->hwndBufferTime, EM_SETRECT, 0, (LPARAM)&Timerect);

	Rect Warnrect;
	GetClientRect(_this->hwndWarnTime, (LPRECT)&Warnrect);
	OffsetRect((LPRECT)&Warnrect, 0, (Warnrect.Height - TimenFontHeight) / 2 - 1);
	SendMessage(_this->hwndWarnTime, EM_SETRECT, 0, (LPARAM)&Warnrect);

	ListView_SetBkColor(_this->hwndPlaylist, RGB(102, 102, 102));
	ListView_SetTextBkColor(_this->hwndPlaylist, RGB(102, 102, 102));
	ListView_SetTextColor(_this->hwndPlaylist, RGB(255, 255, 255));

	SetWindowPos(GetDlgItem(hwnd, IDCANCEL), NULL, 430, 235, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDOK), NULL, 310, 235, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SetWindowPos(GetDlgItem(hwnd, IDC_ADD_MEDIA), NULL, 450, 82, 100, 36, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);

	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADD_MEDIA), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, 1), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, 2), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	editproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_SUBTIME), GWLP_WNDPROC, (LONG_PTR)EditProc);
	editproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADDTIME), GWLP_WNDPROC, (LONG_PTR)EditProc);
	editproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_SUBTIMEWARN), GWLP_WNDPROC, (LONG_PTR)EditProc);
	editproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ADDTIMEWARN), GWLP_WNDPROC, (LONG_PTR)EditProc);
	
	resetvideofilterproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_RESETVIDEOFILTER), GWLP_WNDPROC, (LONG_PTR)ResetVideoFilterProc);
	
    Edit_SetText(_this->hwndWidth,      IntString(config->width).Array());
    Edit_SetText(_this->hwndHeight,     IntString(config->height).Array());

    Button_SetCheck(_this->hwndStretch, config->isStretching);
    Slider_SetRange(_this->hwndVolume, 0, 100);
    Slider_SetPos(_this->hwndVolume, config->volume);
    
	EnableWindow(_this->hwndResetVideoFilter, config->isApplyingVideoFilter);

	EnableWindow(_this->hwndVideoGamma, TRUE);
	EnableWindow(_this->hwndVideoContrast, TRUE);
	EnableWindow(_this->hwndVideoBrightness, TRUE);
	EnableWindow(_this->hwndResetVideoFilter, TRUE);
    
    Button_SetCheck(_this->hwndApplyVideoFilter, config->isApplyingVideoFilter);

    Slider_SetRange(_this->hwndVideoGamma, 1, 1000);
    Slider_SetPos(_this->hwndVideoGamma, config->videoGamma);

    Slider_SetRange(_this->hwndVideoContrast, 0, 200);
    Slider_SetPos(_this->hwndVideoContrast, config->videoContrast);

    Slider_SetRange(_this->hwndVideoBrightness, 0, 200);
    Slider_SetPos(_this->hwndVideoBrightness, config->videoBrightness);

    int index = -1;

    auto audioOutputTypes = config->GetAudioOutputTypes();

    for(auto i = audioOutputTypes.begin(); i < audioOutputTypes.end(); i++) {
        AudioOutputType &audioOutputType = *i;
        ComboBox_AddString(_this->hwndAudioOutputType, audioOutputType.GetDescription().Array());
        if (audioOutputType.GetName() == config->audioOutputType) {
            index = (int)(i - audioOutputTypes.begin());
        }
    }

    if (index < 0) {
        index = 0;                
    }

    ComboBox_SetCurSel(_this->hwndAudioOutputType, index);

    Button_SetCheck(_this->hwndIsAudioOutputToStream, config->isAudioOutputToStream);
    Button_SetCheck(_this->hwndIsAudioOutputToDevice, !config->isAudioOutputToStream);
	Button_SetCheck(_this->hwndScanProgressive, !config->bIsScanInterlace);
	Button_SetCheck(_this->hwndScanInterlace, config->bIsScanInterlace);

    LVCOLUMN lvc;
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT ; 
    lvc.fmt = LVCFMT_LEFT;
    String playlistFile(L"PlaylistFile");
    lvc.pszText = playlistFile;
    lvc.cchTextMax = playlistFile.Length();
    lvc.iSubItem = 0;      
    lvc.iImage = 0;        
    lvc.iOrder = 0;        
    ListView_InsertColumn(_this->hwndPlaylist, 0, &lvc);

    Button_SetCheck(_this->hwndPlaylistLooping, config->isPlaylistLooping);

    _this->PlaylistFilesDropped(config->playlist);

    index = 0;
    for(auto i = deinterlacing_modes.begin(); i < deinterlacing_modes.end(); ++i) {
        ComboBox_AddString(_this->hwndDeinterlacing, *i);
        if(*i == config->deinterlacing) {
            index = i - deinterlacing_modes.begin();
        }
    }
    ComboBox_SetCurSel(_this->hwndDeinterlacing, index);

    FORWARD_WM_COMMAND(
        hwnd, 
        config->isAudioOutputToStream ? IDC_AUDIO_OUTPUT_TO_STREAM : IDC_AUDIO_OUTPUT_TO_DEVICE,
        config->isAudioOutputToStream ? _this->hwndIsAudioOutputToStream : _this->hwndIsAudioOutputToDevice,
        BN_CLICKED,
        SendMessage);
	if(config->playlist.Num())
		SetWindowText(_this->hwndMediaFileOrUrl, config->playlist[0]);
	HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	::SendMessage(_this->hwndMediaFileOrUrl, WM_SETFONT, (WPARAM)hFont, TRUE);
	TEXTMETRIC tm;
	GetTextMetrics(GetDC(_this->hwndMediaFileOrUrl), &tm);
	int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
	Rect rect;
	GetClientRect(_this->hwndMediaFileOrUrl, (LPRECT)&rect);
	OffsetRect((LPRECT)&rect, 0, (rect.Height - nFontHeight) / 2 - 3);
	SendMessage(_this->hwndMediaFileOrUrl, EM_SETRECT, 0, (LPARAM)&rect);

	InvalidateRect(hwnd, NULL, FALSE);
	SendMessage(hwnd, WM_NCPAINT, 0, 0);
	
    return true;
}

BOOL SeniorConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	CSeniorVideoSourceConfigDialog *_this = reinterpret_cast<CSeniorVideoSourceConfigDialog *>(lParam);
	VideoSourceConfig *config = _this->GetConfig();


	_this->hwndVolume = GetDlgItem(hwnd, IDC_VOLUMESENIOR);
	_this->hwndVideoGamma = GetDlgItem(hwnd, IDC_VIDEOGAMMASENIOR);
	_this->hwndVideoContrast = GetDlgItem(hwnd, IDC_VIDEOCONTRASTSENIOR);
	_this->hwndVideoBrightness = GetDlgItem(hwnd, IDC_VIDEOBRIGHTNESSSENIOR);
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDSENIOROK), GWLP_WNDPROC, (LONG_PTR)ButtonProc);
	Slider_SetRange(_this->hwndVolume, 0, 100);
	Slider_SetPos(_this->hwndVolume, config->volume);

	EnableWindow(_this->hwndVideoGamma, true);
	EnableWindow(_this->hwndVideoContrast, true);
	EnableWindow(_this->hwndVideoBrightness, true);


	Slider_SetRange(_this->hwndVideoGamma, 1, 1000);
	Slider_SetPos(_this->hwndVideoGamma, config->videoGamma);

	Slider_SetRange(_this->hwndVideoContrast, 0, 200);
	Slider_SetPos(_this->hwndVideoContrast, config->videoContrast);

	Slider_SetRange(_this->hwndVideoBrightness, 0, 200);
	Slider_SetPos(_this->hwndVideoBrightness, config->videoBrightness);

	return true;
}

void SeniorConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	VideoSourceConfig *config = _this->GetConfig();
	switch (id)
	{

		case IDSENIOROK:
		{
			config->volume = Slider_GetPos(_this->hwndVolume);
			config->videoGamma = Slider_GetPos(_this->hwndVideoGamma);
			config->videoContrast = Slider_GetPos(_this->hwndVideoContrast);
			config->videoBrightness = Slider_GetPos(_this->hwndVideoBrightness);
			EndDialog(hwnd, IDOK);
			break;
		}
	}
}

void Config_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
    case IDC_AUDIO_OUTPUT_TO_DEVICE:
    case IDC_AUDIO_OUTPUT_TO_STREAM:
        {
            if (codeNotify == BN_CLICKED) {
                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                bool isAudioOutputToStream = id == IDC_AUDIO_OUTPUT_TO_STREAM;

                EnableWindow(_this->hwndAudioOutputType, !isAudioOutputToStream);
                EnableWindow(_this->hwndAudioOutputDevice, !isAudioOutputToStream);

                if (!isAudioOutputToStream) {
                    int index = ComboBox_GetCurSel(_this->hwndAudioOutputType);
                    if (index > 0) {
                        AudioOutputType &type = _this->GetConfig()->GetAudioOutputTypes()[index];
                        EnableWindow(_this->hwndAudioOutputDevice, (type.GetAudioOutputDevices().size()) ? true : false);
                    }
                }
            }
            break;
        }
    case IDC_AUDIO_OUTPUT_TYPE:
        {
            if (codeNotify == CBN_SELCHANGE) {

                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

                int index = ComboBox_GetCurSel(_this->hwndAudioOutputType);

                ComboBox_ResetContent(_this->hwndAudioOutputDevice);

				EnableWindow(_this->hwndAudioOutputDevice, false);

            }
            break;
        }
	
    case IDC_ADD_MEDIA:
        {
            if (codeNotify == BN_CLICKED) {				

				CProgramVideoSourceConfigDialog *_this = (CProgramVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
				BLiveDialogBox(HinstanceDLL, MAKEINTRESOURCE(IDD_SOURCECHANNEL), hwnd, ProgramConfig_DlgProc, (LPARAM)_this);
				break;
            }
				          
        }		
	
    case IDC_APPLYVIDEOFILTER:
        {
            if (codeNotify == BN_CLICKED) {
                VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
                bool isChecked = Button_GetCheck(_this->hwndApplyVideoFilter) == 1;
                EnableWindow(_this->hwndVideoGamma, isChecked);
                EnableWindow(_this->hwndVideoContrast, isChecked);
                EnableWindow(_this->hwndVideoBrightness, isChecked);
                EnableWindow(_this->hwndResetVideoFilter, isChecked);
            }
            break;
        }
    case IDC_RESETVIDEOFILTER:
        {
				CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
				DialogBoxParam(HinstanceDLL, MAKEINTRESOURCE(IDD_DIALOGSENIOR), GetMainWindow(), SeniorConfig_DlgProc, (LPARAM)_this);
				break;
        }

    case IDOK:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
            VideoSourceConfig *config = _this->GetConfig();
			
			String BufferTime;
			BufferTime.SetLength(256);
			GetWindowText(_this->hwndBufferTime, BufferTime, BufferTime.Length());
			config->bufferTime = BufferTime.ToInt(10);

			if (config->bufferTime > 60)
			{
				std::wstring error = L"视频缓冲时间不能大于60，请重新设定";
				BLiveMessageBox(GetMainWindow(), error.c_str()/*String(error.c_str()).Array()*/, NULL, MB_ICONERROR);
				break;
			}

			String WarnTime;
			WarnTime.SetLength(256);
			GetWindowText(_this->hwndWarnTime, WarnTime, WarnTime.Length());
			config->WarnTime = WarnTime.ToInt(10);

			if (config->WarnTime > config->bufferTime)
			{
				std::wstring error = L"报警阈值不能大于缓冲时间，请重新设定";
				BLiveMessageBox(GetMainWindow(), error.c_str()/*String(error.c_str()).Array()*/, NULL, MB_ICONERROR);
				break;
			}
			
			
			String newItem(GetEditText(_this->hwndMediaFileOrUrl));

			while (newItem.Length() && newItem[0] == L' ') {
				newItem.RemoveChar(0);
			}

			if (newItem.Length()) {
				//TODO: remove when fix is released
				newItem.Array()[newItem.Length()] = 0;
				StringList media;
				media.Add(String(newItem));
				_this->PlaylistFilesDropped(media);
			}
            config->deinterlacing = deinterlacing_modes[ComboBox_GetCurSel(_this->hwndDeinterlacing)];
          //  config->volume = Slider_GetPos(_this->hwndVolume);
            config->isStretching = Button_IsChecked(_this->hwndStretch);
            config->isAudioOutputToStream = Button_IsChecked(_this->hwndIsAudioOutputToStream);
			config->bIsScanInterlace = Button_IsChecked(_this->hwndScanInterlace);

			config->isApplyingVideoFilter = true;//Button_IsChecked(_this->hwndApplyVideoFilter);
            //config->videoGamma = Slider_GetPos(_this->hwndVideoGamma);
            //config->videoContrast = Slider_GetPos(_this->hwndVideoContrast);
            //config->videoBrightness = Slider_GetPos(_this->hwndVideoBrightness);

//             int audioOutputTypeIndex = ComboBox_GetCurSel(_this->hwndAudioOutputType);
//             int audioOutputDeviceIndex = ComboBox_GetCurSel(_this->hwndAudioOutputDevice);
// 
//             AudioOutputType &type = config->GetAudioOutputTypes()[audioOutputTypeIndex];
//             if (audioOutputDeviceIndex >= 0) {
//                 AudioOutputDevice &device = type.GetAudioOutputDevices()[audioOutputDeviceIndex];
//                 config->audioOutputDevice = device.GetLongName();
//             }

            config->isPlaylistLooping = Button_IsChecked(_this->hwndPlaylistLooping);
            config->playlist.Clear();

			if (newItem.Length())
			{
				config->playlist.Add(newItem);
			}
			else
			{
				std::wstring error = L"URL不能为空，请重新输入";
				BLiveMessageBox(GetMainWindow(), error.c_str()/*String(error.c_str()).Array()*/, NULL, MB_ICONERROR);
				break;
			}

			if (m_pCancleBtnGrayHover)
			{
				delete m_pCancleBtnGrayHover;
				m_pCancleBtnGrayHover = NULL;
			}

			if (m_pCancleBtnGrayNor)
			{
				delete m_pCancleBtnGrayNor;
				m_pCancleBtnGrayNor = NULL;
			}

			if (m_pOKBtnGrayHover)
			{
				delete m_pOKBtnGrayHover;
				m_pOKBtnGrayHover = NULL;
			}

			if (m_pOKBtnGrayNor)
			{
				delete m_pOKBtnGrayNor;
				m_pOKBtnGrayNor = NULL;
			}

			if (config->LastPlayFile.Compare(newItem))
			{
				
				if (config->LastbufferTime != config->bufferTime)
				{
					std::wstring error = L"重新缓冲视频会停止，是否重新缓冲";
					int ret = BLiveMessageBox(GetMainWindow(), error.c_str()/*String(error.c_str()).Array()*/, L"缓冲提示", MB_OKCANCEL);
					if (ret == 2)
					{
						config->bufferTime = config->LastbufferTime;
						EndDialog(hwnd, IDOK);
					}
				}
			}
			if (config->LastPlayFile.Length() == 0)
			{
				config->LastPlayFile = newItem;
			}
			config->LastPlayFile = newItem;
			config->LastbufferTime = config->bufferTime;
            EndDialog(hwnd, IDOK);
            break;
        }
    case IDCANCEL:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
			if (m_pCancleBtnGrayHover)
			{
				delete m_pCancleBtnGrayHover;
				m_pCancleBtnGrayHover = NULL;
			}

			if (m_pCancleBtnGrayNor)
			{
				delete m_pCancleBtnGrayNor;
				m_pCancleBtnGrayNor = NULL;
			}

			if (m_pOKBtnGrayHover)
			{
				delete m_pOKBtnGrayHover;
				m_pOKBtnGrayHover = NULL;
			}

			if (m_pOKBtnGrayNor)
			{
				delete m_pOKBtnGrayNor;
				m_pOKBtnGrayNor = NULL;
			}
            EndDialog(hwnd, IDCANCEL);
            break;
        }
    }
}

void Config_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (!_this->isDragging) {
        return;
    }

    // End the drag-and-drop process
    _this->isDragging = false;

    ImageList_DragLeave(_this->hwndPlaylist);
    ImageList_EndDrag();
    ImageList_Destroy(_this->hDragImageList);

    ReleaseCapture();

    // Determine the dropped item
    LVHITTESTINFO lvhti;
    lvhti.pt.x = x;
    lvhti.pt.y = y;
    ClientToScreen(hwnd, &lvhti.pt);
    ScreenToClient(_this->hwndPlaylist, &lvhti.pt);
    ListView_HitTest(_this->hwndPlaylist, &lvhti);

    // Out of the ListView?
    if (lvhti.iItem == -1) {
        return;
    }

    // Not in an item?
    if (((lvhti.flags & LVHT_ONITEMLABEL) == 0) && 
        ((lvhti.flags & LVHT_ONITEMSTATEICON) == 0)) {
            return;
    }

    // Dropped item is selected?
    LVITEM lvi;
    lvi.iItem = lvhti.iItem;
    lvi.iSubItem = 0;
    lvi.mask = LVIF_STATE;
    lvi.stateMask = LVIS_SELECTED;
    ListView_GetItem(_this->hwndPlaylist, &lvi);

    if (lvi.state & LVIS_SELECTED) {
        return;
    }

    int dropIndex = lvi.iItem;

    // Rearrange the items
    int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
    TCHAR buf[MAX_PATH];
    while (position != -1) {
        // First, copy one item
        lvi.iItem = position;
        lvi.iSubItem = 0;
        lvi.cchTextMax = MAX_PATH;
        lvi.pszText = buf;
        lvi.stateMask = ~LVIS_SELECTED;
        lvi.mask = LVIF_STATE | LVIF_IMAGE 
            | LVIF_INDENT | LVIF_PARAM | LVIF_TEXT;

        ListView_GetItem(_this->hwndPlaylist, &lvi);

        lvi.iItem = lvhti.iItem;
        if (lvhti.iItem < position) {
            lvi.iItem = lvhti.iItem;
        } else {
            lvi.iItem = lvhti.iItem + 1;
        }

        // Insert the main item
        int insertedItemIdex = ListView_InsertItem(_this->hwndPlaylist, &lvi);
        if (lvi.iItem < position) {
            lvhti.iItem++;
        }
        if (insertedItemIdex <= position) {
            position++;
        }

        // Delete from original position
        ListView_DeleteItem(_this->hwndPlaylist, position);
        position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
    }

}

void Config_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

    if (!_this->isDragging) {
        return;
    }

    POINT p;
    p.x = x;
    p.y = y;

    ClientToScreen(hwnd, &p);
    ImageList_DragMove(p.x, p.y);
}

void SeniorConfig_OnPaint(HWND hwnd) {

	CSeniorVideoSourceConfigDialog *_this = (CSeniorVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;

	HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
	RECT rect;
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_153);
	EndPaint(hwnd, &ps);
	DeleteObject(hBK_153);

}

INT_PTR CALLBACK Config_OnNotify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (((LPNMHDR)lParam)->code)
    {
    case LVN_BEGINDRAG:
        {
            VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);

            // You can set your customized cursor here
            POINT p;
            p.x = 8;
            p.y = 8;

            // Ok, now we create a drag-image for all selected items
            bool isFirst = true;
            int position = ListView_GetNextItem(_this->hwndPlaylist, -1, LVNI_SELECTED);
            int totalHeight;

            IMAGEINFO imf;
            HIMAGELIST hOneImageList;
            HIMAGELIST hTempImageList;

            while (position != -1) {
                if (isFirst) {
                    // For the first selected item,
                    // we simply create a single-line drag image
                    _this->hDragImageList = ListView_CreateDragImage(_this->hwndPlaylist, position, &p);
                    ImageList_GetImageInfo(_this->hDragImageList, 0, &imf);
                    totalHeight = imf.rcImage.bottom;
                    isFirst = false;
                } else {
                    // For the rest selected items,
                    // we create a single-line drag image, then
                    // append it to the bottom of the complete drag image
                    hOneImageList = ListView_CreateDragImage(_this->hwndPlaylist, position, &p);
                    hTempImageList = ImageList_Merge(_this->hDragImageList, 
                        0, hOneImageList, 0, 0, totalHeight);
                    ImageList_Destroy(_this->hDragImageList);
                    ImageList_Destroy(hOneImageList);
                    _this->hDragImageList = hTempImageList;
                    ImageList_GetImageInfo(_this->hDragImageList, 0, &imf);
                    totalHeight = imf.rcImage.bottom;
                }
                position = ListView_GetNextItem(_this->hwndPlaylist, position, LVNI_SELECTED);
            }

            // Now we can initialize then start the drag action
            ImageList_BeginDrag(_this->hDragImageList, 0, 0, 0);

            POINT pt;
            pt = ((NM_LISTVIEW*) ((LPNMHDR)lParam))->ptAction;
            ClientToScreen(_this->hwndPlaylist, &pt);

            ImageList_DragEnter(GetDesktopWindow(), pt.x, pt.y);

            _this->isDragging = true;

            // Don't forget to capture the mouse
            SetCapture(hwnd);

            break;
        }
    }

    return FALSE;
}

void Config_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
    if (state == WA_INACTIVE) {
        if (_this && _this->isDragging) {
            _this->isDragging = false;
            ImageList_DragLeave(_this->hwndPlaylist);
            ImageList_EndDrag();
            ImageList_Destroy(_this->hDragImageList);
            ReleaseCapture();
        }
    }
}

void Config_OnPaint(HWND hwnd) {

	VideoSourceConfigDialog *_this = (VideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;

	HBRUSH hBK_42 = CreateSolidBrush(RGB(42, 42, 44));
	RECT rect;
	
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_42);
	EndPaint(hwnd, &ps);

	DeleteObject(hBK_42);

}

HBRUSH Config_OnEditChangeColor(HWND hwnd, HDC wParam, HWND lParam, UINT ID) {
	
	HDC hdc = (HDC)wParam;
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkColor(hdc, RGB(31, 31, 32));
	//HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	//HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
	if (HBrush)
	{
		DeleteObject(HBrush);
	}
	HBrush = CreateSolidBrush(RGB(31, 31, 32));
	return HBrush;

}

HBRUSH Config_OnStaticChangeColor(HWND hwnd, HDC wParam, HWND lParam, UINT ID){

	HDC hdc = (HDC)wParam;
	SetTextColor(hdc, RGB(255, 255, 255));
	SetBkColor(hdc, RGB(42, 42, 44));
	//HFont20 = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	//HFONT  m_hOldFont = (HFONT)SelectObject(hdc, HFont20);
	if (HBrush)
	{
		DeleteObject(HBrush);
	}
	HBrush = CreateSolidBrush(RGB(42, 42, 44));
	return HBrush;

}

static INT_PTR CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(102, 102, 102));
					 HFONT  m_hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(176, 176, 176));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);
					 String DB_Tianjia = WndTitle;
					 TextOut(hDC, 10, 0, DB_Tianjia, DB_Tianjia.Length());
					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 DeleteObject(hBK_102);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(buttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

static INT_PTR CALLBACK ButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	
	static bool m_bMouseTrack = FALSE;

	if (!m_pCancleBtnGrayHover)
	{
		m_pCancleBtnGrayHover = new CGdiPlusImage;
		m_pCancleBtnGrayHover->LoadFromFile(L".\\img\\btn_gray_hover.png");
	}

	if (!m_pCancleBtnGrayNor)
	{
		m_pCancleBtnGrayNor = new CGdiPlusImage;
		m_pCancleBtnGrayNor->LoadFromFile(L".\\img\\btn_gray_nor.png");
	}

	if (!m_pOKBtnGrayHover)
	{
		m_pOKBtnGrayHover = new CGdiPlusImage;
		m_pOKBtnGrayHover->LoadFromFile(L".\\img\\btn_blue_hover.png");
	}

	if (!m_pOKBtnGrayNor)
	{
		m_pOKBtnGrayNor = new CGdiPlusImage;
		m_pOKBtnGrayNor->LoadFromFile(L".\\img\\btn_blue_nor.png");
	}


	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);
					 Graphics graphics(hdc);
					 INT nImageWidth = m_pCancleBtnGrayNor->GetWidth();
					 INT nImageHeight = m_pCancleBtnGrayNor->GetHeight();

					 RectF rcDrawRect;
					 rcDrawRect.X = (REAL)0;
					 rcDrawRect.Y = (REAL)0;
					 rcDrawRect.Width = (REAL)nImageWidth;
					 rcDrawRect.Height = (REAL)nImageHeight;
					 if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
					 {
						 graphics.DrawImage(m_pOKBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 }
					 else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
					 {
						 graphics.DrawImage(m_pCancleBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
					 }


					 SetTextColor(hdc, RGB(255, 255, 255));

					 SetBkColor(hdc, RGB(102, 102, 102));
					 SetBkMode(hdc, TRANSPARENT);

					 SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

					 TCHAR Title[MAX_PATH] = { 0 };

					 GetWindowText(hwnd, Title, sizeof Title);

					 RECT rtClient;
					 GetClientRect(hwnd, &rtClient);

					 if (wcslen(Title) > 0)
					 {
						 DrawText(hdc, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
					 }
					 EndPaint(hwnd, &ps);
					 break;
	}
	case WM_MOUSEHOVER:
	{
						  if (!m_bMouseTrack)
						  {
							  HDC hDC = GetDC(hwnd);
							  Graphics graphics(hDC);
							  INT nImageWidth = m_pCancleBtnGrayHover->GetWidth();
							  INT nImageHeight = m_pCancleBtnGrayHover->GetHeight();

							  //构造位置
							  RectF rcDrawRect;
							  rcDrawRect.X = (REAL)0;
							  rcDrawRect.Y = (REAL)0;
							  rcDrawRect.Width = (REAL)nImageWidth;
							  rcDrawRect.Height = (REAL)nImageHeight;

							  //绘画图像
							  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
							  {
								  graphics.DrawImage(m_pOKBtnGrayHover->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							  }
							  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
							  {
								  graphics.DrawImage(m_pCancleBtnGrayHover->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
							  }

							  SetTextColor(hDC, RGB(255, 255, 255));

							  SetBkColor(hDC, RGB(102, 102, 102));
							  SetBkMode(hDC, TRANSPARENT);

							  SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

							  TCHAR Title[MAX_PATH] = { 0 };

							  GetWindowText(hwnd, Title, sizeof Title);

							  RECT rtClient;
							  GetClientRect(hwnd, &rtClient);

							  if (wcslen(Title) > 0)
							  {
								  DrawText(hDC, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
							  }

							  ReleaseDC(hwnd, hDC);
						  }
						  break;
	}

	case WM_MOUSEMOVE:
	{
						 TRACKMOUSEEVENT csTME;
						 csTME.cbSize = sizeof (csTME);
						 csTME.dwFlags = TME_LEAVE | TME_HOVER;
						 csTME.hwndTrack = hwnd;// 指定要 追踪 的窗口 
						 csTME.dwHoverTime = 10;  // 鼠标在按钮上停留超过 10ms ，才认为状态为 HOVER
						 ::_TrackMouseEvent(&csTME); // 开启 Windows 的 WM_MOUSELEAVE ， WM_MOUSEHOVER 事件支持

						 m_bMouseTrack = FALSE;   // 若已经 追踪 ，则停止 追踪 
						 break;
	}
	case WM_MOUSELEAVE:
	{
						  m_bMouseTrack = TRUE;
						  //改变状态
						  HDC hDC = GetDC(hwnd);
						  RECT meterGray;
						  HBRUSH  HBrushBackGround = CreateSolidBrush(RGB(42, 42, 44));
						  meterGray.left = 0;
						  meterGray.right = 100;
						  meterGray.bottom = 0;
						  meterGray.top = 36;
						  FillRect(hDC, &meterGray, HBrushBackGround);
						  DeleteObject(HBrushBackGround);

						  Graphics graphics(hDC);
						  INT nImageWidth = m_pCancleBtnGrayNor->GetWidth();
						  INT nImageHeight = m_pCancleBtnGrayNor->GetHeight();

						  //构造位置
						  RectF rcDrawRect;
						  rcDrawRect.X = (REAL)0;
						  rcDrawRect.Y = (REAL)0;
						  rcDrawRect.Width = (REAL)nImageWidth;
						  rcDrawRect.Height = (REAL)nImageHeight;

						  //绘画图像
						  if (hwnd == GetDlgItem(GetParent(hwnd), IDOK))
						  {
							  graphics.DrawImage(m_pOKBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						  }
						  else /*if (hwnd == GetDlgItem(GetParent(hwnd), IDCANCEL))*/
						  {
							  graphics.DrawImage(m_pCancleBtnGrayNor->GetImage(), rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
						  }
						  SetTextColor(hDC, RGB(255, 255, 255));

						  SetBkColor(hDC, RGB(102, 102, 102));
						  SetBkMode(hDC, TRANSPARENT);

						  SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));

						  TCHAR Title[MAX_PATH] = { 0 };

						  GetWindowText(hwnd, Title, sizeof Title);

						  RECT rtClient;
						  GetClientRect(hwnd, &rtClient);

						  if (wcslen(Title) > 0)
						  {
							  DrawText(hDC, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
						  }

						  ReleaseDC(hwnd, hDC);
						  break;
	}

	default:
		return CallWindowProc(buttonproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

static INT_PTR CALLBACK ResetVideoFilterProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {

	switch (message)
	{
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HBRUSH hBK_153 = CreateSolidBrush(RGB(42, 42, 44));
					 HBRUSH hBK_57 = CreateSolidBrush(RGB(57, 57, 59));
					 HBRUSH hBK_102 = CreateSolidBrush(RGB(42, 42, 44));
					 HFONT  m_hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
						 OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");

					 RECT rect;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 GetClientRect(hwnd, &rect);
					 FillRect(hDC, &rect, hBK_102);
					 SetTextColor(hDC, RGB(255, 255, 255));
					 SetBkMode(hDC, TRANSPARENT);
					 HFONT  m_hOldFont = (HFONT)SelectObject(hDC, m_hFont);

					 TCHAR WndTitle[256] = { 0 };
					 GetWindowText(hwnd, WndTitle, sizeof WndTitle);

					 String DB_Tianjia = WndTitle;
					 TextOut(hDC, 18, 5, DB_Tianjia, DB_Tianjia.Length());
					 SelectObject(hDC, m_hOldFont);
					 EndPaint(hwnd, &ps);
					 DeleteObject(hBK_153);
					 DeleteObject(hBK_102);
					 DeleteObject(hBK_57);
					 DeleteObject(m_hFont);
					 DeleteObject(m_hOldFont);
					 break;
	}

	default:
		return CallWindowProc(resetvideofilterproc, hwnd, message, wParam, lParam);
		break;
	}
	return 0;
}

//=========================================
CProgramVideoSourceConfigDialog::CProgramVideoSourceConfigDialog(VideoSourceConfig *config)
{
	this->config = config;
	
}

CProgramVideoSourceConfigDialog::~CProgramVideoSourceConfigDialog()
{
	if (m_TProgramListItemVector)
	{
		delete m_TProgramListItemVector;
		m_TProgramListItemVector = NULL;
	}
}

BOOL ProgramConfig_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	CProgramVideoSourceConfigDialog *_this = reinterpret_cast<CProgramVideoSourceConfigDialog *>(lParam);
	VideoSourceConfig *config = _this->GetConfig();
	//config->Reload();
	RECT rect;//矩形
	GetClientRect(hwnd, &rect);
	_this->m_canvas = TCanvas::CreateCanvas(0, 0, 0, 0, hwnd);
	_this->m_canvas->SetWindowPos(0, 0, rect.right, 200);

	
	String newItem(GetEditText(GetDlgItem(GetParent(hwnd), IDC_MEDIA_FILE_OR_URL)));
	
	_this->m_TProgramListItemVector = new std::vector<TProgramListItem*>;
	TCHAR       lpAppPath[MAX_PATH];
	{
		String strDirectory;
		UINT dirSize = GetCurrentDirectory(0, 0);
		strDirectory.SetLength(dirSize);
		GetCurrentDirectory(dirSize, strDirectory.Array());

		scpy(lpAppPath, strDirectory);
	}
	
	int count = config->playlistm3u8.Num();
	_this->m_canvas->SetWindowCanvasPos(0, 0, rect.right - 14, (count + 0) * 46);
	HWND CanvasWinID = _this->m_canvas->GetCanvasWinID();
	for (int index = 0; index < count; index++)
	{
		String ItemURL = config->playlistm3u8[index];
		String ItemName = config->playlistName[index];
		bool bComp = newItem.Compare(ItemURL);
		TProgramListItem* item = TProgramListItem::CreateProgramItem(0, 46 * index, rect.right-14, 46, ItemName, ItemURL, CanvasWinID, NULL, index, config);
		if (bComp)
		{
			item->SetCheckFlag(true);
		}
		_this->m_TProgramListItemVector->push_back(item);
		item->m_TProgramListItemVector = _this->m_TProgramListItemVector;
	}
	buttonproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDSENIOROK), GWLP_WNDPROC, (LONG_PTR)ButtonProc);

	return true;
}

void ProgramConfig_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	CProgramVideoSourceConfigDialog *_this = (CProgramVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	VideoSourceConfig *config = _this->GetConfig();
	switch (id)
	{

	case IDSOURCECHANNELOK:
	{
							  EndDialog(hwnd, IDOK);
							  HWND hdl = GetParent(hwnd);
							  SendMessage(hdl, UPDATAURL, NULL, TRUE);
						
							  break;
	}
	}
}

INT_PTR CALLBACK ProgramConfig_DlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	static bool _bMouseTrack = true;
	switch (msg)
	{
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwndDlg, -1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwndDlg, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwndDlg, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwndDlg;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwndDlg, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwndDlg, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwndDlg,  -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwndDlg, msg, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwndDlg, -1, true);
	}
		break;
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, ProgramConfig_OnInitDialog);
		HANDLE_MSG(hwndDlg, WM_COMMAND, ProgramConfig_OnCommand);
		HANDLE_MSG(hwndDlg, WM_PAINT, ProgramConfig_OnPaint);
		HANDLE_MSG(hwndDlg, WM_LBUTTONUP, Config_OnLButtonUp);
		HANDLE_MSG(hwndDlg, WM_MOUSEMOVE, Config_OnMouseMove);
		HANDLE_MSG(hwndDlg, WM_ACTIVATE, Config_OnActivate);
		HANDLE_MSG(hwndDlg, WM_CTLCOLORSTATIC, Config_OnStaticChangeColor);

	case WM_NOTIFY: return Config_OnNotify(hwndDlg, msg, wParam, lParam);
	case WM_CLOSE:
	{
					 EndDialog(hwndDlg, IDOK);
					 break;
	}

		HANDLE_DEFAULT;
	}
}

void ProgramConfig_OnPaint(HWND hwnd)
{
	CProgramVideoSourceConfigDialog *_this = (CProgramVideoSourceConfigDialog *)GetWindowLongPtr(hwnd, DWLP_USER);
	PAINTSTRUCT ps;
	HBRUSH hBK_153 = CreateSolidBrush(RGB(35, 35, 37));
	RECT rect;
	HDC hDC = BeginPaint(hwnd, &ps);
	GetClientRect(hwnd, &rect);
	FillRect(hDC, &rect, hBK_153);
	EndPaint(hwnd, &ps);
	DeleteObject(hBK_153);
}

//==============================================================
//==============================================================

#define SCENE_SCROLLWINDOW_CLASS TEXT("BLiveScrollWindow")
#define SCENE_CANVASWINDOW_CLASS TEXT("BLiveCanvas")

enum ScrollControlValue
{
	IDC_CANVAS = 4000,
	IDC_SCORLL,
};

LRESULT CALLBACK ScreenProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);//屏幕事件处理函数  
LRESULT CALLBACK CanvasProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);//画布事件处理函数  

#define CANVASSIZE (WM_USER + 4331)
HINSTANCE   Ghinstance = NULL;//程序实例  
WNDPROC btnSliderProc = NULL;

void InitScrollScene(HINSTANCE hInst)
{
	WNDCLASSEX wc;
	MSG Msg;

	//可滚动背景窗口
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpfnWndProc = ScreenProc;
	wc.hInstance = hInst;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(35, 35, 37));
	wc.lpszClassName = SCENE_SCROLLWINDOW_CLASS;
	wc.hIcon = LoadIcon(NULL, (LPCWSTR)IDI_APPLICATION);

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
	}

	//超大窗口
	memset(&wc, 0, sizeof(wc));
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = SCENE_CANVASWINDOW_CLASS;
	wc.lpfnWndProc = CanvasProc;
	wc.hInstance = hInst;//这里可以直接使用全局变量了  
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = CreateSolidBrush(RGB(42, 42, 44));

	if (!RegisterClassEx(&wc)) {
		MessageBox(NULL, L"Canvas Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
	}

	Ghinstance = hInst;
}

//屏幕的事件  
LRESULT CALLBACK ScreenProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

	TCanvas* item = NULL;
	//只需要处理WM_SIZE、WM_HSCROLL、WM_VSCROLL三个消息  
	switch (Message) {
	case WM_NCCREATE:
	{
						CREATESTRUCT *pCreateData = (CREATESTRUCT*)lParam;

						item = new TCanvas;
						//SetLastError(0);
						int errot = GetLastError();
						SetWindowLongPtr(hwnd, -21, (LONG_PTR)item);
						errot = GetLastError();

						item->canvas_width = pCreateData->cx;
						item->canvas_height = pCreateData->cy;

						item->screen_width = pCreateData->cx;
						item->screen_height = pCreateData->cy;
						item->Parent = GetParent(hwnd);
						item->m_hHwnd = hwnd;

						return TRUE;
	}
	case WM_DESTROY:
	{
					   //item = GetCanvasItem(hwnd, 1);
					   item = (TCanvas*)GetWindowLongPtr(hwnd, -21);
					   if (item)
					   {
						   delete item;
						   item = NULL;
					   }
					   break;
	}
	case	 CANVASSIZE:
	{
						   item = (TCanvas*)GetWindowLongPtr(hwnd, -21);
						   //画布大小改变事件
						   item->canvas_width = LOWORD(lParam);
						   item->canvas_height = HIWORD(lParam);
						   if (item->canvas_height)
						   {
							   item->m_vHeight = item->screen_height*item->screen_height / item->canvas_height;
							   if (item->m_vHeight >= item->screen_height)
							   {
								   item->m_vHeight = 0;
							   }

							   //应该根据条目数及当前条目索引调整位置
							   //SetWindowPos(item->m_btnSlider, NULL, item->screen_width - 4 - 10, item->screen_height - item->m_vHeight, 10, item->m_vHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
							   SetWindowPos(item->m_btnSlider, NULL, item->screen_width - 4 - 10, /*item->screen_height - item->m_vHeight*/0, 10, item->m_vHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
						   }
						   break;
	}
		//窗口大小改变时，更新全局变量中的屏幕大小，更新滚动条上滚动块的位置  
	case WM_SIZE: {   //当canvas的height改变是这里应该重新运算

					  ShowScrollBar(hwnd, SB_VERT, FALSE);
					  //item = GetCanvasItem(hwnd, 1);
					  item = (TCanvas*)GetWindowLongPtr(hwnd, -21);
					  //更新屏幕大小begin-----------------------------  
					  item->canvas_width = LOWORD(lParam);
					  item->screen_width = LOWORD(lParam);
					  item->screen_height = HIWORD(lParam);
					  //更新屏幕大小end-----------------------------  
					  break;
	}
	case WM_LBUTTONDOWN:
	{
						   break;
	}
	case WM_LBUTTONUP:
	{
						 break;
	}
	case WM_MOUSEMOVE:
	{
						 //item = GetCanvasItem(hwnd, 1);
						 item = (TCanvas*)GetWindowLongPtr(hwnd, -21);
						 item->MouseMove(LOWORD(lParam), HIWORD(lParam));
						 break;
	}
		//竖直方向滚动条事件，与上面相似不解释了  
	case ITEMSCROLL: {

						 item = (TCanvas*)GetWindowLongPtr(hwnd, -21);

						 int top = lParam;
						 if (top < 0)
						 {
							 SetWindowPos(item->hwnd_canvas, NULL, 0, item->screen_height - item->canvas_height, item->canvas_width, item->canvas_height, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
							 break;
						 }

						 item->position_height = top * (item->canvas_height - item->screen_height) / (item->screen_height - item->m_vHeight);
						 SetWindowPos(item->hwnd_canvas, NULL, 0, -item->position_height, item->canvas_width, item->canvas_height, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
						 break;

	}
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}

TCanvas::TCanvas()
{
	ScrollPos = 0;
}
TCanvas::~TCanvas()
{
	DestroyWindow(hwnd_canvas);
}

void TCanvas::MouseMove(int x, int y)
{
	if (y < canvas_width - 10) return;
}

TCanvas* TCanvas::CreateCanvas(int left, int top, int width, int height, HWND hwndParent)
{
	HWND hwnd = CreateWindow(SCENE_SCROLLWINDOW_CLASS, L"",
		WS_VISIBLE | WS_VSCROLL | WS_CHILDWINDOW | WS_TABSTOP | BS_TEXT | BS_PUSHBUTTON | WS_CLIPSIBLINGS,
		//WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_VSCROLL,
		left, top, width, height, hwndParent, (HMENU)IDC_SCORLL, HinstanceDLL, 0);
	SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
	int error = GetLastError();

	//TCanvas*  result = GetCanvasItem(hwnd, 1);
	TCanvas*  result = (TCanvas*)GetWindowLongPtr(hwnd, -21);

	result->Fresh();

	result->hwnd_canvas = CreateWindow(
		SCENE_CANVASWINDOW_CLASS,
		L"",
		WS_CHILD | WS_VISIBLE,
		0, 0, result->canvas_width, result->canvas_height,
		hwnd,//这里可以直接使用全局变量了，注意，如果是放屏幕的WM_CREATE里面，这时候是还不能使用这个全局变量的，WM_CREATE事件结束后CreateWindow方法才会返回创建窗口的句柄  
		(HMENU)IDC_CANVAS,
		HinstanceDLL,//这里可以直接使用全局变量了  
		0
		);

	result->m_btnSlider = CreateWindow(TEXT("BUTTON"), NULL,
		WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | WS_CLIPSIBLINGS | WS_EX_TOPMOST,
		0, 0, 0, 0, hwnd, (HMENU)ID_DLG_SLIDER, 0, 0);
	SendMessage(result->m_btnSlider, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

	btnSliderProc = (WNDPROC)GetWindowLongPtr(result->m_btnSlider, GWLP_WNDPROC);
	SetWindowLongPtr(result->m_btnSlider, GWLP_WNDPROC, (LONG_PTR)BtnSliderProc);

	return result;
}
void TCanvas::Fresh()
{

}
void TCanvas::SetWindowPos(int left, int top, int width, int height)
{
	::SetWindowPos(m_hHwnd, NULL, left, top, width, height, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}
void TCanvas::SetWindowCanvasPos(int left, int top, int width, int height)
{
	::SetWindowPos(hwnd_canvas, NULL, left, top, width, height, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

void TCanvas::SetVisiable(bool v)
{
	::ShowWindow(m_hHwnd, v ? SW_SHOW : SW_HIDE);
	::ShowWindow(hwnd_canvas, v ? SW_SHOW : SW_HIDE);
	::ShowWindow(m_btnSlider, v ? SW_SHOW : SW_HIDE);
}

void TCanvas::SetScroll(int delta)
{
	RECT ButtonRect;
	GetClientRect(m_btnSlider, &ButtonRect);

	RECT BlackRect;
	GetClientRect(m_hHwnd, &BlackRect);

	RECT CanvasRect;
	GetClientRect(hwnd_canvas, &CanvasRect);

	if (CanvasRect.bottom <= BlackRect.bottom)
	{
		return;
	}

	ScrollPos += delta;

	if (ScrollPos < 0)
	{
		ScrollPos = 0;
	}
	else if (ScrollPos + ButtonRect.bottom + delta > screen_height)
	{
		ScrollPos = screen_height - ButtonRect.bottom;
	}

	int xPos = screen_width - 4 - 10;
	//button位置
	::SetWindowPos(m_btnSlider, NULL, xPos, ScrollPos, 10, m_vHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
	SendMessage(m_hHwnd, ITEMSCROLL, 0, ScrollPos);
}

LRESULT CALLBACK BtnSliderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static POINTS LBtnPt;
	static bool IsCapture = false;
	if (WM_PAINT == message)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		HBRUSH HBrush = CreateSolidBrush(RGB(79, 79, 83));

		//HBRUSH HBrush = CreateSolidBrush(RGB(255, 255, 255));
		FillRect(hdc, &Rect, HBrush);
		DeleteObject(HBrush);

		EndPaint(hwnd, &ps);
	}
	else if (WM_LBUTTONDOWN == message)
	{
		SetCapture(hwnd);
		IsCapture = true;
		LBtnPt = MAKEPOINTS(lParam);
	}
	else if (WM_MOUSEMOVE == message && IsCapture)
	{
		POINTS pt = MAKEPOINTS(lParam);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		TCanvas* item = (TCanvas*)GetWindowLongPtr(GetParent(hwnd), -21);
		static int oldTop = 0;

		int offsetY = pt.y - LBtnPt.y;
		GetWindowRect(hwnd, &Rect);
		RECT rtWnd;
		GetWindowRect(GetParent(hwnd), &rtWnd);
		Rect.top -= rtWnd.top;
		Rect.bottom -= rtWnd.top;
		Rect.top += offsetY;
		Rect.bottom += offsetY;

		if (oldTop != Rect.top)
		{
			oldTop = Rect.top;

			if (Rect.top <= 0 || Rect.bottom >= item->screen_height)
			{
				//	//超边界处理
				if (Rect.top <= 0)
				{
					Rect.top = 0;
					SendMessage(item->m_hHwnd, ITEMSCROLL, 0, 0);
				}
				else
				{
					Rect.top = item->screen_height;
					Rect.bottom = item->screen_height;
					SendMessage(item->m_hHwnd, ITEMSCROLL, 0, -1);
				}
			}
			else
			{
				int xPos = item->screen_width - 4 - 10;
				SetWindowPos(hwnd, NULL, xPos, Rect.top, 10, item->m_vHeight, SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
				SendMessage(item->m_hHwnd, ITEMSCROLL, 0, Rect.top);
			}

			item->ScrollPos = Rect.top;
		}
		//::ReleaseDC(hwnd, hdc);
	}
	else if (WM_LBUTTONUP == message)
	{
		ReleaseCapture();
		IsCapture = false;
	}

	return CallWindowProc(btnSliderProc, hwnd, message, wParam, lParam);
}


//窗口的事件  
LRESULT CALLBACK CanvasProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) {

	switch (Message) {
	case WM_SIZE:
	{
					HWND parent = GetParent(hwnd);
					SendMessage(parent, CANVASSIZE, 0, lParam);

					break;
	}
	case WM_DESTROY: {
						 break;
	}

	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
}


//==============================================================
//==============================================================

//=========================================
//=========================================

bool DrawImagePngEx(HDC DC, Gdiplus::Image* pImage, INT nXPos, INT nYPos, INT Width, INT Height)
{
	//加载判断
	if (pImage == NULL) return false;

	//创建屏幕
	Graphics graphics(DC);

	//获取属性
	INT nImageWidth = pImage->GetWidth();
	INT nImageHeight = pImage->GetHeight();

	//构造位置
	RectF rcDrawRect;
	rcDrawRect.X = (REAL)nXPos;
	rcDrawRect.Y = (REAL)nYPos;
	rcDrawRect.Width = (REAL)Width;
	rcDrawRect.Height = (REAL)Height;

	//绘画图像
	graphics.DrawImage(pImage, rcDrawRect, 0, 0, (REAL)nImageWidth, (REAL)nImageHeight, UnitPixel);
	return true;
}

void InitProgramListEx(HINSTANCE hInst)
{
	WNDCLASS wnd;

	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = sizeof(LPVOID);
	wnd.hbrBackground = NULL;
	wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
	wnd.hIcon = NULL;
	wnd.hInstance = hInst;
	wnd.lpfnWndProc = ProgramProcEx;
	wnd.lpszClassName = PROGRAM_CLASS;
	wnd.lpszMenuName = NULL;
	wnd.style = CS_PARENTDC | CS_VREDRAW | CS_HREDRAW;

	if (!RegisterClass(&wnd))
		CrashError(TEXT("Could not register ProgramList class"));
	InitScrollScene(hInst);
}

TProgramListItem* TProgramListItem::CreateProgramItem(int left, int top, int width, int height, String title, String URL, HWND hwndParent, HMENU menu, int tag, VideoSourceConfig *config)
{
	HWND hwndTmp = CreateWindow(PROGRAM_CLASS, title,
		WS_CHILDWINDOW | WS_VISIBLE | WS_TABSTOP | BS_TEXT | BS_PUSHBUTTON | WS_CLIPSIBLINGS,
		left, top, width, height, hwndParent, menu, 0, 0);
	SendMessage(hwndTmp, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

	TProgramListItem* item = (TProgramListItem*)GetWindowLongPtr(hwndTmp, -21);
	item->Title = title;
	item->mTag = tag;
	item->m_config = config;
	item->m_URL = URL;

	return item;
}

void TProgramListItem::Desory()
{
	::DestroyWindow(m_hWinID);
}

TProgramListItem::TProgramListItem() :m_check(false)
{
	pRadioSelectImage = Gdiplus::Image::FromFile(L".\\img\\option__select.png");
	pRadioImage = Gdiplus::Image::FromFile(L".\\img\\option__hover.png");
}

TProgramListItem::~TProgramListItem()
{
	if (pRadioSelectImage)
	{
		delete pRadioSelectImage;
		pRadioSelectImage = NULL;
	}

	if (pRadioImage)
	{
		delete pRadioImage;
		pRadioImage = NULL;
	}
}

void TProgramListItem::Fresh()
{
	HDC hDC = GetDC(m_hWinID);

	HDC hdcTemp = CreateCompatibleDC(hDC);
	HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, m_iWidth, m_iHeight);
	HBITMAP oldHbmp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);

	//刷底色
	COLORREF color = RGB(44, 44, 44);
	HBRUSH hGray = CreateSolidBrush(color);
	RECT clientRect = { 0, 0, m_iWidth, m_iHeight };
	FillRect(hdcTemp, &clientRect, hGray);

	DrawImagePngEx(hdcTemp, pRadioImage, m_iWidth / 10, (m_iHeight - 32) / 2, 32, 32);
	if (m_check)
	{
		DrawImagePngEx(hdcTemp, pRadioSelectImage, m_iWidth / 10, (m_iHeight - 32) / 2, 32, 32);
		m_config->playlist.Clear();
		m_config->playlist.Add(m_URL);
	}

	//输出文字
	HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	HFONT hfontOld = (HFONT)SelectObject(hdcTemp, hFont);

	SetTextColor(hdcTemp, RGB(255, 255, 255));
	SetBkMode(hdcTemp, TRANSPARENT);
	String volText = Title;
	if (volText.Length())
	{
		TextOut(hdcTemp, m_iWidth / 10 + 40, m_iHeight / 2 - 10, volText, slen(volText));
	}
	SelectObject(hdcTemp, hfontOld);
	DeleteObject(hFont);

	BitBlt(hDC, 0, 0, m_iWidth, m_iHeight, hdcTemp, 0, 0, SRCCOPY);

	SelectObject(hdcTemp, oldHbmp);
	DeleteObject(hbmpTemp);
	DeleteObject(hdcTemp);
	DeleteObject(hGray);
	ReleaseDC(m_hWinID, hDC);
}

int TProgramListItem::OnFrameCreate()
{
	Fresh();
	return 0;
}

void TProgramListItem::OnSelected(HWND hwnd)
{
	POINT point;
	GetCursorPos(&point);
	ScreenToClient(m_hWinID, &point);
	if (point.x < 0 || point.y < 0 || point.x > m_iWidth || point.y > m_iHeight)
	{
		return;
	}

	bool enter = (point.x >= m_iWidth / 10 - 10) && (point.x < m_iWidth / 10 + 32 - 10);
	enter = enter && (point.y >= (m_iHeight - 32) / 2) && (point.y < (m_iHeight - 32) / 2 + 32);

	if (enter)
	{
		SetCheckFlag(true);
		Fresh();

		SendMessage(GetParent(GetParent(GetParent(hwnd))), ITEMCHECK, m_check, (LPARAM)mTag);
	}
}

LRESULT CALLBACK ProgramProcEx(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	TProgramListItem *item;

	switch (message)
	{
	case WM_NCCREATE:
	{
						CREATESTRUCT *pCreateData = (CREATESTRUCT*)lParam;

						item = new TProgramListItem;
						SetWindowLongPtr(hwnd, -21, (LONG_PTR)item);

						item->m_iWidth = pCreateData->cx;
						item->m_iHeight = pCreateData->cy;
						item->m_hParent = GetParent(hwnd);
						item->m_hWinID = hwnd;

						return TRUE;
	}
	case WM_DESTROY:
	{
					   //item = GetSceneItemData(hwnd);
					   item = (TProgramListItem*)GetWindowLongPtr(hwnd, -21);
					   if (item)
					   {
						   delete item;
						   item = NULL;
					   }
					   break;
	}
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hDC = BeginPaint(hwnd, &ps);
					 //item = GetSceneItemData(hwnd);
					 item = (TProgramListItem*)GetWindowLongPtr(hwnd, -21);
					 item->Fresh();
					 EndPaint(hwnd, &ps);
					 break;
	}
	case WM_LBUTTONDOWN:
	{
						   //item = GetSceneItemData(hwnd);
						   item = (TProgramListItem*)GetWindowLongPtr(hwnd, -21);
						   item->UnSelectedOthers();
						   item->OnSelected(hwnd);

						   break;
	}
	case WM_COMMAND:
	{

	}
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

void TProgramListItem::UnSelectedOthers()
{
	std::vector <TProgramListItem*>::iterator iter = m_TProgramListItemVector->begin();
	for (; iter != m_TProgramListItemVector->end(); iter++)
	{
		if ((*iter) != this)
		{
			HDC hDC = GetDC((*iter)->m_hWinID);
			(*iter)->SetCheckFlag(false);
			HDC hdcTemp = CreateCompatibleDC(hDC);
			HBITMAP hbmpTemp = CreateCompatibleBitmap(hDC, m_iWidth, m_iHeight);
			HBITMAP oldHbmp = (HBITMAP)SelectObject(hdcTemp, hbmpTemp);

			//刷底色
			COLORREF color = RGB(44, 44, 44);
			HBRUSH hGray = CreateSolidBrush(color);
			RECT clientRect = { 0, 0, m_iWidth, m_iHeight };
			FillRect(hdcTemp, &clientRect, hGray);

			DrawImagePngEx(hdcTemp, pRadioImage, m_iWidth / 10, (m_iHeight - 32) / 2, 32, 32);

			//输出文字
			HFONT hFont = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
			HFONT hfontOld = (HFONT)SelectObject(hdcTemp, hFont);

			SetTextColor(hdcTemp, RGB(255, 255, 255));
			SetBkMode(hdcTemp, TRANSPARENT);
			String volText = (*iter)->Title;
			if (volText.Length())
			{
				TextOut(hdcTemp, m_iWidth / 10 + 40, m_iHeight / 2 - 10, volText, slen(volText));
			}
			SelectObject(hdcTemp, hfontOld);
			DeleteObject(hFont);

			BitBlt(hDC, 0, 0, m_iWidth, m_iHeight, hdcTemp, 0, 0, SRCCOPY);

			SelectObject(hdcTemp, oldHbmp);
			DeleteObject(hbmpTemp);
			DeleteObject(hdcTemp);
			DeleteObject(hGray);
			ReleaseDC((*iter)->m_hWinID, hDC);
		}
	}
}
