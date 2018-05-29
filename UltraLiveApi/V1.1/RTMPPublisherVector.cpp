#include "RTMPStuff.h"
#include "RTMPPublisherVector.h"
#include "OperatNew.h"
#include <thread>

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

void Rebuild::operator()() //多线程
{
	bRunning = true;

	while ( !bShutdownThread)
	{
		DWORD result = WaitForSingleObject(hRebuildEvent, 1000);
		if (result == WAIT_OBJECT_0)
		{
			std::list<RTMPPublisher*> LocalPublishers;
			EnterCriticalSection(&hArriveMutex);
			LocalPublishers.swap(mPublishers);
			LeaveCriticalSection(&hArriveMutex);

			std::list<RTMPPublisher*>::iterator pos = LocalPublishers.begin();
			std::list<RTMPPublisher*>::iterator end = LocalPublishers.end();

			EnterCriticalSection(&hBuildMutex);
			for (; pos != end; ++pos)
			{
				RTMPPublisher* publisher = (*pos)->Clone();
				delete (*pos);
				(*pos) = NULL;
				Publishers.push_back(publisher);
			}
			LeaveCriticalSection(&hBuildMutex);
		}
	}

	bRunning = false;
}

Rebuild::Rebuild(std::list<RTMPPublisher*>& publishers, CRITICAL_SECTION& buildMutex) :Publishers(publishers), hBuildMutex(buildMutex)
{
	InitializeCriticalSection(&hArriveMutex);
	bRunning = false;
	hRebuildEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	bShutdownThread = false;
	bRunning = false;
	m_Thread = new TThread<Rebuild>(this, &Rebuild::operator(), false);
}

Rebuild::~Rebuild()
{
	bShutdownThread = true;
	SetEvent(hRebuildEvent);

	while (bRunning)
	{
		SetEvent(hRebuildEvent);
		Sleep(10);
	}

	if (m_Thread)
	{
		delete m_Thread;
	}

	if (hRebuildEvent)
		CloseHandle(hRebuildEvent);

	DeleteCriticalSection(&hArriveMutex);
}

void Rebuild::Build(RTMPPublisher* publisher)
{
	EnterCriticalSection(&hArriveMutex);
	mPublishers.push_back(publisher);
	LeaveCriticalSection(&hArriveMutex);
	SetEvent(hRebuildEvent);
}


RTMPPublisherVector::RTMPPublisherVector()
{
	InitializeCriticalSection(&hBuildMutex);
	mRebuild = new Rebuild(Publishers, hBuildMutex);
}

void RTMPPublisherVector::AddRTMPPublisher(RTMPPublisher* publisher)
{
	Publishers.push_back(publisher);
}

RTMPPublisherVector::~RTMPPublisherVector()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始析构", __FUNCTION__);
	if (mRebuild)
	{
		delete mRebuild;
		mRebuild = NULL;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始遍历Network, size = %d", __FUNCTION__,Publishers.size());
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 找到Network并delete", __FUNCTION__);
		
		if (IsDelayedPublisher(*pos))
		{
			std::thread TDelete([](RTMPPublisher *Publisher){if (Publisher) delete Publisher; }, *pos);
			TDelete.detach();
		}
		else
		{
			delete (*pos);
		}
	}

	Publishers.clear();

	
	LeaveCriticalSection(&hBuildMutex);

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 结束遍历Network", __FUNCTION__);

	DeleteCriticalSection(&hBuildMutex);
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 结束析构", __FUNCTION__);
}

void RTMPPublisherVector::SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type, int group)
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if ((*pos)->GetLastError())
		{
			mRebuild->Build((*pos));
			(*pos) = NULL;
			pos = Publishers.erase(pos);
		}

		if (pos != end && ((group == EncodMajor && (*pos)->IsMain()) || (group == EncodMinor && !((*pos)->IsMain()))))
		{
			(*pos)->SendPacket(data, size, timestamp, type);
		}		
	}

	LeaveCriticalSection(&hBuildMutex);
}

int RTMPPublisherVector::NumStream()
{
	int result = 0;

	EnterCriticalSection(&hBuildMutex);
	result = Publishers.size();
	LeaveCriticalSection(&hBuildMutex);

	return result;
}

double RTMPPublisherVector::GetPacketStrain(int streamIndex)
{
	double result = 0;
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if ((*pos)->GetIndex() == streamIndex)
		{
			result = (*pos)->GetPacketStrain();
			break;
		}		
	}

	LeaveCriticalSection(&hBuildMutex);
	return result;
}

QWORD RTMPPublisherVector::GetCurrentSentBytes(int streamIndex)
{
	QWORD result = 0;
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if ((*pos)->GetIndex() == streamIndex)
		{
			result = (*pos)->GetCurrentSentBytes();
			break;
		}		
	}

	LeaveCriticalSection(&hBuildMutex);
	return result;
}

DWORD RTMPPublisherVector::NumDroppedFrames(int streamIndex)
{
	QWORD result = 0;
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if ((*pos)->GetIndex() == streamIndex)
		{
			result = (*pos)->NumDroppedFrames();
			break;
		}		
	}

	LeaveCriticalSection(&hBuildMutex);
	return result;
}

DWORD RTMPPublisherVector::NumTotalVideoFrames(int streamIndex)
{
	QWORD result = 0;
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if ((*pos)->GetIndex() == streamIndex)
		{
			result = (*pos)->NumTotalVideoFrames();
			break;
		}
	}

	LeaveCriticalSection(&hBuildMutex);
	return result;
}

void RTMPPublisherVector::SetStoped()
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		(*pos)->SetStop(true);
	}

	LeaveCriticalSection(&hBuildMutex);
}

bool RTMPPublisherVector::SetStoped(uint64_t ID)
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if (uint64_t(*pos) == ID)
		{
			(*pos)->SetStop(true);
			LeaveCriticalSection(&hBuildMutex);
			return true;
		}
	}

	LeaveCriticalSection(&hBuildMutex);
	return false;
}

void RTMPPublisherVector::ResetNoDelayPublisher(CInstanceProcess *instance, bool bBackUp)
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		delete (*pos);
	}

	Publishers.clear();

	int PublishCount = 2;

	int Rendering = 0;
	String prefix;
	for (int i = 0; i < PublishCount; ++i)
	{
		prefix = String(TEXT("Publish")) + IntString(i);
		if (!bBackUp)
		{
			if (i == 0)
			{
				Rendering = instance->LiveParam.LiveSetting.bUsePush ? 1 : 0;
			}
			else
			{
				Rendering = instance->LiveParam.LiveSetting.bUseBackPush ? 1 : 0;
			}
		}
		else
		{
			if (i == 0)
			{
				Rendering = instance->LiveParam.LiveSetting.bUsePushSec ? 1 : 0;
			}
			else
			{
				Rendering = instance->LiveParam.LiveSetting.bUseBackPushSec ? 1 : 0;
			}
		}
		if (Rendering)
		{
			AddRTMPPublisher(new RTMPPublisher(prefix, instance, bBackUp));
		}
	}
	LeaveCriticalSection(&hBuildMutex);
}

void RTMPPublisherVector::GetRtmpPublist(std::vector<RTMPPublisher *> &PublisherList)
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	for (; pos != end; ++pos)
	{
		if (IsDelayedPublisher(*pos))
				PublisherList.push_back(*pos);
	}

	LeaveCriticalSection(&hBuildMutex);
}

void RTMPPublisherVector::ReConnectPuhlisher(const String &Prefix)
{
	EnterCriticalSection(&hBuildMutex);

	std::list<RTMPPublisher*>::const_iterator pos = Publishers.begin();
	std::list<RTMPPublisher*>::const_iterator end = Publishers.end();

	RTMPPublisher *NewPublisher = NULL;
	for (; pos != end; ++pos)
	{
		String &PublisherPrefix = (*pos)->GetPrefix();

		if (PublisherPrefix.CompareI(Prefix.Array()))
		{
			NewPublisher = (*pos)->CloneWithNoDelayConnect();
			delete (*pos);
			Publishers.erase(pos);
			break;
		}
	}

	if (NewPublisher)
	{
		Publishers.push_back(NewPublisher);
	}

	LeaveCriticalSection(&hBuildMutex);
}

