#include"AgentPlugin.h"
#include <sstream>

const int Width = 320;
const int Hight = 240;

WNDPROC OKBtnProc = NULL;
WNDPROC CancelBtnProc = NULL;
static HWND G_Hwnd = NULL;
extern HINSTANCE hinstMain;
extern LocaleStringLookup *pluginLocale;

IMPLEMENT_DYNIC(AgentSource, "区域占位源", "V1.0.0.1")

AgentSource::AgentSource()
{
	globalSource = NULL;
	bPreProcess = false;

	HLock = NULL;
	bCanEnter = true;
	bRunning = false;
}


bool AgentSource::Init(Value &data)
{
		Image.SetPath(String(".\\img\\VideoArea.png"));
		Image.Init();
		AgentList.Add(this);
		HLock = OSCreateMutex();
		UpdateSettings(data);
		return true;
}

void AgentSource::Render(const Vect2 &pos, const Vect2 &size, Texture *texture, bool bScaleFull, bool bIsLiveC)
{
		OSEnterMutex(HLock);
		if (globalSource) 
			globalSource->Render(pos, size, texture,bScaleFull);
		OSLeaveMutex(HLock);
}

Vect2 AgentSource::GetSize() const
{
		float cx = data["cx"].asInt();
		float cy = data["cy"].asInt();
		Vect2 Value(cx, cy);

		OSEnterMutex(HLock);
		if (globalSource)
			Value = globalSource->GetSize();
		OSLeaveMutex(HLock);

		return Value;
}

void AgentSource::UpdateSettings(Value &data)
{
	this->data = data;
	IBaseVideo *OldSource = globalSource;
    String strName;
	if (!data["SecName"].isNull())
	{
		strName = Asic2WChar(data["SecName"].asString().c_str()).c_str();
	}
    String strNameDisable = L"空区域";
	if (strName.IsValid() && (strNameDisable != strName))
	{

		Value NameList;
		if (GetNameList(NameList))
		{
			Json::Value &ArryList = NameList["Namelist"];

			for (int i = 0; i < ArryList.size(); ++i)
			{
				Value &OneVaule = ArryList[i];

				if (0 == strcmp(WcharToAnsi(strName.Array()).c_str(), OneVaule["Name"].asString().c_str()))
				{
					std::stringstream SourceId;
					uint64_t VideoId = 0;
					if (!OneVaule["SourceID"].isNull())
					{
						if (!OneVaule["DeviceSourceID"].isNull())
						{
							SourceId << OneVaule["DeviceSourceID"].asString().c_str();
							SourceId >> VideoId;
						}
						else
						{
							SourceId << OneVaule["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}
						
					}

					globalSource = (IBaseVideo *)VideoId;
					break;
				}

			}
		}
			if (OldSource != globalSource)
			{
				if (OldSource && bRunning)
				{
					OldSource->GlobalSourceLeaveScene();
					RemoveLiveInstanceAudio(OldSource,false,globalSource);
				}

				if (globalSource && bRunning)
				{
					globalSource->GlobalSourceEnterScene();
					AddLiveInstanceAudio(globalSource,this);
				}
			}
			else if (bRunning)
			{
				globalSource->GlobalSourceEnterScene();
				AddLiveInstanceAudio(globalSource,this);
			}
	} 
	else
	{	
		if (OldSource)
		{
			OldSource->GlobalSourceLeaveScene();
			RemoveLiveInstanceAudio(OldSource,false,OldSource);
		}
		globalSource = NULL;
	}
}

//-------------------------------------------------------------
void AgentSource::RestSetGlobalSource()
{
	data["SecName"] = "空区域"; //Str("Agent.Empty"));

	std::stringstream SourceId;
	uint64_t InstanceID = 0;
	if (!data["InstanceID"].isNull())
	{
		SourceId << data["InstanceID"].asString().c_str();
		SourceId >> InstanceID;
	}

	ReNameStreamSec(InstanceID, (uint64_t)this, "空区域");


	if (globalSource)
	{
		RemoveLiveInstanceAudio(globalSource, true);//移动到这里防止死锁
	}
	OSEnterMutex(HLock);
	if (globalSource != NULL)
	{
		globalSource->GlobalSourceLeaveScene();
		globalSource = NULL;
	}
	OSLeaveMutex(HLock);
}

List<AgentSource*> AgentSource::AgentList;
AgentSource::~AgentSource()
{
	for (int i = 0; i < AgentList.Num(); ++i)
	{
		if (AgentList[i] == this)
		{
			if (globalSource)
			{
				globalSource->GlobalSourceLeaveScene();
				RemoveLiveInstanceAudio(globalSource);
				globalSource = NULL;
			}
			AgentList.Remove(i);
			OSCloseMutex(HLock);
			break;
		}
	}
}

void AgentSource::SetHasPreProcess(bool bHasPre)
{
	bPreProcess = bHasPre;
}

bool AgentSource::GetHasPreProcess() const
{
	return bPreProcess;
}

Texture * AgentSource::GetTexture()
{
	return Image.GetTexture();
}

void AgentSource::GlobalSourceLeaveScene()
{
	if (!enteredSceneCount)
		return;
	if (--enteredSceneCount)
		return;
	bRunning = false;
	if (globalSource)
	{
		RemoveLiveInstanceAudio(globalSource);
	}
	OSEnterMutex(HLock);
	if (globalSource != NULL)
	{
		globalSource->GlobalSourceLeaveScene();
	}
	OSLeaveMutex(HLock);
}

void AgentSource::GlobalSourceEnterScene()
{
	if (enteredSceneCount++)
		return;
	bRunning = true;
	UpdateSettings(data);
}

bool AgentSource::IsFieldSignal() const
{
	OSEnterMutex(HLock);
	if (globalSource)
	{
		bool IsField = globalSource->IsFieldSignal();
		OSLeaveMutex(HLock);
		return IsField;
	}
	OSLeaveMutex(HLock);
	return false;
}

struct AreaData
{
	String GlobalName;
	List<AgentSource*> Agent;
	List<Value> Ele;
	uint64_t IntanceID;
};


INT_PTR CALLBACK AgentButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
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
	if (hwnd == GetDlgItem(G_Hwnd, IDOK))
	{
		WndProcTem = OKBtnProc;
	}
	else if (hwnd == GetDlgItem(G_Hwnd, IDCANCEL))
	{
		WndProcTem = CancelBtnProc;
	}

	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigureAreaProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	G_Hwnd = hwnd;
	switch (message)
	{
	case WM_NCACTIVATE:
	case WM_NCPAINT:
	{
					   DrawFrame(hwnd,-1, true);
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
						  OKBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  CancelBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)AgentButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)AgentButtonProc);

						  AreaData *configInfo = (AreaData *)lParam;
						  SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
						  LocalizeWindow(hwnd, pluginLocale);
						  HWND hwndArea = GetDlgItem(hwnd, IDC_WINDOW_GLOBAL_AREA);
						  UINT deviceID = CB_ERR;
						  bool FirstFind = false;
						  bool CanAccess = true;
						
						  int Num = configInfo->Ele.Num();
						  for (UINT i = 0; i < Num; ++i)
						  {
							  SendMessage(hwndArea, CB_ADDSTRING, 0,
								  (LPARAM)(Asic2WChar(configInfo->Ele[i]["Name"].asString().c_str())).c_str());

							  bool FindFreeArea = strcmp(configInfo->Ele[i]["SecName"].asString().c_str(), "空区域") == 0;
							  if (FindFreeArea && CanAccess)
							  {
								  FirstFind = true;
								  CanAccess = false;
							  }

							  if (FirstFind)
							  {
								  deviceID = i;
								  FirstFind = false;
							  }
						  }
						  if (deviceID == CB_ERR)
						  {
							  SendMessage(hwndArea, CB_SETCURSEL, 0, 0);
						  }
						  else
						  {
							  if (CanAccess)
								  deviceID = 0;

							  SendMessage(hwndArea, CB_SETCURSEL, deviceID, 0);
						  }
						  
						  InvalidateRect(hwnd, NULL, FALSE);
						  SendMessage(hwnd, WM_NCPAINT, 0, 0);
				
						  return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
					 AreaData *configData = (AreaData*)GetWindowLongPtr(hwnd, DWLP_USER);
					 HWND hwndArea = GetDlgItem(hwnd, IDC_WINDOW_GLOBAL_AREA);
					 UINT id = (UINT)SendMessage(hwndArea, CB_GETCURSEL, 0, 0);
					 if (id == CB_ERR)
					 {
						configData->Ele[id]["SecName"] = "空区域";
					 }
					 else
					 {

							 configData->Ele[id]["SecName"] = WcharToAnsi(configData->GlobalName.Array()).c_str();

							 std::stringstream SourceId;
							 uint64_t VideoId = 0;
							 if (!configData->Ele[id]["SourceID"].isNull())
							 {
								 if (!configData->Ele[id]["DeviceSourceID"].isNull())
								 {
									 SourceId << configData->Ele[id]["DeviceSourceID"].asString().c_str();
									 SourceId >> VideoId;
								 }
								 else
								 {
									 SourceId << configData->Ele[id]["SourceID"].asString().c_str();
									 SourceId >> VideoId;
								 }
								
							 }

							 IBaseVideo *Video = (IBaseVideo*)VideoId;
							 if (Video)
							 {
								 AgentSource *Source = dynamic_cast<AgentSource*>(Video);

								 if (Source)
								 {
									 Value &Data = Source->GetData();
									 if (0 != strcmp(Data["SecName"].asString().c_str(), configData->Ele[id]["SecName"].asString().c_str()))
									 {
										 Data["SecName"] = configData->Ele[id]["SecName"].asString().c_str();

										 Source->UpdateSettings(Data);

										 ReNameStreamSec(configData->IntanceID, (uint64_t)Source, Data["SecName"].asString().c_str());
									 }
								 }
							 }
					 }
		}

		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
		break;
	}

	return 0;
}

static bool bFrist = true;
bool STDCALL ConfigAreaSource(Value &element, bool bCreating)
{
	if (bFrist)
	{
		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(AGENTLOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AGENTLOCALPATH).c_str());
		}

		bFrist = false;
	}

	List<Value> DateList;
	ScanSourceElmentByClassName("AgentSource", DateList);

	int iCount = DateList.Num();
	static bool NeedOne = false;
	if (0 == iCount)
	{
		if (!NeedOne)
		{
			NeedOne = true;
			BLiveMessageBox(GetMainWindow(), TEXT("当前场景没有区域占位源，不能添加到场景！"), TEXT("提示"), MB_OK);
			NeedOne = false;
		}
		return false;
	}
	else if (1 == iCount)
	{
		std::stringstream SourceId;
		uint64_t VideoId = 0;
		if (!DateList[0]["SourceID"].isNull())
		{
			if (!DateList[0]["DeviceSourceID"].isNull())
			{
				SourceId << DateList[0]["DeviceSourceID"].asString().c_str();
				SourceId >> VideoId;
			}
			else
			{
				SourceId << DateList[0]["SourceID"].asString().c_str();
				SourceId >> VideoId;
			}
			
		}

		IBaseVideo *Video = (IBaseVideo*)VideoId;
		if (Video)
		{
			AgentSource *Source = dynamic_cast<AgentSource*>(Video);

			if (Source)
			{
				Value &Data = Source->GetData();
				if (0 != strcmp(Data["SecName"].asString().c_str(), element["Name"].asString().c_str()))
				{
					Data["SecName"] = element["Name"].asString().c_str();
					Source->UpdateSettings(Data);

					uint64_t InstanceID = 0;

					std::stringstream SourceId;
					if (!element["IntanceID"].isNull())
					{
						SourceId << element["IntanceID"].asString().c_str();
						SourceId >> InstanceID;
					}

					ReNameStreamSec(InstanceID, (uint64_t)Source, element["Name"].asString().c_str());
				}
			}
		}
		else
		{
			return false;
		}
	}
	else if (iCount >= 2)
	{
		AreaData InData;
		InData.GlobalName = Asic2WChar(element["Name"].asString().c_str()).c_str();

		std::stringstream SourceId;
		if (!element["IntanceID"].isNull())
		{
			SourceId << element["IntanceID"].asString().c_str();
			SourceId >> InData.IntanceID;
		}

		InData.Ele.CopyList(DateList);

		//区域占位源声音切换问题
		BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG_AREA),GetMainWindow(), ConfigureAreaProc, (LPARAM)&InData);
	}
	return true;
}

REGINST_CONFIGFUNC(ConfigArea, ConfigAreaSource)

#include<vector>

typedef struct NameID
{
	String Name;
	std::string SourceID;

}NameID;

struct ConfigDialogData
{
	CTSTR lpName;	
	Value &data;
	std::vector<NameID> globalDevice;
	StringList deviceNameList;
	StringList deviceIDList;
	StringList audioNameList;
	StringList audioIDList;
	StringList audioRenderNameList;
	StringList audioRenderIDList;
	bool bGlobalSource;
	bool bCreating;
	bool bDeviceHasAudio;
	ConfigDialogData(Value& Data) :data(Data){}
	~ConfigDialogData()
	{
		ClearOutputList();
	}

	void ClearOutputList()
	{
// 		for (UINT i = 0; i < outputList.Num(); i++)
// 			outputList[i].FreeData();
// 		outputList.Clear();
	}
};

INT_PTR CALLBACK ConfigureDialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool bSelectingColor = false;
	static bool bMouseDown = false;
	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	G_Hwnd = hwnd;
	switch (message)
	{
	case WM_NCACTIVATE:
	case WM_NCPAINT:
	{
		   DrawFrame(hwnd,  -1, true);
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
							   DrawFrame(hwnd, -1, true);
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
	case WM_INITDIALOG:
	{
						  OKBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
						  CancelBtnProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)AgentButtonProc);
						  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)AgentButtonProc);

			ConfigDialogData *configInfo = (ConfigDialogData*)lParam;
			SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
			LocalizeWindow(hwnd, pluginLocale);
			HWND hwndGobalDevice = GetDlgItem(hwnd, IDC_WINDOW_GLOBAL_DEVICE);
			SendMessage(hwndGobalDevice, IDC_WINDOW_GLOBAL_DEVICE, 0, 0);			
			//SendMessage(hwndGobalDevice, CB_ADDSTRING, 0, (LPARAM)Str("Agent.Empty"));
			UINT deviceID = CB_ERR;
			String strName;
			if (!configInfo->data["SecName"].isNull())
			{
				strName = Asic2WChar(configInfo->data["SecName"].asString()).c_str();
			}

			if (!strName.IsEmpty())
			{
				for (UINT i = 0; i < configInfo->globalDevice.size(); i++)
				{
					String name = configInfo->globalDevice[i].Name;
					SendMessage(hwndGobalDevice, CB_ADDSTRING, 0, (LPARAM)(configInfo->globalDevice[i].Name.Array()));
					if ((strName.IsValid()) && (configInfo->globalDevice[i].Name == strName))
					{
						deviceID = i;
					}
				}
			}
			if (deviceID == CB_ERR)
			{
				SendMessage(hwndGobalDevice, CB_SETCURSEL, 0, 0);
				ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW_GLOBAL_DEVICE, CBN_SELCHANGE), (LPARAM)hwndGobalDevice);
			}
			else
			{
				SendMessage(hwndGobalDevice, CB_SETCURSEL, deviceID, 0);
				ConfigureDialogProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_WINDOW_GLOBAL_DEVICE, CBN_SELCHANGE), (LPARAM)hwndGobalDevice);
			}

			InvalidateRect(hwnd, NULL, FALSE);
			SendMessage(hwnd, WM_NCPAINT, 0, 0);

			//if (hwndCombo != NULL) SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)lpExceptionNames[i]);
            return TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_WINDOW_GLOBAL_DEVICE:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);
				HWND hwndDevices = (HWND)lParam;
				UINT id = (UINT)SendMessage(hwndDevices, CB_GETCURSEL, 0, 0);
				if (id == CB_ERR)
				{
					// configData->data->SetString(TEXT("name"), TEXT(""));
				}
				else
				{
					//configData->data->SetString(TEXT("name"), configData->globalDevice[id]);
				}
			}
	    break;
		case IDOK:
		{
			    ConfigDialogData *configData = (ConfigDialogData*)GetWindowLongPtr(hwnd, DWLP_USER);				
				HWND hwndGobalDevice = GetDlgItem(hwnd, IDC_WINDOW_GLOBAL_DEVICE);
				UINT id = (UINT)SendMessage(hwndGobalDevice, CB_GETCURSEL, 0, 0);
				if (id == CB_ERR)
				{
					configData->data["SecName"] = "";
				}
				else
				{					
					configData->data["SecName"] = WcharToAnsi(configData->globalDevice[id].Name.Array()).c_str();
				}    
		}
		case IDCANCEL:
			EndDialog(hwnd, LOWORD(wParam));
			break;
		}
		break;
	}

	return 0;
}



bool STDCALL ConfigAgentSource(Value &element, bool bCreating)
{
	// 设置空元素

	if (bFrist)
	{
		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(AGENTLOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AGENTLOCALPATH).c_str());
		}

		bFrist = false;
	}

	if (bCreating)
	{
		element["SecName"] = L"空区域";
	}

 	ConfigDialogData configData(element);
	configData.lpName = Asic2WChar(element["SecName"].asString()).c_str();
	configData.bGlobalSource = true;
 	configData.bCreating = bCreating;
	

	NameID __NameID;

	__NameID.Name = L"空区域";
	__NameID.SourceID = "";

	configData.globalDevice.push_back(__NameID);

	Value NameList;
	if (GetNameList(NameList))
	{
		Json::Value &ArryList = NameList["Namelist"];

		for (int i = 0; i < ArryList.size(); ++i)
		{
			Value &OneVaule = ArryList[i];

			__NameID.Name = Asic2WChar(OneVaule["Name"].asString().c_str()).c_str();
			__NameID.SourceID = OneVaule["SourceID"].asString().c_str();

			configData.globalDevice.push_back(__NameID);
		}
	}



	if (BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIG_AGENT_SOURCE), GetMainWindow(), ConfigureDialogProc, (LPARAM)&configData) == IDOK)
	{
		if (bCreating)
		{
			UINT baseCX,baseCY;

			GetBaseSize(baseCX,baseCY);
			baseCX = MIN(MAX(baseCX, 128), 4096);
			baseCY = MIN(MAX(baseCY, 128), 4096);

			element["cx"] = baseCX;
			element["cy"] = baseCY;
		}

		return true;
 	}
	return false;
}

REGINST_CONFIGFUNC(AgentSource, ConfigAgentSource)