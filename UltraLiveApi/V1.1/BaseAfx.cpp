#include "BaseAfx.h"
#include <string.h>
#include "OperatNew.h"
#include "DrawFrame.h"

#include <shlobj.h>
#include <shlwapi.h>
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"Shlwapi.lib")


extern HWND G_MainHwnd;

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif
const int Max_Num = 1024;
CreateOBJ g_DCreateOBJ[Max_Num] = { 0 };
int CreateOBJ::TotalCount = 0;

CreateOBJFilter g_DCreateOBJFilter[Max_Num] = { 0 };
int CreateOBJFilter::TotalCount = 0;

LARGE_INTEGER clockFre;

CBMap<CONFINGFUN> G_ConfigFun;

int AddCreateOBject2Array(const char * ClassName, CREATEOBJ pf, const char *Desc, const char *Version)
{
	if (!ClassName || !pf || !Desc || !Version || CreateOBJ::TotalCount >= Max_Num - 1)
		return -1;

	if (0 == CreateOBJ::TotalCount)
	{
		g_DCreateOBJ[0].Class_Name = ClassName;
		g_DCreateOBJ[0].fp = pf;
		g_DCreateOBJ[0].Desc = Desc;
		g_DCreateOBJ[0].Version = Version;
		++CreateOBJ::TotalCount;
	}
	else
	{
		
		for (int i = 0; i < CreateOBJ::TotalCount; ++i)
		{
			int ret = strcmp(ClassName,g_DCreateOBJ[i].Class_Name);
			if(ret > 0)
			{
				if (i == CreateOBJ::TotalCount - 1)
				{
					g_DCreateOBJ[i + 1].Class_Name = ClassName;
					g_DCreateOBJ[i + 1].fp = pf;
					g_DCreateOBJ[i + 1].Desc = Desc;
					g_DCreateOBJ[i + 1].Version = Version;
					++CreateOBJ::TotalCount;
					break;
				}
		
				else
					continue;
			}
			else if(ret < 0)
			{
				for (int j = CreateOBJ::TotalCount - 1; j >= i; --j)
				{
					g_DCreateOBJ[j + 1] = g_DCreateOBJ[j];
				}
				g_DCreateOBJ[i].Class_Name = ClassName;
				g_DCreateOBJ[i].fp = pf;
				g_DCreateOBJ[i].Desc = Desc;
				g_DCreateOBJ[i].Version = Version;
				
				++CreateOBJ::TotalCount;
				break;
			}
			else
			{
				g_DCreateOBJ[i].fp = pf;
				break;
			}
		}
		
	}

	return CreateOBJ::TotalCount;
}

int AddCreateOBject2ArrayFilter(const char * ClassName, CREATEOBJ pf, const char *Desc, bool bIsShader)
{
	if (!ClassName || !pf || !Desc|| CreateOBJFilter::TotalCount >= Max_Num - 1)
		return -1;

	if (0 == CreateOBJFilter::TotalCount)
	{
		g_DCreateOBJFilter[0].Class_Name = ClassName;
		g_DCreateOBJFilter[0].fp = pf;
		g_DCreateOBJFilter[0].Desc = Desc;
		g_DCreateOBJFilter[0].IsShader = bIsShader;
		++CreateOBJFilter::TotalCount;
	}
	else
	{

		for (int i = 0; i < CreateOBJFilter::TotalCount; ++i)
		{
			int ret = strcmp(ClassName, g_DCreateOBJFilter[i].Class_Name);
			if (ret > 0)
			{
				if (i == CreateOBJFilter::TotalCount - 1)
				{
					g_DCreateOBJFilter[i + 1].Class_Name = ClassName;
					g_DCreateOBJFilter[i + 1].fp = pf;
					g_DCreateOBJFilter[i + 1].Desc = Desc;
					g_DCreateOBJFilter[i + 1].IsShader = bIsShader;
					++CreateOBJFilter::TotalCount;
					break;
				}

				else
					continue;
			}
			else if (ret < 0)
			{
				for (int j = CreateOBJFilter::TotalCount - 1; j >= i; --j)
				{
					g_DCreateOBJFilter[j + 1] = g_DCreateOBJFilter[j];
				}
				g_DCreateOBJFilter[i].Class_Name = ClassName;
				g_DCreateOBJFilter[i].fp = pf;
				g_DCreateOBJFilter[i].Desc = Desc;
				g_DCreateOBJFilter[i].IsShader = bIsShader;

				++CreateOBJFilter::TotalCount;
				break;
			}
			else
			{
				g_DCreateOBJFilter[i].fp = pf;
				break;
			}
		}

	}

	return CreateOBJFilter::TotalCount;
}


CPObject* CreatStreamObject(const char *Classname)
 {
	if (0 == CreateOBJ::TotalCount || !Classname)
		return NULL;
	int l = 0;
	int h = CreateOBJ::TotalCount - 1;
	while(l <= h)
	{
		int m = (l + h) >> 1;
		int ret = strcmp(Classname,g_DCreateOBJ[m].Class_Name);
		
		if(ret > 0)
		{
			l = m + 1;
		}
		else if(ret < 0)
		{
			h = m - 1;
		}
		else 
		{
			return g_DCreateOBJ[m].fp();
		}
	}
	return NULL;
 }

void GetPluginsName(Value &NameList)
{ 
	Value &ArryList = NameList["NameList"];

	for (int i = 0; i < CreateOBJ::TotalCount; ++i)
	{
		ArryList[i] = g_DCreateOBJ[i].Desc;
	}

}

CPObject* CreatStreamObjectFilter(const char *Classname)
{
	if (0 == CreateOBJFilter::TotalCount || !Classname)
		return NULL;
	int l = 0;
	int h = CreateOBJFilter::TotalCount - 1;
	while (l <= h)
	{
		int m = (l + h) >> 1;
		int ret = strcmp(Classname, g_DCreateOBJFilter[m].Class_Name);

		if (ret > 0)
		{
			l = m + 1;
		}
		else if (ret < 0)
		{
			h = m - 1;
		}
		else
		{
			return g_DCreateOBJFilter[m].fp();
		}
	}
	return NULL;
}

bool IsShaderFilter(const char *Classname)
{
	if (0 == CreateOBJFilter::TotalCount || !Classname)
		return false;
	int l = 0;
	int h = CreateOBJFilter::TotalCount - 1;
	while (l <= h)
	{
		int m = (l + h) >> 1;
		int ret = strcmp(Classname, g_DCreateOBJFilter[m].Class_Name);

		if (ret > 0)
		{
			l = m + 1;
		}
		else if (ret < 0)
		{
			h = m - 1;
		}
		else
		{
			return g_DCreateOBJFilter[m].IsShader;
		}
	}
	return false;
}

int GetSourceObjectCount()
{
	return CreateOBJ::TotalCount;
}

const CreateOBJ* GetArrayObject()
{
	return g_DCreateOBJ;
}

int GetSourceObjectCountFilter()
{
	return CreateOBJFilter::TotalCount;
}

const CreateOBJFilter* GetArrayObjectFilter()
{
	return g_DCreateOBJFilter;
}

QWORD GetQPCNS()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double timeVal = double(currentTime.QuadPart);
	timeVal *= 1000000000.0;
	timeVal /= double(clockFre.QuadPart);

	return QWORD(timeVal);
}

QWORD GetQPC100NS()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double timeVal = double(currentTime.QuadPart);
	timeVal *= 10000000.0;
	timeVal /= double(clockFre.QuadPart);

	return QWORD(timeVal);
}

QWORD GetQPCMS()
{
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	QWORD timeVal = currentTime.QuadPart;
	timeVal *= 1000;
	timeVal /= clockFre.QuadPart;

	return timeVal;
}

bool Sleep2NS(QWORD qwNSTime)
{
	QWORD t = GetQPCNS();

	if (t >= qwNSTime)
		return false;

	unsigned int milliseconds = (unsigned int)((qwNSTime - t) / 1000000);
	if (milliseconds > 1) //also accounts for windows 8 sleep problem
	{
		//trap suspicious sleeps that should never happen
		if (milliseconds > 10000)
		{
			DebugBreak();
		}
		Sleep(milliseconds);
	}

	for (;;)
	{
		t = GetQPCNS();
		if (t >= qwNSTime)
			return true;
		Sleep(1);
	}
}

std::wstring Asic2WChar(const std::string& str)
{
	if (str.empty())
	{
		return TEXT("");
	}
	int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
	if (nSize <= 0) return NULL;

	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst) return NULL;

	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
	pwszDst[nSize] = 0;


	std::wstring wcharString(pwszDst);
	delete [] pwszDst;
	return wcharString;
}

std::string WcharToAnsi(const std::wstring& strSrc)
{
	if (strSrc.empty())
	{
		return "";
	}

	int nSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), 0, 0, NULL, NULL);
	char *Tem = new char[nSize + 1];
	if (!Tem)
	{
		return "";
	}
	WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), Tem, nSize, NULL, NULL);
	Tem[nSize] = 0;

	std::string Ret = Tem;

	delete[] Tem;

	return Ret;
}

bool GetFileVersion(const wchar_t *file_path, WORD *major_version, WORD *minor_version, WORD *build_number, WORD *revision_number)
{
	DWORD handle, len;
	UINT buf_len;
	LPTSTR buf_data;
	VS_FIXEDFILEINFO *file_info;
	len = GetFileVersionInfoSize(file_path, &handle);
	if (0 == len)
		return false;

	buf_data = (LPTSTR)malloc(len);
	if (!buf_data)
		return false;

	if (!GetFileVersionInfo(file_path, handle, len, buf_data))
	{
		free(buf_data);
		return false;
	}
	if (VerQueryValue(buf_data, L"\\", (LPVOID *)&file_info, (PUINT)&buf_len))
	{
		*major_version = HIWORD(file_info->dwFileVersionMS);
		*minor_version = LOWORD(file_info->dwFileVersionMS);
		*build_number = HIWORD(file_info->dwFileVersionLS);
		*revision_number = LOWORD(file_info->dwFileVersionLS);
		free(buf_data);
		return true;
	}
	free(buf_data);
	return false;
}

int GetNTDLLVersion()
{
	static int ret = 0;
	if (ret == 0)
	{
		wchar_t buf_dll_name[MAX_PATH] = { 0 };
		HRESULT hr = ::SHGetFolderPathW(NULL, CSIDL_SYSTEM, NULL, SHGFP_TYPE_CURRENT, buf_dll_name);
		if (SUCCEEDED(hr) && ::PathAppendW(buf_dll_name, L"ntdll.dll"))
		{
			WORD major_version, minor_version, build_number, revision_number;
			GetFileVersion(buf_dll_name, &major_version, &minor_version, &build_number, &revision_number);
			ret = major_version * 100 + minor_version;
		}
	}
	return ret;
}

void DrawFrame(HWND hwnd, int Type, bool IsOnlyColse)
{
	return Title::Instance()->DrawFrame(hwnd, Type, IsOnlyColse);
}

std::vector<TitlePicInfo>& GetListTitle()
{
	return Title::Instance()->ListTitle;
}


String GetEditText(HWND hwndEdit)
{
	String strText;
	strText.SetLength((UINT)SendMessage(hwndEdit, WM_GETTEXTLENGTH, 0, 0));
	if (strText.Length())
		SendMessage(hwndEdit, WM_GETTEXT, strText.Length() + 1, (LPARAM)strText.Array());

	return strText;
}

int AddConfigFun2Map(const char *ClassName, CONFINGFUN Fp)
{
	G_ConfigFun[ClassName] = Fp;
	return G_ConfigFun.GetSize();
}

CONFINGFUN GetConfigFunc(const char *ClassName)
{
	CONFINGFUN fp = G_ConfigFun[ClassName];

	if (!fp)
	{
		G_ConfigFun.earse(ClassName);
	}

	return fp;
}

HWND GetMainWindow()
{
	return G_MainHwnd;
}

void PackPlanar(LPBYTE convertBuffer, LPBYTE lpPlanar, UINT renderCX, UINT renderCY, UINT pitch, UINT startY, UINT endY, UINT linePitch, UINT lineShift)
{
	LPBYTE output = convertBuffer;
	LPBYTE input = lpPlanar + lineShift;
	LPBYTE input2 = input + (renderCX*renderCY);
	LPBYTE input3 = input2 + (renderCX*renderCY >> 2);

	UINT halfStartY = startY >> 1;
	UINT halfX = renderCX >> 1;
	UINT halfY = endY >> 1;

	for (UINT y = halfStartY; y < halfY; ++y)
	{
		LPBYTE lpLum1 = input + (y * linePitch << 1);
		LPBYTE lpLum2 = lpLum1 + linePitch;
		LPBYTE lpChroma1 = input2 + y*(linePitch >> 1);
		LPBYTE lpChroma2 = input3 + y*(linePitch >> 1);
		LPDWORD output1 = (LPDWORD)(output + (y * 2)*pitch);
		LPDWORD output2 = (LPDWORD)(((LPBYTE)output1) + pitch);

		for (UINT x = 0; x < halfX; ++x)
		{
			DWORD out = (*(lpChroma1++) << 8) | (*(lpChroma2++) << 16);

			*(output1++) = *(lpLum1++) | out;
			*(output1++) = *(lpLum1++) | out;

			*(output2++) = *(lpLum2++) | out;
			*(output2++) = *(lpLum2++) | out;
		}
	}
}

int BLiveMessageBox(HWND hwnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT flags)
{
	return MessageBox(hwnd, lpText, lpCaption, flags);
}

String GetLBText(HWND hwndList, UINT id)
{
	UINT curSel = (id != LB_ERR) ? id : (UINT)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
	if (curSel == LB_ERR)
		return String();

	String strText;
	strText.SetLength((UINT)SendMessage(hwndList, LB_GETTEXTLEN, curSel, 0));
	if (strText.Length())
		SendMessage(hwndList, LB_GETTEXT, curSel, (LPARAM)strText.Array());

	return strText;
}

String GetLVText(HWND hwndList, UINT id)
{
	String strText;
	strText.SetLength(256);
	ListView_GetItemText(hwndList, id, 0, (LPWSTR)strText.Array(), 256);

	return strText;
}

String GetCBText(HWND hwndCombo, UINT id)
{
	UINT curSel = (id != CB_ERR) ? id : (UINT)SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
	if (curSel == CB_ERR)
		return String();

	String strText;
	strText.SetLength((UINT)SendMessage(hwndCombo, CB_GETLBTEXTLEN, curSel, 0));
	if (strText.Length())
		SendMessage(hwndCombo, CB_GETLBTEXT, curSel, (LPARAM)strText.Array());

	return strText;
}
INT_PTR BLiveDialogBox(HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam /*= 0*/)
{
	return DialogBoxParam(hInstance, lpTemplateName, hWndParent, lpDialogFunc, dwInitParam);
}

void GetBaseSize(UINT &Width, UINT &Height)
{
	Width = CSLiveManager::GetInstance()->baseCX;
	Height = CSLiveManager::GetInstance()->baseCY;

}

int GetMaxFPS()
{
	return CSLiveManager::GetInstance()->BSParam.LiveSetting.FPS;
}
bool CurrentDeviceExists(Value &data)
{
	bool bFind = false;
	CSLiveManager *Manager = CSLiveManager::GetInstance();
	EnterCriticalSection(&Manager->MapInstanceSec);
	for (int i = 0; i < Manager->m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *Process = Manager->m_InstanceList.GetAt(i);
		if (Process && Process->bLittlePre && !Process->bNoPreView)
		{
			for (int j = 0; j < Process->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneStruct = Process->m_VideoList[j];

				if (0 == strcmp(OneStruct.VideoStream->GainClassName(), "DeviceSource"))
				{
					if (!data["deviceID"].isNull())
					{
						if (OneStruct.VideoDevice)
						{
							if (strcmp(OneStruct.VideoDevice->GetDeviceID(), data["deviceID"].asString().c_str()) == 0 && strcmp(OneStruct.VideoDevice->GetDeviceName(), data["deviceName"].asString().c_str()) == 0)
							{
								//同一个设备
								bFind = true;
								break;
							}
						}
						else
						{
							if (strcmp(OneStruct.VideoStream->GetDeviceID(), data["deviceID"].asString().c_str()) == 0 && strcmp(OneStruct.VideoStream->GetDeviceName(), data["deviceName"].asString().c_str()) == 0)
							{
								//同一个设备
								bFind = true;
								break;
							}
						}
					
					}
					else
					{
						if (OneStruct.VideoDevice)
						{
							if (strcmp(OneStruct.VideoDevice->GetDeviceName(), data["deviceName"].asString().c_str()) == 0)
							{
								bFind = true;
								break;
							}
						}
						else
						{
							if (strcmp(OneStruct.VideoStream->GetDeviceName(), data["deviceName"].asString().c_str()) == 0)
							{
								bFind = true;
								break;
							}
						}
						
					}
				}
			}
		}

		if (bFind)
			break;
	}
	LeaveCriticalSection(&Manager->MapInstanceSec);
	return bFind;
}

HANDLE SharedDevice::HLock = OSCreateMutex();
std::vector<ShardVideo> SharedDevice::VideoList;
int SharedDevice::RegisterSharedDevice(DataCallBack Cb)
{
	OSEnterMutex(HLock);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin", __FUNCTION__);
	bool bFind = false;

	for (int i = 0; i < VideoList.size(); ++i)
	{
		ShardVideo &OneVideo = VideoList[i];

		if (0 == strcmp(OneVideo.VideoStream->GainClassName(), "DeviceSource"))
		{
			if (!DeviceID.IsEmpty())
			{
				if (strcmp(OneVideo.VideoStream->GetDeviceID(), WcharToAnsi(DeviceID.Array()).c_str()) == 0 && strcmp(OneVideo.VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
				{

					Vect2 &Size = OneVideo.VideoStream->GetSize();

					//是否要改分辨率
					if (Size.x < iWidth || Size.y < iHeigth)
					{
						(*OneVideo.Config)["customResolution"] = 1;
						(*OneVideo.Config)["resolutionWidth"] = iWidth;
						(*OneVideo.Config)["resolutionHeight"] = iHeigth;

						OneVideo.VideoStream->UpdateSettings(*OneVideo.Config);
					}

					OneVideo.VideoStream->RegisterDataCallBack(this, Cb);
					bFind = true;

					//在添加一次
					VideoList.push_back(OneVideo);

					Log::writeMessage(LOG_RTSPSERV, 1, "%s 在VideoList找到并RegisterDataCallBack,size = %d,DeviceName = %s,DeviceID = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str(), WcharToAnsi(DeviceID.Array()).c_str());
					break;
				}
			}
			else
			{
				if (strcmp(OneVideo.VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
				{

					Vect2 &Size = OneVideo.VideoStream->GetSize();

					//是否要改分辨率
					if (Size.x < iWidth || Size.y < iHeigth)
					{
						(*OneVideo.Config)["customResolution"] = 1;
						(*OneVideo.Config)["resolutionWidth"] = iWidth;
						(*OneVideo.Config)["resolutionHeight"] = iHeigth;

						OneVideo.VideoStream->UpdateSettings(*OneVideo.Config);
					}

					OneVideo.VideoStream->RegisterDataCallBack(this, Cb);

					VideoList.push_back(OneVideo);
					bFind = true;
					Log::writeMessage(LOG_RTSPSERV, 1, "%s 在VideoList找到并RegisterDataCallBack,size = %d,DeviceName = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str());
					break;
				}
			}
		}
	}


	if (!bFind)
	{
		CSLiveManager *Manager = CSLiveManager::GetInstance();
		//EnterCriticalSection(&Manager->MapInstanceSec);
		for (int i = 0; i < Manager->m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *Process = Manager->m_InstanceList.GetAt(i);
			if (Process && Process->bLittlePre && !Process->bNoPreView)
			{
				for (int j = 0; j < Process->m_VideoList.Num(); ++j)
				{
					VideoStruct &OneStruct = Process->m_VideoList[j];

					if (0 == strcmp(OneStruct.VideoStream->GainClassName(), "DeviceSource"))
					{
						if (OneStruct.VideoDevice)
						{
							if (!DeviceID.IsEmpty())
							{
								if (strcmp(OneStruct.VideoDevice->GetDeviceID(), WcharToAnsi(DeviceID.Array()).c_str()) == 0 && strcmp(OneStruct.VideoDevice->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
								{
									//同一个设备

									Vect2 &Size = OneStruct.VideoDevice->GetSize();

									//是否要改分辨率
									if (Size.x < iWidth || Size.y < iHeigth)
									{
										(*OneStruct.Config)["customResolution"] = 1;
										(*OneStruct.Config)["resolutionWidth"] = iWidth;
										(*OneStruct.Config)["resolutionHeight"] = iHeigth;
										Log::writeMessage(LOG_RTSPSERV, 1, "%s 更改分辨率 Size.x = %d,Size.y = %d,iWidth = %d,iHeigth = %d", __FUNCTION__, Size.x, Size.y, iWidth, iHeigth);
										OneStruct.VideoDevice->UpdateSettings(*OneStruct.Config);
									}

									OneStruct.VideoDevice->RegisterDataCallBack(this, Cb);

									//加入Shared队列
									ShardVideo Shard;
									Shard.VideoStream = OneStruct.VideoDevice;
									Shard.Config = make_shared<Value>(*OneStruct.Config);
									VideoList.push_back(Shard);
									bFind = true;

									Log::writeMessage(LOG_RTSPSERV, 1, "%s 在m_InstanceList找到并RegisterDataCallBack,size = %d,DeviceName = %s,DeviceID = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str(), WcharToAnsi(DeviceID.Array()).c_str());
									break;
								}
							}
							else
							{
								if (strcmp(OneStruct.VideoDevice->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
								{
									//同一个设备

									Vect2 &Size = OneStruct.VideoDevice->GetSize();

									//是否要改分辨率
									if (Size.x < iWidth || Size.y < iHeigth)
									{
										(*OneStruct.Config)["customResolution"] = 1;
										(*OneStruct.Config)["resolutionWidth"] = iWidth;
										(*OneStruct.Config)["resolutionHeight"] = iHeigth;
										Log::writeMessage(LOG_RTSPSERV, 1, "%s 更改分辨率 Size.x = %d,Size.y = %d,iWidth = %d,iHeigth = %d", __FUNCTION__, Size.x, Size.y, iWidth, iHeigth);
										Process->UpdateStream((uint64_t)OneStruct.VideoDevice.get(), OneStruct.Config->toStyledString().c_str());
									}

									OneStruct.VideoDevice->RegisterDataCallBack(this, Cb);

									//加入Shared队列
									ShardVideo Shard;
									Shard.VideoStream = OneStruct.VideoDevice;
									Shard.Config = make_shared<Value>(*OneStruct.Config);
									VideoList.push_back(Shard);
									bFind = true;

									Log::writeMessage(LOG_RTSPSERV, 1, "%s 在m_InstanceList找到并RegisterDataCallBack,size = %d,DeviceName = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str());
									break;
								}
							}
						}
						else
						{
							if (!DeviceID.IsEmpty())
							{
								if (strcmp(OneStruct.VideoStream->GetDeviceID(), WcharToAnsi(DeviceID.Array()).c_str()) == 0 && strcmp(OneStruct.VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
								{
									//同一个设备

									Vect2 &Size = OneStruct.VideoStream->GetSize();

									//是否要改分辨率
									if (Size.x < iWidth || Size.y < iHeigth)
									{
										(*OneStruct.Config)["customResolution"] = 1;
										(*OneStruct.Config)["resolutionWidth"] = iWidth;
										(*OneStruct.Config)["resolutionHeight"] = iHeigth;
										Log::writeMessage(LOG_RTSPSERV, 1, "%s 更改分辨率 Size.x = %d,Size.y = %d,iWidth = %d,iHeigth = %d", __FUNCTION__, Size.x, Size.y, iWidth, iHeigth);
										OneStruct.VideoStream->UpdateSettings(*OneStruct.Config);
									}

									OneStruct.VideoStream->RegisterDataCallBack(this, Cb);

									//加入Shared队列
									ShardVideo Shard;
									Shard.VideoStream = OneStruct.VideoStream;
									Shard.Config = make_shared<Value>(*OneStruct.Config);
									VideoList.push_back(Shard);
									bFind = true;

									Log::writeMessage(LOG_RTSPSERV, 1, "%s 在m_InstanceList找到并RegisterDataCallBack,size = %d,DeviceName = %s,DeviceID = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str(), WcharToAnsi(DeviceID.Array()).c_str());
									break;
								}
							}
							else
							{
								if (strcmp(OneStruct.VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
								{
									//同一个设备

									Vect2 &Size = OneStruct.VideoStream->GetSize();

									//是否要改分辨率
									if (Size.x < iWidth || Size.y < iHeigth)
									{
										(*OneStruct.Config)["customResolution"] = 1;
										(*OneStruct.Config)["resolutionWidth"] = iWidth;
										(*OneStruct.Config)["resolutionHeight"] = iHeigth;
										Log::writeMessage(LOG_RTSPSERV, 1, "%s 更改分辨率 111Size.x = %d,Size.y = %d,iWidth = %d,iHeigth = %d", __FUNCTION__, Size.x, Size.y, iWidth, iHeigth);
										Process->UpdateStream((uint64_t)OneStruct.VideoStream.get(), OneStruct.Config->toStyledString().c_str());
									}

									OneStruct.VideoStream->RegisterDataCallBack(this, Cb);

									//加入Shared队列
									ShardVideo Shard;
									Shard.VideoStream = OneStruct.VideoStream;
									Shard.Config = make_shared<Value>(*OneStruct.Config);
									VideoList.push_back(Shard);
									bFind = true;

									Log::writeMessage(LOG_RTSPSERV, 1, "%s 111在m_InstanceList找到并RegisterDataCallBack,size = %d,DeviceName = %s", __FUNCTION__, VideoList.size(), WcharToAnsi(DeviceName.Array()).c_str());
									break;
								}
							}
						}

						
					}
				}
			}

			if (bFind)
				break;
		}
		//LeaveCriticalSection(&Manager->MapInstanceSec);
	}

	if (!bFind)
	{
		//没有找到创建一个新的设备
		Log::writeMessage(LOG_RTSPSERV, 1, "%s 没有找到创建一个新的设备,DeviceName = %s", __FUNCTION__, WcharToAnsi(DeviceName.Array()).c_str());
		IBaseVideo *NewVideo = (IBaseVideo*)CreatStreamObject("DeviceSource");

		if (NewVideo)
		{
			Value JValue;
			JValue["device"] = WcharToAnsi(DeviceName.Array()).c_str();
			JValue["deviceName"] = WcharToAnsi(DeviceName.Array()).c_str();
			if (!DeviceID.IsEmpty())
				JValue["deviceID"] = WcharToAnsi(DeviceID.Array()).c_str();
			else
			{
				JValue["deviceID"] = "";
			}
			JValue["customResolution"] = 1;
			JValue["resolutionWidth"] = iWidth;
			JValue["resolutionHeight"] = iHeigth;
			JValue["audioDevice"] = "禁用";
			JValue["frameInterval"] = 333333;

			if (NewVideo->Init(JValue))
			{
				ShardVideo Shard;

				Shard.VideoStream = shared_ptr<IBaseVideo>(NewVideo);
				Shard.VideoStream->RegisterDataCallBack(this, Cb);
				Shard.Config = make_shared<Value>(JValue);
				VideoList.push_back(Shard);
				Log::writeMessage(LOG_RTSPSERV, 1, "%s 新的设备创建成功并RegisterDataCallBack,DeviceName = %s", __FUNCTION__, WcharToAnsi(DeviceName.Array()).c_str());
			}
			else
			{
				delete NewVideo;
				Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! Init failed! return -1", __FUNCTION__);
				OSLeaveMutex(HLock);
				return -1;
			}
		}
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
	OSLeaveMutex(HLock);
	return 0;
}

const char * SharedDevice::GainClassName()
{
	return "SharedDevice";
}

void SharedDevice::UnRegisterSharedDevice()
{
	OSEnterMutex(HLock);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin",__FUNCTION__);
	std::vector<ShardVideo>::iterator begin = VideoList.begin();

	if (DeviceID.IsEmpty() && DeviceName.IsEmpty())
	{
		OSLeaveMutex(HLock);
		return;
	}

	for (begin; begin != VideoList.end();)
	{
		if (begin->VideoStream)
		{
			if (0 == strcmp(begin->VideoStream->GainClassName(), "DeviceSource"))
			{
				if (!DeviceID.IsEmpty())
				{
					if (strcmp(begin->VideoStream->GetDeviceID(), WcharToAnsi(DeviceID.Array()).c_str()) == 0 && strcmp(begin->VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
					{
						begin->VideoStream->UnRegisterDataCallBack(this);
						Log::writeMessage(LOG_RTSPSERV, 1, "找到并UnRegisterDataCallBack DeviceName %s,DeviceID %s", WcharToAnsi(DeviceName.Array()).c_str(), WcharToAnsi(DeviceID.Array()).c_str());
						begin = VideoList.erase(begin);
						break;
					}
				}
				else
				{
					if (strcmp(begin->VideoStream->GetDeviceName(), WcharToAnsi(DeviceName.Array()).c_str()) == 0)
					{
						begin->VideoStream->UnRegisterDataCallBack(this);
						Log::writeMessage(LOG_RTSPSERV, 1, "找到并UnRegisterDataCallBack DeviceName %s", WcharToAnsi(DeviceName.Array()).c_str());
						begin = VideoList.erase(begin);
						break;
					}
				}
			}
		}

		++begin;
	}

	Clear();

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
	OSLeaveMutex(HLock);
}

void SharedDevice::Clear()
{
	DeviceName.Clear();
	DeviceID.Clear();
	Server = NULL;
}

bool GetNameList(Value &data)
{
	return CSLiveManager::GetInstance()->GetNameList(data);
}

bool ScanSourceElmentByClassName(const char *ClassName, List<Value>& List)
{
	return CSLiveManager::GetInstance()->ScanSourceElmentByClassName(ClassName, List);
}

void ReNameStreamSec(__int64 IntanceID, __int64 StreamID, const char *NewName)
{
	CSLiveManager::GetInstance()->ReNameStreamSec(IntanceID, StreamID, NewName);
}

BOOL CALLBACK MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, List<MonitorInfo> &monitors)
{
	monitors << MonitorInfo(hMonitor, lprcMonitor);
	return TRUE;
}

UINT NumMonitors()
{
	return CSLiveManager::GetInstance()->monitors.Num();
}

const MonitorInfo& GetMonitor(UINT id)
{
	if (id < CSLiveManager::GetInstance()->monitors.Num()) return CSLiveManager::GetInstance()->monitors[id]; else return CSLiveManager::GetInstance()->monitors[0];
}

static LPBYTE GetBitmapData(HBITMAP hBmp, BITMAP &bmp)
{
	if (!hBmp)
		return NULL;

	if (GetObject(hBmp, sizeof(bmp), &bmp) != 0) {
		UINT bitmapDataSize = bmp.bmHeight*bmp.bmWidth*bmp.bmBitsPixel;
		bitmapDataSize >>= 3;

		LPBYTE lpBitmapData = (LPBYTE)Allocate_Bak(bitmapDataSize);
		GetBitmapBits(hBmp, bitmapDataSize, lpBitmapData);

		return lpBitmapData;
	}

	return NULL;
}

static inline BYTE BitToAlpha(LPBYTE lp1BitTex, int pixel, bool bInvert)
{
	BYTE pixelByte = lp1BitTex[pixel / 8];
	BYTE pixelVal = pixelByte >> (7 - (pixel % 8)) & 1;

	if (bInvert)
		return pixelVal ? 0xFF : 0;
	else
		return pixelVal ? 0 : 0xFF;
}

LPBYTE GetCursorData(HICON hIcon, ICONINFO &ii, UINT &width, UINT &height)
{
	BITMAP bmpColor, bmpMask;
	LPBYTE lpBitmapData = NULL, lpMaskData = NULL;

	if (lpBitmapData = GetBitmapData(ii.hbmColor, bmpColor)) {
		if (bmpColor.bmBitsPixel < 32) {
			Free(lpBitmapData);
			return NULL;
		}

		if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
			int pixels = bmpColor.bmHeight*bmpColor.bmWidth;
			bool bHasAlpha = false;

			//god-awful horrible hack to detect 24bit cursor
			for (int i = 0; i < pixels; i++) {
				if (lpBitmapData[i * 4 + 3] != 0) {
					bHasAlpha = true;
					break;
				}
			}

			if (!bHasAlpha) {
				for (int i = 0; i < pixels; i++) {
					lpBitmapData[i * 4 + 3] = BitToAlpha(lpMaskData, i, false);
				}
			}

			Free(lpMaskData);
		}

		width = bmpColor.bmWidth;
		height = bmpColor.bmHeight;
	}
	else if (lpMaskData = GetBitmapData(ii.hbmMask, bmpMask)) {
		bmpMask.bmHeight /= 2;

		int pixels = bmpMask.bmHeight*bmpMask.bmWidth;
		lpBitmapData = (LPBYTE)Allocate_Bak(pixels * 4);
		zero(lpBitmapData, pixels * 4);

		UINT bottom = bmpMask.bmWidthBytes*bmpMask.bmHeight;

		for (int i = 0; i < pixels; i++) {
			BYTE transparentVal = BitToAlpha(lpMaskData, i, false);
			BYTE colorVal = BitToAlpha(lpMaskData + bottom, i, true);

			if (!transparentVal)
				lpBitmapData[i * 4 + 3] = colorVal; //as an alternative to xoring, shows inverted as black
			else
				*(LPDWORD)(lpBitmapData + (i * 4)) = colorVal ? 0xFFFFFFFF : 0xFF000000;
		}

		Free(lpMaskData);

		width = bmpMask.bmWidth;
		height = bmpMask.bmHeight;
	}

	return lpBitmapData;
}

void GetDisplayDevices(DeviceOutputs &deviceList)
{
	HRESULT err;

	deviceList.ClearData();

#ifdef USE_DXGI1_2
	REFIID iidVal = OSGetVersion() >= 8 ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);
#else
	REFIIF iidVal = __uuidof(IDXGIFactory1);
#endif

	IDXGIFactory1 *factory;
	if (SUCCEEDED(err = CreateDXGIFactory1(iidVal, (void**)&factory)))
	{
		UINT i = 0;
		IDXGIAdapter1 *giAdapter;

		while (factory->EnumAdapters1(i++, &giAdapter) == S_OK)
		{
			//Log(TEXT("------------------------------------------"));

			DXGI_ADAPTER_DESC adapterDesc;
			if (SUCCEEDED(err = giAdapter->GetDesc(&adapterDesc)))
			{
				if (adapterDesc.DedicatedVideoMemory != 0) {
					DeviceOutputData &deviceData = *deviceList.devices.CreateNew();
					deviceData.strDevice = adapterDesc.Description;

					UINT j = 0;
					IDXGIOutput *giOutput;
					while (giAdapter->EnumOutputs(j++, &giOutput) == S_OK)
					{
						DXGI_OUTPUT_DESC outputDesc;
						if (SUCCEEDED(giOutput->GetDesc(&outputDesc)))
						{
							if (outputDesc.AttachedToDesktop)
							{
								deviceData.monitorNameList << outputDesc.DeviceName;

								MonitorInfo &monitorInfo = *deviceData.monitors.CreateNew();
								monitorInfo.hMonitor = outputDesc.Monitor;
								mcpy(&monitorInfo.rect, &outputDesc.DesktopCoordinates, sizeof(RECT));
								switch (outputDesc.Rotation) {
								case DXGI_MODE_ROTATION_ROTATE90:
									monitorInfo.rotationDegrees = 90.0f;
									break;
								case DXGI_MODE_ROTATION_ROTATE180:
									monitorInfo.rotationDegrees = 180.0f;
									break;
								case DXGI_MODE_ROTATION_ROTATE270:
									monitorInfo.rotationDegrees = 270.0f;
								}
							}
						}

						giOutput->Release();
					}
				}
			}
			else
				AppWarning(TEXT("Could not query adapter %u"), i);

			giAdapter->Release();
		}

		factory->Release();
	}
}

String GetDirectorMonitorDevices()
{
	if (CSLiveManager::GetInstance()->BSParam.DeviceSetting.MonitorDevice[0] == '\0')
		return String(L"停用");
	return Asic2WChar(CSLiveManager::GetInstance()->BSParam.DeviceSetting.MonitorDevice).c_str();
}

String GetSecMonitorDevices()
{
	if (CSLiveManager::GetInstance()->BSParam.DeviceSetting.ScrProDevice[0] == '\0')
		return String(L"停用");
	return Asic2WChar(CSLiveManager::GetInstance()->BSParam.DeviceSetting.ScrProDevice).c_str();
}


void LocalizeWindow(HWND hwnd, LocaleStringLookup *lookup)
{
	if (!lookup)
		return;
	int textLen = (int)SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
	String strText;
	strText.SetLength(textLen);
	GetWindowText(hwnd, strText, textLen + 1);

	if (strText.IsValid() && lookup->HasLookup(strText))
		SetWindowText(hwnd, lookup->LookupString(strText));

	//-------------------------------
	RECT rect = { 0 };
	GetClientRect(hwnd, &rect);

	HWND hwndChild = GetWindow(hwnd, GW_CHILD);
	while (hwndChild)
	{
		int textLen = (int)SendMessage(hwndChild, WM_GETTEXTLENGTH, 0, 0);
		strText.SetLength(textLen);
		GetWindowText(hwndChild, strText, textLen + 1);

		if (strText.IsValid())
		{
			if (strText[0] == '.')
				SetWindowText(hwndChild, strText.Array() + 1);
			else
			{
				if (strText.IsValid() && lookup->HasLookup(strText))
					SetWindowText(hwndChild, lookup->LookupString(strText));
			}
		}

		hwndChild = GetNextWindow(hwndChild, GW_HWNDNEXT);
	}
};

void RemoveLiveInstanceAudio(IBaseVideo *BaseVideo, bool bMustDel, IBaseVideo *NewGobal)
{
	return CSLiveManager::GetInstance()->RemoveLiveInstanceAudio(BaseVideo, bMustDel, NewGobal);
}

void AddLiveInstanceAudio(IBaseVideo *Video, IBaseVideo *Agent)
{
	return CSLiveManager::GetInstance()->AddLiveInstanceAudio(Video,Agent);
}

UINT GetOutputAudioSampleRate()
{
	return CSLiveManager::GetInstance()->BSParam.LiveSetting.AudioSampleRate;
}

void* GetNickNameCallBack()
{
	return CSLiveManager::GetInstance()->BSParam.NickNameCb;
}


void EnterLiveVideoSection()
{
	if (CSLiveManager::GetInstance()->LiveInstance)
		EnterCriticalSection(&CSLiveManager::GetInstance()->LiveInstance->AudioSection);
}

void LeaveLiveVideoSection()
{
	if(CSLiveManager::GetInstance()->LiveInstance)
		LeaveCriticalSection(&CSLiveManager::GetInstance()->LiveInstance->AudioSection);
}

void ChangeLiveInstanceSameAsLocalInstance(IBaseVideo *Video)
{
	CSLiveManager::GetInstance()->ChangeLiveInstanceSameAsLocalInstance(Video);
}

D3DAPI *GetD3DRender()
{
	return CSLiveManager::GetInstance()->GetD3DRender();
}

bool IsSupportRecord(const TCHAR *DisplayName)
{
	DeviceOutputs OutPuts;
	GetDisplayDevices(OutPuts);

	bool bSupport = false;
	if (OutPuts.devices.Num() > 0)
	{
		for (int i = 0; i < OutPuts.devices.Num(); ++i)
		{
			if (sstri((const TCHAR*)OutPuts.devices[i].strDevice.Array(), DisplayName) != NULL)
			{
				bSupport = true;
				break;
			}
		}
	}
	return bSupport;
}

int GetCurrentFPS()
{
	return CSLiveManager::GetInstance()->GetCurrentFPS();
}

LocaleStringLookup::LocaleStringLookup()
{
	top = new StringLookupNode;
}

LocaleStringLookup::~LocaleStringLookup()
{
	cache.Clear();
	delete top;
}


void LocaleStringLookup::AddLookup(CTSTR lookupVal, LocaleStringItem *item, StringLookupNode *node)
{
	if (!node) node = top;

	if (!lookupVal)
		return;

	if (!*lookupVal)
	{
		delete node->leaf;
		node->leaf = item;
		return;
	}

	StringLookupNode *child = node->FindSubNodeByChar(*lookupVal);

	if (child)
	{
		UINT len;

		for (len = 0; len < child->str.Length(); len++)
		{
			TCHAR val1 = child->str[len],
				val2 = lookupVal[len];

			if ((val1 >= 'A') && (val1 <= 'Z'))
				val1 += 0x20;
			if ((val2 >= 'A') && (val2 <= 'Z'))
				val2 += 0x20;

			if (val1 != val2)
				break;
		}

		if (len == child->str.Length())
			return AddLookup(lookupVal + len, item, child);
		else
		{
			StringLookupNode *childSplit = new StringLookupNode;
			childSplit->str = child->str.Array() + len;
			childSplit->leaf = child->leaf;
			childSplit->subNodes.TransferFrom(child->subNodes);

			child->leaf = NULL;
			child->str.SetLength(len);

			child->subNodes << childSplit;

			if (lookupVal[len] != 0)
			{
				StringLookupNode *newNode = new StringLookupNode;
				newNode->leaf = item;
				newNode->str = lookupVal + len;

				child->subNodes << newNode;
			}
			else
				child->leaf = item;
		}
	}
	else
	{
		StringLookupNode *newNode = new StringLookupNode;
		newNode->leaf = item;
		newNode->str = lookupVal;

		node->subNodes << newNode;
	}
}

void LocaleStringLookup::RemoveLookup(CTSTR lookupVal, StringLookupNode *node)
{
	if (!node) node = top;

	UINT childID = node->FindSubNodeID(lookupVal);
	if (childID == INVALID)
		return;

	StringLookupNode *child = node->subNodes[childID];

	lookupVal += child->str.Length();
	TCHAR ch = *lookupVal;
	if (ch)
		RemoveLookup(lookupVal, child);

	if (!ch)
	{
		if (!child->subNodes.Num())
		{
			if (child->leaf)
			{
				cache.RemoveItem(child->leaf);
				delete child->leaf;
			}

			node->subNodes.Remove(childID);
			delete child;
		}
		else
		{
			if (child->leaf)
			{
				cache.RemoveItem(child->leaf);
				delete child->leaf;
			}

			child->leaf = NULL;

			if (child->subNodes.Num() == 1)
			{
				StringLookupNode *subNode = child->subNodes[0];

				child->str += subNode->str;
				child->leaf = subNode->leaf;
				child->subNodes.CopyList(subNode->subNodes);
				subNode->subNodes.Clear();
				delete subNode;
			}
		}
	}
	else if (!child->subNodes.Num() && !child->leaf)
	{
		node->subNodes.Remove(childID);
		delete child;
	}

	//if not a leaf node and only have one child node, then merge with child node
	if (!node->leaf && node->subNodes.Num() == 1 && node != top)
	{
		StringLookupNode *subNode = node->subNodes[0];

		node->str += subNode->str;
		node->leaf = subNode->leaf;
		node->subNodes.CopyList(subNode->subNodes);
		subNode->subNodes.Clear();
		delete subNode;
	}
}

//ugh yet more string parsing, you think you escape it for one minute and then bam!  you discover yet more string parsing code needs to be written
BOOL LocaleStringLookup::LoadStringFile(CTSTR lpFile, bool bClear)
{
	if (bClear)
	{
		cache.Clear();
		delete top;
		top = new StringLookupNode;
	}
	else if (!top)
		top = new StringLookupNode;

	//------------------------

	XFile file;

	if (!file.Open(lpFile, XFILE_READ, XFILE_OPENEXISTING))
		return FALSE;

	String fileString;
	file.ReadFileToString(fileString);
	file.Close();

	if (fileString.IsEmpty())
		return FALSE;

	//------------------------

	fileString.FindReplace(TEXT("\r"), TEXT(" "));

	TSTR lpTemp = fileString.Array() - 1;
	TSTR lpNextLine;

	do
	{
		++lpTemp;
		lpNextLine = schr(lpTemp, '\n');

		while (*lpTemp == ' ' || *lpTemp == L'　' || *lpTemp == '\t')
			++lpTemp;

		if (!*lpTemp || *lpTemp == '\n') continue;

		if (lpNextLine) *lpNextLine = 0;

		//----------

		TSTR lpValueStart = lpTemp;
		while (*lpValueStart && *lpValueStart != '=')
			++lpValueStart;

		String lookupVal, strVal;

		TCHAR prevChar = *lpValueStart;
		*lpValueStart = 0;
		lookupVal = lpTemp;
		*lpValueStart = prevChar;
		lookupVal.KillSpaces();

		String value = ++lpValueStart;
		value.KillSpaces();
		if (value.IsValid() && value[0] == '"')
		{
			value = String::RepresentationToString(value);
			strVal = value;
		}
		else
			strVal = value;

		if (lookupVal.IsValid())
			AddLookupString(lookupVal, strVal);

		//----------

		if (lpNextLine) *lpNextLine = '\n';
	} while (lpTemp = lpNextLine);

	//------------------------

	return TRUE;
}


StringLookupNode *LocaleStringLookup::FindNode(CTSTR lookupVal, StringLookupNode *node) const
{
	if (!node) node = top;

	StringLookupNode *child = node->FindSubNode(lookupVal);
	if (child)
	{
		lookupVal += child->str.Length();
		TCHAR ch = *lookupVal;
		if (ch)
			return FindNode(lookupVal, child);

		if (child->leaf)
			return child;
	}

	return NULL;
}

void LocaleStringLookup::AddLookupString(CTSTR lookupVal, CTSTR lpVal)
{
	assert(lookupVal && *lookupVal);

	if (!lookupVal || !*lookupVal)
		return;

	StringLookupNode *child = FindNode(lookupVal);
	if (child)
		child->leaf->strValue = lpVal;
	else
	{
		LocaleStringItem *item = new LocaleStringItem;
		item->lookup = lookupVal;
		item->strValue = lpVal;
		cache << item;

		AddLookup(item->lookup, item);
	}
}

CTSTR LocaleStringLookup::LookupString(CTSTR lookupVal)
{
	StringLookupNode *child = FindNode(lookupVal);
	if (!child)
		return TEXT("(string not found)");

	if (!child->leaf)
		return TEXT("(lookup error)");

	return child->leaf->strValue;
}

BOOL LocaleStringLookup::HasLookup(CTSTR lookupVal) const
{
	return (FindNode(lookupVal) != NULL);
}


void LocaleStringLookup::RemoveLookupString(CTSTR lookupVal)
{
	RemoveLookup(lookupVal);
}


const LocaleStringCache& LocaleStringLookup::GetCache() const
{
	return cache;
}
