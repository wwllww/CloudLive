#include "Instance.h"
#include "OperatNew.h"
#include "Error.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

extern CInstanceProcess *G_LiveInstance;

void CInstanceProcess::DeleteStream(uint64_t StreamId)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__,StreamId);
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == StreamId)
		{
			if (m_VideoList[i].AudioStream)
			{
				EnterCriticalSection(&AudioSection);

				for (UINT j = 0; j < m_AudioList.Num(); ++j)
				{
					if (m_AudioList[j].AudioStream.get() == m_VideoList[i].AudioStream)
					{
						m_AudioList[j].AudioStream.reset();//2017/09/27
						m_AudioList.Remove(j);
						break;
					}
				}
				LeaveCriticalSection(&AudioSection);
			}

			if (m_VideoList[i].bGlobalStream && m_VideoList[i].VideoStream.use_count() == 2)
			{
				IBaseVideo *BaseVideo = m_VideoList[i].VideoStream.get();

				if (BaseVideo && m_VideoList[i].bRender && strcmp(BaseVideo->GainClassName(),"AgentSource") != 0)
				{
					BaseVideo->GlobalSourceLeaveScene();
					if (m_VideoList[i].VideoDevice)
					{
						m_VideoList[i].VideoDevice->GlobalSourceLeaveScene();
						m_VideoList[i].VideoDevice->SetCanEnterScene(true);
					}
					BaseVideo->SetCanEnterScene(true);
				}
			}
			else if (m_VideoList[i].bGlobalStream)
			{
				m_VideoList[i].VideoStream->SetHasSwitchSences(true);
			}

			//IBaseVideo *GlobalVideo = m_VideoList[i].VideoStream->GetGlobalSource();

			bool bNeedRemoved = true;
			bool bAgent = false;

			if (strcmp(m_VideoList[i].VideoStream->GainClassName(), "AgentSource") != 0)
			{
				//如果在区域占位源中就不要移徐
				for (UINT k = 0; k < m_VideoList.Num(); ++k)
				{
					if (m_VideoList[k].VideoStream && (strcmp(m_VideoList[k].VideoStream->GainClassName(), "AgentSource") == 0))
					{
						if (m_VideoList[k].VideoStream->GetGlobalSource() == m_VideoList[i].VideoStream.get())
						{
							bNeedRemoved = false;
							break;
						}
					}
				}
			}
			else
			{
				IBaseVideo *Video = m_VideoList[i].VideoStream->GetGlobalSource();
				if (Video)
				{
					bAgent = true;
					for (UINT k = 0; k < m_VideoList.Num(); ++k)
					{
						if (m_VideoList[k].VideoStream)
						{
							if (m_VideoList[k].VideoStream.get() == Video)
							{
								bNeedRemoved = false;
								break;
							}

							if (strcmp(m_VideoList[k].VideoStream->GainClassName(), "AgentSource") == 0 && m_VideoList[k].VideoStream.get() != m_VideoList[i].VideoStream.get())
							{
								if (m_VideoList[k].VideoStream->GetGlobalSource() == Video)
								{
									bNeedRemoved = false;
									break;
								}
							}
						}
					}
				}
			}


			if (bNeedRemoved)
			{
				//增加删除Filter
				for (UINT j = 0; j < m_Filter.Num(); ++j)
				{
					Filter &OneFilter = m_Filter[j];
					IBaseVideo *GlobalVideo = m_VideoList[i].VideoStream->GetGlobalSource();
					if (OneFilter.IVideo == m_VideoList[i].VideoStream.get() || (bAgent && OneFilter.IVideo == GlobalVideo) || (OneFilter.IVideo == m_VideoList[i].VideoDevice.get()))
					{
						vector<__FilterSturct> &BaseFilter = OneFilter.BaseFilter;

						for (UINT k = 0; k < BaseFilter.size(); ++k)
						{
							__FilterSturct &OneFilter = BaseFilter[k];

							if (OneFilter._Filter)
								OneFilter._Filter.reset();
						}

						BaseFilter.clear();

						if (OneFilter.RenderTarget)
							OneFilter.RenderTarget.reset();

						m_Filter.Remove(j);
						break;
					}
				}
			}

			m_VideoList[i].VideoStream.reset();

			if (m_VideoList[i].VideoDevice)
				m_VideoList[i].VideoDevice.reset();

			m_VideoList[i].Config.reset();
			m_VideoList.Remove(i);
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);

		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == StreamId)
			{
				//注释掉这个音频都要删除掉
// 				if (m_AudioList[j].VideoStream)
// 				{
// 					LeaveCriticalSection(&AudioSection);
// 					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
// 					BUTEL_THORWERROR("该音频Id %llu 有视频,不允许删除!", StreamId);
// 				}

				m_AudioList[j].AudioStream.reset();
				m_AudioList[j].Config.reset();
				bFind = true;
				m_AudioList.Remove(j);
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		//BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", StreamId);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::GetStreamInfo(uint64_t SteamId, Value &JValue)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, SteamId);
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == SteamId)
		{
			if (!m_VideoList[i].VideoStream->GetStreamInfo(JValue))
			{
				LeaveCriticalSection(&VideoSection);
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
				BUTEL_THORWERROR("获取视频 %llu 信息失败!", SteamId);
			}
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);

		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == SteamId)
			{
				if (!m_AudioList[j].AudioStream->GetStreamInfo(JValue))
				{
					LeaveCriticalSection(&AudioSection);
					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
					BUTEL_THORWERROR("获取音频 %llu 信息失败!", SteamId);
				}

				bFind = true;
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", SteamId);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::GetStreamStatus(uint64_t iStreamID, DBOperation *Status)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			*Status = m_VideoList[i].VideoStream->GetStreamStatus();
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::SetStreamPlayPos(uint64_t iStreamID, UINT Pos)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu, Pos = %d", __FUNCTION__, iStreamID,Pos);
	bool bFind = false;
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			if (!m_VideoList[i].VideoStream->SetStreamPos(Pos))
			{
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
				BUTEL_THORWERROR("设置视频 %llu 位置 %d 失败!", iStreamID, Pos);
			}
			bFind = true;
			break;
		}
	}


	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::GetStreamPlayPos(uint64_t iStreamID, UINT& Pos)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			if (!m_VideoList[i].VideoStream->GetStreamPos(Pos))
			{
				LeaveCriticalSection(&VideoSection);
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
				BUTEL_THORWERROR("获取视频 %llu 位置失败!", iStreamID);
			}
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}


void CInstanceProcess::OperaterStream(uint64_t iStreamID, DBOperation OperType)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	//EnterCriticalSection(&VideoSection);//去掉这个是因为点重新播放的时候会掉帧
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			if (!m_VideoList[i].VideoStream->OperaterStream(OperType))
			{
				//LeaveCriticalSection(&VideoSection);
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
				BUTEL_THORWERROR("操作视频 %llu 失败!", iStreamID);
			}
			bFind = true;
			break;
		}
	}
	//LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::UpdateStream(uint64_t iStreamID, const char* cJsonParam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	Value Jvalue;
	Reader JReader;
	if (!JReader.parse(cJsonParam, Jvalue))
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("Json %s 解析失败", cJsonParam);
	}

	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s EnterCriticalSection(&VideoSection),m_VideoList size = %d", __FUNCTION__, m_VideoList.Num());
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			try
			{
//				m_VideoList[i].bRender = true;
				m_VideoList[i].VideoStream->UpdateSettings(Jvalue);

// 				string &ClassName = Jvalue["ClassName"].asString();
// 
// 				if (strcmp(ClassName.c_str(), "DShowSource") == 0)
// 				{
// 					m_VideoList[i].size.x = Jvalue["Width"].asInt();
// 					m_VideoList[i].size.y = Jvalue["Height"].asInt();
// 				}
			}
			catch (CErrorBase& e)
			{
				m_VideoList[i].bRender = false;
				LeaveCriticalSection(&VideoSection);
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s LeaveCriticalSection(&VideoSection)", __FUNCTION__);
				BUTEL_THORWERROR(e.m_Error.c_str());
			}
			
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s LeaveCriticalSection(&VideoSection)", __FUNCTION__);
	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);
		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
			{
				m_AudioList[j].AudioStream->UpdateSettings(Jvalue);
				bFind = true;
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);

	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::UpdateStreamPosition(uint64_t iStreamID, VideoArea *Area, bool bScale)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			m_VideoList[i].pos.x = Area->left;
			m_VideoList[i].pos.y = Area->top;
			m_VideoList[i].size.x = Area->width;
			m_VideoList[i].size.y = Area->height;
			m_VideoList[i].bScale = bScale;
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
}

void CInstanceProcess::MoveStream(uint64_t iStreamID, StreamMoveOperation Type)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	int Index = -1;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			Index = i;
			bFind = true;
			break;
		}
	}

	if (-1 != Index)
	{
		MoveStream(Type, Index);
	}

	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
}

void CInstanceProcess::MoveStream(StreamMoveOperation Type,int Index)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! Index = %d, VideoList size = %d", __FUNCTION__, Index, m_VideoList.Num());
	if (Index < 0 || Index >= m_VideoList.Num())
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error Index = %d", __FUNCTION__, Index);
		return;
	}
	
	switch (Type)
	{
	case MoveUp:
		if (Index == m_VideoList.Num() - 1)
			return;
		m_VideoList.SwapValues(Index, Index + 1);
		break;
	case MoveDown:
		if (Index == 0)
			return;
		m_VideoList.SwapValues(Index - 1,Index);
		break;
	case MoveTop:
		if (Index == m_VideoList.Num() - 1)
			return;

		{
			VideoStruct VStruct = m_VideoList[Index];
			m_VideoList.Remove(Index);
			m_VideoList.Insert(m_VideoList.Num(), VStruct);
		}
		break;
	case MoveBottom:
		if (Index == 0)
			return;
		{
			VideoStruct VStruct = m_VideoList[Index];
			m_VideoList.Remove(Index);
			m_VideoList.Insert(0, VStruct);
		}
		break;
	default:
		break;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::StartRenderAStream(uint64_t iStreamID, const char* cRenderAudioDevice)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! iStreamID = %llu,cRenderAudioDevice = %s", __FUNCTION__,iStreamID,cRenderAudioDevice);
	bool bFind = false;
	EnterCriticalSection(&AudioSection);

	for (UINT j = 0; j < m_AudioList.Num(); ++j)
	{
		if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
		{
			m_AudioList[j].AudioStream->StartRenderAStream(cRenderAudioDevice);
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&AudioSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是音频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::StopRenderAStream(uint64_t iStreamID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invode begin! iStreadID = %llu", __FUNCTION__,iStreamID);
	bool bFind = false;
	EnterCriticalSection(&AudioSection);

	for (UINT j = 0; j < m_AudioList.Num(); ++j)
	{
		if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
		{
			m_AudioList[j].AudioStream->StopRenderAStream();
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&AudioSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是音频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::SetAudioStreamDBCallBack(uint64_t iStreamID, AudioDBCallBack DBCallBack)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	EnterCriticalSection(&AudioSection);

	for (UINT j = 0; j < m_AudioList.Num(); ++j)
	{
		if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
		{
			m_AudioList[j].AudioStream->SetAudioDBCallBack(DBCallBack);
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&AudioSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到或该流不是音频流,StreamID为 %llu", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::StartResize(bool bDragResize)
{
	if (!bDragResize)
	{
		ResizeRenderFrame(true);
	}
	else
	{
		ResizeRenderFrame(false);
	}
}

void CInstanceProcess::StopResize()
{
	ResizeRenderFrame(true);
}

void CInstanceProcess::SetRenderStream(uint64_t iStreamID, bool bRender,const AudioStruct &_Audio)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu,bRender = %s", __FUNCTION__, iStreamID,bRender ? "ture":"false");
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			m_VideoList[i].bRender = bRender;

			EnterCriticalSection(&AudioSection);
			if (bRender && _Audio.AudioStream && ((strcmp(m_VideoList[i].VideoStream->GainClassName(), "AgentSource") == 0 && IsLiveInstance) || strcmp(m_VideoList[i].VideoStream->GainClassName(), "AgentSource") != 0))
			{
				bool bFind = false;
				for (int j = 0; j < m_AudioList.Num(); ++j)
				{
					AudioStruct &ASTemD = m_AudioList[j];

					if (ASTemD.AudioStream.get() == _Audio.AudioStream.get())
					{
						bFind = true;
						break;
					}
				}

				if (!bFind)
				{
					m_AudioList.SetSize(m_AudioList.Num() + 1);
					AudioStruct &AS = m_AudioList[m_AudioList.Num() - 1];
					AS = _Audio;
				}

			}
			else if (!bRender && _Audio.AudioStream && strcmp(m_VideoList[i].VideoStream->GainClassName(), "AgentSource") != 0)
			{
				//如果在区域占位源中就不要移徐
				bool bNeedRemoved = true;
				for (UINT k = 0; k < m_VideoList.Num(); ++k)
				{
					if (m_VideoList[k].VideoStream && m_VideoList[k].bRender && (strcmp(m_VideoList[k].VideoStream->GainClassName(), "AgentSource") == 0))
					{
						if ((uint64_t)m_VideoList[k].VideoStream->GetGlobalSource() == iStreamID)
						{
							bNeedRemoved = false;
							break;
						}
					}
				}
				if (bNeedRemoved)
				{
					for (int j = 0; j < m_AudioList.Num(); ++j)
					{
						AudioStruct &OneAudio = m_AudioList[j];
						if (OneAudio.AudioStream.get() == _Audio.AudioStream.get())
						{
							OneAudio.AudioStream.reset();

							if (OneAudio.Config)
								OneAudio.Config.reset();

							m_AudioList.Remove(j);
							break;
						}
					}
				}
			}
			else if (!bRender && _Audio.AudioStream && strcmp(m_VideoList[i].VideoStream->GainClassName(), "AgentSource") == 0)
			{
				//如果在真实源中就不要移徐
				bool bNeedRemoved = true;
				IBaseVideo *Video = m_VideoList[i].VideoStream->GetGlobalSource();
				if (Video)
				{
					for (UINT k = 0; k < m_VideoList.Num(); ++k)
					{

						if (m_VideoList[k].VideoStream && m_VideoList[k].bRender)
						{
							if (m_VideoList[k].VideoStream.get() == Video)
							{
								bNeedRemoved = false;
								break;
							}

							if (strcmp(m_VideoList[k].VideoStream->GainClassName(), "AgentSource") == 0 && m_VideoList[k].VideoStream.get() != m_VideoList[i].VideoStream.get())
							{
								if (m_VideoList[k].VideoStream->GetGlobalSource() == Video)
								{
									bNeedRemoved = false;
									break;
								}
							}
						}
					}
				}
				if (bNeedRemoved)
				{
					for (int j = 0; j < m_AudioList.Num(); ++j)
					{
						AudioStruct &OneAudio = m_AudioList[j];
						if (OneAudio.AudioStream.get() == _Audio.AudioStream.get())
						{
							OneAudio.AudioStream.reset();

							if (OneAudio.Config)
								OneAudio.Config.reset();

							m_AudioList.Remove(j);
							break;
						}
					}
				}
			}

			LeaveCriticalSection(&AudioSection);
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);

		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
			{
				m_AudioList[j].bRender = bRender;
				bFind = true;
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CInstanceProcess::SelectStream(uint64_t iStreamID, bool bSelect)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu,bSelect = %s", __FUNCTION__, iStreamID, bSelect ? "ture" : "false");
	bool bFind = false;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			m_VideoList[i].bSelect = bSelect;
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);
		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
			{
				bFind = true;
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);

		if (bFind)
		{
			Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
			BUTEL_THORWERROR("StreamID为 %llu 的流为音频流", iStreamID);
		}
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::ConfigStream(uint64_t iStreamID, const char *cJson)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;
	bool bVideo = false;
	shared_ptr<Value> Config;
	CPObject *Object = NULL;
	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			Config = m_VideoList[i].Config;
			Object = m_VideoList[i].VideoStream.get();
			bFind = true;
			bVideo = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		EnterCriticalSection(&AudioSection);

		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
			{
				if (m_AudioList[j].VideoStream)
				{
					LeaveCriticalSection(&AudioSection);
					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
					BUTEL_THORWERROR("该音频Id %llu 有视频,不允许配置!", iStreamID);
				}
				Config = m_AudioList[j].Config;
				Object = m_AudioList[j].AudioStream.get();
				bFind = true;
				break;
			}
		}
		LeaveCriticalSection(&AudioSection);
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	}

	if (Object)
	{
		CONFINGFUN fp = GetConfigFunc(Object->GainClassName());

		if (fp)
		{
			IBaseVideo *BaseVideo = dynamic_cast<IBaseVideo*>(Object);
			Value Old = *Config.get();
			if (fp(*Config.get(), false))
			{
				if (bVideo)
				{
					if (BaseVideo)
					{
						//DeviceSource先不UpdateSettings
						if (strcmp(BaseVideo->GainClassName(),"DeviceSource") != 0)
								BaseVideo->UpdateSettings(*Config.get());
						else
						{
							bCanRecord = (*Config)["UseRecorder"].asInt() == 1;
						}
					}

					if (strcmp(BaseVideo->GainClassName(), "DeviceSource") == 0)
					{
						bool bFind = false;
						for (int i = 0; i < SharedDevice::VideoList.size(); ++i)
						{
							ShardVideo& OneVideo = SharedDevice::VideoList[i];

							if (OneVideo.VideoStream->GetDeviceID())
							{
								if (strcmp(OneVideo.VideoStream->GetDeviceName(), (*Config)["deviceName"].asString().c_str()) == 0 && strcmp(OneVideo.VideoStream->GetDeviceID(), (*Config)["deviceID"].asString().c_str()) == 0)
								{
									bFind = true;
									shared_ptr<IBaseVideo> OldVideo;
									//EnterCriticalSection(&VideoSection);
									for (UINT i = 0; i < m_VideoList.Num(); ++i)
									{
										if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
										{
											if (m_VideoList[i].VideoDevice)
											{
												m_VideoList[i].VideoDevice->UnRegisterDataCallBack(this);
											}
											OldVideo = m_VideoList[i].VideoDevice;
											m_VideoList[i].VideoDevice = OneVideo.VideoStream;

											m_VideoList[i].VideoDevice->UpdateSettings(*Config.get());

											m_VideoList[i].VideoDevice->RegisterDataCallBack(this, StreamCallBack);
											m_VideoList[i].VideoDevice->RegisterDataCallBack(this, RecordCallBack);
											//不在当前场景中需要BeginScene
											if (!CSLiveManager::GetInstance()->IsHaveStream(m_VideoList[i].VideoDevice.get()))
												m_VideoList[i].VideoDevice->BeginScene();

											if (OldVideo && !OldVideo->CanEnterScene())//OldVideo不可进入场景，说明OldVideo在场景中
											{
												m_VideoList[i].VideoDevice->GlobalSourceEnterScene();
												m_VideoList[i].VideoDevice->SetCanEnterScene(false);

												OldVideo->GlobalSourceLeaveScene();
												OldVideo->SetCanEnterScene(true);
											}

											BaseVideo->UnRegisterDataCallBack(this);

											char Tem[50] = { 0 };
											sprintf_s(Tem, "%llu", (uint64_t)OneVideo.VideoStream.get());
											(*m_VideoList[i].Config)["DeviceSourceID"] = Tem;

											break;
										}
									}
									//LeaveCriticalSection(&VideoSection);
									CSLiveManager::GetInstance()->ResetDevice(BaseVideo, OneVideo.VideoStream, false, OldVideo.get());
									break;
								}
							}
							else
							{
								if (strcmp(OneVideo.VideoStream->GetDeviceName(), (*Config)["deviceName"].asString().c_str()) == 0)
								{
									bFind = true;
									shared_ptr<IBaseVideo> OldVideo;
									//EnterCriticalSection(&VideoSection);
									for (UINT i = 0; i < m_VideoList.Num(); ++i)
									{
										if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
										{
											if (m_VideoList[i].VideoDevice)
											{
												m_VideoList[i].VideoDevice->UnRegisterDataCallBack(this);
											}
											OldVideo = m_VideoList[i].VideoDevice;
											m_VideoList[i].VideoDevice = OneVideo.VideoStream;

											m_VideoList[i].VideoDevice->UpdateSettings(*Config.get());

											m_VideoList[i].VideoDevice->RegisterDataCallBack(this, StreamCallBack);
											m_VideoList[i].VideoDevice->RegisterDataCallBack(this, RecordCallBack);

											//不在当前场景中需要BeginScene
											if (!CSLiveManager::GetInstance()->IsHaveStream(m_VideoList[i].VideoDevice.get()))
												m_VideoList[i].VideoDevice->BeginScene();

											if (OldVideo && !OldVideo->CanEnterScene())//OldVideo不可进入场景，说明OldVideo在场景中
											{
												m_VideoList[i].VideoDevice->GlobalSourceEnterScene();
												m_VideoList[i].VideoDevice->SetCanEnterScene(false);

												OldVideo->GlobalSourceLeaveScene();
												OldVideo->SetCanEnterScene(true);
											}

											char Tem[50] = { 0 };
											sprintf_s(Tem, "%llu", (uint64_t)OneVideo.VideoStream.get());
											(*m_VideoList[i].Config)["DeviceSourceID"] = Tem;
											BaseVideo->UnRegisterDataCallBack(this);

											CSLiveManager::GetInstance()->ResetDevice(BaseVideo, OneVideo.VideoStream, false, OldVideo.get());
											break;
										}
									}
									//LeaveCriticalSection(&VideoSection);

									break;
								}
							}
						}

						if (!bFind)
						{
							//EnterCriticalSection(&VideoSection);
							for (UINT i = 0; i < m_VideoList.Num(); ++i)
							{
								if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
								{
									shared_ptr<IBaseVideo> OldVideo;
									if (m_VideoList[i].VideoDevice)
									{
										OldVideo = m_VideoList[i].VideoDevice;


										if (m_VideoList[i].VideoDevice->GetDeviceID())
										{
											if (strcmp(m_VideoList[i].VideoDevice->GetDeviceName(), (*Config)["deviceName"].asString().c_str()) != 0 || strcmp(m_VideoList[i].VideoDevice->GetDeviceID(), (*Config)["deviceID"].asString().c_str()) != 0)
											{
												//设备不同

												m_VideoList[i].VideoDevice->UnRegisterDataCallBack(this);
												m_VideoList[i].VideoDevice.reset();
												IBaseVideo *BaseStream = dynamic_cast<IBaseVideo*>(CreatStreamObject("DeviceSource"));
												BaseStream->Init(const_cast<Value&>(*Config.get()));
												char Tem[50] = { 0 };
												sprintf_s(Tem, "%llu", (uint64_t)BaseStream);
												(*Config)["DeviceSourceID"] = Tem;
												m_VideoList[i].VideoDevice = shared_ptr<IBaseVideo>(BaseStream);
												BaseStream->RegisterDataCallBack(this, StreamCallBack);

												//增加录制回调
												BaseStream->RegisterDataCallBack(this, RecordCallBack);

												if (OldVideo && !OldVideo->CanEnterScene())//OldVideo不可进入场景，说明OldVideo在场景中
												{
													m_VideoList[i].VideoDevice->GlobalSourceEnterScene();
													m_VideoList[i].VideoDevice->SetCanEnterScene(false);

													OldVideo->GlobalSourceLeaveScene();
													OldVideo->SetCanEnterScene(true);
												}

												//新建的需要BeginScene
												BaseStream->BeginScene();
												
												//Config->removeMember("DeviceSourceID");
												//查找所有包括BaseVideo的LocalInstance和LiveInstance把它们的VideoDevice reset;
												CSLiveManager::GetInstance()->ResetDevice(BaseVideo, m_VideoList[i].VideoDevice, false, OldVideo.get());
											}
											else
											{
												m_VideoList[i].VideoDevice->UpdateSettings(*Config.get());
											}
										}
										else
										{
											if (strcmp(m_VideoList[i].VideoDevice->GetDeviceName(), (*Config)["deviceName"].asString().c_str()) != 0)
											{
												//设备不同
												m_VideoList[i].VideoDevice->UnRegisterDataCallBack(this);
												m_VideoList[i].VideoDevice.reset();
												IBaseVideo *BaseStream = dynamic_cast<IBaseVideo*>(CreatStreamObject("DeviceSource"));
												BaseStream->Init(const_cast<Value&>(*Config.get()));
												char Tem[50] = { 0 };
												sprintf_s(Tem, "%llu", (uint64_t)BaseStream);
												(*Config)["DeviceSourceID"] = Tem;
												m_VideoList[i].VideoDevice = shared_ptr<IBaseVideo>(BaseStream);
												BaseStream->RegisterDataCallBack(this, StreamCallBack);

												//增加录制回调
												BaseStream->RegisterDataCallBack(this, RecordCallBack);

												if (OldVideo && !OldVideo->CanEnterScene())//OldVideo不可进入场景，说明OldVideo在场景中
												{
													m_VideoList[i].VideoDevice->GlobalSourceEnterScene();
													m_VideoList[i].VideoDevice->SetCanEnterScene(false);

													OldVideo->GlobalSourceLeaveScene();
													OldVideo->SetCanEnterScene(true);
												}
												//新建的需要BeginScene
												BaseStream->BeginScene();

												//Config->removeMember("DeviceSourceID");
												//查找所有包括BaseVideo的LocalInstance和LiveInstance把它们的VideoDevice reset;
												CSLiveManager::GetInstance()->ResetDevice(BaseVideo, m_VideoList[i].VideoDevice, false, OldVideo.get());
											}
											else
											{
												m_VideoList[i].VideoDevice->UpdateSettings(*Config.get());
											}
										}
									}
									break;
								}
							}
							BaseVideo->UnRegisterDataCallBack(this);

							
							//LeaveCriticalSection(&VideoSection);
						}
						BaseVideo->UpdateSettings(*Config.get());
					}

				}
				else
				{
					IBaseAudio *BaseAudio = dynamic_cast<IBaseAudio*>(Object);
					if (BaseAudio)
					{
						BaseAudio->UpdateSettings(*Config.get());
					}
				}

			}
			else
			{
				*Config = Old;
				BUTEL_THORWERROR("取消操作");
			}
		}
		else
		{
			BUTEL_THORWERROR("没有找到%s类的配置函数", Object->GainClassName());
		}

	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::StreamCallBack(void *Context, CSampleData* Data)
{
	CInstanceProcess *Process = reinterpret_cast<CInstanceProcess*>(Context);
	if (Process && !Process->bNoPreView)
	{
		Process->MultiRender->Render(Data, Data->bDisableAudio);
	}
}

void CInstanceProcess::RecordCallBack(void *Context, CSampleData* Data)
{
	CInstanceProcess *Process = reinterpret_cast<CInstanceProcess*>(Context);
	if (Process && !Process->bNoPreView)
	{
		if (Data)
		{
			Data->timestamp = GetQPCNS() / 1000000; //必须自己打时戳

			Process->bCanRecord = (*Data->UserData)["UseRecorder"].asInt() == 1;
			if (G_LiveInstance && G_LiveInstance->bStartLive && (Data->colorType == DeviceOutputType_I420 || Data->colorType == DeviceOutputType_YV12))
					Process->ProcessRecord(Data);
			else
			{
				Process->StopRecord();
			}
		}
		else if (Process->bRecord)
		{
			Process->StopRecord();
		}
	}
}

string CInstanceProcess::ReNameStream(uint64_t iStreamID, const char *NewName)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu,NewName = %s", __FUNCTION__, iStreamID, NewName);
	bool bFind = false;
	string RetStr;
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			RetStr = (*m_VideoList[i].Config)["Name"].asString().c_str();
			(*m_VideoList[i].Config)["Name"] = NewName;
			m_VideoList[i].VideoStream->RenameSource(NewName);
			if (m_VideoList[i].VideoDevice)
				m_VideoList[i].VideoDevice->RenameSource(NewName);

			bFind = true;
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s oldName = %s,NewName = %s", __FUNCTION__,RetStr.c_str(), NewName);
			break;
		}
	}

	if (!bFind)
	{
		for (UINT j = 0; j < m_AudioList.Num(); ++j)
		{
			if ((uint64_t)m_AudioList[j].AudioStream.get() == iStreamID)
			{
				if (m_AudioList[j].VideoStream)
				{
					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end! Error occured!!", __FUNCTION__);
					BUTEL_THORWERROR("该音频Id %llu 有视频,不允许重命名!", iStreamID);
				}

				RetStr = (*m_AudioList[j].Config)["Name"].asString().c_str();

				(*m_AudioList[j].Config)["Name"] = NewName;
				bFind = true;
				break;
			}
		}
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);

	return RetStr;
}

void CInstanceProcess::ReNameStreamSec(uint64_t iStreamID, const char *NewName)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode begin! StreamId = %llu,NewName = %s", __FUNCTION__, iStreamID, NewName);
	bool bFind = false;
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			if (0 == strcmp(m_VideoList[i].VideoStream->GainClassName(),"AgentSource"))
					(*m_VideoList[i].Config)["SecName"] = NewName;
			else
			{
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
				BUTEL_THORWERROR("StreamID为 %llu 的流不是区域占位源", iStreamID);
			}
			bFind = true;
			break;
		}
	}

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!,Error occured", __FUNCTION__);
		BUTEL_THORWERROR("没有找到StreamID为 %llu 的流", iStreamID);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invode end!", __FUNCTION__);
}

void CInstanceProcess::GetStreamSize(uint64_t StreamID, UINT *Width, UINT *Height)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu", __FUNCTION__, StreamID);
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == StreamID)
		{
			Vect2 Size;
			if (m_VideoList[i].VideoDevice)
			{
				Size = m_VideoList[i].VideoDevice->GetSize();
			}
			else
			{
				Size = m_VideoList[i].VideoStream->GetSize();
			}
			
			*Width = Size.x;
			*Height = Size.y;
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, StreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", StreamID);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, StreamID);
}

void CInstanceProcess::SetCrop(uint64_t iStreamID, float left, float top, float right, float bottom)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			m_VideoList[i].Crop.x = left;
			m_VideoList[i].Crop.y = top;
			m_VideoList[i].Crop.z = bottom;
			m_VideoList[i].Crop.w = right;
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
}

void CInstanceProcess::SetPlayPreAudio(uint64_t iStreamID, bool *bRet)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu", __FUNCTION__, iStreamID);
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}
	if (MultiRender)
		*bRet = MultiRender->SetAudioRender();
	else
	{
		*bRet = false;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! PlayAudio = %s", __FUNCTION__, *bRet ? "true" : "false");
}

void CInstanceProcess::SetTopest(uint64_t iStreamID, bool bTopest)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! StreamId = %llu, bTopest = %s", __FUNCTION__, iStreamID,bTopest ? "true":"false");
	bool bFind = false;

	EnterCriticalSection(&VideoSection);
	for (UINT i = 0; i < m_VideoList.Num(); ++i)
	{
		if ((uint64_t)m_VideoList[i].VideoStream.get() == iStreamID)
		{
			m_VideoList[i].bTop = bTopest;

			if (bTopest)
			{
				MoveStream(MoveTop, i);
			}
			else
			{
				//往下移动直到没有顶层源的位置之上
				for (int j = int(i - 1); j >= 0; --j)
				{
					if (m_VideoList[j].bTop)
					{
						MoveStream(MoveDown, j + 1);
					}
					else
					{
						break;
					}
				}
			}
			bFind = true;
			break;
		}
	}
	LeaveCriticalSection(&VideoSection);

	if (!bFind)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! StreamId = %llu", __FUNCTION__, iStreamID);
		BUTEL_THORWERROR("没有找到或该流不是视频流,StreamID为 %llu", iStreamID);
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}
