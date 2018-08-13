#ifndef HTTPNETWORK_H
#define HTTPNETWORK_H
#include "HttpSocketControl.h"
#include <queue>
#include "concurrent_hash_map.h"
#include "concurrent_queue.h"
#include "SLiveApi.h"
#include "json.h"

#ifndef FunCall
#define FunCall(Ret) \
{\
if (Ret < 0)\
{\
	std::cout << "__Call  " << #Ret  << "  Failed!!  " << SLiveGetLastError() << std::endl; \
}else\
{\
std::cout << "__Call  " << #Ret << "  Success!!__" << std::endl; \
}\
}
#endif

struct hash_compare_socket {
	static size_t hash( SOCKET a ) { return a;}
	static bool equal( SOCKET a, SOCKET b ) { return a == b; }
};

typedef struct
{
	bool bFirstSend;
	bool bSendHeader;
	ULL64 FirstTimeStamp;
}__TimeInfo;

class CHttpNetWork
{
	friend class CTCPSocketControl;
protected:
	CHttpNetWork();
	~CHttpNetWork();
public:
	static CHttpNetWork *GetInstance();
	static void HttpNetWorkRelese();
	int InitLive();
	void UnInitLive();
	static DWORD CloseSocketThread(LPVOID Param);
	void WaitAndCloseSocket(SOCKET sock);
	void ParseResquest(const char *recvbuf, int buflen, std::string &CmdName,bool bGet);
	void DoAcceptProcess(AIOID Id, char *recbuf, int iRecLen, ULL64 ctx1);
	bool DoAccept(AIOID Id);
	void SetExit(bool bExit);
	void SendCommandToManager(const Json::Value &Param, const char* Command);
	void ReportSwitcherStatus();
	Json::Value GetDefaultSences() const;

	int HttpMsgLoop();
	UINT GetMediaPort() const;

	void ResetDataBuf(__PDATAINFO Data);
	char* GBToUTF8(const char* gb2312);
	char* UTF8ToGB(const char* utf8);

private:
	static CHttpNetWork *m_Intances;
	bool bHasInit = false;
	HANDLE HAccept = NULL;
	bool bRuning = true;
	CTCPSocketControl TcpSocketControlCallBack;
	TcpControl2 *TcpControl = NULL;
	tbb::concurrent_queue<__PDATAINFO> m_DataQueue;
	tbb::concurrent_hash_map<AIOID, __TimeInfo, hash_compare_socket> m_ListSocket;
	tbb::concurrent_queue<__MsgInfo> m_MsgQueue;

	AIOID CenterManagerId;
	std::string ServerIp;
	UINT        ServerPort;
	std::string ServerId;
	std::string CloudId;
	
	UINT uListenPort;
	UINT MediaPort;

	Json::Value DefaultSences;
};

#endif