#include "BaseAfx.h"
#include "RTMPStuff.h"
#include "RtmpPublish.h"
#include "RTMPPublisherVector.h"
#include "OperatNew.h"
#include "resource.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

CSectionLock DelaySection;

class DelayedPublisher : public RTMPPublisher
{
    DWORD delayTime;
    DWORD lastTimestamp;
    List<NetworkPacket> delayedPackets;
    bool bStreamEnding, bCancelEnd, bDelayConnected;
	bool mDestroy;
	bool bCanShowDialog = false;

	static bool bShowDialog;
	static bool bShowDialogIn;
	static bool bHasConnect;

    void ProcessDelayedPackets(DWORD timestamp)
    {
        if(bCancelEnd)
            return;

        if(timestamp >= delayTime)
        {
            if(!bConnected && !bConnecting && !bStopping)
            {
				hConnectionThread = OSCreateThread((XTHREAD)CreateConnectionThread, this);
                bConnecting = true;
            }

            if(bConnected)
            {
                if(!bDelayConnected)
                {
                    delayTime = timestamp;
                    bDelayConnected = true;
                }

                DWORD sendTime = timestamp-delayTime;
                for(UINT i=0; i < delayedPackets.Num(); ++i)
                {
                    NetworkPacket &packet = delayedPackets[i];
                    if(packet.timestamp <= sendTime)
                    {
                        RTMPPublisher::SendPacket(packet.data.Array(), packet.data.Num(), packet.timestamp, packet.type);
                        packet.data.Clear();
                        delayedPackets.Remove(i--);
                    }
                }
			}
        }
    }

protected:
	void Destructor()
	{
		if (mDestroy)
		{
			return;
		}
		mDestroy = true;
		static int iCount = 0;
		static int PublisherCounter = 0;
		DelaySection.Lock(); 
		if (bShowDialogIn && CSLiveManager::GetInstance()->LiveInstance && !CSLiveManager::GetInstance()->LiveInstance->bStartLive)// 一定要StopLive之后进入这里
		{
			bHasConnect = false;
			bShowDialogIn = false;
			PublisherCounter = CSLiveManager::GetInstance()->LiveInstance->ListPublisher.size();
			int MaxDelayTime = 0;
			for (int i = 0; i < PublisherCounter; ++i)
			{
				DelayedPublisher *Publiher = dynamic_cast<DelayedPublisher*>(CSLiveManager::GetInstance()->LiveInstance->ListPublisher[i]);
				if (Publiher)
				{
					if (!Publiher->bStopping && Publiher->bConnected && Publiher->rtmp && RTMP_IsConnected(Publiher->rtmp))
					{
						if (MaxDelayTime < Publiher->delayTime)
							MaxDelayTime = Publiher->delayTime;
						bHasConnect = true;
					}
				}
			}

			for (int i = 0; i < PublisherCounter; ++i)
			{
				DelayedPublisher *Publiher = dynamic_cast<DelayedPublisher*>(CSLiveManager::GetInstance()->LiveInstance->ListPublisher[i]);
				if (Publiher)
				{
					if (!Publiher->bStopping && Publiher->bConnected && Publiher->rtmp && RTMP_IsConnected(Publiher->rtmp))
					{
						if (MaxDelayTime == Publiher->delayTime)
						{
							Publiher->bCanShowDialog = true;
							break;
						}
					}
				}
			}
		}
		DelaySection.UnLock();

		if (!bStopping && rtmp && RTMP_IsConnected(rtmp) && bConnected)
		{
			bStreamEnding = true;
			bool bIsShowDialog = false;
			DelaySection.Lock();

			if (bShowDialog && bCanShowDialog)
			{
				bShowDialog = false;

				if (CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb)
					CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb(true,NULL,false);
				bIsShowDialog = true;

			}

			DelaySection.UnLock();

			DWORD totalTimeLeft = delayTime;

			String strTimeLeftVal = L"剩余时间：$1";

			DWORD lastTimeLeft = -1;

			DWORD firstTime = GetQPCMS();
			while (delayedPackets.Num() && !bCancelEnd)
			{
				DWORD timeElapsed = (GetQPCMS() - firstTime);

				DWORD timeLeft = (totalTimeLeft - timeElapsed) / 1000;
				DWORD timeLeftMinutes = timeLeft / 60;
				DWORD timeLeftSeconds = timeLeft % 60;

				if ((timeLeft != lastTimeLeft) && (totalTimeLeft >= timeElapsed))
				{
					String strTimeLeft = strTimeLeftVal;
					strTimeLeft.FindReplace(TEXT("$1"), FormattedString(TEXT("%u:%02u"), timeLeftMinutes, timeLeftSeconds));

					if (bIsShowDialog &&  CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb)
						CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb(false, WcharToAnsi(strTimeLeft.Array()).c_str(),false);

					lastTimeLeft = timeLeft;
				}

				ProcessDelayedPackets(lastTimestamp + timeElapsed);
				if (bStopping)
					bCancelEnd = true;

				Sleep(10);
			}

			if (!bCancelEnd)
			{
				if (bIsShowDialog && CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb)
					CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb(false, NULL, true);
			}

			bShowDialog = true;
		}
		
		for (UINT i = 0; i<delayedPackets.Num(); ++i)
			delayedPackets[i].data.Clear();

		delayedPackets.Clear();

	
		if (CSLiveManager::GetInstance()->LiveInstance && !CSLiveManager::GetInstance()->LiveInstance->bStartLive)
		{
			DelaySection.Lock();// 一定要StopLive之后进入这里

			if (++iCount == PublisherCounter)
			{
				iCount = 0;
				bShowDialogIn = true;
				PublisherCounter = 0;
			}

			if (!bHasConnect)
			{
				if (iCount == 1 || iCount == 0)
				{
					if (CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb)
						CSLiveManager::GetInstance()->BSParam.DelayPushTimeCb(false, NULL, true);
				}

			}

			CSLiveManager::GetInstance()->LiveInstance->ListPublisher.erase(remove_if(CSLiveManager::GetInstance()->LiveInstance->ListPublisher.begin(), 
				CSLiveManager::GetInstance()->LiveInstance->ListPublisher.end(),
				[=](RTMPPublisher *Publisher){ return Publisher == this; }));

			DelaySection.UnLock();
		}

	}

public:
	DelayedPublisher(DWORD delayTime, String& prefix, CInstanceProcess *Instances, bool bBackPush) : RTMPPublisher(prefix, Instances,bBackPush)
    {
		mDestroy = false;
        this->delayTime = delayTime;

		bStreamEnding = false;
		bCancelEnd = false;
		bDelayConnected = false;
		bConnecting = false;
    }

    ~DelayedPublisher()
    {
		if (mDestroy == false)
		{
			Destructor();
		}
    }

    void SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type)
    {
		if (ConnectedDelayed)
		{
			QWORD newTime = GetQPCMS();
			if (newTime - OldTime < ReconnectDelayMillisecond) //10s后在连接
			{
				NetworkPacket *newPacket = delayedPackets.CreateNew();
				newPacket->data.CopyArray(data, size);
				newPacket->timestamp = timestamp;
				newPacket->type = type;

				lastTimestamp = timestamp;

				if (timestamp - delayedPackets[0].timestamp > delayTime)
				{
					NetworkPacket &packet = delayedPackets[0];
					packet.data.Clear();
					delayedPackets.Remove(0);
				}

				int secondsConnected = (ReconnectDelayMillisecond - (newTime - OldTime)) / 1000 + 1;
				if (secondsConnected == SecondsConnected)
				{
					return;
				}
				SecondsConnected = secondsConnected;

				std::wstring strIndex = Prefix.Array();
				strIndex = strIndex.substr(7, strIndex.size() - 7);

				int Index = 1;

				if (bUseBack)
				{
					Index = 3;
				}

				String str = IntString(String(strIndex.c_str()).ToInt() + Index);

				String strInfo;
				strInfo << TEXT("URL") << str << TEXT(": ") << IntString(SecondsConnected) << TEXT("秒后重新连接...");

				if (__Instances->LiveParam.TipsCb)
				{
					__Instances->LiveParam.TipsCb(-100, WcharToAnsi(strInfo.Array()).c_str());
				}

				return;
			}
		}

		if (!bConnected)
		{
			//清除重连信息
			if (__Instances->LiveParam.TipsCb)
			{
				__Instances->LiveParam.TipsCb(-100, " ");
			}
		}

		ConnectedDelayed = false;

        InitEncoderData();

        ProcessDelayedPackets(timestamp);

        NetworkPacket *newPacket = delayedPackets.CreateNew();
        newPacket->data.CopyArray(data, size);
        newPacket->timestamp = timestamp;
        newPacket->type = type;

        lastTimestamp = timestamp;
    }

	RTMPPublisher* Clone()
	{
		DelayedPublisher* publisher = new DelayedPublisher(delayTime, Prefix, __Instances, bUseBack);
		publisher->ConnectedDelayed = true;
		publisher->bDelayConnected = true;

		publisher->delayedPackets.TransferFrom(delayedPackets);

		if (mDestroy == false)
		{
			Destructor();
		}

		return publisher;
	}

	RTMPPublisher* CloneWithNoDelayConnect()
	{
		DelayedPublisher* publisher = new DelayedPublisher(delayTime, Prefix, __Instances, bUseBack);
		publisher->bDelayConnected = true;

		publisher->delayedPackets.TransferFrom(delayedPackets);

		if (mDestroy == false)
		{
			Destructor();
		}

		return publisher;
	}

    void RequestKeyframe(int waitTime) {}
};

bool         DelayedPublisher::bShowDialog = true;
bool         DelayedPublisher::bShowDialogIn = true;
bool         DelayedPublisher::bHasConnect = false;

RTMPPublisherVectorBase* CreateDelayedPublisher(DWORD delayTime,CInstanceProcess *Instance,bool bUseBack)
{
  	int PublishCount = 2;

	int Rendering = 0;

	RTMPPublisherVector* VRTMPPublisher = new RTMPPublisherVector;
	//添加第一个
	String prefix;
	for (int i = 0; i < PublishCount; ++i)
	{
		prefix = String(TEXT("Publish")) + IntString(i);
		if (!bUseBack)
		{
			if (i == 0)
			{
				Rendering = Instance->LiveParam.LiveSetting.bUsePush ? 1 : 0;
			}
			else
			{
				Rendering = Instance->LiveParam.LiveSetting.bUseBackPush ? 1 : 0;
			}
		}
		else
		{
			if (i == 0)
			{
				Rendering = Instance->LiveParam.LiveSetting.bUsePushSec ? 1 : 0;
			}
			else
			{
				Rendering = Instance->LiveParam.LiveSetting.bUseBackPushSec ? 1 : 0;
			}
		}
		if (Rendering)
		{
			VRTMPPublisher->AddRTMPPublisher(new DelayedPublisher(delayTime * 1000, prefix, Instance,bUseBack));
		}
	}

	return VRTMPPublisher;
}

bool IsDelayedPublisher(RTMPPublisher *Publisher)
{
	if (Publisher)
	{
		DelayedPublisher *DelayPubliher = dynamic_cast<DelayedPublisher*>(Publisher);
		if (DelayPubliher)
			return true;
	}
	return false;
}