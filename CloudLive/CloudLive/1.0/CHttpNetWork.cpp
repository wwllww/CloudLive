#undef  new
#include "CHttpNetWork.h"
#include "BaseAfx.h"
#include "SLiveApi.h"
#include "IExeAgent.h"
#include "Requse.h"
#include "Respond.h"
#include <fstream>
#include "IRealAgent.h"

#define STREAM_NAME "1"

#define MAX_BUFLEN      1024 * 1024 / 2
#define PRECREATECOUNT  100


extern uint64_t GetSysTickCount64();
extern IRealAgent *EXE;

typedef tbb::concurrent_hash_map<AIOID, __TimeInfo,hash_compare_socket> SocketMap;

CHttpNetWork *CHttpNetWork::m_Intances = NULL;

CHttpNetWork * CHttpNetWork::GetInstance()
{
	if (!m_Intances)
		return m_Intances = new CHttpNetWork;

	return m_Intances;
}

void CHttpNetWork::HttpNetWorkRelese()
{
	delete m_Intances;

	if (EXE)
		delete EXE;
}

CHttpNetWork::CHttpNetWork()
{
	SLiveInitException();
	EXE = new IRealAgent;
    int nPreCreate = PRECREATECOUNT;
	while( nPreCreate-- )
	{
		__PDATAINFO Data = new __DATAINFO;
		Data->buf = new BYTE[MAX_BUFLEN];
		Data->len = MAX_BUFLEN;
		Data->CanWrite = true;

		m_DataQueue.push(Data);
	}

	CenterManagerId = -1;
	ServerIp = "127.0.0.1";
	ServerPort = 8088;
}

CHttpNetWork::~CHttpNetWork()
{
	while( !m_DataQueue.empty() )
	{
		__PDATAINFO Data = NULL;

		m_DataQueue.try_pop(Data);

		if(Data)
		{
			 if(Data->buf)
				delete [] Data->buf;

			delete Data;
		}
	}
}

int CHttpNetWork::InitLive()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);
	if(bHasInit)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Has already Init", __FUNCTION__);
		return -1;
	}

	//读取配置文件

	std::ifstream ConfigFile(L".\\ConfigFile\\Config.json");
	
	if (!ConfigFile)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "Config.json 打开失败", __FUNCTION__);
		std::cout << "Config.json 打开失败"<< std::endl;
		return -1;
	}
	ConfigFile.seekg(0,std::ios::end);
	int FileSize = ConfigFile.tellg();
	char *ReadBuf = new char[FileSize + 1];

	ZeroMemory(ReadBuf, FileSize + 1);

	ConfigFile.seekg(0,std::ios::beg);

	ConfigFile.read(ReadBuf,FileSize);

	ConfigFile.close();


	std::cout << ReadBuf << std::endl;

	Json::Reader JReader;
	Json::Value JValue;

	if (JReader.parse(ReadBuf, JValue))
	{

		delete [] ReadBuf;

		ServerIp = JValue["serverip"].asString();

		if (ServerIp.empty())
		{
			std::cout << "ServerIp 为空" << std::endl;
			return -1;
		}

		ServerPort = JValue["serverport"].asUInt();

		if (0 == ServerPort)
		{
			std::cout << "ServerPort = 0" << std::endl;
			return -1;
		}

		ServerId = JValue["serverid"].asString();

		if (ServerId.empty())
		{
			std::cout << "ServerId 为空" << std::endl;
			return -1;
		}

		uListenPort = JValue["httpmsgport"].asUInt();

		if (0 == uListenPort)
		{
			std::cout << "uListenPort = 0" << std::endl;
			return -1;
		}

		MediaPort = JValue["mediaport"].asUInt();

		if (0 == MediaPort)
		{
			std::cout << "MediaPort = 0" << std::endl;
			return -1;
		}
	}
	else
	{
		delete [] ReadBuf;
		std::cout << "解析配置文件失败" << std::endl;
		return -1;
	}


	std::ifstream DefaultSencesFile(L".\\ConfigFile\\DefaultSences.json");

	if (!DefaultSencesFile)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "DefaultSences.json 打开失败", __FUNCTION__);
		std::cout << "DefaultSences.json 打开失败" << std::endl;
		return -1;
	}
	DefaultSencesFile.seekg(0, std::ios::end);
	FileSize = DefaultSencesFile.tellg();
	ReadBuf = new char[FileSize + 1];

	ZeroMemory(ReadBuf, FileSize + 1);

	DefaultSencesFile.seekg(0, std::ios::beg);

	DefaultSencesFile.read(ReadBuf, FileSize);

	DefaultSencesFile.close();

	if (JReader.parse(ReadBuf, DefaultSences))
	{
		delete [] ReadBuf;
	}
	else
	{
		delete[] ReadBuf;
		std::cout << "解析 DefaultSences.json 场景文件失败" << std::endl;
		return -1;
	}

	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) 
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s WSAStartup failed with error: %d", __FUNCTION__, err);
		return -1;
	}

	TcpControl = create_tcp_control2(&TcpSocketControlCallBack);

	if(!TcpControl)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s create_tcp_control failed!", __FUNCTION__);
		return -1;
	}

	AioAddr Addr;
	memset(&Addr,0,sizeof AioAddr);

	Addr.m_port = uListenPort;
	Addr.m_ip = ntohl(inet_addr("0.0.0.0"));
	Addr.m_v4 = true;

	if(!TcpControl->listen_tcp(Addr,5))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s listen_tcp failed!", __FUNCTION__);
		return -1;
	}

	TcpControl->read_timeout(10000);
	TcpControl->write_timeout(5000);

	bHasInit = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
	return 0;
}

void CHttpNetWork::UnInitLive()
{
	if(bHasInit)
	{
		bRuning = false;

		if(HAccept)
		{
			if(WAIT_TIMEOUT == WaitForSingleObject(HAccept, 1000))
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "HAccept 线程 WAIT_TIMEOUT 强杀");
				TerminateThread(HAccept,0);
			}
		}

		if (m_ListSocket.size() && TcpControl)
		{
			SocketMap::iterator MapBegin = m_ListSocket.begin();
			SocketMap::iterator MapEnd = m_ListSocket.end();

			for (SocketMap::iterator it = MapBegin; it != MapEnd; ++it)
			{
				TcpControl->close_connect(it->first);
			}

			m_ListSocket.clear();
		}

		if (CenterManagerId != -1)
		{
			TcpControl->close_connect(CenterManagerId);
		}

		if(TcpControl)
		{
			destroy_tcp_control2(TcpControl);
		}

		TcpControl = NULL;

		WSACleanup();

		bHasInit = false;
	}
}

DWORD CHttpNetWork::CloseSocketThread(LPVOID Param)
{
	AIOID Id = *(AIOID*)Param;
	const char *Error = "HTTP/1.1 404 Not Found\r\n"
		"Access-Control-Allow-Origin: *\r\n\r\n";

	Sleep(3000);

	size_t len = strlen(Error);
	__DATAINFO *Data = new __DATAINFO;
	Data->buf = new BYTE[len];
	Data->SendStatus = SEND_NOFOUND;
	memcpy(Data->buf ,Error,len);

	CHttpNetWork::GetInstance()->TcpControl->asyn_write(Id, (char*)Data->buf, len, 0, (ULL64)Data);
	return 0;
}

void CHttpNetWork::ParseResquest(const char *recvbuf, int buflen, std::string &CmdName, bool bGet)
{
	if (bGet)
		recvbuf += 4;
	else
	{
		recvbuf += 5;
	}

	char Tem[1024] = {0};
	char *pLine = strstr((char*)recvbuf, "\r\n");
	int HeaderLen = pLine - recvbuf;
	memcpy(Tem, recvbuf, HeaderLen);
	int i = 0;
	for (; i < HeaderLen; ++i)
	{
		if(Tem[i] == ' ')
		{
			Tem[i] = '\0';
			break;
		}
	}

	CmdName = Tem;

	size_t Last = CmdName.find_last_of('/');

	if (Last != -1)
	{
		if (!bGet)
			CmdName = CmdName.substr(Last + 1, CmdName.length() - 1);
		else
		{
			size_t Pos = CmdName.find_first_of('?');

			if (Pos != -1)
			{
				CmdName = CmdName.substr(Last + 1, Pos - Last - 1);
			}
		}
	}
}

void CHttpNetWork::WaitAndCloseSocket(SOCKET sock)
{
	HANDLE HT = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)CloseSocketThread,&sock,0,NULL);

	if(HT)
		CloseHandle(HT);
}


bool CHttpNetWork::DoAccept(AIOID Id)
{
	bool bFind = false;
	{
		SocketMap::accessor SocketAccessor;
		bFind = m_ListSocket.find(SocketAccessor, Id);
	}

	if (!bFind)
	{
		SocketMap::accessor SocketAccessor;
		if (m_ListSocket.insert(SocketAccessor, Id))
		{
			SocketAccessor->second.bFirstSend = true;
			SocketAccessor->second.bSendHeader = false;
			SocketAccessor->second.FirstTimeStamp = 0;
		}
	}
	return true;
}

void CHttpNetWork::DoAcceptProcess(AIOID Id, char *recbuf, int iRecLen, ULL64 ctx1)
{
	recbuf[iRecLen] = 0;
	if(iRecLen > 3)
	{
		bool bGet = strncmp(recbuf, "GET",3) == 0;
		bool bPost = false;
		if (!bGet)
		{
			bPost = iRecLen > 4 && (strncmp(recbuf, "POST", 4) == 0);
		}

		if (bGet || bPost)
		{
			__MsgInfo Msg;
			Msg.id = Id;
			Msg.Msgbuf = recbuf;
			Msg.MsgLen = iRecLen;
			Msg.Context = ctx1;
			Msg.bPost = bPost;

			m_MsgQueue.push(Msg);

		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, " %s 不是GET 请求 recbuf %s", __FUNCTION__, recbuf);
			WaitAndCloseSocket(Id);
		}
	}
	else
	{
		Log::writeMessage(LOG_RTSPSERV, 1, " %s 接收请求数据太小 recbuf %s", __FUNCTION__, recbuf);
		WaitAndCloseSocket(Id);
	}
}

int CHttpNetWork::HttpMsgLoop()
{
	DEBUG_PRINT("进入消息循环...");
	
	CRequse Req;
	CRespond Res;
	__MsgInfo MsgInfo;
	std::string CmdName;
	uint64_t StartTime = GetSysTickCount64();
	uint64_t EndReprotTime;
	Req.SetProcessClassName("WebInterFaceProcess");

	while (bRuning)
	{
		if (m_MsgQueue.try_pop(MsgInfo))
		{
			DEBUG_PRINT(MsgInfo.Msgbuf);
			Req.SetParam(MsgInfo.Msgbuf);

			ParseResquest(MsgInfo.Msgbuf, MsgInfo.MsgLen, CmdName,!MsgInfo.bPost);


			Req.SetCmd(CmdName.c_str());
			Res.SetResType(RES_FAILED);

			QueryInterface()->ExecuseInvoke(Req, Res);

			const char *Respond = "HTTP/1.1 200 OK\r\n"
				"Content-Type: application/json\r\n"
				"Content-Length: %d\r\n"
				"Accept-Language: zh-CN,en,*\r\n"
				"User-Agent: Mozilla/5.0\r\n\r\n"
				"%s";

			__PDATAINFO DataInfo = (__PDATAINFO)MsgInfo.Context;

			ZeroMemory(DataInfo->buf, DataInfo->len);
			DataInfo->SendStatus = SEND_OK;

			std::string &StrRes = Res.GetRespond().toStyledString();

			//这里要把ANSI转成UTF-8

			char *Utf_8 = GBToUTF8(StrRes.c_str());

			if (Utf_8)
			{
				sprintf_s((char*)DataInfo->buf, DataInfo->len, Respond, strlen(Utf_8), Utf_8);

				delete[] Utf_8;
			}
			else
			{
				sprintf_s((char*)DataInfo->buf, DataInfo->len, Respond, StrRes.length(), StrRes.c_str());
			}

			DEBUG_PRINT((const char*)DataInfo->buf);

			TcpControl->asyn_write(MsgInfo.id, (char*)DataInfo->buf, strlen((char*)DataInfo->buf), (ULL64)this, MsgInfo.Context);

			if (CmdName.compare("closeswitcher") == 0 && Res.GetResType() == RES_OK)
			{
				DEBUG_PRINT("closeswitcher 销毁实例");
				QueryInterface()->DeleteClass("WebInterFaceProcess");

				CloudId.clear();
			}
			else if (CmdName.compare("initswitcher") == 0 && Res.GetResType() == RES_OK)
			{
				//取出cloudswitcherid
				CloudId = Res.GetRespond()["cloudswitcherid"].asString();

				//成功之后上报一次
				ReportSwitcherStatus();
			}
		}

		//可以在这里5秒上报一次状态
		EndReprotTime = GetSysTickCount64();

		if (EndReprotTime - StartTime >= 5000)
		{
			ReportSwitcherStatus();
			StartTime = EndReprotTime;
		}

		Sleep(1);
	}

	DEBUG_PRINT("退出消息循环,释放资源程序即将关闭");
	return 0;
}

void CHttpNetWork::ResetDataBuf(__PDATAINFO Data)
{
	if (!Data)
		return;
	Data->CanWrite = true;
	Data->BufRealLen = 0;
	ZeroMemory(Data->buf, Data->len);
}

void CHttpNetWork::SetExit(bool bExit)
{
	bRuning = !bExit;
}

void CHttpNetWork::SendCommandToManager(const Json::Value &Param,const char* Command)
{
	if (CenterManagerId == -1)
	{
		AioAddr ServerAddr;
		memset(&ServerAddr, 0, sizeof AioAddr);

		ServerAddr.m_port = ServerPort;
		ServerAddr.m_ip = ntohl(inet_addr(ServerIp.c_str()));
		ServerAddr.m_v4 = true;


		__PDATAINFO Data;
		m_DataQueue.try_pop(Data);
		if (Data && Data->CanWrite)
		{
			Data->CanWrite = false;
			m_DataQueue.push(Data);

			Data->SendStatus = SEND_DATA;


			const char *Post = "POST %s HTTP/1.1\r\n"
				"Content-Type: application/json\r\n"
				"Content-Length: %d\r\n"
				"Accept: text/html, application/xhtml+xml, */*\r\n"
				"Connection: Keep-Alive\r\n"
				"Accept-Language: zh-CN,en,*\r\n"
			    "Host: %s:%d\r\n"
				"User-Agent: Mozilla/5.0\r\n\r\n"
				"%s";

			std::string &strParam = Param.toStyledString();

			char *Utf_8 = GBToUTF8(strParam.c_str());

			if (Utf_8)
			{
				sprintf_s((char*)Data->buf, Data->len, Post, Command, strlen(Utf_8),ServerIp.c_str(),ServerPort, Utf_8);

				delete[] Utf_8;
			}
			else
			{
				sprintf_s((char*)Data->buf, Data->len, Post, Command, strParam.length(), ServerIp.c_str(), ServerPort, strParam.c_str());
			}


			TcpControl->asyn_connect(ServerAddr, AioAddr(), ULL64(this),ULL64(Data));
		}
		else
		{
			if (Data)
			{
				m_DataQueue.push(Data);
			}

			Log::writeError(((long long)1 << 49), 1, "当时队列不可写 %s %d", __FUNCTION__, __LINE__);
		}

	}


	if (CenterManagerId != -1)
	{
		__PDATAINFO Data;
		m_DataQueue.try_pop(Data);
		if (Data && Data->CanWrite)
		{
			Data->CanWrite = false;
			m_DataQueue.push(Data);

			Data->SendStatus = SEND_DATA;


			const char *Post = "POST %s HTTP/1.1\r\n"
				"Content-Type: application/json\r\n"
				"Content-Length: %d\r\n"
				"Accept: text/html, application/xhtml+xml, */*\r\n"
				"Connection: Keep-Alive\r\n"
				"Accept-Language: zh-CN,en,*\r\n"
				"Host: %s:%d\r\n"
				"User-Agent: Mozilla/5.0\r\n\r\n"
				"%s";

			std::string &strParam = Param.toStyledString();

			char *Utf_8 = GBToUTF8(strParam.c_str());

			if (Utf_8)
			{
				sprintf_s((char*)Data->buf, Data->len, Post, Command, strlen(Utf_8), ServerIp.c_str(), ServerPort, Utf_8);

				delete[] Utf_8;
			}
			else
			{
				sprintf_s((char*)Data->buf, Data->len, Post, Command, strParam.length(), ServerIp.c_str(), ServerPort, strParam.c_str());
			}

			TcpControl->asyn_write(CenterManagerId, (char*)Data->buf, strlen((char*)Data->buf), (ULL64)this, (ULL64)Data);
		}
		else
		{
			if (Data)
			{
				m_DataQueue.push(Data);
			}

			Log::writeError(((long long)1 << 49), 1, "当时队列不可写 %s %d", __FUNCTION__, __LINE__);
		}
	}
}

void CHttpNetWork::ReportSwitcherStatus()
{
	Json::Value Status;
	Status["cloudswitcherid"] = CloudId.c_str();
	Status["serverid"] = ServerId.c_str();//读取配置文件
	if (CloudId.empty())
		Status["workstatus"] = 1;
	else
	{
		Status["workstatus"] = 2;
	}
	SendCommandToManager(Status, "/bcs/internal/reportswitcherstatus");
}


char* CHttpNetWork::GBToUTF8(const char* gb2312)
{
	int len = MultiByteToWideChar(CP_ACP, 0, gb2312, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	if (!wstr)
		return NULL;
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_ACP, 0, gb2312, -1, wstr, len);
	len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	if (!str)
	{
		delete[] wstr;
		return NULL;
	}
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	delete[] wstr;
	return str;
}

char* CHttpNetWork::UTF8ToGB(const char* utf8)
{
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	wchar_t* wstr = new wchar_t[len + 1];
	if (!wstr)
		return NULL;
	memset(wstr, 0, len + 1);
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, len);
	len = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	if (!str)
	{
		delete[] wstr;
		return NULL;
	}
	memset(str, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, wstr, -1, str, len, NULL, NULL);
	delete[] wstr;
	return str;
}

UINT CHttpNetWork::GetMediaPort() const
{
	return MediaPort;
}

Json::Value CHttpNetWork::GetDefaultSences() const
{
	return DefaultSences;
}

