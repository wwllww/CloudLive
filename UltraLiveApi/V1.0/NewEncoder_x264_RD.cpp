#include "BaseAfx.h"
#include <inttypes.h>
#include <ws2tcpip.h>
#include "OperatNew.h"
#include <ppltasks.h>

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

extern "C"
{
#include "CodecDef.h"                   // 硬件编解码库
#include "RDVideoCodec.h"
}
extern "C"
{
#include "../x264/x264.h"
}

#include "Encoder.h"


void LogMessage(const char* msg, ...)
{
	va_list var;
	char Msg[4096] = { 0 };
	va_start(var, msg);
	vsprintf(Msg, msg, var);
	va_end(var);

	string MsgStr = "RDCodec_Log:";
	MsgStr += Msg;
	Log::writeMessage(LOG_RTSPSERV, 1, MsgStr.c_str());
}

void LogError(const char* msg, ...)
{
	va_list var;
	char Msg[4096] = { 0 };
	va_start(var, msg);
	vsprintf(Msg, msg, var);
	va_end(var);

	string MsgStr = "RDCodec_Log:";
	MsgStr += Msg;
	Log::writeError(LOG_RTSPSERV, 1, MsgStr.c_str());
}


volatile long g_RD264Init = 0;

	//static FILE *fp = NULL;
	struct VideoPacketRD
	{
		List<BYTE> Packet;
		uint64_t   pts;
		PacketType type;
		inline void FreeData() { Packet.Clear(); pts = 0; type = PacketType_VideoDisposable; }
	};
    
	class CRDX264EncoderNew : public VideoEncoder
	{
	protected:
		void                *m_pEncoder = NULL;
		RDCodec::VCodecParam vparam;

		int fps_ms;

		bool m_bRequestKeyframe;       // 第一次出视频
		UINT width, height;

		// 第一次出视频
		bool bFirstFrameProcessed;

		List<BYTE> m_PacketPPS;
		List<BYTE> m_PacketSPS;

		// 缓冲区数据
		HANDLE  m_hLock;
		List<VideoPacketRD> m_CurrentPackets;
		List<BYTE> HeaderPacket, SEIData;		

		inline void ClearPackets()
		{
			OSEnterMutex(m_hLock);
			for (UINT i = 0; i < m_CurrentPackets.Num(); i++)
				m_CurrentPackets[i].FreeData();
			m_CurrentPackets.Clear();
			OSLeaveMutex(m_hLock);
		}

		inline void SetBitRateParams(DWORD maxBitrate, DWORD bufferSize)
		{
			vparam.bitrate = maxBitrate * 1000;
			return;
		}

	public:
		CRDX264EncoderNew(int fps, int width, int height, int quality, CTSTR preset, bool bUse444, ColorDescription &colorDesc, int maxBitrate, int bufferSize, bool bUseCFR, int ColorT)
		{
			m_hLock = OSCreateMutex();
			bFirstFrameProcessed = false;
			if (0 == InterlockedIncrement(&g_RD264Init) - 1)
			{
				EnterCodec();
				SetLogFun(LogMessage, LogError, LogMessage, LogMessage);
			}
			fps_ms = 1000 / fps;

			this->width = width;
			this->height = height;

			memset(&vparam, 0, sizeof RDCodec::VCodecParam);

			vparam.isDesktop = false;
			vparam.bitrate = maxBitrate * 1000;
			vparam.width = width;
			vparam.height = height;
			vparam.fps = fps;
			if (ColorT == 0)
			{
				vparam.pix_format = RDCodec::PIXEL_FORMAT_YV12;
			}
			else if (ColorT == 1)
			{
				vparam.pix_format = RDCodec::PIXEL_FORMAT_NV12;
			}
			else if (ColorT == 2)
			{
				vparam.pix_format = RDCodec::PIXEL_FORMAT_I420;
			}

			vparam.isSkip = false;
			vparam.type = RDCodec::VIDEO_CODEC_TYPE_H264_HARD;
			vparam.rcmode = RDCodec::RC_MODE_CBR;

			//encode
// 			auto TaskEnCoder = concurrency::create_task([this](){m_pEncoder = RDVideoCodecCreate(&vparam, true, this); return m_pEncoder; });
// 			TaskEnCoder.then([this](void *Param){
// 				int ret = RDVideoCodecInit(Param, &vparam);
// 				if (ret != 0)
// 				{
// 					Log::writeError(LOG_RTSPSERV, 1, "encode init error,ret: %d\n", ret);
// 					return;
// 				}
// 			}).then([this]{
// 				int ret = RDVideoCodecSetCallBack(m_pEncoder, VideoEncodeCallback, errorCallback);
// 				if (ret != 0)
// 				{
// 					Log::writeError(LOG_RTSPSERV, 1, "encode set call back error\n");
// 					return;
// 				}
// 			});

			m_pEncoder = RDVideoCodecCreate(&vparam, true, this);
			int ret = RDVideoCodecInit(m_pEncoder, &vparam);
			if (ret != 0)
			{
				Log::writeError(LOG_RTSPSERV,1,"encode init error,ret: %d\n", ret);
				return;
			}
			ret = RDVideoCodecSetCallBack(m_pEncoder, VideoEncodeCallback, errorCallback);
			if (ret != 0)
			{
				Log::writeError(LOG_RTSPSERV, 1, "encode set call back error\n");
				return;
			}
			m_CurrentPackets.CreateNew();
			//GetHeaders(packet);

		}
		enum nal_unit_type_e
		{
			NAL_UNKNOWN = 0,
			NAL_SLICE = 1,
			NAL_SLICE_DPA = 2,
			NAL_SLICE_DPB = 3,
			NAL_SLICE_DPC = 4,
			NAL_SLICE_IDR = 5,    /* ref_idc != 0 */
			NAL_SEI = 6,    /* ref_idc == 0 */
			NAL_SPS = 7,
			NAL_PPS = 8,
			NAL_AUD = 9,
			NAL_FILLER = 12,
			/* ref_idc == 0 for 6,9,10,11,12 */
		};
		bool isRDX264() { return true; }
		
		void HandleVideo(RDCodec::VCodecBuffer *buffer, bool index)
		{
			// 封装rtmp数据包
			int timeOffset = 0;
			timeOffset = htonl(timeOffset);
			BYTE *timeOffsetAddr = ((BYTE*)&timeOffset) + 1;
			VideoPacketRD *newPacket = NULL;
			PacketType bestType = PacketType_VideoDisposable;
			bool bFoundFrame = false;
			// 查找到开始头
			BYTE *skip = buffer->pFrame;
			while (*(skip++) != 0x1);
			int skipBytes = (int)(skip - buffer->pFrame);
			int newPayloadSize = (buffer->frameLen - skipBytes);
			unsigned nNalType = skip[0] & 0x1F;
			OSEnterMutex(m_hLock);
			if (nNalType == NAL_SEI)
			{
				SEIData.Clear();
				BufferOutputSerializer packetOut(SEIData);
				packetOut.OutputDword(htonl(newPayloadSize));
				packetOut.Serialize(skip, newPayloadSize);
			}
			// 填充数据
			else if (nNalType == NAL_FILLER)
			{
				if (!newPacket)
					newPacket = m_CurrentPackets.CreateNew();

				BufferOutputSerializer packetOut(newPacket->Packet);
				packetOut.OutputDword(htonl(newPayloadSize));
				packetOut.Serialize(skip, newPayloadSize);

			}
			else if (nNalType == NAL_SLICE_IDR || nNalType == NAL_SLICE)
			{
				if (!newPacket)
					newPacket = m_CurrentPackets.CreateNew();

				if (!bFoundFrame)
				{
					newPacket->Packet.Insert(0, (nNalType == NAL_SLICE_IDR) ? 0x17 : 0x27);
					newPacket->Packet.Insert(1, 1);
					newPacket->Packet.InsertArray(2, timeOffsetAddr, 3);
					bFoundFrame = true;
				}
				BufferOutputSerializer packetOut(newPacket->Packet);

				packetOut.OutputDword(htonl(newPayloadSize));
				packetOut.Serialize(skip, newPayloadSize);
				bestType = PacketType_VideoLow;
				if (buffer->is_key)
				{
					bestType = PacketType_VideoHighest;
				}
			}
			else if (nNalType == NAL_SPS/* && m_PacketSPS.Num() <=0*/)
			{
				m_PacketSPS.AppendArray(skip, newPayloadSize);
			}
			else if (nNalType == NAL_PPS /*&& m_PacketPPS.Num() <=0*/)
			{				
				m_PacketPPS.AppendArray(skip, newPayloadSize);
			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV,1,"%s cant handle type <%d>", __FUNCTION__,nNalType);
			}
			if (NULL != newPacket)
			{
				newPacket->type = bestType;
				newPacket->pts = buffer->ts;
			}
			OSLeaveMutex(m_hLock);
			return;

		}
		
		static void VideoEncodeCallback(void *pUserData, RDCodec::VCodecBuffer *buffer, bool index)
		{			

			CRDX264EncoderNew *pThis = (CRDX264EncoderNew *)pUserData;
			// 缓冲区获取数据
			if (NULL != pThis || NULL != buffer)
			{
				pThis->HandleVideo(buffer, index);
			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "%s pThis 或 buffer为空",__FUNCTION__);
			}

			static QWORD StartTime = GetQPCMS();

			if (GetQPCMS() - StartTime > 80)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "%s 硬编码回调间隔过大 %llu ms", __FUNCTION__, GetQPCMS() - StartTime);
			}

			StartTime = GetQPCMS();

			return;
		}
		static  void errorCallback(void *pUserData, int errcode, char *desciption)
		{
			Log::writeError(LOG_RTSPSERV, 1, "error<%d>: %s", errcode, desciption);
			return;
		}

		~CRDX264EncoderNew()
		{
			RDVideoCodecRelease(m_pEncoder);
			RDVideoCodecDestroy(m_pEncoder);

			ClearPackets();
			OSCloseMutex(m_hLock);
			if (0 == InterlockedDecrement(&g_RD264Init))
			{
				LeaveCodec();
			}			
		}

		bool Encode(LPVOID picInPtr, List<DataPacket> &packets, List<PacketType> &packetTypes, QWORD outputTimestamp, DWORD &out_pts)
		{
			x264_picture_t *picIn = (x264_picture_t*)picInPtr;
			packets.Clear();
			if (NULL != picIn)
			{
				RDCodec::VCodecBuffer temp;
				temp.frameLen = width*height * 3 / 2;//picIn->img.i_stride[0];// ;
				temp.pFrame   = picIn->img.plane[0];
				temp.ts = picIn->i_pts;

				// 要求关键帧
				if (m_bRequestKeyframe)
				{
					temp.is_key = true;
					m_bRequestKeyframe = false;
				}
				// 送入数据
				if (m_pEncoder != NULL)
				{
					RDPushVideoFrame(m_pEncoder, &temp);
				}
			}
			else
			{
				packets.Clear();
			}
			// 回调数据
			OSEnterMutex(m_hLock);
			if (m_CurrentPackets.Num()>1) {
				packets.SetSize(1);
				packets[0].lpPacket = m_CurrentPackets[1].Packet.Array();
				packets[0].size = m_CurrentPackets[1].Packet.Num();
				packetTypes << m_CurrentPackets[1].type;
				out_pts = m_CurrentPackets[1].pts;
				m_CurrentPackets[0].Packet.Clear();
				m_CurrentPackets.Remove(0);
			}
			OSLeaveMutex(m_hLock);
			return true;
		}

		void GetHeaders(DataPacket &packet)
		{
			if (!HeaderPacket.Num() && m_PacketPPS.Num() > 0 && m_PacketSPS.Num() > 0)
			{
				BufferOutputSerializer headerOut(HeaderPacket);
				headerOut.OutputByte(0x17);
				headerOut.OutputByte(0);
				headerOut.OutputByte(0);
				headerOut.OutputByte(0);
				headerOut.OutputByte(0);
				headerOut.OutputByte(1);
				headerOut.Serialize(m_PacketSPS.Array() + 1, 3);
				headerOut.OutputByte(0xff);
				headerOut.OutputByte(0xe1);
				headerOut.OutputWord(htons(m_PacketSPS.Num()));
				headerOut.Serialize(m_PacketSPS.Array(), m_PacketSPS.Num());

				// 			x264_nal_t &pps = nalOut[i + 1]; //the PPS always comes after the SPS
				// 
				headerOut.OutputByte(1);
				headerOut.OutputWord(htons(m_PacketPPS.Num()));
				headerOut.Serialize(m_PacketPPS.Array(), m_PacketPPS.Num());
			}

			packet.lpPacket = HeaderPacket.Array();
			packet.size = HeaderPacket.Num();
		}
		virtual void RequestKeyframe()
		{
			m_bRequestKeyframe = true;
		}
		virtual void GetSEI(DataPacket &packet)
		{
			packet.lpPacket = SEIData.Array();
			packet.size = SEIData.Num();
		}

		int GetBitRate() const
		{
			return vparam.bitrate / 1000;
		}

		virtual int  GetFps() const
		{
			return vparam.fps;
		}

		virtual void GetWH(int& Width, int& Height){ Width = width; Height = height; }

		String GetInfoString() const
		{
			String strInfo;

			strInfo << TEXT("Video Encoding: RDx264") <<
				TEXT("\r\n    fps: ") << IntString(fps_ms) <<
				TEXT("\r\n    width: ") << IntString(width) << TEXT(", height: ") << IntString(height) <<
				TEXT("\r\n    max bitrate: ") << IntString(vparam.bitrate / 1000);
			return strInfo;
		}

		// 是否支持动态调整码率
		bool DynamicBitrateSupported() const
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "不支持动态调整码率");
			return false;
		}
		bool HasBufferedFrames()
		{
			return false;
		}
		bool SetBitRate(DWORD maxBitrate, DWORD bufferSize)
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "不支持动态调整码率");
			return false;
		}
	};

	// 创建函数
	VideoEncoder* CreateRDX264EncoderNew(int fps, int width, int height, int quality, CTSTR preset, bool bUse444, ColorDescription &colorDesc, int maxBitRate, int bufferSize, bool bUseCFR,int ColorT)
	{
		return new CRDX264EncoderNew(fps, width, height, quality, preset, bUse444, colorDesc, maxBitRate, bufferSize, bUseCFR, ColorT);
	}

