#include "BaseAfx.h"
#include "OperatNew.h"
#include <inttypes.h>
#include <ws2tcpip.h>

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

extern "C"
{
#include "nvEncodeAPI.h"
#include "NvCodecDef.h"
#include "NvEncoderExport.h"
#include "../x264/x264.h"
}
#include "Encoder.h"

struct VideoPacketNvidia
{
    List<BYTE> Packet;
	uint64_t   pts;
	PacketType type;
	inline void FreeData() { Packet.Clear(); pts = 0; type = PacketType_VideoDisposable; }
};

const int MAX_SPS_PPS_LEN = 128;
const char g_start_code[4] = { 0x00, 0x00, 0x00, 0x01 };
const int g_start_code_len = 4;
const char g_start_code_thr[3] = { 0x00, 0x00, 0x01 };
const int g_start_code_len_thr = 3;

typedef enum NALTYPE
{
	H264_P = 1,
	H264_IDR = 5,
	H264_SPS = 7,
	H264_PPS = 8,
};

static int getNALLen(char *data, int datalen)
{
	char *tmp = data;
	while (datalen > 0)
	{
		if (memcmp(tmp, g_start_code, g_start_code_len) == 0 || memcmp(tmp, g_start_code_thr, g_start_code_len_thr) == 0)
		{
			return (int)(tmp - data);
		}
		datalen -= 1;
		tmp += 1;
	}
	return 0;
}

#define DEFAULT_I_QFACTOR 0.8f
#define DEFAULT_B_QFACTOR 1.25f
#define DEFAULT_I_QOFFSET 0.f
#define DEFAULT_B_QOFFSET 1.25f

static int GetNvidiaAdpterID()
{
	DeviceOutputs OutPuts;
	GetDisplayDevices(OutPuts);

	if (OutPuts.devices.Num() > 0)
	{
		for (int i = 0; i < OutPuts.devices.Num(); ++i)
		{
			if (sstri((const TCHAR*)OutPuts.devices[i].strDevice.Array(), L"NVIDIA") != NULL)
			{
				return i;
			}
		}
	}
	return -1;
}

class CNvidiaEncoder : public VideoEncoder
{
protected:	
	void                *m_pEncoder;	
	EncodeConfig        m_encodeConfig;
    
	uint64_t			m_nFrameCount;
    int fps_ms;
    bool m_bRequestKeyframe;      
    UINT m_width, m_height;   
	int frameShift;

	List<BYTE> m_PacketPPS;
	List<BYTE> m_PacketSPS;

	List<VideoPacketNvidia> m_CurrentPackets;
    List<BYTE> HeaderPacket, SEIData;      

	char				m_sps[MAX_SPS_PPS_LEN];
	char				m_pps[MAX_SPS_PPS_LEN];
	int					m_spslen;
	int					m_ppslen;
	bool                ConstructSuc;

    inline void ClearPackets()
    {
		for (UINT i = 0; i<m_CurrentPackets.Num(); i++)
			m_CurrentPackets[i].FreeData();
		m_CurrentPackets.Clear();
    }

    inline void SetBitRateParams(DWORD maxBitrate, DWORD bufferSize)
    {
		m_encodeConfig.bitrate = maxBitrate * 1000;
		return;
    }

public:
	CNvidiaEncoder(int fps, int width, int height, int quality, CTSTR preset, bool bUseDefaultConfig, ColorDescription &colorDesc, int maxBitrate, int bufferSize, bool bUseBack, int ColorT)
    {  
		Log::writeMessage(LOG_RTSPSERV, 1, "CNvidiaEncoder 构造!\n");

		m_nFrameCount = 0;
		frameShift = 0;
		memset(m_sps, 0, MAX_SPS_PPS_LEN);
		memset(m_pps, 0, MAX_SPS_PPS_LEN);
		m_spslen = 0;
		m_ppslen = 0;

        fps_ms = 1000/fps;
		m_width = width;
		m_height = height;

		memset(&m_encodeConfig, 0, sizeof(EncodeConfig));

		m_encodeConfig.encoderPreset = "hq";
		
		if (bUseDefaultConfig)
		{
			m_encodeConfig.numB = 0;
			m_encodeConfig.gopLength = fps;
			m_encodeConfig.rcMode = 16;
		}
		else
		{
			UINT keyframeInterval = CSLiveManager::GetInstance()->BSParam.LiveSetting.KeyFrame;

			int nBFrameCount = CSLiveManager::GetInstance()->BSParam.LiveSetting.BFrameCount;

			if (bUseBack)
			{
				keyframeInterval = CSLiveManager::GetInstance()->BSParam.LiveSetting.KeyFrameSec;
				nBFrameCount = CSLiveManager::GetInstance()->BSParam.LiveSetting.BFrameCountSec;

				if (keyframeInterval == 0)
				{
					keyframeInterval = 1;
				}
				if (-1 != nBFrameCount)
				{
					m_encodeConfig.numB = nBFrameCount;
				}
				
				m_encodeConfig.gopLength = fps * keyframeInterval;
				m_encodeConfig.rcMode = CSLiveManager::GetInstance()->BSParam.LiveSetting.bUseCBRSec ? 16 : 32;
			}
			else
			{
				if (keyframeInterval == 0)
				{
					keyframeInterval = 1;
				}

				if (-1 != nBFrameCount)
				{
					m_encodeConfig.numB = nBFrameCount;
				}

				m_encodeConfig.gopLength = fps * keyframeInterval;
				m_encodeConfig.rcMode = CSLiveManager::GetInstance()->BSParam.LiveSetting.bUseCBR ? 16 : 32;
			}
		
		}
	
		m_encodeConfig.pictureStruct = 1;
		m_encodeConfig.deviceType = 1;
		m_encodeConfig.deviceID = GetNvidiaAdpterID();

		m_encodeConfig.fps = fps;
		m_encodeConfig.width = width;
		m_encodeConfig.height = height;
		m_encodeConfig.bitrate = maxBitrate * 1000;
		m_encodeConfig.vbvMaxBitrate = maxBitrate * 1000;
		m_encodeConfig.vbvSize = bufferSize * 1000;

		if (ColorT == 0)
		{
			m_encodeConfig.RealInputFormat = NV_ENC_BUFFER_FORMAT_YV12;
		}
		else if (ColorT == 1)
		{
			m_encodeConfig.RealInputFormat = NV_ENC_BUFFER_FORMAT_NV12;
		}
		else if (ColorT == 2)
		{
			m_encodeConfig.RealInputFormat = NV_ENC_BUFFER_FORMAT_IYUV;
		}
		m_encodeConfig.inputFormat = NV_ENC_BUFFER_FORMAT_NV12;
		m_encodeConfig.codec = 0;
		m_encodeConfig.qp = 28;
		m_encodeConfig.i_quant_factor = DEFAULT_I_QFACTOR;
		m_encodeConfig.b_quant_factor = DEFAULT_B_QFACTOR;
		m_encodeConfig.i_quant_offset = DEFAULT_I_QOFFSET;
		m_encodeConfig.b_quant_offset = DEFAULT_B_QOFFSET;
		m_encodeConfig.maxWidth = 0;
		m_encodeConfig.maxHeight = 0;

		Log::writeMessage(LOG_RTSPSERV, 1, "CNvidiaEncoder构造中, bitrate:%d\
			codec:%d,\
			deviceID:%d,\
			deviceType:%d \
			encoderPreset:%s \
			fps:%d \
			gopLength:%d \
			width:%d \
			height:%d \
			maxWidth:%d \
			maxHeight:%d \
			numB:%d \
			rcMode:%d\
			pictureStruct:%d\
			inputFormat:%x!\n",
			m_encodeConfig.bitrate,
			m_encodeConfig.codec,
			m_encodeConfig.deviceID,
			m_encodeConfig.deviceType,
			m_encodeConfig.encoderPreset,
			m_encodeConfig.fps,
			m_encodeConfig.gopLength,
			m_encodeConfig.width,
			m_encodeConfig.height,
			m_encodeConfig.maxWidth,
			m_encodeConfig.maxHeight,
			m_encodeConfig.numB,
			m_encodeConfig.rcMode,
			m_encodeConfig.pictureStruct,
			m_encodeConfig.inputFormat);

		m_pEncoder = NvEncodeCreate(&m_encodeConfig);
		if (nullptr == m_pEncoder)
		{
			ConstructSuc = false;
			Log::writeMessage(LOG_RTSPSERV, 1, "NvEncodeCreate error!\n");
			return;			
		}
		ConstructSuc = true;
		m_CurrentPackets.CreateNew();
    }
	~CNvidiaEncoder()
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "CNvidiaEncoder 析构!\n");

		ClearPackets();
		if (m_pEncoder)
			NvEncodeDestroy(m_pEncoder);
	}

	void HandleVideo(unsigned char *pFrame, int frameLen, uint64_t ts, bool bKey, bool bFrame, NV_ENC_PIC_TYPE pictureType)
	{
		int timeOffset = 0;
		if (m_encodeConfig.numB != 0)
		{
			timeOffset = int(ts - m_nFrameCount * fps_ms);
		}
		
		timeOffset += frameShift;
		if (timeOffset < 0)
		{
			frameShift -= timeOffset;
			timeOffset = 0;
		}
		timeOffset = htonl(timeOffset);
		BYTE *timeOffsetAddr = ((BYTE*)&timeOffset) + 1;

		m_nFrameCount++;

		VideoPacketNvidia *newPacket = NULL;
		bool bFoundFrame   = false;	

		BYTE *skip = pFrame;
		if (!bFrame)
		{
			while (*(skip++) != 0x1);
		}
		int skipBytes = (int)(skip - pFrame);
		int newPayloadSize = (frameLen - skipBytes);
		unsigned nNalType  = skip[0] & 0x1F;	
		if (nNalType == NAL_SEI)
		{			
			SEIData.Clear();
			BufferOutputSerializer packetOut(SEIData);
			packetOut.OutputDword(htonl(newPayloadSize));
			packetOut.Serialize(skip, newPayloadSize);
		}
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
		}
		else if (nNalType == NAL_SPS)
		{
			m_PacketSPS.Clear();
			m_PacketSPS.AppendArray(skip, newPayloadSize);
		}
		else if (nNalType == NAL_PPS)
		{
			m_PacketPPS.Clear();
			m_PacketPPS.AppendArray(skip, newPayloadSize);
		}
		if (newPacket)
		{
			if (pictureType == NV_ENC_PIC_TYPE_I || pictureType == NV_ENC_PIC_TYPE_IDR)
				newPacket->type = PacketType_VideoHighest;
			else if (pictureType == NV_ENC_PIC_TYPE_P)
				newPacket->type = PacketType_VideoHigh;
			else 
				newPacket->type = PacketType_VideoDisposable;
			newPacket->pts = ts;
		}
	}

	void parse_encode_stream(char* pbuffer, int bufferlen, bool iskey, uint64_t ts, NV_ENC_PIC_TYPE pictureType)
	{
		char *tmpframe = pbuffer;
		int tmpfrmlen = bufferlen;
		while (true)
		{
			if (memcmp(tmpframe, g_start_code, g_start_code_len) == 0)
			{
				tmpframe += 4;
				tmpfrmlen -= 4;
			}
			else if (memcmp(tmpframe, g_start_code_thr, g_start_code_len_thr) == 0)
			{
				tmpframe += 3;
				tmpfrmlen -= 3;
			}
			else
			{
				return;
			}

			int naltype = tmpframe[0] & 0x1f;
			int nallen = getNALLen(tmpframe, tmpfrmlen);
			if (nallen > 0)
			{
				if ((tmpframe[0] & 0x60) != 0 || naltype == 1)
				{
					if (naltype == H264_SPS)
					{
						memcpy(m_sps, g_start_code, g_start_code_len);
						memcpy(m_sps + g_start_code_len, tmpframe, nallen);
						m_spslen = nallen + g_start_code_len;
					}
					else if (naltype == H264_PPS)
					{
						memcpy(m_pps, g_start_code, g_start_code_len);
						memcpy(m_pps + g_start_code_len, tmpframe, nallen);
						m_ppslen = nallen + g_start_code_len;
					}
// 					else
// 					{
// 						if (naltype == H264_IDR && m_spslen > 0 && m_ppslen > 0)
// 						{
// 							HandleVideo((unsigned char*)m_sps, m_spslen, ts, iskey, false);
// 							HandleVideo((unsigned char*)m_pps, m_ppslen, ts, iskey, false);
// 						}
// 						HandleVideo((unsigned char*)tmpframe, nallen, ts, iskey, true);
// 					}
				}
				tmpframe += nallen;
				tmpfrmlen -= nallen;
				if (tmpfrmlen <= 0)
					break;
			}
			else
			{
				if ((tmpframe[0] & 0x60) != 0 || naltype == 1)
				{
// 					if (naltype == H264_SPS)
// 					{
// 						memcpy(m_sps, g_start_code, g_start_code_len);
// 						memcpy(m_sps + g_start_code_len, tmpframe, tmpfrmlen);
// 						m_spslen = tmpfrmlen + g_start_code_len;
// 					}
// 					else if (naltype == H264_PPS)
// 					{
// 						memcpy(m_pps, g_start_code, g_start_code_len);
// 						memcpy(m_pps + g_start_code_len, tmpframe, tmpfrmlen);
// 						m_ppslen = tmpfrmlen + g_start_code_len;
// 					}
// 					else
					{
						if (naltype == H264_IDR && m_spslen > 0 && m_ppslen > 0)
						{
							HandleVideo((unsigned char*)m_sps, m_spslen, ts, iskey, false, pictureType);
							HandleVideo((unsigned char*)m_pps, m_ppslen, ts, iskey, false, pictureType);
						}
						HandleVideo((unsigned char*)tmpframe, tmpfrmlen, ts, iskey, true, pictureType);
					}
				}
				break;
			}
		}
	}
	bool Encode(LPVOID picInPtr, List<DataPacket> &packets, List<PacketType> &packetTypes, QWORD outputTimestamp, DWORD &out_pts)
    {
		x264_picture_t *picIn = (x264_picture_t*)picInPtr;
		packets.Clear();
		if (picIn)
		{
			VCodecBuffer in;
			in.frameLen = m_width * m_height * 3 / 2;
			in.pFrame = picIn->img.plane[0];
			in.ts = picIn->i_pts;	
			in.is_key = picIn->b_keyframe;
// 			Log::writeMessage(LOG_RTSPSERV, 1, "Encode_Nvidia 输入ts=%d, 帧类型=%d\n", in.ts, picIn->i_type);

			VCodecBuffer* pOut = nullptr;
			if (m_pEncoder)
				NvEncodeFrame(m_pEncoder, &in, &pOut);
			if (pOut && pOut->pFrame)
			{
// 				Log::writeMessage(LOG_RTSPSERV, 1, "Encode_Nvidia 输出ts=%d, 帧类型=%d\n", pOut->ts * fps_ms, pOut->pictureType);
				parse_encode_stream((char*)pOut->pFrame, pOut->frameLen, pOut->is_key, pOut->ts * fps_ms, pOut->pictureType);
			}
		}	
		else
		{
			VCodecBuffer* pOut = nullptr;
			if (m_pEncoder)
				NvEncodeFrame(m_pEncoder, nullptr, &pOut);
			if (pOut && pOut->pFrame)
			{
// 				Log::writeMessage(LOG_RTSPSERV, 1, "Encode_Nvidia Flush输出ts=%d, 帧类型=%d\n", pOut->ts * fps_ms, pOut->pictureType);
				parse_encode_stream((char*)pOut->pFrame, pOut->frameLen, pOut->is_key, pOut->ts * fps_ms, pOut->pictureType);
			}
		}

		if (m_CurrentPackets.Num()>1) 
		{
			packets.SetSize(1);
			packets[0].lpPacket = m_CurrentPackets[1].Packet.Array();
			packets[0].size = m_CurrentPackets[1].Packet.Num();
			packetTypes << m_CurrentPackets[1].type;
			out_pts = m_CurrentPackets[1].pts;
			m_CurrentPackets[0].Packet.Clear();
			m_CurrentPackets.Remove(0);
		}		
        return true;
    }

    void GetHeaders(DataPacket &packet)
    {
        if(!HeaderPacket.Num() && m_PacketPPS.Num() > 0 && m_PacketSPS.Num() > 0)
        {
			BufferOutputSerializer headerOut(HeaderPacket);
			headerOut.OutputByte(0x17);
			headerOut.OutputByte(0);
			headerOut.OutputByte(0);
			headerOut.OutputByte(0);
			headerOut.OutputByte(0);
			headerOut.OutputByte(1);
			headerOut.Serialize(m_PacketSPS.Array()+1, 3);
			headerOut.OutputByte(0xff);
			headerOut.OutputByte(0xe1);
			headerOut.OutputWord(htons(m_PacketSPS.Num()));
			headerOut.Serialize(m_PacketSPS.Array(), m_PacketSPS.Num());

			headerOut.OutputByte(1);
			headerOut.OutputWord(htons(m_PacketPPS.Num()));
			headerOut.Serialize(m_PacketPPS.Array(), m_PacketPPS.Num());
        }
        packet.lpPacket = HeaderPacket.Array();
        packet.size     = HeaderPacket.Num();
    }

    virtual void GetSEI(DataPacket &packet)
    {
        packet.lpPacket = SEIData.Array();
        packet.size     = SEIData.Num();
    }

    int GetBitRate() const
    {
		return m_encodeConfig.bitrate / 1000;
    }
	virtual int  GetFps() const
	{
		return m_encodeConfig.fps;
	}
	virtual void GetWH(int& Width, int& Height){ Width = m_width; Height = m_height; }
    String GetInfoString() const
    {
        String strInfo;
		strInfo << TEXT("Video Encoding: Nvidia") <<
			TEXT("\r\n    fps: ") << IntString(fps_ms) <<
			TEXT("\r\n    width: ") << IntString(m_width) << TEXT(", height: ") << IntString(m_height) <<			
			TEXT("\r\n    max bitrate: ") << IntString(m_encodeConfig.bitrate / 1000);
        return strInfo;
    }

    bool DynamicBitrateSupported() const
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "不支持动态调整码率");
        return false;
    }
	bool HasBufferedFrames()
	{ 
		if (m_pEncoder)
			return NvGetBufferedCount(m_pEncoder) > 0;
		else
			return false;
	}
	bool SetBitRate(DWORD maxBitrate, DWORD bufferSize)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "不支持动态调整码率");
		return false;
	}

	bool IsConstructSuc() const{ return ConstructSuc; }
};

VideoEncoder* CreateNvidiaEncoder(int fps, int width, int height, int quality, CTSTR preset, bool bUseDefaultConfig, ColorDescription &colorDesc, int maxBitRate, int bufferSize, bool bUseBack, int ColorT)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "调用CreateNvidiaEncoder");
	CNvidiaEncoder *NvidiaEncoder = new CNvidiaEncoder(fps, width, height, quality, preset, bUseDefaultConfig, colorDesc, maxBitRate, bufferSize, bUseBack, ColorT);
	if (!NvidiaEncoder->IsConstructSuc())
	{
		Log::writeError(LOG_RTSPSERV, 1, "CreateNvidiaEncoder创建失败");
		delete NvidiaEncoder;
		return NULL;
	}
	return NvidiaEncoder;
}

