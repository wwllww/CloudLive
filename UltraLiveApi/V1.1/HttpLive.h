#ifndef HTTPLIVE_H
#define HTTPLIVE_H
#include "TCPSocketControl.h"
#include <queue>
#include "concurrent_hash_map.h"
#include "concurrent_queue.h"

class CInstanceProcess;
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

class HttpLive
{
	friend class CTCPSocketControl;
public:
	HttpLive(CInstanceProcess *Process);
	~HttpLive();
public:
	static bool InitLive(UINT uListenPort);
	static void UnInitLive();
	bool SendPacket(LPBYTE data, UINT size, UINT Timestamp, bool bVideo);
	static bool bHasInit;
protected:
	static void ParseResquest(const char *recvbuf,int buflen,std::string &StreamName);
	static void WaitAndCloseSocket(SOCKET sock);
	//static DWORD AcceptThread(LPVOID Param);
	static DWORD CloseSocketThread(LPVOID Param);
	static void DoAcceptProcess(AIOID Id,char *recbuf,int iRecLen);
	UINT InitMetaData();
	char* EncMetaData(char *enc, char *pend,bool bFLVFile);
	bool SendFLVHeader(AIOID Id);
	int PacketFLVData(const BYTE *lpData, UINT size, BYTE type, DWORD timestamp,BYTE *DataBuf);
	unsigned char* GetNextStartCode(unsigned char* buf,int len, int& nStartCodeLen);
	bool ProcessVideoHeader();
	bool ProcessAudioHeader();
	bool ProcessData( AIOID Id,unsigned char* buf,int len,uint64_t timestamp,bool bVideo);
	bool ProcessVideo(unsigned char* buf,int len,int& StartFrame);
	bool DoAccept(AIOID Id);
private:
	static bool bRuning;
	static SOCKET ListenSocket;
	static char FLVHeader[14];
	static HANDLE HAccept;
	static CTCPSocketControl TcpSocketControlCallBack;
	static TcpControl2 *TcpControl;
	bool bFirstSend;
	uint64_t FirstTimeStamp;
	tbb::concurrent_hash_map<AIOID, __TimeInfo,hash_compare_socket> m_ListSocket;
	std::string BindChannel;
	char MetaData[2048];
	int Width;
	int Height;
	int VideoBitRate;
	int AudioSamplerate;
	int NumChannels;
	int AudioRate;
	std::vector<BYTE> VideoHeader;
	std::vector<BYTE> AudioHeader;
	int OldMallocSize;
	tbb::concurrent_queue<__PDATAINFO> m_DataQueue;
	//std::queue<__PDATAINFO> m_DataQueue;
	CInstanceProcess *Process;
};

#endif