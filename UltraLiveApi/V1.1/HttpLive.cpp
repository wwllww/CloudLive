#include "SLiveManager.h"
#undef  new
#include "HttpLive.h"
#include "MetaData.h"
#include "LogDeliver.h"
#include "Encoder.h"


#define STREAM_NAME "1"

#define MAX_BUFLEN      2 * 1024 * 1024
#define PRECREATECOUNT  30

bool HttpLive::bHasInit = false;
SOCKET HttpLive::ListenSocket = INVALID_SOCKET;
HANDLE HttpLive::HAccept = NULL;
bool HttpLive::bRuning = false;
char HttpLive::FLVHeader[14] = {0};
CTCPSocketControl HttpLive::TcpSocketControlCallBack;
TcpControl2 *HttpLive::TcpControl = NULL;

typedef tbb::concurrent_hash_map<AIOID, __TimeInfo,hash_compare_socket> SocketMap;

HttpLive::HttpLive(CInstanceProcess *Process)
{
	this->Process = Process;
	bFirstSend = true;
	memset(MetaData,0,sizeof MetaData);
	VideoEncoder *Video = Process->GetVideoEncoder();
	if (Video)
	{
		Video->GetWH(Width, Height);
		VideoBitRate = Video->GetBitRate();
	}
	else
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s 还未创建视频编码器",__FUNCTION__);
	}

	AudioEncoder *Audio = Process->GetAudioEncoder();
	if (Audio)
	{
		AudioSamplerate = Audio->GetsampleRate();
		NumChannels = Audio->GetnumChannels();
		AudioRate = Audio->GetBitRate();
	}
	else
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s 还未创建音频编码器", __FUNCTION__);
	}

	int nPreCreate = PRECREATECOUNT;
	while( nPreCreate-- )
	{
		__PDATAINFO Data = new __DATAINFO;
		Data->buf = new BYTE[MAX_BUFLEN];
		Data->len = MAX_BUFLEN;
		Data->CanWrite = true;

		m_DataQueue.push(Data);
	}
}

HttpLive::~HttpLive()
{
	if(m_ListSocket.size())
	{
		SocketMap::iterator MapBegin = m_ListSocket.begin();
		SocketMap::iterator MapEnd = m_ListSocket.end();
		
		for(SocketMap::iterator it = MapBegin;it != MapEnd;++it)
		{
			TcpControl->close_connect(it->first);
		}

		m_ListSocket.clear();
	}


	while( !m_DataQueue.empty() )
	{
// 		__PDATAINFO Data = m_DataQueue.front();
// 		m_DataQueue.pop();
		
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

bool HttpLive::InitLive( UINT uListenPort )
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin! Port = %d", __FUNCTION__, uListenPort);
	if(bHasInit)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Has already Init", __FUNCTION__);
		return false;
	}

	FLVHeader[0] = 'F';
	FLVHeader[1] = 'L';
	FLVHeader[2] = 'V';
	FLVHeader[3] = 0x1;
	FLVHeader[4] = 0x5;
	DWORD Size = DWORD_BE(9);
	memcpy(&FLVHeader[5],&Size,4);
	Size = 0;
	memcpy(&FLVHeader[9],&Size,4);


	WORD wVersionRequested;
	WSADATA wsaData;
	int err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) 
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s WSAStartup failed with error: %d", __FUNCTION__, err);
		return false;
	}

	TcpControl = create_tcp_control2(&TcpSocketControlCallBack);

	if(!TcpControl)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s create_tcp_control failed!", __FUNCTION__);
		return false;
	}

	AioAddr Addr;
	memset(&Addr,0,sizeof AioAddr);

	Addr.m_port = uListenPort;
	Addr.m_ip = inet_addr("0.0.0.0");
	Addr.m_v4 = true;

	if(!TcpControl->listen_tcp(Addr,5))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s listen_tcp failed!", __FUNCTION__);
		return false;
	}


// 	ListenSocket = socket(AF_INET, SOCK_STREAM, 0);
// 
// 	if(ListenSocket == INVALID_SOCKET)
// 	{
// 		Log::writeMessage(562949953421312,1,"%s socket failed LastError: %d", __FUNCTION__,WSAGetLastError()); 
// 		return false;
// 	}
// 
// 	sockaddr_in serverAddr;
// 
// 	memset(&serverAddr,0,sizeof serverAddr);
// 	serverAddr.sin_family = AF_INET;  
// 	serverAddr.sin_port = htons(uListenPort);  
// 	serverAddr.sin_addr.s_addr = inet_addr("0.0.0.0");    
// 
// 	int iRes = ::bind(ListenSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));  
// 
// 	if(iRes == SOCKET_ERROR)
// 	{
// 		Log::writeMessage(562949953421312,1,"%s bind failed LastError: %d", __FUNCTION__,WSAGetLastError()); 
// 		return false;
// 	}
// 
//  	iRes = listen(ListenSocket,5);
// 
// 	if(iRes == SOCKET_ERROR)
// 	{
// 		Log::writeMessage(562949953421312,1,"%s listen failed LastError: %d", __FUNCTION__,WSAGetLastError()); 
// 		return false;
// 	}
//	bRuning = true;
//	HAccept = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)AcceptThread,NULL,0,NULL);

	bHasInit = true;
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__, uListenPort);
	return true;
}

void HttpLive::UnInitLive()
{
	if(bHasInit)
	{
		bRuning = false;


		if(ListenSocket != INVALID_SOCKET)
		{
			closesocket(ListenSocket);
		}

		if(HAccept)
		{
			if(WAIT_TIMEOUT == WaitForSingleObject(HAccept, 1000))
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "HAccept 线程 WAIT_TIMEOUT 强杀");
				TerminateThread(HAccept,0);
			}
		}

		if(TcpControl)
		{
			destroy_tcp_control2(TcpControl);
		}

		TcpControl = NULL;

		WSACleanup ();

		bHasInit = false;
	}
}

// DWORD HttpLive::AcceptThread( LPVOID Param )
// {
// 
// 	while( bRuning )
// 	{
// 		sockaddr_in serverAddr;
// 		int len = sizeof sockaddr_in;
// 		SOCKET AcceptSocket = accept(ListenSocket,(sockaddr*)&serverAddr,&len);
// 
// 		if(AcceptSocket == INVALID_SOCKET)
// 		{
// 			Log::writeMessage(562949953421312,1," %s accept Failed! LastError: %d",__FUNCTION__,WSAGetLastError());
// 		}
// 		else
// 		{
// 			char recbuf[4096] = {0};
// 			int iRecLen = recv(AcceptSocket,recbuf,sizeof recbuf,0);
// 			/*
// 			GET /1.flv HTTP/1.1
// 			Host: 127.0.0.1:1935
// 			User-Agent: VLC/2.2.4 LibVLC/2.2.4
// 			Range: bytes=0-
// 			Connection: close
// 			Icy-MetaData: 1
// 			*/
// 			
// 			if(iRecLen > 3)
// 			{
// 				if(recbuf[0] == 'G' && recbuf[1] == 'E' && recbuf[2] == 'T')
// 				{
// 					std::string StreamName;
// 					ParseResquest(recbuf,iRecLen,StreamName);
// 
// 					if(!StreamName.empty())
// 					{
// 						Log::writeMessage(562949953421312,1," %s ParseResquest流 Name %s recbuf %s",__FUNCTION__,StreamName.c_str(),recbuf);
// 						concurrent_hash_map<std::string, ConsumerTask,hash_compare>::accessor ConAccessor;
// 						if(CInteractionInstance::GetInstance()->m_ConsumerMap.find(ConAccessor,StreamName.c_str()))
// 						{
// 							//找到流名
// 							Log::writeMessage(562949953421312,1," %s 找到流 Name",__FUNCTION__,StreamName.c_str());
// 							if(!ConAccessor->second.httpLive)
// 							{
// 								ConAccessor->second.httpLive = new HttpLive;
// 								ConAccessor->second.httpLive->NubeName = StreamName;
// 							}
// 
// 							const char *Respond = "HTTP/1.1 200 OK\r\n"
// 												 "Access-Control-Allow-Origin: *\r\n"
// 												 "Cache-Control: no-cache\r\n"
// 												 "Content-Type: video/x-flv\r\n"
// 												 "Expires: -1\r\n"
// 												 "Pragma: no-cache\r\n\r\n";
// 
// 							int iSendlen = send(AcceptSocket,Respond,strlen(Respond),0);
// 
// 							if(iSendlen > 0)
// 							{
// 								SocketMap::accessor SocketAccessor;
// 								if(ConAccessor->second.httpLive->m_ListSocket.insert(SocketAccessor,AcceptSocket))
// 								{
// 									SocketAccessor->second.bFirstSend = true;
// 									SocketAccessor->second.bSendHeader = false;
// 									SocketAccessor->second.FirstTimeStamp = 0;
// 
// 									UINT tcpBufferSize = 64 * 1024;
// 									int curTCPBufSize, curTCPBufSizeSize = sizeof(curTCPBufSize);
// 
// 									if (!getsockopt(AcceptSocket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
// 									{
// 										Log::writeMessage(562949953421312,1,"LiveSDK_Log:SO_SNDBUF was at %u", curTCPBufSize);
// 
// 										if (curTCPBufSize < int(tcpBufferSize))
// 										{
// 											if (!setsockopt(AcceptSocket, SOL_SOCKET, SO_SNDBUF, (const char *)&tcpBufferSize, sizeof(tcpBufferSize)))
// 											{
// 												if (!getsockopt(AcceptSocket, SOL_SOCKET, SO_SNDBUF, (char *)&curTCPBufSize, &curTCPBufSizeSize))
// 												{
// 													if (curTCPBufSize != tcpBufferSize)
// 														Log::writeMessage(562949953421312, 1, "LiveSDK_Log:Could not raise SO_SNDBUF to %u, value is now %d", tcpBufferSize, curTCPBufSize);
// 
// 													Log::writeMessage(562949953421312, 1, "LiveSDK_Log:SO_SNDBUF is now %d", curTCPBufSize);
// 												}
// 												else
// 												{
// 													Log::writeMessage(562949953421312, 1, "LiveSDK_Log:getsockopt: Failed to query SO_SNDBUF, error %d", WSAGetLastError());
// 												}
// 											}
// 											else
// 											{
// 												Log::writeMessage(562949953421312, 1, "LiveSDK_Log:setsockopt: Failed to raise SO_SNDBUF to %u, error %d", tcpBufferSize, WSAGetLastError());
// 											}
// 										}
// 									}
// 								}
// 							}
// 							else
// 							{
// 								Log::writeMessage(562949953421312,1," %s send失败 LastError: %d iSendlen %d",__FUNCTION__,WSAGetLastError(),iSendlen);
// 
// 								WaitAndCloseSocket(AcceptSocket);
// 							}
// 
// 						}
// 						else
// 						{
// 							Log::writeMessage(562949953421312,1," %s m_ConsumerMap没有找流",__FUNCTION__);
// 							WaitAndCloseSocket(AcceptSocket);
// 						}
// 
// 					}
// 					else
// 					{
// 						Log::writeMessage(562949953421312,1," %s 没有找到流名recbuf %s",__FUNCTION__,recbuf);
// 						WaitAndCloseSocket(AcceptSocket);
// 					}
// 
// 				}
// 				else
// 				{
// 					Log::writeMessage(562949953421312,1," %s 不是GET 请求 recbuf %s",__FUNCTION__,recbuf);
// 					WaitAndCloseSocket(AcceptSocket);
// 				}
// 
// 			}
// 			else
// 			{
// 				if(iRecLen > 0)
// 				{
// 					WaitAndCloseSocket(AcceptSocket);
// 				}
// 				else
// 					Log::writeMessage(562949953421312,1," %s recv Failed! LastError: %d,iRecLen %d",__FUNCTION__,WSAGetLastError(),iRecLen);
// 			}
// 		}
// 
// 
// 	}
// 
// 	return 0;
// }

unsigned char* HttpLive::GetNextStartCode(unsigned char* buf,int len, int& nStartCodeLen)
{
	if (!buf || len < 4)
		return NULL;

	unsigned char* pStart = buf;
	unsigned char* pEnd = pStart + len - 3;
	//nStartCodeLen = 0;
	while (pStart != pEnd)
	{
		if (pStart[0] == 0x00 && pStart[1] == 0x00 && pStart[2] == 0x01)
		{
			if (pStart != buf)
			{
				if (*(pStart - 1) == 0x00)
				{
					nStartCodeLen = 4;
					return pStart -1;
				}
				else
				{
					nStartCodeLen = 3;
					return pStart;
				}
			}
			else
			{
				nStartCodeLen = 3;
				return pStart;
			}
		}
		pStart++;
	}

	return NULL;
}

bool HttpLive::ProcessVideoHeader()
{
	DataPacket Packet;
	Process->GetVideoHeaders(Packet);
	if (Packet.lpPacket)
	{
		VideoHeader.resize(Packet.size);
		memcpy(&VideoHeader[0], Packet.lpPacket, Packet.size);
		return true;
	}
	return false;
}

bool HttpLive::ProcessAudioHeader()
{
	DataPacket Packet;
	Process->GetAudioHeaders(Packet);

	if (Packet.lpPacket)
	{
		AudioHeader.resize(Packet.size);
		memcpy(&AudioHeader[0], Packet.lpPacket, Packet.size);
		return true;
	}

	return false;
}


bool HttpLive::SendPacket(LPBYTE data, UINT size, UINT Timestamp, bool bVideo)
{
	SocketMap::iterator begin = m_ListSocket.begin();
	SocketMap::iterator end = m_ListSocket.end();
	
	for(begin;begin != end;++begin)
	{
		if(begin->second.bFirstSend)
		{
			//第一次等关键帧发送
			Log::writeMessage(LOG_RTSPSERV, 1, "%s bFirstSend 等待关键帧发送", __FUNCTION__);
			if (data[0] == 0x17)
			{
				if (!begin->second.bSendHeader)
				{
					if (bVideo && ProcessVideoHeader())
					{
						ProcessAudioHeader();
						if (SendFLVHeader(begin->first))
						{
							begin->second.bSendHeader = true;
							Log::writeMessage(LOG_RTSPSERV, 1, "%s SendFLVHeader Sucess 等待关键帧发送", __FUNCTION__);
						}


						if (ProcessData(begin->first, data, size, 0, bVideo))
						{
							begin->second.bFirstSend = false;
							begin->second.FirstTimeStamp = Timestamp;

							Log::writeMessage(LOG_RTSPSERV, 1, "%s 关键帧已经发送！", __FUNCTION__);
						}
						

					}
				}
				else if (bVideo)
				{

					if (ProcessData(begin->first, data, size, 0, bVideo))
					{
						begin->second.bFirstSend = false;
						begin->second.FirstTimeStamp = Timestamp;

						Log::writeMessage(LOG_RTSPSERV, 1, "%s 关键帧已经发送！", __FUNCTION__);
					}


				}
			}

		}
		else
		{
			if (Timestamp > begin->second.FirstTimeStamp)
			{
				ProcessData(begin->first, data, size, Timestamp - begin->second.FirstTimeStamp, bVideo);
			}

		}

	}

	return true;
}

DWORD HttpLive::CloseSocketThread( LPVOID Param )
{
	AIOID Id = *(AIOID*)Param;
	const char *Error = "HTTP/1.1 404 Not Found\r\n"
		"Access-Control-Allow-Origin: *\r\n\r\n";

	Sleep(3000);

	int len = strlen(Error);
	__DATAINFO *Data = new __DATAINFO;
	Data->buf = new BYTE[len];
	Data->SendStatus = SEND_NOFOUND;
	memcpy(Data->buf ,Error,len);

	HttpLive::TcpControl->asyn_write(Id,(char*)Data->buf,len,0,(ULL64)Data);
	return 0;
}

void HttpLive::ParseResquest( const char *recvbuf,int buflen,std::string &StreamName )
{
	char Tem[4096] = {0};
	memcpy(Tem,recvbuf,buflen);
	for(int i = 5;i < buflen;++i)
	{
		if(Tem[i] == '.' || Tem[i] == ' ')
		{
			Tem[i] = '\0';
			break;
		}
	}

	StreamName = Tem + 5;
}

void HttpLive::WaitAndCloseSocket( SOCKET sock )
{
	HANDLE HT = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)CloseSocketThread,&sock,0,NULL);

	if(HT)
		CloseHandle(HT);
}

char* HttpLive::EncMetaData(char *enc, char *pend,bool bFLVFile)
{
	const AVal *av_codecFourCC;


	av_codecFourCC = &av_mp4a;
	//audioCodecID = 10.0;

	if(bFLVFile)
	{
		*enc++ = AMF_ECMA_ARRAY;
		enc = AMF_EncodeInt32(enc, pend, 14);
	}
	else
		*enc++ = AMF_OBJECT;


	enc = AMF_EncodeNamedNumber(enc, pend, &av_duration, 0.0);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_fileSize, 0.0);

	enc = AMF_EncodeNamedNumber(enc, pend, &av_width, Width);
	enc = AMF_EncodeNamedNumber(enc, pend, &av_height, Height);


	enc = AMF_EncodeNamedString(enc, pend, &av_videocodecid, &av_avc1);//7.0);//

	enc = AMF_EncodeNamedNumber(enc, pend, &av_videodatarate, double(VideoBitRate));
	enc = AMF_EncodeNamedNumber(enc, pend, &av_framerate, double(25));

	bool bNeedAudio = true;
	if(bNeedAudio)
	{
		enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, av_codecFourCC);//audioCodecID);//
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, double(AudioRate)); //ex. 128kb\s
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, double(AudioSamplerate));
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, double(NumChannels));

		enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, NumChannels == 2);
	}
	else
	{
		enc = AMF_EncodeNamedString(enc, pend, &av_audiocodecid, &av_mp4a);//audioCodecID);//
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiodatarate, double(0)); //ex. 128kb\s
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplerate, double(0));
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiosamplesize, 16.0);
		enc = AMF_EncodeNamedNumber(enc, pend, &av_audiochannels, double(0));
		//enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo,         true);

		enc = AMF_EncodeNamedBoolean(enc, pend, &av_stereo, false);
	}

	const AVal Vision = AVC("redcdn");
	enc = AMF_EncodeNamedString(enc, pend, &av_encoder, &Vision);
	*enc++ = 0;
	*enc++ = 0;
	*enc++ = AMF_OBJECT_END;

	return enc;
}

UINT HttpLive::InitMetaData()
{
	char *enc = MetaData;
	char *pend = MetaData + sizeof(MetaData);
	enc = AMF_EncodeString(enc, pend, &av_onMetaData);
	char *endMetaData = EncMetaData(enc, pend,true);
	return endMetaData - MetaData;
}

bool HttpLive::SendFLVHeader( AIOID Id )
{
	__DATAINFO *Data = NULL;
	m_DataQueue.try_pop(Data);
	if(Data && Data->CanWrite)
	{
		Data->CanWrite = false;
		Data->SendStatus = SEND_DATA;
		//m_DataQueue.pop();
		m_DataQueue.push(Data);

		memcpy(Data->buf,FLVHeader,sizeof(FLVHeader) - 1);

		TcpControl->asyn_write(Id,(char*)Data->buf,sizeof(FLVHeader) - 1,(ULL64)this,(ULL64)Data);
	}
	else
	{
		if (Data)
		{
			m_DataQueue.push(Data);
		}
		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
	}

	UINT MetaSize = InitMetaData();

	Data = NULL;
	m_DataQueue.try_pop(Data);
	if(Data && Data->CanWrite)
	{
		Data->CanWrite = false;
		Data->SendStatus = SEND_DATA;
		//m_DataQueue.pop();
		m_DataQueue.push(Data);

		int size = PacketFLVData((const BYTE*)MetaData,MetaSize,18,0,Data->buf);

		TcpControl->asyn_write(Id,(char*)Data->buf,size,(ULL64)this,(ULL64)Data);
	}
	else
	{
		if (Data)
		{
			m_DataQueue.push(Data);
		}
		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
	}


	Data = NULL;//m_DataQueue.front();
	m_DataQueue.try_pop(Data);

	if(Data && Data->CanWrite)
	{
		Data->CanWrite = false;
		Data->SendStatus = SEND_DATA;
		//m_DataQueue.pop();
		m_DataQueue.push(Data);

		int size = PacketFLVData((const BYTE*)&AudioHeader[0],AudioHeader.size(),8,0,Data->buf);

		TcpControl->asyn_write(Id,(char*)Data->buf,size,(ULL64)this,(ULL64)Data);
	}
	else
	{
		if (Data)
		{
			m_DataQueue.push(Data);
		}
		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
	}

	Data = NULL;//m_DataQueue.front();

	m_DataQueue.try_pop(Data);

	if(Data && Data->CanWrite)
	{
		Data->CanWrite = false;
		Data->SendStatus = SEND_DATA;
		//m_DataQueue.pop();
		m_DataQueue.push(Data);

		int size = PacketFLVData((const BYTE*)&VideoHeader[0],VideoHeader.size(),9,0,Data->buf);
		TcpControl->asyn_write(Id,(char*)Data->buf,size,(ULL64)this,(ULL64)Data);
	}
	else
	{
		if (Data)
		{
			m_DataQueue.push(Data);
		}
		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
	}

	return true;
}

int HttpLive::PacketFLVData( const BYTE *lpData, UINT size, BYTE type, DWORD timestamp,BYTE *DataBuf)
{
	UINT networkDataSize  = DWORD_BE(size);

//	int NewSize = size;
// 	if(bAddTagHeader && (type == 0x9))
// 	{
// 		networkDataSize  = DWORD_BE((NewSize + 9));
// 	}
// 	else if(bAddTagHeader && (type == 0x8))
// 	{
// 		networkDataSize  = DWORD_BE((size + 2));
// 	}

	UINT networkTimestamp = DWORD_BE(timestamp);
	UINT streamID = 0;
	UINT offset = 0;
	DataBuf[offset ++] = type;
	memcpy(&DataBuf[offset],((LPBYTE)(&networkDataSize))+1,  3);
	offset += 3;
	memcpy(&DataBuf[offset],((LPBYTE)(&networkTimestamp))+1, 3);
	offset += 3;
	memcpy(&DataBuf[offset ++],&networkTimestamp, 1);
	memcpy(&DataBuf[offset],&streamID, 3);
	offset += 3;
	DWORD DSize = DWORD_BE((size + 11));

// 	if(bAddTagHeader && (type == 0x9))
// 	{
// 		//视频
// 		if(int(lpData[0] & 0x1f) == 5)
// 		{
// 			//关键帧
// 			DataBuf[offset ++] = 0x17;
// 			
// 		}
// 		else
// 		{
// 			DataBuf[offset ++] = 0x27;
// 		}
// 		DataBuf[offset ++] = 0x1;
// 
// 		//只适用于没有B帧的情况
// 		DataBuf[offset ++] = 0x0;
// 		DataBuf[offset ++] = 0x0;
// 		DataBuf[offset ++] = 0x0;
// 
// 		DWORD BESize = htonl(NewSize);
// 
// 		memcpy(&DataBuf[offset],&BESize,sizeof DWORD);
// 		offset += 4;
// 
// 		DSize = DWORD_BE((NewSize + 11 + 9));
// 	}
// 	else if(bAddTagHeader && (type == 0x8))
// 	{
// 		//音频
// 		DataBuf[offset ++] = 0xAF;
// 		DataBuf[offset ++] = 0x01;
// 
// 		DSize = DWORD_BE((size + 11 + 2));
// 	}

	memcpy(&DataBuf[offset],lpData, size);
	offset += size;
	memcpy(&DataBuf[offset],&DSize, sizeof DWORD);
	return offset + 4;
}

bool HttpLive::ProcessData( AIOID Id,unsigned char* buf,int len,uint64_t timestamp,bool bVideo)
{
	__DATAINFO *Data = NULL;// m_DataQueue.front();
	m_DataQueue.try_pop(Data);

	if (Data && Data->CanWrite)
	{
		Data->CanWrite = false;
		Data->SendStatus = SEND_DATA;
		//m_DataQueue.pop();
		m_DataQueue.push(Data);
		
		int size = PacketFLVData(buf, len, bVideo ? 9 : 8, timestamp, Data->buf);
		TcpControl->asyn_write(Id, (char*)Data->buf, size, (ULL64)this, (ULL64)Data);
	}
	else
	{
		if (Data)
		{
			m_DataQueue.push(Data);
		}

		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
	}
	return true;
}

bool HttpLive::ProcessVideo( unsigned char* buf,int len,int& StartFrame )
{
	int nStartCodecLen = 0;
	unsigned char* pNalu = GetNextStartCode(buf,len, nStartCodecLen);
	//处理PPS/SPS
	if (!pNalu)
	{
		Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::找不到startcode", __FUNCTION__, __LINE__);
		return false;
	}

	int offset = nStartCodecLen;

	if(int(pNalu[nStartCodecLen] & 0x1f) == 0x7 || int(pNalu[nStartCodecLen] & 0x1f) == 0x8)
	{
		return false;
	}

	StartFrame = pNalu - buf + nStartCodecLen;
	
	return true;
}

bool HttpLive::DoAccept( AIOID Id )
{
	if (0 == strcmp(STREAM_NAME, BindChannel.c_str()))
	{
		//找到流名
		Log::writeMessage(LOG_RTSPSERV, 1, " %s 找到流 Name %s", __FUNCTION__, BindChannel.c_str());

		SocketMap::accessor SocketAccessor;
		if(m_ListSocket.insert(SocketAccessor,Id))
		{
			SocketAccessor->second.bFirstSend = true;
			SocketAccessor->second.bSendHeader = false;
			SocketAccessor->second.FirstTimeStamp = 0;
		}

		return true;
	}
	
	
	Log::writeMessage(LOG_RTSPSERV, 1, " %s 没有找到流 Name %s", __FUNCTION__, BindChannel.c_str());
	return false;
}

void HttpLive::DoAcceptProcess( AIOID Id,char *recbuf,int iRecLen )
{
	recbuf[iRecLen] = 0;
	if(iRecLen > 3)
	{
		if(recbuf[0] == 'G' && recbuf[1] == 'E' && recbuf[2] == 'T')
		{
			std::string StreamName;
			ParseResquest(recbuf,iRecLen,StreamName);

			if(!StreamName.empty())
			{
				Log::writeMessage(LOG_RTSPSERV, 1, " %s ParseResquest流 Name %s recbuf %s", __FUNCTION__, StreamName.c_str(), recbuf);

	
				bool bFind = false;
				if (0 == strcmp(STREAM_NAME, StreamName.c_str()))
				{
					//找到流名
					Log::writeMessage(LOG_RTSPSERV, 1, " %s 找到流 Name %s", __FUNCTION__, StreamName.c_str());
					
					CInstanceProcess *Instance = CSLiveManager::GetInstance()->PreviewInstance;

					if (Instance && Instance->GetVideoEncoder() && Instance->GetAudioEncoder())
					{
						if (!Instance->__HttpLive)
						{
							Instance->__HttpLive = new HttpLive(Instance);
							Instance->__HttpLive->BindChannel = StreamName;
						}

						const char *Respond = "HTTP/1.1 200 OK\r\n"
							"Access-Control-Allow-Origin: *\r\n"
							"Cache-Control: no-cache\r\n"
							"Content-Type: video/x-flv\r\n"
							"Expires: -1\r\n"
							"Pragma: no-cache\r\n\r\n";


						__DATAINFO *Data = NULL;
						
						if (Instance->__HttpLive->m_DataQueue.try_pop(Data) && Data->CanWrite)
						{
							memcpy(Data->buf, Respond, strlen(Respond));
							Data->CanWrite = false;
							Data->SendStatus = SEND_OK;
							//Instance->__HttpLive->m_DataQueue.pop();
							Instance->__HttpLive->m_DataQueue.push(Data);
							TcpControl->asyn_write(Id, (char*)Data->buf, strlen(Respond), (ULL64)Instance->__HttpLive, (ULL64)Data);
						}
						else
						{
							if (Data)
							{
								Instance->__HttpLive->m_DataQueue.push(Data);
							}

							Log::writeError(LOG_RTSPSERV, 1, "%s[%d]::当前队列不可写!", __FUNCTION__, __LINE__);
						}

						bFind = true;
					}
					else
					{
						Log::writeError(LOG_RTSPSERV, 1, " %s 视频编码或音频编码器还未创建", __FUNCTION__);
					}
				}
				

				if(!bFind)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, " %s m_ConsumerMap没有找流", __FUNCTION__);
					WaitAndCloseSocket(Id);
				}

			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, " %s 没有找到流名recbuf %s", __FUNCTION__, recbuf);
				WaitAndCloseSocket(Id);
			}

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

