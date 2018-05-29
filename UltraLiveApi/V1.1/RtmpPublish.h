#ifndef RTMPPUBLISH_H
#define RTMPPUBLISH_H
#include <Iphlpapi.h>
#include "Encoder.h"
#include <winsock2.h>
#include "rtmp.h"

struct NetworkPacket
{
	List<BYTE> data;
	DWORD timestamp;
	PacketType type;
	UINT distanceFromDroppedFrame;
};

struct StIPAddr
{
	std::wstring m_strIpAddr;
	unsigned    m_nPort;
};
typedef std::vector<StIPAddr>  VEC_IP_ADDR;
typedef VEC_IP_ADDR::iterator  IT_VEC_IP_ADDR;

class CSectionLock
{
public:
	CSectionLock()
	{
		InitializeCriticalSection(&m_Section);
	}
	~CSectionLock()
	{
		DeleteCriticalSection(&m_Section);
	}

	void Lock() {
		EnterCriticalSection(&m_Section);
	}

	void UnLock()
	{
		LeaveCriticalSection(&m_Section);
	}

private:
	CRITICAL_SECTION m_Section;
};

const DWORD maxBufferTime = 600;

typedef enum
{
	LL_MODE_NONE = 0,
	LL_MODE_FIXED,
	LL_MODE_AUTO,
} latencymode_t;


class ClosableStream
{
public:
	virtual ~ClosableStream() {}
};

class VideoFileStream : public ClosableStream
{
public:
	virtual ~VideoFileStream() {}
	virtual void AddPacket(const BYTE *data, UINT size, DWORD timestamp, DWORD pts, PacketType type) = 0;
	virtual void AddPacket(std::shared_ptr<const std::vector<BYTE>> data, DWORD timestamp, DWORD pts, PacketType type)
	{
		AddPacket(data->data(), static_cast<UINT>(data->size()), timestamp, pts, type);
	}
};

class NetworkStream : public ClosableStream
{
public:
	virtual ~NetworkStream() {}
	virtual void SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type) = 0;
	virtual void BeginPublishing() {}

	virtual double GetPacketStrain() const = 0;
	virtual QWORD GetCurrentSentBytes() = 0;
	virtual DWORD NumDroppedFrames() const = 0;
	virtual DWORD NumTotalVideoFrames() const = 0;
};


class RTMPPublisher : public NetworkStream
{
	void BeginPublishingInternal();

	static int BufferedSend(RTMPSockBuf *sb, const char *buf, int len, RTMPPublisher *network);

	static String strRTMPErrors;
	static HANDLE m_handleLog;

	static void librtmpErrorCallback(int level, const char *format, va_list vl);
	static String GetRTMPErrors();
	static int  ReTryCount;

private:

	// 解析json格式
	static bool parseRTMPSrvAddr(String strJson, VEC_IP_ADDR &vecIp);

	volatile bool bError;
	CRITICAL_SECTION hErrorMutex;
	void TriggerError();
	bool bDestroy;
	bool bIsMain;

	static std::map<std::wstring, VEC_IP_ADDR>  m_pMap_addr;
	static  HANDLE                              m_handleLock;;

protected:
	String Prefix;
	QWORD OldTime;
	int ReconnectDelayMillisecond;
	UINT ConnectedInfo;
	int SecondsConnected;
	virtual void Destructor();
	bool CloneTag = false;
	bool bUseBack;

protected:

	bool numStartFrames, bNetworkStrain;
	double dNetworkStrain;

	//-----------------------------------------------
	// stream startup stuff
	bool bStreamStarted;
	bool bConnecting, bConnected;
	DWORD firstTimestamp;
	bool bSentFirstKeyframe, bSentFirstAudio;

	List<TimedPacket> bufferedPackets; //发送的数据包
	DWORD audioTimeOffset;
	bool bBufferFull;

	bool bFirstKeyframe;
	UINT FindClosestQueueIndex(DWORD timestamp);
	UINT FindClosestBufferIndex(DWORD timestamp);
	void InitializeBuffer();
	void SendPacketForReal(BYTE *data, UINT size, DWORD timestamp, PacketType type);

	bool encoderDataInitialized = false;
	std::vector<char> metaDataPacketBuffer;
	DataPacket audioHeaders, videoHeaders;
	void InitEncoderData();
	char* EncMetaData(char *enc, char *pend, bool bFLVFile, bool bBack);

	//-----------------------------------------------
	// frame drop stuff

	DWORD minFramedropTimestsamp;
	DWORD dropThreshold, bframeDropThreshold;
	List<NetworkPacket> queuedPackets;
	UINT currentBufferSize;//, outputRateWindowTime;
	UINT lastBFrameDropTime = 0;

	//-----------------------------------------------

	RTMP *rtmp = NULL;

	HANDLE hSendSempahore;
	CRITICAL_SECTION hDataMutex;
	HANDLE hSendThread;
	HANDLE hSocketThread;
	HANDLE hWriteEvent;
	HANDLE hBufferEvent;
	HANDLE hBufferSpaceAvailableEvent;
	CRITICAL_SECTION hDataBufferMutex;
	CRITICAL_SECTION hRTMPMutex;
	HANDLE hConnectionThread;

	HANDLE hSendLoopExit;
	HANDLE hSocketLoopExit;

	HANDLE hSendBacklogEvent;
	OVERLAPPED sendBacklogOverlapped;

	bool bStopping;

	int packetWaitType;

	QWORD bytesSent;
	int m_index;

	UINT totalFrames = 0;
	UINT totalVideoFrames = 0;
	UINT numPFramesDumped = 0;
	UINT numBFramesDumped = 0;

	BYTE *dataBuffer = 0;
	int dataBufferSize = 0;

	int curDataBufferLen;

	latencymode_t lowLatencyMode;
	int latencyFactor = 0;
	int totalTimesWaited = 0;
	int totalBytesWaited = 0;

	QWORD totalSendBytes;
	DWORD totalSendPeriod;
	DWORD totalSendCount;

	bool bFastInitialKeyframe;
	int VideoBitRate;
	int AudioBitRate;
	int Fps;

	CInstanceProcess *__Instances;

	void SendLoop(); //真正发送音视频数据的线程
	void SocketLoop(); //接收服务器发来的命令，名返回处理结果
	int FlushDataBuffer();
	void SetupSendBacklogEvent();
	void FatalSocketShutdown();
	static DWORD SendThread(RTMPPublisher *publisher);
	static DWORD SocketThread(RTMPPublisher *publisher);

	void DropFrame(UINT id);
	bool DoIFrameDelay(bool bBFramesOnly);

	virtual void ProcessPackets();
	virtual void FlushBufferedPackets();

	virtual void RequestKeyframe(int waitTime);
	bool Init(UINT tcpBufferSize);
	static DWORD WINAPI CreateConnectionThread(RTMPPublisher *publisher);
public:
	RTMPPublisher(String& prefix, CInstanceProcess *Instances,bool bBackPush);
	virtual ~RTMPPublisher();

	void SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type);
	void BeginPublishing();
	void SetStop(bool bStop);

	double GetPacketStrain() const;
	QWORD GetCurrentSentBytes();
	DWORD NumDroppedFrames() const;
	DWORD NumTotalVideoFrames() const { return totalVideoFrames; }
	String GetPrefix() const;
	bool GetLastError();
	virtual RTMPPublisher* Clone();
	virtual RTMPPublisher* CloneWithNoDelayConnect();
	int GetIndex()
	{
		return m_index;
	}
	// 清空map
	static void  ClearRRHash();
	bool IsMain() { return bIsMain; }
	bool ConnectedDelayed; //延迟连接
};

#endif // !RTMPPUBLISH_H
