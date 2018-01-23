#include "BaseAfx.h"
#include "RTMPStuff.h"
#include "RtmpPublish.h"

#include "DataPacketHelpers.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

class CFLVFileStream : public VideoFileStream
{
	XFileOutputSerializer fileOut;
	String strFile;
	bool   m_bBack;
	bool   m_bHasAudio = false;
	UINT64 metaDataPos = 0;
	DWORD lastTimeStamp = 0, initialTimestamp = 0;
	CInstanceProcess *m_pIntanceProcess = NULL;
	bool m_bNewSubsectionFlag = false;           //分时间段存储文件
	UINT64 m_uSubsectionDuration = 0;  //读取配置文件

	List<BYTE> sei;
	List<BYTE> audioHeaders;
	List<BYTE> videoHeaders;

	bool bSentFirstPacket = false, bSentSEI = false;

	void AppendFLVPacket(const BYTE *lpData, UINT size, BYTE type, DWORD timestamp)
	{
		if (!bSentSEI && type == 9 && lpData[0] == 0x17 && lpData[1] == 0x1) { //send SEI with first keyframe packet
			UINT networkDataSize = fastHtonl(size + sei.Num());
			UINT networkTimestamp = fastHtonl(timestamp);
			UINT streamID = 0;
			fileOut.OutputByte(type);
			fileOut.Serialize(((LPBYTE)(&networkDataSize)) + 1, 3);
			fileOut.Serialize(((LPBYTE)(&networkTimestamp)) + 1, 3);
			fileOut.Serialize(&networkTimestamp, 1);
			fileOut.Serialize(&streamID, 3);
			fileOut.Serialize(lpData, 5);
			fileOut.Serialize(sei.Array(), sei.Num());
			fileOut.Serialize(lpData + 5, size - 5);
			fileOut.OutputDword(fastHtonl(size + sei.Num() + 11/*14*/));

			bSentSEI = true;
		}
		else {
			UINT networkDataSize = fastHtonl(size);
			UINT networkTimestamp = fastHtonl(timestamp);
			UINT streamID = 0;
			fileOut.OutputByte(type);
			fileOut.Serialize(((LPBYTE)(&networkDataSize)) + 1, 3);
			fileOut.Serialize(((LPBYTE)(&networkTimestamp)) + 1, 3);
			fileOut.Serialize(&networkTimestamp, 1);
			fileOut.Serialize(&streamID, 3);
			fileOut.Serialize(lpData, size);
			fileOut.OutputDword(fastHtonl(size + 11/*14*/));
		}

		lastTimeStamp = timestamp;
	}

public:
	bool ResetFileInit()
	{
		UINT64 fileSize = fileOut.GetPos();
		fileOut.Close();

		XFile file;
		if (file.Open(strFile, XFILE_WRITE, XFILE_OPENEXISTING))
		{
			double doubleFileSize = double(fileSize);
			double doubleDuration = double(lastTimeStamp / 1000);

			file.SetPos(metaDataPos + 0x28, XFILE_BEGIN);
			QWORD outputVal = *reinterpret_cast<QWORD*>(&doubleDuration);
			outputVal = fastHtonll(outputVal);
			file.Write(&outputVal, 8);

			file.SetPos(metaDataPos + 0x3B, XFILE_BEGIN);
			outputVal = *reinterpret_cast<QWORD*>(&doubleFileSize);
			outputVal = fastHtonll(outputVal);
			file.Write(&outputVal, 8);

			file.Close();
		}
		//Init------------------------------------------------------------------------------
		initialTimestamp = -1;
		m_bNewSubsectionFlag = false;
		bSentFirstPacket = false, bSentSEI = false;
		//audioHeaders.Clear();
		//videoHeaders.Clear();
		//sei.Clear();

		SYSTEMTIME st;
		GetLocalTime(&st);
		String strOutputTime = FormattedString(TEXT("%u-%02u-%02u-%02u%02u-%02u"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

		int nRightPos = -1;
		for (int iIndex = strFile.Length(); iIndex > 0; iIndex--)
		{
			if (strFile[iIndex] == L'(')
			{
				nRightPos = iIndex;
				break;
			}
		}

		if (nRightPos >= 18)
		{
			String Tmp = strFile.Left(nRightPos - 18);
			Tmp = Tmp + strOutputTime;
			Tmp = Tmp + strFile.Right(strFile.Length() - nRightPos);
			strFile = Tmp;
		}
		else
		{
			return false;
		}

		if (!fileOut.Open(strFile, XFILE_CREATEALWAYS, 1024 * 1024))
			return false;

		fileOut.OutputByte('F');
		fileOut.OutputByte('L');
		fileOut.OutputByte('V'); // 3个字节标准头
		fileOut.OutputByte(1);   // 一个字节版本号
		// 无音频数据
		if (!m_bHasAudio)
		{
			fileOut.OutputByte(1);  //bit 0 = (hasAudio), bit 2 = (hasAudio) 音视频标记
		}
		else
		{
			fileOut.OutputByte(5);  //bit 0 = (hasAudio), bit 2 = (hasAudio) 音视频标记
		}

		fileOut.OutputDword(DWORD_BE(9)); // flv头长度
		fileOut.OutputDword(0);           // 前面长度，默认0 4个字节

		metaDataPos = fileOut.GetPos();

		char  metaDataBuffer[2048];
		char *enc = metaDataBuffer;
		char *pend = metaDataBuffer + sizeof(metaDataBuffer);

		enc = AMF_EncodeString(enc, pend, &av_onMetaData);
		char *endMetaData = m_pIntanceProcess->EncMetaData(enc, pend, true, m_bBack);
		UINT  metaDataSize = endMetaData - metaDataBuffer;

		AppendFLVPacket((LPBYTE)metaDataBuffer, metaDataSize, 18, 0);
		return true;
	}
	bool Init(CTSTR lpFile,bool hasAudio, bool bBack,CInstanceProcess *IntanceProcess)
	{
		m_pIntanceProcess = IntanceProcess;
		m_bHasAudio = hasAudio;
		m_bBack = bBack;			
		strFile = lpFile;
		initialTimestamp = -1;

		if (!fileOut.Open(lpFile, XFILE_CREATEALWAYS, 1024 * 1024))
			return false;

		fileOut.OutputByte('F');
		fileOut.OutputByte('L');
		fileOut.OutputByte('V'); // 3个字节标准头
		fileOut.OutputByte(1);   // 一个字节版本号
		// 无音频数据
		if (!hasAudio)
		{
			fileOut.OutputByte(1);  //bit 0 = (hasAudio), bit 2 = (hasAudio) 音视频标记
		}
		else
		{
			fileOut.OutputByte(5);  //bit 0 = (hasAudio), bit 2 = (hasAudio) 音视频标记
		}
			
		fileOut.OutputDword(DWORD_BE(9)); // flv头长度
		fileOut.OutputDword(0);           // 前面长度，默认0 4个字节

		metaDataPos = fileOut.GetPos();

		char  metaDataBuffer[2048];
		char *enc = metaDataBuffer;
		char *pend = metaDataBuffer + sizeof(metaDataBuffer);

		enc = AMF_EncodeString(enc, pend, &av_onMetaData);
		char *endMetaData = IntanceProcess->EncMetaData(enc, pend, true, m_bBack);
		UINT  metaDataSize = endMetaData - metaDataBuffer;

		AppendFLVPacket((LPBYTE)metaDataBuffer, metaDataSize, 18, 0);
		return true;
	}

	~CFLVFileStream()
	{
		UINT64 fileSize = fileOut.GetPos();
		fileOut.Close();

		XFile file;
		if (file.Open(strFile, XFILE_WRITE, XFILE_OPENEXISTING))
		{
			double doubleFileSize = double(fileSize);
			double doubleDuration = double(lastTimeStamp / 1000);

			file.SetPos(metaDataPos + 0x28, XFILE_BEGIN);
			QWORD outputVal = *reinterpret_cast<QWORD*>(&doubleDuration);
			outputVal = fastHtonll(outputVal);
			file.Write(&outputVal, 8);

			file.SetPos(metaDataPos + 0x3B, XFILE_BEGIN);
			outputVal = *reinterpret_cast<QWORD*>(&doubleFileSize);
			outputVal = fastHtonll(outputVal);
			file.Write(&outputVal, 8);

			file.Close();
		}
	}

	void InitBufferedPackets()
	{
		sei.Clear();
		audioHeaders.Clear();
		videoHeaders.Clear();
	}

	virtual void AddPacket(const BYTE *data, UINT size, DWORD timestamp, DWORD /*pts*/, PacketType type) override
	{
		if (m_pIntanceProcess)
		{
			m_uSubsectionDuration = m_pIntanceProcess->GetRecordTime() * 60 * 1000;
		}
		
		if (m_uSubsectionDuration <= 0)
		{
			m_uSubsectionDuration = 10000000000000/*3600 * 1000 * 4*/;
		}

		if (m_bNewSubsectionFlag)
		{
			ResetFileInit();
			m_bNewSubsectionFlag = false;
		}

		if (PacketType_Video_SEI == type)
		{
			sei.Clear();
			sei.AppendArray(data, size);
			return;
		}
		else if(PacketType_Video_HEAD == type)
		{
			videoHeaders.Clear();
			videoHeaders.AppendArray(data, size);
			return;
		}
		else if (PacketType_Audio_HEAD == type)
		{
			audioHeaders.Clear();
			audioHeaders.AppendArray(data, size);
			return;
		}
			
		if (!bSentFirstPacket)
		{
			bSentFirstPacket = true;
			if (audioHeaders.Num()>0)
			{
				AppendFLVPacket(audioHeaders.Array(), audioHeaders.Num(), 8, 0);
			}				
			AppendFLVPacket(videoHeaders.Array(), videoHeaders.Num(), 9, 0);
		}

		if (initialTimestamp == -1 && data[0] != 0x17)
			return;
		else if (initialTimestamp == -1 && data[0] == 0x17) 
		{
			initialTimestamp = timestamp;
		}

		AppendFLVPacket(data, size, (type == PacketType_Audio) ? 8 : 9, timestamp - initialTimestamp);
		if (timestamp - initialTimestamp > m_uSubsectionDuration)  //满足文件分片时长
		{
			m_bNewSubsectionFlag = true;
		}
	}
};
VideoFileStream* CreateFLVFileStreamNew(CTSTR lpFile, bool hasAudio, bool bBack, CInstanceProcess *IntanceProcess)
{
	CFLVFileStream *fileStream = new CFLVFileStream;
	if (fileStream->Init(lpFile, hasAudio, bBack, IntanceProcess))
		return fileStream;

	delete fileStream;
	return NULL;
}


