#ifndef RTMPPUBLISHERVECTOR_H
#define RTMPPUBLISHERVECTOR_H
#include<list>
#include "TThread.h"
#include "RtmpPublish.h"

class RTMPPublisher;
class Rebuild;
//编码器索引
enum EncodIndex { EncodMajor = 0, EncodMinor };

class RTMPPublisherVectorBase  :public ClosableStream
{
	friend CInstanceProcess;
public: 
	RTMPPublisherVectorBase():bBack(false){}
	virtual void SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type, int group) = 0;
	virtual void BeginPublishing() {}
	virtual double GetPacketStrain(int streamIndex)  = 0;
	virtual QWORD GetCurrentSentBytes(int streamIndex) = 0;
	virtual DWORD NumDroppedFrames(int streamIndex)  = 0;
	virtual DWORD NumTotalVideoFrames(int streamIndex)  = 0;
	virtual int NumStream()  = 0;
	virtual void SetStoped(){}
	virtual bool SetStoped(uint64_t ID){ return false; }
	virtual void ResetNoDelayPublisher(CInstanceProcess *instance,bool bBackUp) {}
	virtual ~RTMPPublisherVectorBase() {}
	virtual void GetRtmpPublist(std::vector<RTMPPublisher *> &PublisherList){}
	virtual void ReConnectPuhlisher(const String &Prefix) {}
protected:
	bool bBack;
};

class RTMPPublisherVector : public RTMPPublisherVectorBase
{
public:
	RTMPPublisherVector();
	virtual ~RTMPPublisherVector();
	//高清设置在组EncodHEIGHT中，标清设置在组EncodLOW中
	virtual void SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type, int group);
	double GetPacketStrain(int streamIndex);
	QWORD GetCurrentSentBytes(int streamIndex);
	DWORD NumDroppedFrames(int streamIndex);
	DWORD NumTotalVideoFrames(int streamIndex);
	int NumStream();
	void SetStoped();
	virtual bool SetStoped(uint64_t ID);
	void ResetNoDelayPublisher(CInstanceProcess *instance, bool bBackUp);
	virtual void GetRtmpPublist(std::vector<RTMPPublisher *> &PublisherList);
	void AddRTMPPublisher(RTMPPublisher* publisher);
	virtual void ReConnectPuhlisher(const String &Prefix);
private:
	std::list<RTMPPublisher*> Publishers;
	CRITICAL_SECTION hBuildMutex;
	Rebuild* mRebuild;
};

class Rebuild
{
public:
	Rebuild(std::list<RTMPPublisher*>& publishers, CRITICAL_SECTION& buildMutex);
	~Rebuild();
	void operator()();
	void Build(RTMPPublisher* publisher);

private:
	TThread<Rebuild>* m_Thread;//新线程
	HANDLE hRebuildEvent;
	CRITICAL_SECTION& hBuildMutex;
	CRITICAL_SECTION hArriveMutex;
	std::list<RTMPPublisher*>& Publishers;
	volatile bool bShutdownThread;
	volatile bool bRunning;
	std::list<RTMPPublisher*> mPublishers;
};
RTMPPublisherVectorBase* CreateRTMPPublisher(CInstanceProcess *Instance, bool bBackUp);
RTMPPublisherVectorBase* CreateDelayedPublisher(DWORD delayTime, CInstanceProcess *Instance, bool bUseBack);
bool IsDelayedPublisher(RTMPPublisher *Publisher);
#endif
