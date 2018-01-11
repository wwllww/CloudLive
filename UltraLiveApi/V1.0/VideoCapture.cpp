#include "Instance.h"
#include "OperatNew.h"
#include "RTMPStuff.h"
#include "RTMPPublisherVector.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

bool CInstanceProcess::ProcessFrame(FrameProcessInfo &frameInfo)
{
	List<DataPacket> videoPackets;
	List<PacketType> videoPacketTypes;


	bufferedTimes << frameInfo.frameTimestamp;

	//VideoSegment curSegment;
	std::list<std::shared_ptr<VideoSegment>> segmentOuts;

	bool bProcessedFrame, bSendFrame = false;

	static QWORD CallCount = 0;

	DWORD out_pts = 0;
	videoEncoder->Encode(frameInfo.pic, videoPackets, videoPacketTypes, bufferedTimes[0], out_pts);

	bProcessedFrame = (videoPackets.Num() != 0);// encode success if videoPackets have data 

	//buffer video data before sending out
	if (bProcessedFrame)
	{
		bSendFrame = BufferVideoDataList(videoPackets, videoPacketTypes, bufferedTimes[0], bufferedTimes[0], frameInfo.firstFrameTime, segmentOuts);
		bufferedTimes.Remove(0);
		++CallCount;
	}
	else
	{
		++CallCount;
		Log::writeMessage(LOG_RTSPSERV, 1, "当前数据还没准好 bufferedTimes.num %d,调用序号 %llu", bufferedTimes.Num(), CallCount);
	}


	//send headers before the first frame if not yet sent
	if (bSendFrame)
	{
		std::list<std::shared_ptr<VideoSegment>>::iterator pos = segmentOuts.begin();
		std::list<std::shared_ptr<VideoSegment>>::iterator end = segmentOuts.end();

		for (; pos != end; ++pos)
		{
			SendFrame(*((*pos).get()), frameInfo.firstFrameTime);
		}
	}

	return bProcessedFrame;
}

bool CInstanceProcess::ProcessFrame_back(FrameProcessInfo &frameInfo)
{
	List<DataPacket> videoPackets;
	List<PacketType> videoPacketTypes;


	bufferedTimes_back << frameInfo.frameTimestamp;

	//VideoSegment curSegment;
	std::list<std::shared_ptr<VideoSegment>> segmentOuts;

	bool bProcessedFrame, bSendFrame = false;

	DWORD out_pts = 0;
	if (videoEncoder_back)
		videoEncoder_back->Encode(frameInfo.pic, videoPackets, videoPacketTypes, bufferedTimes_back[0], out_pts);

	bProcessedFrame = (videoPackets.Num() != 0);// encode success if videoPackets have data 

	//buffer video data before sending out
	if (bProcessedFrame && videoEncoder_back)
	{
		bSendFrame = BufferVideoDataList_back(videoPackets, videoPacketTypes, bufferedTimes_back[0], out_pts, frameInfo.firstFrameTime, segmentOuts);
		bufferedTimes_back.Remove(0);
	}


	//send headers before the first frame if not yet sent
	if (bSendFrame)
	{
		std::list<std::shared_ptr<VideoSegment>>::iterator pos = segmentOuts.begin();
		std::list<std::shared_ptr<VideoSegment>>::iterator end = segmentOuts.end();

		for (; pos != end; ++pos)
		{
			SendFrame_back(*((*pos).get()), frameInfo.firstFrameTime);
		}
	}

	return bProcessedFrame;
}

void CInstanceProcess::SendFrame(VideoSegment &curSegment, QWORD firstFrameTime)
{
	EnterCriticalSection(&SoundDataMutex);

	if (pendingAudioFrames.Num())
	{
		while (pendingAudioFrames.Num())
		{
			if (firstFrameTime < pendingAudioFrames[0].timestamp)
			{
				UINT audioTimestamp = UINT(pendingAudioFrames[0].timestamp - firstFrameTime);

				//stop sending audio packets when we reach an audio timestamp greater than the video timestamp
				if (audioTimestamp > curSegment.timestamp)
				{
					//Log::writeMessage(LOG_RTSPSERV, 1, "%s pendingAudioFrames.num %d,audioTimestamp %d,curSegment.timestamp %d firstFrameTime %llu pendingAudioFrames[0].timestamp %llu__break", __FUNCTION__, pendingAudioFrames.Num(), audioTimestamp, curSegment.timestamp, firstFrameTime, pendingAudioFrames[0].timestamp);
					break;
				}

				if (audioTimestamp == 0 || audioTimestamp > lastAudioTimestamp)
				{
					List<BYTE> &audioData = pendingAudioFrames[0].audioData;
					if (audioData.Num())
					{
						if (network)
							network->SendPacket(audioData.Array(), audioData.Num(), audioTimestamp, PacketType_Audio, EncodMajor);

						if (fileStream)
						{
							auto shared_data = std::make_shared<const std::vector<BYTE>>(audioData.Array(), audioData.Array() + audioData.Num());
							fileStream->AddPacket(shared_data, audioTimestamp, audioTimestamp, PacketType_Audio);
						}

						audioData.Clear();

						lastAudioTimestamp = audioTimestamp;
					}
				}

			}
			//Log::writeMessage(LOG_RTSPSERV, 1, "%s Remove Audio Frame pendingAudioFrames.num %d,audioTimestamp %d,curSegment.timestamp %llu", __FUNCTION__, pendingAudioFrames.Num(), pendingAudioFrames[0].timestamp - firstFrameTime, curSegment.timestamp);
			pendingAudioFrames[0].audioData.Clear();
			pendingAudioFrames.Remove(0);
		}
	}
	LeaveCriticalSection(&SoundDataMutex);

	for (UINT i = 0; i < curSegment.packets.Num(); i++)
	{
		VideoPacketData &packet = curSegment.packets[i];

		if (packet.type == PacketType_VideoHighest)
			bRequestKeyframe = false;
		if (network)
		{
			network->SendPacket(packet.data.Array(), packet.data.Num(), curSegment.timestamp, packet.type, EncodMajor);
			//Log::writeMessage(LOG_RTSPSERV, 1, "%s Send Video Frame VidoeTimestamp %d", __FUNCTION__, curSegment.timestamp);
		}

		if (fileStream)
		{
			auto shared_data = std::make_shared<const std::vector<BYTE>>(packet.data.Array(), packet.data.Array() + packet.data.Num());
			fileStream->AddPacket(shared_data, curSegment.timestamp, curSegment.pts, packet.type);
		}
	}

	
}


void CInstanceProcess::SendFrame_back(VideoSegment &curSegment, QWORD firstFrameTime)
{
	EnterCriticalSection(&SoundDataMutex_back);

	if (pendingAudioFrames_back.Num())
	{
		while (pendingAudioFrames_back.Num())
		{
			if (firstFrameTime < pendingAudioFrames_back[0].timestamp)
			{
				UINT audioTimestamp = UINT(pendingAudioFrames_back[0].timestamp - firstFrameTime);

				//stop sending audio packets when we reach an audio timestamp greater than the video timestamp
				if (audioTimestamp > curSegment.timestamp)
					break;

				if (audioTimestamp == 0 || audioTimestamp > lastAudioTimestamp_back)
				{
					List<BYTE> &audioData = pendingAudioFrames_back[0].audioData;
					if (audioData.Num())
					{
						if (network_back)
							network_back->SendPacket(audioData.Array(), audioData.Num(), audioTimestamp, PacketType_Audio, EncodMajor);

						if (fileStream_back)
						{
							auto shared_data = std::make_shared<const std::vector<BYTE>>(audioData.Array(), audioData.Array() + audioData.Num());
							fileStream_back->AddPacket(shared_data, audioTimestamp, audioTimestamp, PacketType_Audio);
						}

						audioData.Clear();

						lastAudioTimestamp_back = audioTimestamp;
					}
				}
			}

			pendingAudioFrames_back[0].audioData.Clear();
			pendingAudioFrames_back.Remove(0);
		}
	}

	LeaveCriticalSection(&SoundDataMutex_back);

	for (UINT i = 0; i < curSegment.packets.Num(); i++)
	{
		VideoPacketData &packet = curSegment.packets[i];

		if (packet.type == PacketType_VideoHighest)
			bRequestKeyframe_back = false;

		if (network_back)
		{
			network_back->SendPacket(packet.data.Array(), packet.data.Num(), curSegment.timestamp, packet.type, EncodMajor);
		}

		if (fileStream_back)
		{
			auto shared_data = std::make_shared<const std::vector<BYTE>>(packet.data.Array(), packet.data.Array() + packet.data.Num());
			fileStream_back->AddPacket(shared_data, curSegment.timestamp, curSegment.pts, packet.type);
		}
	}


}

bool CInstanceProcess::BufferVideoDataList(const List<DataPacket> &inputPackets, const List<PacketType> &inputTypes, QWORD timestamp, DWORD out_pts, QWORD firstFrameTime, std::list<std::shared_ptr<VideoSegment>>& segmentOuts)
{
	VideoSegment &segmentIn = *bufferedVideo.CreateNew();
	segmentIn.timestamp = timestamp;
	segmentIn.pts = out_pts;

	segmentIn.packets.SetSize(inputPackets.Num());
	for (UINT i = 0; i < inputPackets.Num(); i++)
	{
		segmentIn.packets[i].data.CopyArray(inputPackets[i].lpPacket, inputPackets[i].size);
		segmentIn.packets[i].type = inputTypes[i];
	}
	
	bool result = false;
	int count = 0;

	while (true)
	{
		bool dataReady = false;

		EnterCriticalSection(&SoundDataMutex); //判断视频帧是否超前

		UINT haveVideo = (bufferedVideo.Num() > 0);
		UINT AudioNum = pendingAudioFrames.Num();

		for (UINT index = 0; (index < AudioNum) && haveVideo; ++index)
		{
			if (firstFrameTime < pendingAudioFrames[index].timestamp && pendingAudioFrames[index].timestamp - firstFrameTime >= bufferedVideo[0].timestamp)
			{
				dataReady = true;
				break;
			}
		}

		LeaveCriticalSection(&SoundDataMutex);

		if (dataReady && (count < 2))
		{
			++count;

			auto segmentOut = std::make_shared<VideoSegment>();
			segmentOut->packets.TransferFrom(bufferedVideo[0].packets);
			segmentOut->timestamp = bufferedVideo[0].timestamp;
			segmentOut->pts = bufferedVideo[0].pts;

			segmentOuts.push_back(segmentOut);
			bufferedVideo.Remove(0);

			result = true;
		}
		else
		{
			break;
		}
	}

	return result;
}

bool CInstanceProcess::BufferVideoDataList_back(const List<DataPacket> &inputPackets, const List<PacketType> &inputTypes, QWORD timestamp, DWORD out_pts, QWORD firstFrameTime, std::list<std::shared_ptr<VideoSegment>>& segmentOuts)
{
	VideoSegment &segmentIn = *bufferedVideo_back.CreateNew();
	segmentIn.timestamp = timestamp;
	segmentIn.pts = out_pts;

	segmentIn.packets.SetSize(inputPackets.Num());
	for (UINT i = 0; i < inputPackets.Num(); i++)
	{
		segmentIn.packets[i].data.CopyArray(inputPackets[i].lpPacket, inputPackets[i].size);
		segmentIn.packets[i].type = inputTypes[i];
	}

	bool result = false;
	int count = 0;

	while (true)
	{
		bool dataReady = false;

		EnterCriticalSection(&SoundDataMutex_back); //判断视频帧是否超前

		UINT haveVideo = (bufferedVideo_back.Num() > 0);
		UINT AudioNum = pendingAudioFrames_back.Num();

		for (UINT index = 0; (index < AudioNum) && haveVideo; ++index)
		{
			if (firstFrameTime < pendingAudioFrames_back[index].timestamp && pendingAudioFrames_back[index].timestamp - firstFrameTime >= bufferedVideo_back[0].timestamp)
			{
				dataReady = true;
				break;
			}
		}

		LeaveCriticalSection(&SoundDataMutex_back);

		if (dataReady && (count < 2))
		{
			++count;

			auto segmentOut = std::make_shared<VideoSegment>();
			segmentOut->packets.TransferFrom(bufferedVideo_back[0].packets);
			segmentOut->timestamp = bufferedVideo_back[0].timestamp;
			segmentOut->pts = bufferedVideo_back[0].pts;

			segmentOuts.push_back(segmentOut);
			bufferedVideo_back.Remove(0);

			result = true;
		}
		else
		{
			break;
		}
	}

	return result;
}
