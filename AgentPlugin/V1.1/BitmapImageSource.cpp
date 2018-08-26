#include "BitmapImageSource.h"
#include <sstream>

WNDPROC BrowerBtnProc = NULL;
WNDPROC ResetBtnProc = NULL;
WNDPROC QDBtnProc = NULL;
WNDPROC QXBtnProc = NULL;
static HWND Hwnd = NULL;

extern HINSTANCE hinstMain;
extern LocaleStringLookup *pluginLocale;

IMPLEMENT_DYNIC(BitmapImageSource, "图片源", "V1.0.0.1")

BitmapImageSource::BitmapImageSource()
{
	colorKeyShader = NULL;
	alphaIgnoreShader = NULL;
	bPreProcess = false;
	D3DRender = GetD3DRender();
}

BitmapImageSource::~BitmapImageSource()
{
	if (colorKeyShader)
		delete colorKeyShader;
	if (alphaIgnoreShader)
		delete alphaIgnoreShader;
}

bool BitmapImageSource::Init(Value &JsonParam)
{
	colorKeyShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders\\ColorKey_RGB.pShader"));
	alphaIgnoreShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders\\AlphaIgnore.pShader"));

	UpdateSettings(JsonParam);
	Log::writeMessage(LOG_RTSPSERV,1,"Using bitmap image");
	return true;
}

void BitmapImageSource::Tick(float fSeconds)
{
	bitmapImage.Tick(fSeconds);
}

void BitmapImageSource::Render(const Vect2 &pos, const Vect2 &size, Texture* Text, bool bScaleFull, bool bIsLiveC)
{
	Texture *texture = bitmapImage.GetTexture();
	bScaleFull = true;
	if (texture)
	{
		if (bScaleFull)
		{
			DWORD alpha = ((opacity * 255 / 100) & 0xFF);
			DWORD outputColor = (alpha << 24) | color & 0xFFFFFF;
			if (Text)
			{
				D3DRender->DrawSprite(Text, outputColor, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
			}
			else
			{
				if (bUseColorKey)
				{
					Shader *lastPixelShader = D3DRender->GetCurrentPixelShader();
					D3DRender->LoadPixelShader(colorKeyShader);

					float fSimilarity = float(keySimilarity)*0.01f;
					float fBlend = float(keyBlend)*0.01f;

					colorKeyShader->SetColor(colorKeyShader->GetParameter(2), keyColor);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(3), fSimilarity);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(4), fBlend);

					D3DRender->DrawSprite(texture, outputColor, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
					D3DRender->LoadPixelShader(lastPixelShader);
				}
				else
				{
					D3DRender->DrawSprite(texture, outputColor, pos.x, pos.y, pos.x + size.x, pos.y + size.y);
				}
			}
			
		}
		else
		{
			//等比例缩放
			float aspect = (float)size.x / size.y;

			float x, x2;
			float y, y2;

			if (aspect > 1.0f)
			{
				y = pos.y;
				y2 = y + size.y;

				float NewWidth = Width * size.y / Height;

				x = pos.x + (size.x - NewWidth) / 2;
				x2 = x + NewWidth;

				if (NewWidth > size.x)
				{
					x = pos.x;
					x2 = pos.x + size.x;

					float NewHeight = Height * size.x / Width;

					y = pos.y + (size.y - NewHeight) / 2;
					y2 = y + NewHeight;
				}
			}
			else
			{
				x = pos.x;
				x2 = pos.x + size.x;

				float NewHeight = Height * size.x / Width;

				y = pos.y + (size.y - NewHeight) / 2;
				y2 = y + NewHeight;

				if (NewHeight > size.y)
				{
					y = pos.y;
					y2 = y + size.y;

					float NewWidth = Width * size.y / Height;

					x = pos.x + (size.x - NewWidth) / 2;
					x2 = x + NewWidth;
				}
			}

			DWORD alpha = ((opacity * 255 / 100) & 0xFF);
			DWORD outputColor = (alpha << 24) | color & 0xFFFFFF;
			if (Text)
			{
				D3DRender->DrawSprite(Text, outputColor, x, y, x2, y2);
			}
			else
			{
				if (bUseColorKey)
				{
					Shader *lastPixelShader = D3DRender->GetCurrentPixelShader();
					D3DRender->LoadPixelShader(colorKeyShader);

					float fSimilarity = float(keySimilarity)*0.01f;
					float fBlend = float(keyBlend)*0.01f;

					colorKeyShader->SetColor(colorKeyShader->GetParameter(2), keyColor);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(3), fSimilarity);
					colorKeyShader->SetFloat(colorKeyShader->GetParameter(4), fBlend);

					D3DRender->DrawSprite(texture, outputColor, x, y, x2, y2);
					D3DRender->LoadPixelShader(lastPixelShader);
				}
				else
				{
					D3DRender->DrawSprite(texture, outputColor, x, y, x2, y2);
				}
			}
		}
	}
}

void BitmapImageSource::UpdateSettings(Value &Data)
{
	this->data = Data;
	if (data["path"].isNull())
	{
		return;
	}
	String path = Asic2WChar(data["path"].asString().c_str()).c_str();
// 	if (!OSFileExists(path)) // 文件不存在
// 	{
// 		Log::writeError(LOG_RTSPSERV, 1, "%s 不存在", data["path"].asCString());
// 		return;
// 	}

	bitmapImage.SetPath(path);
	bitmapImage.EnableFileMonitor(data["monitor"].asInt() == 1);
	bitmapImage.Init();

	//------------------------------------

	opacity = 100;
	if (!data["opacity"].isNull())
	{
		opacity = data["opacity"].asInt();
	}
	color = 0xFFFFFFFF;

	if (!data["color"].isNull())
	{
		color = data["color"].asUInt();
	}
	if (opacity > 100)
		opacity = 100;

	bool bNewUseColorKey = data["useColorKey"].asInt() != 0;
	keyColor = 0xFFFFFFFF;
	if (!data["keyColor"].isNull())
	{
		keyColor = data["keyColor"].asUInt();
	}
	keySimilarity = 10;
	if (!data["keySimilarity"].isNull())
		keySimilarity = data["keySimilarity"].asInt();

	keyBlend = data["keyBlend"].asInt();

	bUseColorKey = bNewUseColorKey;


	Width = bitmapImage.GetSize().x;
	Height = bitmapImage.GetSize().y;
}

void BitmapImageSource::SetInt(CTSTR lpName, int iValue)
{
	if (String("opacity").Compare(lpName))
	{
		opacity = iValue;
		if (opacity > 100)
			opacity = 100;
	}
}

void BitmapImageSource::SetHasPreProcess(bool bHasPre)
{
	bPreProcess = bHasPre;
}

bool BitmapImageSource::GetHasPreProcess() const
{
	return bPreProcess;
}


struct ConfigBitmapInfo
{
    Value& data;
	ConfigBitmapInfo(Value &Data) :data(Data){}
};


INT_PTR CALLBACK BitMapButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (WM_PAINT == message)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		HBRUSH HBrush = CreateSolidBrush(RGB(102, 102, 102));

		FillRect(hdc, &Rect, HBrush);
		DeleteObject(HBrush);

		if (IsWindowEnabled(hwnd))
		{
			SetTextColor(hdc, RGB(255, 255, 255));
		}
		else
		{
			SetTextColor(hdc, RGB(153, 153, 153));
		}
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
	}


	WNDPROC WndProcTem = NULL;
	if (hwnd == GetDlgItem(Hwnd, IDOK))
	{
		WndProcTem = QDBtnProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDC_BROWSE))
	{
		WndProcTem = BrowerBtnProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = QXBtnProc;
	}
	else
	{
		WndProcTem = ResetBtnProc;
	}
	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigureBitmapProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool bSelectingColor = false;
    static bool bMouseDown = false;
    static ColorSelectionData colorData;
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	Hwnd = hwnd;
    switch(message)
    {
	case WM_NCACTIVATE:
	{
						  DrawFrame(hwnd, -1, true);
						  if (wParam == 0)
							  return FALSE;
						  return TRUE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwnd, -1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwnd, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwnd, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT Rect;
					 GetClientRect(hwnd, &Rect);

					 HBRUSH HBrush = CreateSolidBrush(RGB(153, 153, 153));

					 FillRect(hdc, &Rect, HBrush);
					 DeleteObject(HBrush);

					 EndPaint(hwnd, &ps);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwnd;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwnd, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwnd, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwnd,-1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwnd, -1, true);
	}
		break;

	case WM_CTLCOLORSTATIC:
	{
							  HDC hdc = (HDC)wParam;
							  SetTextColor(hdc, RGB(255, 255, 255));
							  SetBkColor(hdc, RGB(153, 153, 153));

							  if (HBrush)
							  {
								  DeleteObject(HBrush);
							  }
							  HBrush = CreateSolidBrush(RGB(153, 153, 153));

							  return (LRESULT)HBrush;
	}
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	{
							   HDC hdc = (HDC)wParam;
							   SetTextColor(hdc, RGB(255, 255, 255));
							   SetBkColor(hdc, RGB(121, 121, 121));

							   if (HBrush)
							   {
								   DeleteObject(HBrush);
							   }
							   HBrush = CreateSolidBrush(RGB(121, 121, 121));
							   return (LRESULT)HBrush;
	}
	case WM_DESTROY:
	{
					   if (HBrush)
					   {
						   DeleteObject(HBrush);
						   HBrush = NULL;
					   }
	}
		break;
        case WM_INITDIALOG:
            {
							  BrowerBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_BROWSE), GWLP_WNDPROC);
							  ResetBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_DEFUALT), GWLP_WNDPROC);
							  QDBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
							  QXBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDC_BROWSE), GWLP_WNDPROC, (LONG_PTR)BitMapButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDC_DEFUALT), GWLP_WNDPROC, (LONG_PTR)BitMapButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)BitMapButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)BitMapButtonProc);

                ConfigBitmapInfo *configInfo = (ConfigBitmapInfo*)lParam;
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
				LocalizeWindow(hwnd,pluginLocale);

                //--------------------------

				String lpBitmap = L"";
				if (!configInfo->data["path"].isNull())
					lpBitmap = Asic2WChar(configInfo->data["path"].asString().c_str()).c_str();

                SetWindowText(GetDlgItem(hwnd, IDC_BITMAP), lpBitmap.Array());

                //--------------------------

				int opacity = 100;
				if (!configInfo->data["opacity"].isNull())
					opacity = configInfo->data["opacity"].asInt();

                if(opacity > 100)
                    opacity = 100;
                else if(opacity < 0)
                    opacity = 0;

                SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_SETPOS32, 0, opacity);

				HWND hwndTemp = GetDlgItem(hwnd, IDC_NO_BRIGHTNESS);
				SendMessage(hwndTemp, TBM_CLEARTICS, FALSE, 0);
				SendMessage(hwndTemp, TBM_SETRANGE, FALSE, MAKELPARAM(0, 100));
				SendMessage(hwndTemp, TBM_SETTIC, 0, 100);
				SendMessage(hwndTemp, TBM_SETPOS, TRUE, opacity);

				SetWindowText(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS_VALUE), FormattedString(TEXT("%d"), opacity));

                //--------------------------

				DWORD color = 0xFFFFFFFF;

				if (!configInfo->data["color"].isNull())
				{
					color = configInfo->data["color"].asUInt();
				}
				
				DWORD colorkey = 0xFFFFFFFF;

				if (!configInfo->data["keyColor"].isNull())
					colorkey = configInfo->data["keyColor"].asUInt();

				UINT  similarity = 10;
				if (!configInfo->data["keySimilarity"].isNull())
					similarity = configInfo->data["keySimilarity"].asInt();

                UINT  blend         = configInfo->data["keyBlend"].asInt();

                CCSetColor(GetDlgItem(hwnd, IDC_COLOR), color);
                CCSetColor(GetDlgItem(hwnd, IDC_KEYCOLOR), colorkey);

                SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_SETPOS32, 0, similarity);

                SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_SETPOS32, 0, blend);

                //--------------------------

                int monitor = configInfo->data["monitor"].asInt();
                SendMessage(GetDlgItem(hwnd, IDC_MONITOR), BM_SETCHECK, monitor ? BST_CHECKED : BST_UNCHECKED, 0);
                int colorkeyChk = configInfo->data["useColorKey"].asInt();
                SendMessage(GetDlgItem(hwnd, IDC_USECOLORKEY), BM_SETCHECK, colorkeyChk ? BST_CHECKED : BST_UNCHECKED, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_KEYCOLOR), colorkeyChk);
                EnableWindow(GetDlgItem(hwnd, IDC_SELECT), colorkeyChk);
                EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT), colorkeyChk);
                EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD), colorkeyChk);
                EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT), colorkeyChk);
                EnableWindow(GetDlgItem(hwnd, IDC_BLEND), colorkeyChk);

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

                return TRUE;
            }

        case WM_LBUTTONDOWN:
            if(bSelectingColor)
            {
                bMouseDown = true;
                CCSetColor(GetDlgItem(hwnd, IDC_KEYCOLOR), colorData.GetColor());
                ConfigureBitmapProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_KEYCOLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_KEYCOLOR));
            }
            break;

        case WM_MOUSEMOVE:
            if(bSelectingColor && bMouseDown)
            {
                CCSetColor(GetDlgItem(hwnd, IDC_KEYCOLOR), colorData.GetColor());
                ConfigureBitmapProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_KEYCOLOR, CCN_CHANGED), (LPARAM)GetDlgItem(hwnd, IDC_KEYCOLOR));
            }
            break;

        case WM_LBUTTONUP:
            if(bSelectingColor)
            {
                colorData.Clear();
                ReleaseCapture();
                bMouseDown = false;
                bSelectingColor = false;

				ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

				std::stringstream SourceId;
				uint64_t VideoId = 0;
				if (!configData->data["SourceID"].isNull())
				{
					SourceId << configData->data["SourceID"].asString().c_str();
					SourceId >> VideoId;
				}

				IBaseVideo *Video = (IBaseVideo*)VideoId;

				if (Video)
					Video->SetInt(TEXT("useColorKey"), true);
            }
            break;

		case WM_HSCROLL:
		{
						   if (GetDlgCtrlID((HWND)lParam) == IDC_NO_BRIGHTNESS)
						   {
							   int sliderVal = (int)SendMessage(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS), TBM_GETPOS, 0, 0);
							   SetWindowText(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS_VALUE), FormattedString(TEXT("%d"), sliderVal));
							   ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

							   std::stringstream SourceId;
							   uint64_t VideoId = 0;
							   if (!configData->data["SourceID"].isNull())
							   {
								   SourceId << configData->data["SourceID"].asString().c_str();
								   SourceId >> VideoId;
							   }

							   IBaseVideo *Video = (IBaseVideo*)VideoId;

							   if (Video)
								   Video->SetInt(TEXT("opacity"), sliderVal);
						   }
		}
			break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_BROWSE:
                    {
                        TCHAR lpFile[MAX_PATH+1];
                        zero(lpFile, sizeof(lpFile));

                        OPENFILENAME ofn;
                        zero(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.lpstrFile = lpFile;
                        ofn.hwndOwner = hwnd;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.lpstrFilter = TEXT("All Formats (*.bmp;*.dds;*.jpg;*.png;*.gif)\0*.bmp;*.dds;*.jpg;*.png;*.gif\0");
                        ofn.nFilterIndex = 1;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                        TCHAR curDirectory[MAX_PATH+1];
                        GetCurrentDirectory(MAX_PATH, curDirectory);

                        BOOL bOpenFile = GetOpenFileName(&ofn);
                        SetCurrentDirectory(curDirectory);

                        if(bOpenFile)
                            SetWindowText(GetDlgItem(hwnd, IDC_BITMAP), lpFile);

                        break;
                    }
				case IDC_DEFUALT:
				{
									SendMessage(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS), TBM_SETPOS, TRUE, 100);
									SetWindowText(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS_VALUE), FormattedString(TEXT("%d"), 100));
									ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

									std::stringstream SourceId;
									uint64_t VideoId = 0;
									if (!configData->data["SourceID"].isNull())
									{
										SourceId << configData->data["SourceID"].asString().c_str();
										SourceId >> VideoId;
									}

									IBaseVideo *Video = (IBaseVideo*)VideoId;
									if (Video)
										Video->SetInt(TEXT("opacity"), 100);
				}
					break;
                case IDC_USECOLORKEY:
                    {
                        HWND hwndUseColorKey = (HWND)lParam;
                        BOOL bUseColorKey = SendMessage(hwndUseColorKey, BM_GETCHECK, 0, 0) == BST_CHECKED;

						ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["SourceID"].isNull())
						{
							SourceId << configData->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;
						if (Video)
							Video->SetInt(TEXT("useColorKey"), bUseColorKey);

                        EnableWindow(GetDlgItem(hwnd, IDC_KEYCOLOR), bUseColorKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_SELECT), bUseColorKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD_EDIT), bUseColorKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BASETHRESHOLD), bUseColorKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BLEND_EDIT), bUseColorKey);
                        EnableWindow(GetDlgItem(hwnd, IDC_BLEND), bUseColorKey);
                        break;
                    }

                case IDC_KEYCOLOR:
                    {
						ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configData->data["SourceID"].isNull())
						{
							SourceId << configData->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *Video = (IBaseVideo*)VideoId;

						if (Video)
                        {
                            DWORD color = CCGetColor((HWND)lParam);
							Video->SetInt(TEXT("keyColor"), color);
                        }
                        break;
                    }

                case IDC_SELECT:
                    {
                        if(!bSelectingColor)
                        {
                            if(colorData.Init())
                            {
                                bMouseDown = false;
                                bSelectingColor = true;
                                SetCapture(hwnd);
//                                 HCURSOR hCursor = (HCURSOR)LoadImage(hinstMain, MAKEINTRESOURCE(IDC_COLORPICKER), IMAGE_CURSOR, 32, 32, 0);
//                                 SetCursor(hCursor);

								ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
								std::stringstream SourceId;
								uint64_t VideoId = 0;
								if (!configData->data["SourceID"].isNull())
								{
									SourceId << configData->data["SourceID"].asString().c_str();
									SourceId >> VideoId;
								}

								IBaseVideo *Video = (IBaseVideo*)VideoId;
								if (Video)
									Video->SetInt(TEXT("useColorKey"), false);
                            }
                            else
                                colorData.Clear();
                        }
                        break;
                    }
                    break;

                case IDC_OPACITY_EDIT:
                case IDC_BASETHRESHOLD_EDIT:
                case IDC_BLEND_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE)
                    {
						ConfigBitmapInfo *configData = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(configData)
                        {
							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configData->data["SourceID"].isNull())
							{
								SourceId << configData->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *Video = (IBaseVideo*)VideoId;

							if (Video)
                            {
                                HWND hwndVal = NULL;
                                switch(LOWORD(wParam))
                                {
                                    case IDC_BASETHRESHOLD_EDIT:    hwndVal = GetDlgItem(hwnd, IDC_BASETHRESHOLD); break;
                                    case IDC_BLEND_EDIT:            hwndVal = GetDlgItem(hwnd, IDC_BLEND); break;
                                    case IDC_OPACITY_EDIT:          hwndVal = GetDlgItem(hwnd, IDC_OPACITY); break;
                                }

                                int val = (int)SendMessage(hwndVal, UDM_GETPOS32, 0, 0);
                                switch(LOWORD(wParam))
                                {
								case IDC_BASETHRESHOLD_EDIT:    Video->SetInt(TEXT("keySimilarity"), val); break;
								case IDC_BLEND_EDIT:            Video->SetInt(TEXT("keyBlend"), val); break;
								case IDC_OPACITY_EDIT:          Video->SetInt(TEXT("opacity"), val); break;
                                }
                            }
                        }
                    }
                    break;

                case IDOK:
                    {
                        String strBitmap = GetEditText(GetDlgItem(hwnd, IDC_BITMAP));
                        if(strBitmap.IsEmpty())
                        {
							BLiveMessageBox(hwnd, pluginLocale->LookupString(L"Sources.BitmapSource.Empty"), NULL, 0);
                            break;
                        }

                        ConfigBitmapInfo *configInfo = (ConfigBitmapInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        configInfo->data["path"] = WcharToAnsi(strBitmap.Array()).c_str();

                        BOOL bFailed = FALSE;
                        //int opacity = (int)SendMessage(GetDlgItem(hwnd, IDC_OPACITY), UDM_GETPOS32, 0, (LPARAM)&bFailed);
						int opacity = (int)SendMessage(GetDlgItem(hwnd, IDC_NO_BRIGHTNESS), TBM_GETPOS, 0, 0);
                        if(opacity > 100)
                            opacity = 100;
                        else if(opacity < 0)
                            opacity = 0;
                        configInfo->data["opacity"] =  bFailed ? 100 : opacity;

                        DWORD color = CCGetColor(GetDlgItem(hwnd, IDC_COLOR));
                        configInfo->data["color"] = (UINT)color;

                        BOOL  bUseColorKey  = SendMessage(GetDlgItem(hwnd, IDC_USECOLORKEY), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        DWORD keyColor      = CCGetColor(GetDlgItem(hwnd, IDC_KEYCOLOR));
                        UINT  keySimilarity = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BASETHRESHOLD), UDM_GETPOS32, 0, 0);
                        UINT  keyBlend      = (UINT)SendMessage(GetDlgItem(hwnd, IDC_BLEND), UDM_GETPOS32, 0, 0);

                        configInfo->data["useColorKey"] = bUseColorKey;
                        configInfo->data["keyColor"] = (UINT)keyColor;
                        configInfo->data["keySimilarity"] = keySimilarity;
                        configInfo->data["keyBlend"] = keyBlend;
                        int monitor = (int)SendMessage(GetDlgItem(hwnd, IDC_MONITOR), BM_GETCHECK, 0, 0);

                        if (monitor == BST_CHECKED)
                            configInfo->data["monitor"] = 1;
                        else
                            configInfo->data["monitor"] = 0;
                    }

                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    break;
            }
            break;
    }

    return 0;
}

static bool bFirst = true;
bool STDCALL ConfigureBitmapSource(Value& element, bool bCreating)
{
	if (bFirst)
	{
		bFirst = false;
		InitColorControl(hinstMain);

		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(AGENTLOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AGENTLOCALPATH).c_str());
		}
	}
    ConfigBitmapInfo configInfo(element);

    if(BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIGUREBITMAPSOURCE), GetMainWindow(), ConfigureBitmapProc, (LPARAM)&configInfo) == IDOK)
    {
        return true;
    }

    return false;
}

REGINST_CONFIGFUNC(BitmapImageSource, ConfigureBitmapSource)
