#include "RTMPStuff.h"
#include "RTMPPublisherVector.h"
#include "RtmpPublish.h"
#include "HTTPClient.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

#define MAX_BUFFERED_PACKETS 10

String RTMPPublisher::strRTMPErrors;
int    RTMPPublisher::ReTryCount = 0;

String IntString1(int i, int radix)
{
	TCHAR lpString[14];
	itots_s(i, lpString, 13, radix);
	return String(lpString);
}

std::wstring Utf82WChar(char *pUtf8, int nLen)
{
	if (0 == nLen)
	{
		return TEXT("");
	}
	int nSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUtf8, nLen, 0, 0);
	if (nSize <= 0) return NULL;

	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst) return NULL;

	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUtf8, nLen, pwszDst, nSize);
	pwszDst[nSize] = 0;

	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
	for (int i = 0; i < nSize; i++)
		pwszDst[i] = pwszDst[i + 1];

	std::wstring wcharString(pwszDst);
	delete [] pwszDst;
	return wcharString;
}

int WcharToUtf8(std::wstring strSrc, char *pUtf8, int &nLen)
{
	if (strSrc.empty())
	{
		nLen = 0;
		return 0;
	}
	int nSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), pUtf8, nLen, NULL, NULL);
	pUtf8[nSize] = 0;
	nLen = nSize;
	return 0;
}


std::wstring Utf82WChar(std::string str)
{
	if (str.empty())
	{
		return TEXT("");
	}
	int nSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
	if (nSize <= 0) return NULL;

	WCHAR *pwszDst = new WCHAR[nSize + 1];
	if (NULL == pwszDst) return NULL;

	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
	pwszDst[nSize] = 0;

	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
	for (int i = 0; i < nSize; i++)
		pwszDst[i] = pwszDst[i + 1];

	std::wstring wcharString(pwszDst);
	delete [] pwszDst;
	return wcharString;
}

void rtmp_log_output(int level, const char *format, va_list vl)
{
	int size = _vscprintf(format, vl);
	LPSTR lpTemp = (LPSTR)new CHAR[(size + 1)];
	vsprintf_s(lpTemp, size + 1, format, vl);

	// OSDebugOut(TEXT("%S\r\n"), lpTemp);
	//Log(TEXT("%S\r\n"), lpTemp);

	delete [] lpTemp;
}

void RTMPPublisher::librtmpErrorCallback(int level, const char *format, va_list vl)
{
	char ansiStr[1024];
	TCHAR logStr[1024];

	if (level > RTMP_LOGERROR)
		return;

	vsnprintf(ansiStr, sizeof(ansiStr)-1, format, vl);
	ansiStr[sizeof(ansiStr)-1] = 0;

	MultiByteToWideChar(CP_ACP, 0, ansiStr, -1, logStr, _countof(logStr) - 1);
	Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:librtmp error: %s", WcharToAnsi(logStr).c_str());
	OSEnterMutex(m_handleLog);
	strRTMPErrors << logStr << TEXT("\n");
	OSLeaveMutex(m_handleLog);
}

String RTMPPublisher::GetRTMPErrors()
{
	String strRTMPErrorsR = TEXT("");
	OSEnterMutex(m_handleLog);
	strRTMPErrorsR = strRTMPErrors;
	OSLeaveMutex(m_handleLog);
	return strRTMPErrorsR;
}

int WcharToString(std::wstring strSrc, std::string &strDesc)
{
	char html[1024 * 10];
	int  nLen = 1024 * 10;
	int nSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), html, nLen, NULL, NULL);
	html[nSize] = 0;
	strDesc = html;
	return 0;
}

bool RTMPPublisher::parseRTMPSrvAddr(String strJson, VEC_IP_ADDR &vecIp)
{
	std::string strHtml;
	WcharToString(strJson.Array(), strHtml);
	Json::Reader cReader;
	Json::Value root;
	Json::Value SrvState;
	Json::Value SrvList;
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:parseRTMPSrvAddr json is : %s",strHtml.c_str());
	bool bRet = cReader.parse(strHtml, root);
	if (!bRet)
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:cant parse json: %s", WcharToAnsi(strJson.Array()).c_str());
		return false;
	}
	SrvState = root["state"];
	int nRet = SrvState["rc"].asInt();
	if (0 != nRet)
	{
		Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:json return error %d", nRet);
		return false;
	}
	SrvList = root["IESServerList"];
	int nListCnt = SrvList["rows"].size();
	if (nListCnt <= 0)
	{
		Log::writeError(LOG_RTSPSERV,1,"json error count %d", nListCnt);
		return false;
	}
	for (int i = 0; i < nListCnt; i++)
	{
		StIPAddr  temp;
		temp.m_nPort = SrvList["rows"][i]["port"].asInt();
		temp.m_strIpAddr = Asic2WChar(SrvList["rows"][i]["ip"].asString()).c_str();
		Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:定位出推流服务器 %s:%d", WcharToAnsi(temp.m_strIpAddr).c_str(), temp.m_nPort);
		vecIp.push_back(temp);
	}
	return true;
}
bool RTMPPublisher::GetLastError()
{
	bool result = false;

	EnterCriticalSection(&hErrorMutex);
	result = bError;
	LeaveCriticalSection(&hErrorMutex);

	return result;
}

void RTMPPublisher::TriggerError()
{
	EnterCriticalSection(&hErrorMutex);
	bError = true;
	LeaveCriticalSection(&hErrorMutex);
}

HANDLE                              RTMPPublisher::m_handleLock = OSCreateMutex();
HANDLE                              RTMPPublisher::m_handleLog = OSCreateMutex();
std::map<std::wstring, VEC_IP_ADDR> RTMPPublisher::m_pMap_addr;

RTMPPublisher::RTMPPublisher(String& prefix, CInstanceProcess *Instances, bool bBackPush)
{
	__Instances = Instances;
	bUseBack = bBackPush;
	Prefix = prefix;
	bDestroy = false;
	hSendThread = NULL;
	hConnectionThread = NULL;
	hSocketThread = NULL;
	rtmp = NULL;
	dataBuffer = NULL;
	dataBufferSize = 1;
	hBufferEvent = NULL;
	hBufferSpaceAvailableEvent = NULL;
	hWriteEvent = NULL;

	hSendLoopExit = NULL;
	hSocketLoopExit = NULL;
	hSendBacklogEvent = NULL;
	hSocketThread = NULL;
	bConnecting = false;
	bConnected = false;
	bStopping = false;
	curDataBufferLen = 0;
	bytesSent = 0;
	bSentFirstKeyframe = false;
	bBufferFull = false;
	bStreamStarted = false;
	bSentFirstAudio = false;
	audioTimeOffset = 0;
	totalVideoFrames = 0;
	minFramedropTimestsamp = 0;
	currentBufferSize = 0;
	lastBFrameDropTime = 0;
	InitializeCriticalSection(&hDataBufferMutex);

	
	std::wstring strIndex = Prefix.Array();
	strIndex = strIndex.substr(7, strIndex.size() - 7);
	m_index = String(strIndex.c_str()).ToInt();
	bIsMain = true;

	bError = false;
	ConnectedDelayed = false;
	ConnectedInfo = 0;
	SecondsConnected = 0;
	InitializeCriticalSection(&hErrorMutex);
	bFirstKeyframe = true;
	OldTime = GetQPCMS();

	hSendSempahore = CreateSemaphore(NULL, 0, 0x7FFFFFFFL, NULL);

	InitializeCriticalSection(&hDataMutex);
	InitializeCriticalSection(&hRTMPMutex);

	//------------------------------------------
	int retryTime = __Instances->LiveParam.LiveSetting.AutoConnect;

	if (bUseBack)
	{
		retryTime = __Instances->LiveParam.LiveSetting.AutoConnectSec;
	}

	if (retryTime > 60)      retryTime = 60;
	else if (retryTime < 0)  retryTime = 0;
	ReconnectDelayMillisecond = retryTime * 1000;

	bframeDropThreshold = 400;
	if (bframeDropThreshold < 50)        bframeDropThreshold = 50;
	else if (bframeDropThreshold > 1000) bframeDropThreshold = 1000;

	dropThreshold = 600;
	if (dropThreshold < 50)        dropThreshold = 50;
	else if (dropThreshold > 1000) dropThreshold = 1000;

	
	lowLatencyMode = LL_MODE_NONE;

	bFastInitialKeyframe = false;

	strRTMPErrors.Clear();
}

bool RTMPPublisher::Init(UINT tcpBufferSize)
{
	//------------------------------------------
	//Log(TEXT("Using Send Buffer Size: %u"), sendBufferSize);

	rtmp->m_customSendFunc = (CUSTOMSEND)RTMPPublisher::BufferedSend;
	rtmp->m_customSendParam = this;
	rtmp->m_bCustomSend = TRUE;

	//------------------------------------------

	int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);

	if (!getsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
	{
		Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:SO_SNDBUF was at %u", curTCPBufSize);

		if (curTCPBufSize < int(tcpBufferSize))
		{
			if (!setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize)))
			{
				if (!getsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
				{
					if (curTCPBufSize != tcpBufferSize)
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:Could not raise SO_SNDBUF to %u, value is now %d", tcpBufferSize, curTCPBufSize);

					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:SO_SNDBUF is now %d", curTCPBufSize);
				}
				else
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:getsockopt: Failed to query SO_SNDBUF, error %d", WSAGetLastError());
				}
			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:setsockopt: Failed to raise SO_SNDBUF to %u, error %d", tcpBufferSize, WSAGetLastError());
			}
		}
	}
	else
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:getsockopt: Failed to query SO_SNDBUF, error %d", WSAGetLastError());
	}

	//------------------------------------------

	hSendThread = CreateThread(NULL,0,(XTHREAD)RTMPPublisher::SendThread, this,0,NULL);

	hBufferEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hBufferSpaceAvailableEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	hSendLoopExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	hSocketLoopExit = CreateEvent(NULL, TRUE, FALSE, NULL);
	hSendBacklogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	dataBuffer = new BYTE[dataBufferSize];

	hSocketThread = CreateThread(NULL,0,(XTHREAD)RTMPPublisher::SocketThread, this,0,NULL);

	//------------------------------------------

	packetWaitType = 0;
	
	return true;
}

char* RTMPPublisher::EncMetaData(char *enc, char *pend, bool bFLVFile, bool bBack)
{
	VideoEncoder *Video;
	if (bUseBack)
	{
		Video = __Instances->GetVideoEncoder_back();
	}
	else
	{
		Video = __Instances->GetVideoEncoder();
	}
	AudioEncoder *Audio = __Instances->GetAudioEncoder();
	int    maxBitRate = Video->GetBitRate();
	int    fps = Video->GetFps();
	int    audioBitRate = Audio->GetBitRate();
	CTSTR  lpAudioCodec = Audio->GetCodec();
	//double audioCodecID;
	const AVal *av_codecFourCC;

#ifdef USE_AAC
	if (scmpi(lpAudioCodec, TEXT("AAC")) == 0)
	{
		av_codecFourCC = &av_mp4a;
		//audioCodecID = 10.0;
	}
	else
#endif
	{
		av_codecFourCC = &av_mp3;
		//audioCodecID = 2.0;
	}

	if (bFLVFile)
	{
		*enc++ = AMF_ECMA_ARRAY;
		enc = AMF_EncodeInt32(enc, pend, 14);
	}
	else
		*enc++ = AMF_OBJECT;

	enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);

	enc = AMF_EncodeNamedNumber(enc, pend, &av_width, double(__Instances->outputCX));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_height, double(__Instances->outputCY));


	enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//

	enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, double(maxBitRate));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, double(fps));

	enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, av_codecFourCC);//audioCodecID);//

	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, double(audioBitRate)); //ex. 128kb\s
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, double(Audio->GetsampleRate()));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, double(Audio->GetnumChannels()));
	//enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo,         true);

	enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, Audio->GetnumChannels() == 2);
	const AVal Vision =  AVC("SLive");
	enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &Vision);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	return enc;
}


void RTMPPublisher::InitEncoderData()
{
	if (encoderDataInitialized)
		return;

	encoderDataInitialized = true;
	if (bUseBack)
	{
		VideoBitRate = __Instances->GetVideoEncoder_back()->GetBitRate();
	}
	else
	{
		VideoBitRate = __Instances->GetVideoEncoder()->GetBitRate();
	}
	AudioBitRate = __Instances->GetAudioEncoder()->GetBitRate();
	dataBufferSize = (VideoBitRate + AudioBitRate) / 8 * 1024;
	if (dataBufferSize < 131072)
		dataBufferSize = 131072;

	metaDataPacketBuffer.resize(2048);

	char *enc = metaDataPacketBuffer.data() + RTMP_MAX_HEADER_SIZE;
	char *pend = metaDataPacketBuffer.data() + metaDataPacketBuffer.size();
	enc = AMF_EncodeString(enc, pend, &av_setDataFrame);
	enc = AMF_EncodeString(enc, pend, &av_onMetaData);
	enc = EncMetaData(enc, pend, false, !bIsMain); //m_index 0 1 是主编码器， m_index 2 3 是附编码器
	metaDataPacketBuffer.resize(enc - metaDataPacketBuffer.data());

	__Instances->GetAudioHeaders(audioHeaders);
	if (bUseBack)
	{
		__Instances->GetVideoHeaders_back(videoHeaders);
	}
	else
	{
		__Instances->GetVideoHeaders(videoHeaders);
	}
}

RTMPPublisher* RTMPPublisher::Clone()
{
	if (bDestroy == false)
	{
		CloneTag = true;
		Destructor();
		CloneTag = false;
	}
	RTMPPublisher* publisher = new RTMPPublisher(Prefix,__Instances,bUseBack);
	publisher->ConnectedDelayed = true;

	return publisher;
}

RTMPPublisher* RTMPPublisher::CloneWithNoDelayConnect()
{
	if (bDestroy == false)
	{
		CloneTag = true;
		Destructor();
		CloneTag = false;
	}
	RTMPPublisher* publisher = new RTMPPublisher(Prefix, __Instances, bUseBack);

	return publisher;
}

void RTMPPublisher::Destructor()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	if (bDestroy)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
		return;
	}

	bDestroy = true;
	bStopping = true;

	DeleteCriticalSection(&hErrorMutex);
	//we're in the middle of connecting! wait for that to happen to avoid all manner of race conditions
	if (hConnectionThread)
	{
		//the connect thread could be stalled in a blocking call, kill the socket to ensure it wakes up
		if (WaitForSingleObject(hConnectionThread, 0) == WAIT_TIMEOUT)
		{
			EnterCriticalSection(&hRTMPMutex);
			if (rtmp && rtmp->m_sb.sb_socket != -1)
			{
				closesocket(rtmp->m_sb.sb_socket);
				rtmp->m_sb.sb_socket = -1;
			}
			LeaveCriticalSection(&hRTMPMutex);
		}

		WaitForSingleObject(hConnectionThread, INFINITE);
		OSCloseThread(hConnectionThread);
	}

	DWORD startTime = GetQPCMS();

	//send all remaining buffered packets, this may block since it respects timestamps
	FlushBufferedPackets();

	Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:%s: Packet flush completed in %d ms",__FUNCTION__, GetQPCMS() - startTime);

	//OSDebugOut (TEXT("%d queued after flush\n"), queuedPackets.Num());

	if (hSendThread)
	{
		startTime = GetQPCMS();

		//this marks the thread to exit after current work is done
		SetEvent(hSendLoopExit);

		//these wake up the thread
		ReleaseSemaphore(hSendSempahore, 1, NULL);
		SetEvent(hBufferSpaceAvailableEvent);

		//wait 50 sec for all data to finish sending
		if (WaitForSingleObject(hSendThread, 5000) == WAIT_TIMEOUT)
		{
			Log::writeMessage(LOG_RTSPSERV,1,"~RTMPPublisher: Network appears stalled with %d / %d buffered, dropping connection!", curDataBufferLen, dataBufferSize);
			FatalSocketShutdown();

			//this will wake up and flush the sendloop if it's still trying to send out stuff
			ReleaseSemaphore(hSendSempahore, 1, NULL);
			SetEvent(hBufferSpaceAvailableEvent);

			Log::writeMessage(LOG_RTSPSERV, 1, "~RTMPPublisher: Send thread TimeOut 5000ms kill force!");
			TerminateThread(hSendThread, 0);
		}
		
		CloseHandle(hSendThread);

		Log::writeMessage(LOG_RTSPSERV,1,"~RTMPPublisher: Send thread terminated in %d ms", GetQPCMS() - startTime);
	}

	if (hSendSempahore)
		CloseHandle(hSendSempahore);

	//OSDebugOut (TEXT("*** ~RTMPPublisher hSendThread terminated (%d queued, %d buffered, %d data)\n"), queuedPackets.Num(), bufferedPackets.Num(), curDataBufferLen);

	if (hSocketThread)
	{
		startTime = GetQPCMS();

		//mark the socket loop to shut down after the buffer is empty
		SetEvent(hSocketLoopExit);

		//wake it up in case it already is empty
		SetEvent(hBufferEvent);

		//wait 60 sec for it to exit
		//OSTerminateThread(hSocketThread, 60000);
		if (WaitForSingleObjectEx(hSocketThread, 5000, 0) == WAIT_TIMEOUT)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "~RTMPPublisher: Socket thread TimeOut 5000ms kill force!");
			TerminateThread(hSocketThread, 0);
		}

		CloseHandle(hSocketThread);

		Log::writeMessage(LOG_RTSPSERV,1,"~RTMPPublisher: Socket thread terminated in %d ms", GetQPCMS() - startTime);
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "*** ~RTMPPublisher hSocketThread terminated (%d queued, %d buffered, %d data)", queuedPackets.Num(), bufferedPackets.Num(), curDataBufferLen);

	if (rtmp)
	{
		if (RTMP_IsConnected(rtmp))
		{
			startTime = GetQPCMS();

			//at this point nothing should be in the buffer, flush out what remains to the net and make it blocking
			curDataBufferLen = 0;
			FlushDataBuffer();

			//disable the buffered send, so RTMP_* functions write directly to the net (and thus block)
			rtmp->m_bCustomSend = 0;

			//manually shut down the stream and issue a graceful socket shutdown
			// 姚孟来-不再发送删除流，停止太慢,暴力关闭
			//RTMP_DeleteStream(rtmp);
			//Log(TEXT("~RTMPPublisher: RTMP_DeleteStream in %d ms"), OSGetTime() - startTime);

			//int tcpBufferSize = 1024*8;
			//int bError = setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize));
			//shutdown(rtmp->m_sb.sb_socket, SD_SEND);
			closesocket(rtmp->m_sb.sb_socket);

			Log::writeMessage(LOG_RTSPSERV, 1, "~RTMPPublisher: Final socket shutdown completed in %d ms", GetQPCMS() - startTime);

			//this waits for the socket shutdown to complete gracefully
// 			for (;;)
// 			{
// 				char buff[1024];
// 				int ret;
// 
// 				ret = recv(rtmp->m_sb.sb_socket, buff, sizeof(buff), 0);
// 				if (!ret)
// 					break;
// 				else if (ret == -1)
// 				{
// 					Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:~RTMPublisher: Received error %d while waiting for graceful shutdown.", WSAGetLastError());
// 					break;
// 				}
// 			}

			Log::writeMessage(LOG_RTSPSERV, 1, "~RTMPPublisher: Final socket shutdown completed in %d ms", GetQPCMS() - startTime);

			//OSDebugOut(TEXT("Graceful shutdown complete.\n"));			
		}

		//this closes the socket if not already done
		RTMP_Close(rtmp);
	}

	DeleteCriticalSection(&hDataMutex);

	startTime = GetQPCMS();
	while (bufferedPackets.Num())
	{
		//this should not happen any more...
		bufferedPackets[0].data.Clear();
		bufferedPackets.Remove(0);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "~RTMPPublisher: while (bufferedPackets.Num()) % d ms", GetQPCMS() - startTime);

	if (dataBuffer)
		delete[] dataBuffer;

	DeleteCriticalSection(&hDataBufferMutex);

	if (hBufferEvent)
		CloseHandle(hBufferEvent);

	if (hSendLoopExit)
		CloseHandle(hSendLoopExit);

	if (hSocketLoopExit)
		CloseHandle(hSocketLoopExit);

	if (hSendBacklogEvent)
		CloseHandle(hSendBacklogEvent);

	if (hBufferSpaceAvailableEvent)
		CloseHandle(hBufferSpaceAvailableEvent);

	if (hWriteEvent)
		CloseHandle(hWriteEvent);

	if (rtmp)
	{
		if (rtmp->Link.pubUser.av_val)
			Free(rtmp->Link.pubUser.av_val);
		if (rtmp->Link.pubPasswd.av_val)
			Free(rtmp->Link.pubPasswd.av_val);
		/*if (rtmp->Link.tcUrl.av_val)
		delete[] rtmp->Link.tcUrl.av_val;*/

		RTMP_Free(rtmp);
	}

	//--------------------------
	startTime = GetQPCMS();
	for (UINT i = 0; i<queuedPackets.Num(); i++)
		queuedPackets[i].data.Clear();
	queuedPackets.Clear();
	Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:~RTMPPublisher: queuedPackets.Clear %d ms", GetQPCMS() - startTime);

	double dBFrameDropPercentage = double(numBFramesDumped) / max(1, NumTotalVideoFrames())*100.0;
	double dPFrameDropPercentage = double(numPFramesDumped) / max(1, NumTotalVideoFrames())*100.0;
	
	OSEnterMutex(m_handleLog);
	strRTMPErrors.Clear();
	OSLeaveMutex(m_handleLog);

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	//--------------------------
}

RTMPPublisher::~RTMPPublisher()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始析构",__FUNCTION__);
	if (bDestroy == false)
	{
		Destructor();
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 结束析构",__FUNCTION__);
}

UINT RTMPPublisher::FindClosestQueueIndex(DWORD timestamp)
{
	UINT index;
	for (index = 0; index<queuedPackets.Num(); index++) {
		if (queuedPackets[index].timestamp > timestamp)
			break;
	}

	return index;
}

UINT RTMPPublisher::FindClosestBufferIndex(DWORD timestamp)
{
	UINT index;
	for (index = 0; index<bufferedPackets.Num(); index++) {
		if (bufferedPackets[index].timestamp > timestamp)
			break;
	}

	return index;
}

void RTMPPublisher::InitializeBuffer()
{
	bool bFirstAudio = true;
	for (UINT i = 0; i < bufferedPackets.Num(); i++) {
		TimedPacket &packet = bufferedPackets[i];

		//first, get the audio time offset from the first audio packet
		if (packet.type == PacketType_Audio) {
			if (bFirstAudio) {
				audioTimeOffset = packet.timestamp;
				//OSDebugOut(TEXT("Set audio offset: %d\n"), audioTimeOffset);
				bFirstAudio = false;
			}

			DWORD newTimestamp = packet.timestamp - audioTimeOffset;

			UINT newIndex = FindClosestBufferIndex(newTimestamp);
			if (newIndex < i) {
				bufferedPackets.MoveItem(i, newIndex);
				bufferedPackets[newIndex].timestamp = newTimestamp;
			}
			else {
				bufferedPackets[i].timestamp = newTimestamp;
			}
		}
	}
}

void RTMPPublisher::FlushBufferedPackets()
{
	if (!bufferedPackets.Num())
		return;

	QWORD startTime = GetQPCMS();
	DWORD baseTimestamp = bufferedPackets[0].timestamp;

	for (unsigned int i = 0; i < bufferedPackets.Num(); i++)
	{
		TimedPacket &packet = bufferedPackets[i];

// 		QWORD curTime;
// 		do
// 		{
// 			curTime = GetQPCMS();
// 			Sleep(1);
// 		} while (curTime - startTime < packet.timestamp - baseTimestamp);

// 		if (!CloneTag)
// 			SendPacketForReal(packet.data.Array(), packet.data.Num(), packet.timestamp, packet.type);

		packet.data.Clear();
	}

	bufferedPackets.Clear();
}

void RTMPPublisher::ProcessPackets()
{
	if (!bStreamStarted && !bStopping)
	{
		BeginPublishingInternal();
		bStreamStarted = true;
	}

	//never drop frames if we're in the shutdown sequence, just wait it out
	if (!bStopping)
	{
		if (queuedPackets.Num() && minFramedropTimestsamp < queuedPackets[0].timestamp)
		{
			DWORD queueDuration = (queuedPackets.Last().timestamp - queuedPackets[0].timestamp);

			DWORD curTime = GetQPCMS();

			if (queueDuration >= dropThreshold + audioTimeOffset)
			{
				minFramedropTimestsamp = queuedPackets.Last().timestamp;

				//OSDebugOut(TEXT("dropped all at %u, threshold is %u, total duration is %u, %d in queue\r\n"), currentBufferSize, dropThreshold + audioTimeOffset, queueDuration, queuedPackets.Num());

				//what the hell, just flush it all for now as a test and force a keyframe 1 second after
				while (DoIFrameDelay(false));

				if (packetWaitType > PacketType_VideoLow)
					RequestKeyframe(1000);
			}
			else if (queueDuration >= bframeDropThreshold + audioTimeOffset && curTime - lastBFrameDropTime >= dropThreshold + audioTimeOffset)
			{
				//OSDebugOut(TEXT("dropped b-frames at %u, threshold is %u, total duration is %u\r\n"), currentBufferSize, bframeDropThreshold + audioTimeOffset, queueDuration);

				while (DoIFrameDelay(true));

				lastBFrameDropTime = curTime;
			}
		}
	}

	if (queuedPackets.Num())
		ReleaseSemaphore(hSendSempahore, 1, NULL);
}

void RTMPPublisher::SendPacket(BYTE *data, UINT size, DWORD timestamp, PacketType type)
{
	if (ConnectedDelayed)
	{
		QWORD newTime = GetQPCMS();
		if (newTime - OldTime < ReconnectDelayMillisecond)
		{
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
			//ConnectedInfo = App->AddStreamInfo(strInfo, StreamInfoPriority_Critical);
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

	if (!bConnected && !bConnecting && !bStopping)
	{
		hConnectionThread = OSCreateThread((XTHREAD)CreateConnectionThread, this);
		bConnecting = true;
	}

	if (bFastInitialKeyframe)
	{
		if (!bConnected)
		{
			//while not connected, keep at most one keyframe buffered
			if (type != PacketType_VideoHighest)
				return;

			bufferedPackets.Clear();
		}

		if (bConnected && bFirstKeyframe)
		{
			bFirstKeyframe = false;
			firstTimestamp = timestamp;

			//send out our buffered keyframe immediately, unless this packet happens to also be a keyframe
			if (type != PacketType_VideoHighest && bufferedPackets.Num() == 1)
			{
				TimedPacket packet;
				mcpy(&packet, &bufferedPackets[0], sizeof(TimedPacket));
				bufferedPackets.Remove(0);
				packet.timestamp = 0;

				SendPacketForReal(packet.data.Array(), packet.data.Num(), packet.timestamp, packet.type);
			}
			else
				bufferedPackets.Clear();
		}
	}
	else
	{
		if (bFirstKeyframe)
		{
			if (!bConnected || type != PacketType_VideoHighest)
				return;

			firstTimestamp = timestamp;
			bFirstKeyframe = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "%s firstTimestamp %d", __FUNCTION__, firstTimestamp);
		}
	}

	//OSDebugOut (TEXT("%u: SendPacket (%d bytes - %08x @ %u)\n"), OSGetTime(), size, quickHash(data,size), timestamp);

	if (bufferedPackets.Num() == MAX_BUFFERED_PACKETS)
	{
		if (!bBufferFull)
		{
			InitializeBuffer();
			bBufferFull = true;
			Log::writeMessage(LOG_RTSPSERV, 1, "%s bufferedPackets[0].timestamps %d", __FUNCTION__, bufferedPackets[0].timestamp);
		}

		TimedPacket packet;
		mcpy(&packet, &bufferedPackets[0], sizeof(TimedPacket));
		bufferedPackets.Remove(0);

		SendPacketForReal(packet.data.Array(), packet.data.Num(), packet.timestamp, packet.type);
	}

	timestamp -= firstTimestamp;

	TimedPacket *packet;

	if (type == PacketType_Audio)
	{
		UINT newID;
		timestamp -= audioTimeOffset;

		newID = FindClosestBufferIndex(timestamp);
		packet = bufferedPackets.InsertNew(newID);
	}
	else
	{
		packet = bufferedPackets.CreateNew();
	}

	packet->data.CopyArray(data, size);
	packet->timestamp = timestamp;
	packet->type = type;

	/*for (UINT i=0; i<bufferedPackets.Num(); i++)
	{
	if (bufferedPackets[i].data.Array() == 0)
	nop();
	}*/
}

void RTMPPublisher::SendPacketForReal(BYTE *data, UINT size, DWORD timestamp, PacketType type)
{
	//OSDebugOut (TEXT("%u: SendPacketForReal (%d bytes - %08x @ %u, type %d)\n"), OSGetTime(), size, quickHash(data,size), timestamp, type);
	//Log(TEXT("packet| timestamp: %u, type: %u, bytes: %u"), timestamp, (UINT)type, size);

	EnterCriticalSection(&hDataMutex);

	if (bConnected)
	{
		ProcessPackets();

		bool bSend = bSentFirstKeyframe;

		if (!bSentFirstKeyframe)
		{
			if (type == PacketType_VideoHighest)
			{
				bSend = true;

				Log::writeMessage(LOG_RTSPSERV, 1, "got First Videoframe timestamp %d", timestamp);
				//OSDebugOut(TEXT("got keyframe: %u\r\n"), OSGetTime());
			}
		}

		if (bSend)
		{
			if (!bSentFirstAudio && type == PacketType_Audio)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "got First Audioframe timestamp %d", timestamp);
				timestamp = 0;
				bSentFirstAudio = true;
			}

			totalFrames++;
			if (type != PacketType_Audio)
				totalVideoFrames++;

			bool bAddPacket = false;
			if (type >= packetWaitType)
			{
				if (type != PacketType_Audio)
					packetWaitType = PacketType_VideoDisposable;

				bAddPacket = true;
			}

			if (bAddPacket)
			{
				List<BYTE> paddedData;
				paddedData.SetSize(size + RTMP_MAX_HEADER_SIZE);
				mcpy(paddedData.Array() + RTMP_MAX_HEADER_SIZE, data, size);

				if (!bSentFirstKeyframe)
				{
					DataPacket sei;
					if (bUseBack)
					{
						__Instances->GetVideoEncoder_back()->GetSEI(sei);
					}
					else
					{
						__Instances->GetVideoEncoder()->GetSEI(sei);
					}
					
					paddedData.InsertArray(RTMP_MAX_HEADER_SIZE + 5, sei.lpPacket, sei.size);

					bSentFirstKeyframe = true;
				}

				currentBufferSize += paddedData.Num();

				UINT droppedFrameVal = queuedPackets.Num() ? queuedPackets.Last().distanceFromDroppedFrame + 1 : 10000;

				UINT id = FindClosestQueueIndex(timestamp);

				NetworkPacket *queuedPacket = queuedPackets.InsertNew(id);
				queuedPacket->distanceFromDroppedFrame = droppedFrameVal;
				queuedPacket->data.TransferFrom(paddedData);
				queuedPacket->timestamp = timestamp;
				queuedPacket->type = type;
			}
			else
			{
				if (type < PacketType_VideoHigh)
					numBFramesDumped++;
				else
					numPFramesDumped++;
			}
		}
	}

	while (queuedPackets.Num() > 150000)
	{
		queuedPackets[0].data.Clear();
		queuedPackets.Remove(0);
	}

	LeaveCriticalSection(&hDataMutex);
}

void RTMPPublisher::BeginPublishingInternal()
{
	RTMPPacket packet;

	packet.m_nChannel = 0x03;     // control channel (invoke)
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = RTMP_PACKET_TYPE_INFO;
	packet.m_nTimeStamp = 0;
	packet.m_nInfoField2 = rtmp->m_stream_id;
	packet.m_hasAbsTimestamp = TRUE;
	packet.m_body = metaDataPacketBuffer.data() + RTMP_MAX_HEADER_SIZE;

	packet.m_nBodySize = metaDataPacketBuffer.size() - RTMP_MAX_HEADER_SIZE;
	if (!RTMP_SendPacket(rtmp, &packet, FALSE))
	{
		//App->PostStopMessage();
		TriggerError();
		return;
	}

	//----------------------------------------------

	List<BYTE> packetPadding;

	//----------------------------------------------

	packet.m_nChannel = 0x05; // source channel
	packet.m_packetType = RTMP_PACKET_TYPE_AUDIO;

	packetPadding.SetSize(RTMP_MAX_HEADER_SIZE);
	packetPadding.AppendArray(audioHeaders.lpPacket, audioHeaders.size);

	packet.m_body = (char*)packetPadding.Array() + RTMP_MAX_HEADER_SIZE;
	packet.m_nBodySize = audioHeaders.size;
	if (!RTMP_SendPacket(rtmp, &packet, FALSE))
	{
		//App->PostStopMessage();
		TriggerError();
		return;
	}

	//----------------------------------------------
	packet.m_nChannel = 0x04; // source channel
	packet.m_headerType = RTMP_PACKET_SIZE_LARGE;
	packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;

	packetPadding.SetSize(RTMP_MAX_HEADER_SIZE);
	packetPadding.AppendArray(videoHeaders.lpPacket, videoHeaders.size);

	packet.m_body = (char*)packetPadding.Array() + RTMP_MAX_HEADER_SIZE;
	packet.m_nBodySize = videoHeaders.size;
	if (!RTMP_SendPacket(rtmp, &packet, FALSE))
	{
		//App->PostStopMessage();
		TriggerError();
		return;
	}
}

void RTMPPublisher::BeginPublishing()
{
}

void LogInterfaceType(RTMP *rtmp)
{
	MIB_IPFORWARDROW    route;
	DWORD               destAddr;
	CHAR                hostname[256];

	if (rtmp->Link.hostname.av_len >= sizeof(hostname)-1)
		return;

	strncpy(hostname, rtmp->Link.hostname.av_val, sizeof(hostname)-1);
	hostname[rtmp->Link.hostname.av_len] = 0;

	HOSTENT *h = gethostbyname(hostname);
	if (!h)
		return;

	destAddr = *(DWORD *)h->h_addr_list[0];

	char str[INET_ADDRSTRLEN];
	PCSTR Addr = inet_ntop(AF_INET, &destAddr, str, sizeof str);

	if (Addr)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "hostname %s 解析 Ip %s", hostname,Addr);
	}

	if (!GetBestRoute(destAddr, rtmp->m_bindIP.addr.sin_addr.S_un.S_addr, &route))
	{
		MIB_IFROW row;
		zero(&row, sizeof(row));
		row.dwIndex = route.dwForwardIfIndex;

		if (!GetIfEntry(&row))
		{
			DWORD speed = row.dwSpeed / 1000000;
			char *type;
			String otherType;
			std::string TemType;

			if (row.dwType == IF_TYPE_ETHERNET_CSMACD)
				type = "ethernet";
			else if (row.dwType == IF_TYPE_IEEE80211)
				type = "802.11";
			else
			{
				otherType = FormattedString(TEXT("type %d"), row.dwType);

				TemType = WcharToAnsi(otherType.Array());

				type = (char *)TemType.c_str();
			}

			Log::writeMessage(LOG_RTSPSERV,1,"  Interface: %s (%s, %d mbps)", row.bDescr, type, speed);
		}
	}
}

BOOL RTMPParseURL(String url, String &strHost, unsigned int &port)
{
	wchar_t *p, *end, *col, *ques, *slash;

	port = 0;
	p = wcsstr(url.Array(), L"://");
	if (!p)
	{
		Log::writeError(LOG_RTSPSERV,1,"SLive_Log:%s RTMP URL: No :// in url! %s", __FUNCTION__,WcharToAnsi(url.Array()).c_str());
		return FALSE;
	}
	{
		int len = (int)(p - url.Array());
		if (len == 4 && wcsnicmp(url, L"rtmp", 4) == 0)
			;
		else if (len == 5 && wcsnicmp(url.Array(), L"rtmpt", 5) == 0)
			;
		else if (len == 5 && wcsnicmp(url.Array(), L"rtmps", 5) == 0)
			;
		else if (len == 5 && wcsnicmp(url.Array(), L"rtmpe", 5) == 0)
			;
		else if (len == 5 && wcsnicmp(url.Array(), L"rtmfp", 5) == 0)
			;
		else if (len == 6 && wcsnicmp(url.Array(), L"rtmpte", 6) == 0)
			;
		else if (len == 6 && wcsnicmp(url.Array(), L"rtmpts", 6) == 0)
			;
		else
		{
			Log::writeError(LOG_RTSPSERV,1,"SLive_Log:%s Unknown protocol! %s",__FUNCTION__, WcharToAnsi(url.Array()).c_str());
			goto parsehost;
		}
	}
parsehost:
	/* let's get the hostname */
	p += 3;

	/* check for sudden death */
	if (*p == 0)
	{
		Log::writeError(LOG_RTSPSERV, 1, "SLive_Log:%s No hostname in URL! %s", __FUNCTION__,WcharToAnsi(url.Array()).c_str());
		return FALSE;
	}
	end = p + wcslen(p);
	col = wcschr(p, L':');
	ques = wcschr(p, L'?');
	slash = wcschr(p, L'/');

	{
		int hostlen;
		if (slash)
			hostlen = slash - p;
		else
			hostlen = end - p;
		if (col && col - p < hostlen)
			hostlen = col - p;

		if (hostlen < 256)
		{
			strHost = L"";
			wchar_t phost[256] = { 0 };
			memcpy(phost, p, hostlen*sizeof(wchar_t));
			phost[hostlen] = 0;
			//Log(L"Parsed hosta    : %s", phost);
			strHost.AppendString(phost, hostlen);
			Log::writeMessage(LOG_RTSPSERV,1,"SLive_Log:%s Parsed host    : %s",__FUNCTION__, WcharToAnsi(strHost.Array()).c_str());
		}
		else
		{
			Log::writeError(LOG_RTSPSERV, 1, "SLive_Log:%s %s Hostname exceeds 255 characters!", __FUNCTION__, WcharToAnsi(url.Array()).c_str());
		}
		p += hostlen;
	}

	/* get the port number if available */
	if (*p == ':')
	{
		unsigned int p2;
		p++;
		p2 = _wtoi(p);
		if (p2 > 65535)
		{
			Log::writeError(LOG_RTSPSERV,1,"SLive_Log:%s Invalid port number %d!",__FUNCTION__,p2);
		}
		else
		{
			port = p2;
		}
	}
	return TRUE;
}

#include <string>
static char* ToCString(String ws)
{
	if (ws.IsEmpty())
	{
		return "";
	}
	std::string curLocale = setlocale(LC_ALL, NULL); // curLocale = "C";

	setlocale(LC_ALL, "chs");

	const wchar_t* _Source = ws.Array();
	size_t _Dsize = 2 * ws.Length() + 1;
	char *_Dest = new char[_Dsize];
	memset(_Dest, 0, _Dsize);
	wcstombs(_Dest, _Source, _Dsize);

	setlocale(LC_ALL, curLocale.c_str());
	return _Dest;
}

static bool ReplaceUrl(String url, String& palyUrl)
{
	if (url.IsEmpty())
	{
		return false;
	}

	url.KillSpaces();

	bool bHttp = ((url.Left(5).MakeLower() == "https") || (url.Left(4).MakeLower() == "http"));
	if (bHttp)
	{
		std::wstring strFind = url.Array();
		int nFind = strFind.find(TEXT("/live/"));
		if (-1 == nFind)
		{
			Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:%s 无法解析出Live", WcharToAnsi(url.Array()).c_str());
			return false;
		}

		palyUrl = strFind.substr(nFind, strFind.size() - nFind).c_str();
		return true;
	}

	// rtmp://192.168.13.11:1399/live/livestream?livetype=3&pareframe=1am?Bitrate=1608000
	url.FindReplace(TEXT("\\"), TEXT("/"));
	url.FindReplace(TEXT("rtmp://"), TEXT(""));
	url.FindReplace(TEXT("RTMP://"), TEXT(""));

	std::wstring strFind = url.Array();
	int nEnd = nEnd = strFind.length();

	int nFind = strFind.find(TEXT("/"));
	palyUrl = strFind.substr(nFind, nEnd - nFind).c_str();

	if (palyUrl.Length() == 0)
	{
		Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:error push url %s", WcharToAnsi(url.Array()).c_str());
	}

	return palyUrl.Length() > 0;
}

DWORD WINAPI RTMPPublisher::CreateConnectionThread(RTMPPublisher *publisher)
{
	//------------------------------------------------------
	// set up URL
	Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:CreateConnectionThread ID = %d Begin", GetCurrentThreadId());
	bool bSuccess = false;
	bool bCanRetry = true;

	RTMP *rtmp = nullptr;

	String failReason = L"Unkown Error";
	String strBindIP;
	//String OrgURL;

	String strURL = publisher->__Instances->PushURL0;
	String strPlayPath = publisher->__Instances->PushPath0;

	if (publisher->Prefix.CompareI(TEXT("Publish1")) || publisher->bUseBack)
	{
		strURL = publisher->__Instances->PushURL1;
		strPlayPath = publisher->__Instances->PushPath1;
		if (publisher->bUseBack)
		{
			strURL = publisher->__Instances->PushURL0_back;
			strPlayPath = publisher->__Instances->PushPath0_back;

			if (publisher->Prefix.CompareI(TEXT("Publish1")))
			{
				strURL = publisher->__Instances->PushURL1_back;
				strPlayPath = publisher->__Instances->PushPath1_back;
			}
		}
	}


	strURL.KillSpaces();
	strPlayPath.KillSpaces();

	LPSTR lpAnsiURL = NULL, lpAnsiPlaypath = NULL;

	ServiceIdentifier sid = {0,""};

	//--------------------------------

	if (!strURL.IsValid())
	{
		failReason = TEXT("No server specified to connect to");
		goto end;
	}

//	strURL = L"rtmp://114.112.74.22:1935/live/livetv.butel.com/";

	bool bHttp = ((strURL.Left(5).MakeLower() == "https") || (strURL.Left(4).MakeLower() == "http"));
	// A service ID implies the settings have come from the xconfig file.
	if (sid.id != 0 || sid.file.IsValid() || (bHttp))
	{
		String strHttpURL = "";
		// Stream urls start with RTMP. If there's an HTTP(S) then assume this is a web API call
		// to get the proper data.
		if (bHttp)
		{
			std::wstring strFind = strURL.Array();
			int nFind = strFind.find(TEXT("/live"));
			if (-1 == nFind)
			{
				goto end;
			}
			strHttpURL = strFind.substr(0, nFind).c_str();
			strHttpURL = strHttpURL + TEXT("/Getrtmppushserver");

			// Query the web API for stream details
			String web_url = strURL + strPlayPath;

			int responseCode;
			TCHAR extraHeaders[256];

			extraHeaders[0] = 0;
			OSEnterMutex(m_handleLock);
			VEC_IP_ADDR vecIp;
			if (m_pMap_addr.find(strHttpURL.Array()) == m_pMap_addr.end())
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:HTTPGetString begin! strHttpURL is : %s", WcharToAnsi(strHttpURL.Array()).c_str());
				String response = HTTPGetString(strHttpURL.Array(), extraHeaders, &responseCode);

				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:HTTPGetString end! strHttpURL is : %s,responseCode %d", WcharToAnsi(strHttpURL.Array()).c_str(), responseCode);
				if (responseCode != 200 && responseCode != 304)
				{
					failReason = TEXT("Webserver failed to respond with valid stream details.");
					OSLeaveMutex(m_handleLock);
					goto end;
				}

				Json::Value jsonRoot;
				XConfig apiData;

				// Expecting a response from the web API to look like this:
				// {"data":{"stream_url":"rtmp://some_url", "stream_name": "some-name"}}
				// A nice bit of JSON which is basically the same as the structure for XConfig.

				if (!parseRTMPSrvAddr(response, vecIp) || vecIp.size() < 0)
				{
					failReason = TEXT("Could not understand response from webserver.");
					OSLeaveMutex(m_handleLock);
					goto end;
				}
				m_pMap_addr[strHttpURL.Array()] = vecIp;
			}
			else
			{
				vecIp = m_pMap_addr[strHttpURL.Array()];
			}
			OSLeaveMutex(m_handleLock);
			StIPAddr tempIp;
			if (vecIp.size() <= 0)
			{
				Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:%s 路由返回空的推流服务器", WcharToAnsi(strHttpURL.Array()).c_str());
				goto end;
			}
			else if (vecIp.size() <= 1)
			{
				tempIp = vecIp[0];
			}
			else
			{
				// 主码流取第一个
				if (publisher->Prefix.Compare(TEXT("Publish0")) || publisher->Prefix.Compare(TEXT("Publish2")))
				{
					tempIp = vecIp[0];
				}
				else
				{
					tempIp = vecIp[1];
					//tempIp = vecIp[0];
				}
			}
			wchar_t newUrl[1024];
			std::wstring strFind1 = strURL.Array();
			int nFind1 = strFind1.find(TEXT("/live/"));
			if (-1 == nFind1)
			{
				Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:%s 无法解析出Live", WcharToAnsi(strURL.Array()).c_str());
				goto end;
			}
			wsprintf(newUrl, TEXT("rtmp://%s:%d%s"), tempIp.m_strIpAddr.c_str(), tempIp.m_nPort, strFind1.substr(nFind, strFind1.size() - nFind).c_str());
			strURL = newUrl;
			Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Web API returned URL: %s", WcharToAnsi(publisher->Prefix.Array()).c_str(), WcharToAnsi(strURL.Array()).c_str());
			// We could have read an error string back from the server.
			// So we need to trap any missing bits of data.

			//strPlayPath = p_stream_name_data->GetData();

			Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:Web API returned URL: %s", WcharToAnsi(strURL.Array()).c_str());
		}

		//(TEXT("Using RTMP service: %s"), service->GetName());
		Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:Server selection: %s", WcharToAnsi(strURL.Array()).c_str());
	}
// 	//临时替换IP和端口
// 	strURL = L"rtmp://114.112.74.22:1935/live/livetv.butel.com/";
// 	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 替换的推流地址为 %s%s", __FUNCTION__, WcharToAnsi(strURL.Array()).c_str(), WcharToAnsi(strPlayPath.Array()).c_str());
// 	//临时替换结束
	// 主推流地址发送录制
	if (publisher->Prefix.Compare(TEXT("Publish0")) || publisher->Prefix.Compare(TEXT("Publish2")))
	{
		String  strHost;
		unsigned int port;
		if (!RTMPParseURL(strURL, strHost, port))
		{
			Log::writeError(LOG_RTSPSERV,1,"LiveSDK_Log:%s rtmp推流地址无法解析", WcharToAnsi(strURL.Array()).c_str());
			goto end;
		}
		//CNetInerface::GetInstance()->SetRtmpPort(strHost.Array(), port);
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s主流服务器推流地址获取成功%s path %s ", WcharToAnsi(publisher->Prefix.Array()).c_str(), WcharToAnsi(strURL.Array()).c_str(), WcharToAnsi(strPlayPath.Array()).c_str());
		//App->bCanReportNow = true;
	}
	else
	{
		Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:%s备用流服务器推流地址获取成功%s path %s ", WcharToAnsi(publisher->Prefix.Array()).c_str(), WcharToAnsi(strURL.Array()).c_str(), WcharToAnsi(strPlayPath.Array()).c_str());
	}


	EnterCriticalSection(&publisher->hRTMPMutex);
	publisher->rtmp = RTMP_Alloc();

	rtmp = publisher->rtmp;
	RTMP_Init(rtmp);

	RTMP_LogSetCallback(librtmpErrorCallback);

	LeaveCriticalSection(&publisher->hRTMPMutex);

	//RTMP_LogSetLevel(RTMP_LOGERROR);

	lpAnsiURL = strURL.CreateUTF8String();
	lpAnsiPlaypath = strPlayPath.CreateUTF8String();

	if (!RTMP_SetupURL2(rtmp, lpAnsiURL, lpAnsiPlaypath))
	{
		failReason = L"Connection.CouldNotParseURL";
		goto end;
	}

	RTMP_EnableWrite(rtmp); //set it to publish

	rtmp->Link.swfUrl.av_len = rtmp->Link.tcUrl.av_len;
	rtmp->Link.swfUrl.av_val = rtmp->Link.tcUrl.av_val;

	rtmp->Link.flashVer.av_val = "FMLE/3.0 (compatible; FMSc/1.0)";
	rtmp->Link.flashVer.av_len = (int)strlen(rtmp->Link.flashVer.av_val);

	//-----------------------------------------

	UINT tcpBufferSize = 64 * 1024;

	if (tcpBufferSize < 8192)
		tcpBufferSize = 8192;
	else if (tcpBufferSize > 1024 * 1024)
		tcpBufferSize = 1024 * 1024;

	rtmp->m_outChunkSize = 4096;//RTMP_DEFAULT_CHUNKSIZE;//
	rtmp->m_bSendChunkSizeInfo = TRUE;

	rtmp->m_bUseNagle = TRUE;

	strBindIP = L"Default";
	if (scmp(strBindIP, TEXT("Default")))
	{
		//Log(TEXT("  Binding to non-default IP %s"), strBindIP.Array());
		rtmp->m_bindIP.addr.sin_family = AF_INET;
		rtmp->m_bindIP.addrLen = sizeof(rtmp->m_bindIP.addr);
		if (WSAStringToAddress(strBindIP.Array(), AF_INET, NULL, (LPSOCKADDR)&rtmp->m_bindIP.addr, &rtmp->m_bindIP.addrLen) == SOCKET_ERROR)
		{
			// no localization since this should rarely/never happen
			failReason = TEXT("WSAStringToAddress: Could not parse address");
			goto end;
		}
	}

	LogInterfaceType(rtmp);

	//-----------------------------------------

	QWORD startTime = GetQPCMS();

	if (!RTMP_Connect(rtmp, NULL))
	{
		failReason = L"Connection.CouldNotConnect";
		failReason << TEXT("\r\n\r\n") << RTMPPublisher::GetRTMPErrors();
		bCanRetry = strstr(WcharToAnsi(failReason.Array()).c_str(), "NetStream.Publish.Rejected") ? false : true;
		goto end;
	}

	Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:Completed handshake with %s in %u ms.", WcharToAnsi(strURL.Array()).c_str(), GetQPCMS() - startTime);

	if (!RTMP_ConnectStream(rtmp, 0))
	{
		failReason = L"Connection.InvalidStream";
		failReason << TEXT("\r\n\r\n") << RTMPPublisher::GetRTMPErrors();

		bCanRetry = strstr(WcharToAnsi(failReason.Array()).c_str(), "NetStream.Publish.Rejected") ? false : true;
		goto end;
	}

	//-----------------------------------------
	publisher->RequestKeyframe(1000);

	//-----------------------------------------

	bSuccess = true;

end:

	if (lpAnsiURL)
		Free(lpAnsiURL);

	if (lpAnsiPlaypath)
		Free(lpAnsiPlaypath);

	if (!bSuccess)
	{
		EnterCriticalSection(&publisher->hRTMPMutex);
		if (rtmp)
		{
			RTMP_Close(rtmp);
			RTMP_Free(rtmp);
			publisher->rtmp = NULL;
		}
		LeaveCriticalSection(&publisher->hRTMPMutex);

		if (!strURL.IsEmpty())
			Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:Connection to %s failed: %s", WcharToAnsi(strURL.Array()).c_str(), WcharToAnsi(failReason.Array()).c_str());
		else
		{
			Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:Connection failed: %s",WcharToAnsi(failReason.Array()).c_str());
		}
		if (!publisher->bStopping)
		{
			publisher->TriggerError();
		}

		publisher->bStopping = true;
	}
	else
	{
		publisher->Init(tcpBufferSize);
		publisher->bConnected = true;
		publisher->bConnecting = false;
		ReTryCount = 0;
	}

	Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:CreateConnectionThread ID = %d End", GetCurrentThreadId());
	return 0;
}

double RTMPPublisher::GetPacketStrain() const
{
	return (curDataBufferLen / (double)dataBufferSize) * 100.0;
	/*if(packetWaitType >= PacketType_VideoHigh)
	return min(100.0, dNetworkStrain*100.0);
	else if(bNetworkStrain)
	return dNetworkStrain*66.0;

	return dNetworkStrain*33.0;*/
}

QWORD RTMPPublisher::GetCurrentSentBytes()
{
	return bytesSent;
}

DWORD RTMPPublisher::NumDroppedFrames() const
{
	return numBFramesDumped + numPFramesDumped;
}

int RTMPPublisher::FlushDataBuffer()
{
	unsigned long zero = 0;

	//OSDebugOut (TEXT("*** ~RTMPPublisher FlushDataBuffer (%d)\n"), curDataBufferLen);

	//make it blocking again
	WSAEventSelect(rtmp->m_sb.sb_socket, NULL, 0);
	ioctlsocket(rtmp->m_sb.sb_socket, FIONBIO, &zero);

	int ret = 0;
	EnterCriticalSection(&hDataBufferMutex);
	if (curDataBufferLen > 0)
	{
		ret = send(rtmp->m_sb.sb_socket, (const char *)dataBuffer, curDataBufferLen, 0);
	}
	curDataBufferLen = 0;
	LeaveCriticalSection(&hDataBufferMutex);

	return ret;
}

void RTMPPublisher::SetupSendBacklogEvent()
{
	zero(&sendBacklogOverlapped, sizeof(sendBacklogOverlapped));

	ResetEvent(hSendBacklogEvent);
	sendBacklogOverlapped.hEvent = hSendBacklogEvent;

	idealsendbacklognotify(rtmp->m_sb.sb_socket, &sendBacklogOverlapped, NULL);
}

void RTMPPublisher::FatalSocketShutdown()
{
	//We close the socket manually to avoid trying to run cleanup code during the shutdown cycle since
	//if we're being called the socket is already in an unusable state.
	closesocket(rtmp->m_sb.sb_socket);
	rtmp->m_sb.sb_socket = -1;

	//anything buffered is invalid now
	curDataBufferLen = 0;

	if (!bStopping)
	{
		//if (AppConfig->GetInt(Prefix.Array(), TEXT("ExperimentalReconnectMode")) == 1 && AppConfig->GetInt(Prefix.Array(), TEXT("Delay")) == 0)
		//    App->NetworkFailed();
		//else
		//App->PostStopMessage();
		TriggerError();
	}
}

void RTMPPublisher::SocketLoop()
{
	bool canWrite = false;

	int delayTime;
	int latencyPacketSize;
	DWORD lastSendTime = 0;

	WSANETWORKEVENTS networkEvents;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	WSAEventSelect(rtmp->m_sb.sb_socket, hWriteEvent, FD_READ | FD_WRITE | FD_CLOSE);

	//Low latency mode works by delaying delayTime ms between calls to send() and only sending
	//a buffer as large as latencyPacketSize at once. This causes keyframes and other data bursts
	//to be sent over several sends instead of one large one.
	if (lowLatencyMode == LL_MODE_AUTO)
	{
		//Auto mode aims for a constant rate of whatever the stream bitrate is and segments into
		//MTU sized packets (test packet captures indicated that despite nagling being enabled,
		//the size of the send() buffer is still important for some reason). Note that delays
		//become very short at this rate, and it can take a while for the buffer to empty after
		//a keyframe.
		delayTime = 1400.0f / (dataBufferSize / 1000.0f);
		latencyPacketSize = 1460;
	}
	else if (lowLatencyMode == LL_MODE_FIXED)
	{
		//We use latencyFactor - 2 to guarantee we're always sending at a slightly higher
		//rate than the maximum expected data rate so we don't get backed up
		latencyPacketSize = dataBufferSize / (latencyFactor - 2);
		delayTime = 1000 / latencyFactor;
	}
	else
	{
		latencyPacketSize = dataBufferSize;
		delayTime = 0;
	}


	SetupSendBacklogEvent();


	HANDLE hObjects[4];

	hObjects[0] = hWriteEvent;
	hObjects[1] = hBufferEvent;
	hObjects[2] = hSendBacklogEvent;
	hObjects[3] = hSocketLoopExit;
	for (;;)
	{
		if (bStopping && WaitForSingleObject(hSocketLoopExit, 0) != WAIT_TIMEOUT)
		{
			EnterCriticalSection(&hDataBufferMutex);

			// 强制退出-姚孟来
			if (1/*curDataBufferLen == 0*/)
			{
				Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:RTMPPublisher::SocketLoop()网络发送模块丢弃数据%d字节", curDataBufferLen);
				curDataBufferLen = 0;
				//OSDebugOut (TEXT("Exiting on empty buffer.\n"));
				LeaveCriticalSection(&hDataBufferMutex);
				break;
			}

			//OSDebugOut (TEXT("Want to exit, but %d bytes remain.\n"), curDataBufferLen);
			LeaveCriticalSection(&hDataBufferMutex);
		}

		int status = WaitForMultipleObjects(4, hObjects, FALSE, INFINITE);
		if (status == WAIT_ABANDONED || status == WAIT_FAILED)
		{
			Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Aborting due to WaitForMultipleObjects failure");
			//App->PostStopMessage();
			TriggerError();
			return;
		}

		//wzq/A.工程/编解码/流媒体相关子系统/Source/OBS/V2.0/Source/RTMPPublisher.cpp
		if (status == WAIT_OBJECT_0)
		{
			//Socket event      
			if (WSAEnumNetworkEvents(rtmp->m_sb.sb_socket, NULL, &networkEvents))
			{
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Aborting due to WSAEnumNetworkEvents failure, %d", WSAGetLastError());
				//App->PostStopMessage();
				TriggerError();
				return;
			}

			if (networkEvents.lNetworkEvents & FD_WRITE)
				canWrite = true;

			if (networkEvents.lNetworkEvents & FD_CLOSE)
			{
				if (lastSendTime)
				{
					DWORD diff = GetQPCMS() - lastSendTime;
					Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:RTMPPublisher::SocketLoop: Received FD_CLOSE, %u ms since last send (buffer: %d / %d)", diff, curDataBufferLen, dataBufferSize);
				}

				if (bStopping)
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Aborting due to FD_CLOSE during shutdown, %d bytes lost, error %d", curDataBufferLen, networkEvents.iErrorCode[FD_CLOSE_BIT]);
				else
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Aborting due to FD_CLOSE, curDataBufferLen %d ,error %d", curDataBufferLen,networkEvents.iErrorCode[FD_CLOSE_BIT]);
				FatalSocketShutdown();
				return;
			}

			if (networkEvents.lNetworkEvents & FD_READ)
			{
				BYTE discard[16384];
				int ret, errorCode;
				BOOL fatalError = FALSE;

				for (;;)
				{
					ret = recv(rtmp->m_sb.sb_socket, (char *)discard, sizeof(discard), 0);
					if (ret == -1)
					{
						errorCode = WSAGetLastError();

						if (errorCode == WSAEWOULDBLOCK)
							break;

						fatalError = TRUE;
					}
					else if (ret == 0)
					{
						errorCode = 0;
						fatalError = TRUE;
					}

					if (fatalError)
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Socket error, recv() returned %d, GetLastError() %d", ret, errorCode);
						FatalSocketShutdown();
						return;
					}
				}
			}
		}
		else if (status == WAIT_OBJECT_0 + 2)
		{
			//Ideal send backlog event
			ULONG idealSendBacklog;

			if (!idealsendbacklogquery(rtmp->m_sb.sb_socket, &idealSendBacklog))
			{
				int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);
				if (!getsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
				{
					if (curTCPBufSize < (int)idealSendBacklog)
					{
						int bufferSize = (int)idealSendBacklog;
						setsockopt(rtmp->m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF, (const char *)&bufferSize, sizeof(bufferSize));
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Increasing send buffer to ISB %d (buffer: %d / %d", idealSendBacklog, curDataBufferLen, dataBufferSize);
					}
				}
				else
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Got hSendBacklogEvent but getsockopt() returned %d", WSAGetLastError());
			}
			else
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Got hSendBacklogEvent but WSAIoctl() returned %d", WSAGetLastError());

			SetupSendBacklogEvent();
			continue;
		}
		else if (status == WAIT_OBJECT_0 + 3)
		{
			//FatalSocketShutdown();
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Graceful loop exit by event");
			return;
		}

		if (canWrite)
		{
			bool exitLoop = false;
			do
			{
				EnterCriticalSection(&hDataBufferMutex);

				if (!curDataBufferLen)
				{
					//this is now an expected occasional condition due to use of auto-reset events, we could end up emptying the buffer
					//as it's filled in a previous loop cycle, especially if using low latency mode.
					LeaveCriticalSection(&hDataBufferMutex);
					//Log::writeMessage(LOG_RTSPSERV,1,"RTMPPublisher::SocketLoop: Trying to send, but no data available?!");
					break;
				}

				int ret;
				if (lowLatencyMode != LL_MODE_NONE)
				{
					int sendLength = min(latencyPacketSize, curDataBufferLen);
					ret = send(rtmp->m_sb.sb_socket, (const char *)dataBuffer, sendLength, 0);
				}
				else
				{
					ret = send(rtmp->m_sb.sb_socket, (const char *)dataBuffer, curDataBufferLen, 0);
				}

				if (ret > 0)
				{
					if (curDataBufferLen - ret)
						memmove(dataBuffer, dataBuffer + ret, curDataBufferLen - ret);
					curDataBufferLen -= ret;

					bytesSent += ret;

					if (lastSendTime)
					{
						DWORD diff = GetQPCMS() - lastSendTime;

						if (diff >= 1500)
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Stalled for %u ms to write %d bytes (buffer: %d / %d), unstable connection?", diff, ret, curDataBufferLen, dataBufferSize);

						totalSendPeriod += diff;
						totalSendBytes += ret;
						totalSendCount++;
					}

					lastSendTime = GetQPCMS();

					SetEvent(hBufferSpaceAvailableEvent);
				}
				else
				{
					int errorCode;
					BOOL fatalError = FALSE;

					if (ret == -1)
					{
						errorCode = WSAGetLastError();

						if (errorCode == WSAEWOULDBLOCK)
						{
							canWrite = false;
							LeaveCriticalSection(&hDataBufferMutex);
							Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: errorCode = WSAEWOULDBLOCK,发送窗口已满!!curDataBufferLen %d", curDataBufferLen);
							break;
						}

						fatalError = TRUE;
					}
					else if (ret == 0)
					{
						errorCode = 0;
						fatalError = TRUE;
					}

					if (fatalError)
					{
						//connection closed, or connection was aborted / socket closed / etc, that's a fatal error for us.
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Socket error, send() returned %d, GetLastError() %d", ret, errorCode);
						LeaveCriticalSection(&hDataBufferMutex);
						FatalSocketShutdown();
						return;
					}
				}

				//finish writing for now
				if (curDataBufferLen <= 1000)
					exitLoop = true;

				LeaveCriticalSection(&hDataBufferMutex);

				if (delayTime)
					Sleep(delayTime);
			} while (!exitLoop);
		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop:发送窗口已满!!curDataBufferLen %d", curDataBufferLen);
		}
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SocketLoop: Graceful loop exit");
}

void RTMPPublisher::SendLoop()
{
	bool bAudioPrintf = false;
	bool bVideoPrintf = false;
	QWORD StartTime = GetQPCMS();
	DWORD ThreadId = GetCurrentThreadId();
	bool  bFrist = true;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	while (WaitForSingleObject(hSendSempahore, INFINITE) == WAIT_OBJECT_0)
	{
		while (true)
		{
			EnterCriticalSection(&hDataMutex);

			// 停止很慢，强制不发送最后数据
			if (bStopping)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:强制清空网络模块%d个数据包", queuedPackets.Num());
				for (UINT i = 0; i < queuedPackets.Num(); i++)
					queuedPackets[i].data.Clear();
				queuedPackets.Clear();
				LeaveCriticalSection(&hDataMutex);
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SendLoop 退出,内部循环");
				return;
			}
			if (queuedPackets.Num() == 0)
			{
				LeaveCriticalSection(&hDataMutex);
				break;
			}

			List<BYTE> packetData;
			PacketType type = queuedPackets[0].type;
			DWORD      timestamp = queuedPackets[0].timestamp;
			packetData.TransferFrom(queuedPackets[0].data);

			currentBufferSize -= packetData.Num();

			queuedPackets.Remove(0);

			LeaveCriticalSection(&hDataMutex);

			//--------------------------------------------

			RTMPPacket packet;
			packet.m_nChannel = (type == PacketType_Audio) ? 0x5 : 0x4;
			// 			if (type != PacketType_Audio)
			// 			{
			// 				Log(TEXT("video  PTS %d type %d"), timestamp, type);
			// 			}
			packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
			packet.m_packetType = (type == PacketType_Audio) ? RTMP_PACKET_TYPE_AUDIO : RTMP_PACKET_TYPE_VIDEO;
			packet.m_nTimeStamp = timestamp;
			packet.m_nInfoField2 = rtmp->m_stream_id;
			packet.m_hasAbsTimestamp = TRUE;

			packet.m_nBodySize = packetData.Num() - RTMP_MAX_HEADER_SIZE;
			packet.m_body = (char*)packetData.Array() + RTMP_MAX_HEADER_SIZE;


			if (GetQPCMS() - StartTime > 1000 * 60 || bFrist)
			{
				if (packet.m_packetType == RTMP_PACKET_TYPE_AUDIO)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "-----Current Send AudioSendTimestamp %d,ThreadID = %d----", timestamp, ThreadId);
					bAudioPrintf = true;
				}
				else
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "-----Current Send VideoSendTimestamp %d,ThreadID = %d----", timestamp, ThreadId);
					bVideoPrintf = true;
				}

				if (bAudioPrintf && bVideoPrintf)
				{
					bAudioPrintf = false;
					bVideoPrintf = false;
					bFrist = false;
					StartTime = GetQPCMS(); //重新计时
				}
			}
			
			

			if (!RTMP_SendPacket(rtmp, &packet, FALSE))
			{
				//should never reach here with the new shutdown sequence.
				RUNONCE Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMP_SendPacket failure, should not happen!");
				if (!RTMP_IsConnected(rtmp))
				{
					//App->PostStopMessage();					
					if (bStopping && WaitForSingleObject(hSendLoopExit, 0) == WAIT_OBJECT_0)
					{
						Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SendLoop 退出TriggerError 后");
						return;
					}
					TriggerError();
					break;
				}
			}
		}

		if (bStopping && WaitForSingleObject(hSendLoopExit, 0) == WAIT_OBJECT_0)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:RTMPPublisher::SendLoop 退出");
			return;
		}

	}
}

DWORD RTMPPublisher::SendThread(RTMPPublisher *publisher)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:SendThread ID = %d Begin", GetCurrentThreadId());
	publisher->SendLoop();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:SendThread ID = %d Eegin", GetCurrentThreadId());
	return 0;
}

DWORD RTMPPublisher::SocketThread(RTMPPublisher *publisher)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:SocketThread ID = %d Begin", GetCurrentThreadId());
	publisher->SocketLoop();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:SocketThread ID = %d End", GetCurrentThreadId());
	return 0;
}

void RTMPPublisher::DropFrame(UINT id)
{
	NetworkPacket &dropPacket = queuedPackets[id];
	currentBufferSize -= dropPacket.data.Num();
	PacketType type = dropPacket.type;
	dropPacket.data.Clear();

	if (dropPacket.type < PacketType_VideoHigh)
		numBFramesDumped++;
	else
		numPFramesDumped++;

	for (UINT i = id + 1; i < queuedPackets.Num(); i++)
	{
		UINT distance = (i - id);
		if (queuedPackets[i].distanceFromDroppedFrame <= distance)
			break;

		queuedPackets[i].distanceFromDroppedFrame = distance;
	}

	for (int i = int(id) - 1; i >= 0; i--)
	{
		UINT distance = (id - UINT(i));
		if (queuedPackets[i].distanceFromDroppedFrame <= distance)
			break;

		queuedPackets[i].distanceFromDroppedFrame = distance;
	}

	bool bSetPriority = true;
	for (UINT i = id + 1; i < queuedPackets.Num(); i++)
	{
		NetworkPacket &packet = queuedPackets[i];
		if (packet.type < PacketType_Audio)
		{
			if (type >= PacketType_VideoHigh)
			{
				if (packet.type < PacketType_VideoHighest)
				{
					currentBufferSize -= packet.data.Num();
					packet.data.Clear();
					queuedPackets.Remove(i--);

					if (packet.type < PacketType_VideoHigh)
						numBFramesDumped++;
					else
						numPFramesDumped++;
				}
				else
				{
					bSetPriority = false;
					break;
				}
			}
			else
			{
				if (packet.type >= type)
				{
					bSetPriority = false;
					break;
				}
			}
		}
	}

	if (bSetPriority)
	{
		if (type >= PacketType_VideoHigh)
			packetWaitType = PacketType_VideoHighest;
		else
		{
			if (packetWaitType < type)
				packetWaitType = type;
		}
	}
}

//video packet count exceeding maximum.  find lowest priority frame to dump
bool RTMPPublisher::DoIFrameDelay(bool bBFramesOnly)
{
	int curWaitType = PacketType_VideoDisposable;

	while (!bBFramesOnly && curWaitType < PacketType_VideoHighest ||
		bBFramesOnly && curWaitType < PacketType_VideoHigh)
	{
		UINT bestPacket = INVALID;
		UINT bestPacketDistance = 0;

		if (curWaitType == PacketType_VideoHigh)
		{
			bool bFoundIFrame = false;

			for (int i = int(queuedPackets.Num()) - 1; i >= 0; i--)
			{
				NetworkPacket &packet = queuedPackets[i];
				if (packet.type == PacketType_Audio)
					continue;

				if (packet.type == curWaitType)
				{
					if (bFoundIFrame)
					{
						bestPacket = UINT(i);
						break;
					}
					else if (bestPacket == INVALID)
						bestPacket = UINT(i);
				}
				else if (packet.type == PacketType_VideoHighest)
					bFoundIFrame = true;
			}
		}
		else
		{
			for (UINT i = 0; i < queuedPackets.Num(); i++)
			{
				NetworkPacket &packet = queuedPackets[i];
				if (packet.type <= curWaitType)
				{
					if (packet.distanceFromDroppedFrame > bestPacketDistance)
					{
						bestPacket = i;
						bestPacketDistance = packet.distanceFromDroppedFrame;
					}
				}
			}
		}

		if (bestPacket != INVALID)
		{
			DropFrame(bestPacket);
			queuedPackets.Remove(bestPacket);
			return true;
		}

		curWaitType++;
	}

	return false;
}

void RTMPPublisher::RequestKeyframe(int waitTime)
{
	__Instances->RequestKeyframe(waitTime);
}

int RTMPPublisher::BufferedSend(RTMPSockBuf *sb, const char *buf, int len, RTMPPublisher *network)
{
	//NOTE: This function is called from the SendLoop thread, be careful of race conditions.

retrySend:

	//We may have been disconnected mid-shutdown or something, just pretend we wrote the data
	//to avoid blocking if the socket loop exited.
	if (!RTMP_IsConnected(network->rtmp))
		return len;

	EnterCriticalSection(&network->hDataBufferMutex);

	if (network->curDataBufferLen + len >= network->dataBufferSize)
	{
		//Log(TEXT("RTMPPublisher::BufferedSend: Socket buffer is full (%d / %d bytes), waiting to send %d bytes"), network->curDataBufferLen, network->dataBufferSize, len);
		++network->totalTimesWaited;
		network->totalBytesWaited += len;

		LeaveCriticalSection(&network->hDataBufferMutex);

		HANDLE hWaitEvent[2];
		hWaitEvent[0] = network->hSendLoopExit;
		hWaitEvent[1] = network->hBufferSpaceAvailableEvent;
		int status = WaitForMultipleObjects(2, hWaitEvent, FALSE, INFINITE);
		if (WSA_WAIT_EVENT_0 == status)
		{
			//OSLeaveMutex(network->hDataBufferMutex);
			Log::writeMessage(LOG_RTSPSERV,1,"LiveSDK_Log:发送回调过程中，发现退出");
			return 0;
		}
		if (status == WAIT_ABANDONED + 1 || status == WAIT_FAILED)
			return 0;
		goto retrySend;
	}

	mcpy(network->dataBuffer + network->curDataBufferLen, buf, len);
	network->curDataBufferLen += len;

	LeaveCriticalSection(&network->hDataBufferMutex);

	SetEvent(network->hBufferEvent);

	return len;
}

void RTMPPublisher::SetStop(bool bStop)
{
	bStopping = bStop;
}

void RTMPPublisher::ClearRRHash()
{
	OSEnterMutex(m_handleLock);
	m_pMap_addr.clear();
	OSLeaveMutex(m_handleLock);
}

String RTMPPublisher::GetPrefix() const
{
	return Prefix;
}

RTMPPublisherVectorBase* CreateRTMPPublisher(CInstanceProcess *Instance, bool bBackUp)
{
	RTMPPublisher::ClearRRHash();
	int PublishCount = 2;

	int Rendering = 0;

	RTMPPublisherVector* VRTMPPublisher = new RTMPPublisherVector;
	String prefix;
	for (int i = 0; i < PublishCount; ++i)
	{
		prefix = String(TEXT("Publish")) + IntString(i);

		if (!bBackUp)
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
			VRTMPPublisher->AddRTMPPublisher(new RTMPPublisher(prefix, Instance, bBackUp));
		}
	}

	return VRTMPPublisher;
}