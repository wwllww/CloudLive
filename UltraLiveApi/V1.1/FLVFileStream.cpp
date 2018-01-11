#include "BaseAfx.h"
#include "RTMPStuff.h"
#include "FileStream.h"
#include "OperatNew.h"

#include "DataPacketHelpers.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

class FLVFileStream : public VideoFileStream
{
    XFileOutputSerializer fileOut;
    String strFile;
	bool   m_bBack;
    UINT64 metaDataPos;
    DWORD lastTimeStamp, initialTimestamp;

    decltype(GetBufferedSEIPacket()) sei = GetBufferedSEIPacket();
    decltype(GetBufferedAudioHeadersPacket()) audioHeaders = GetBufferedAudioHeadersPacket();
    decltype(GetBufferedVideoHeadersPacket()) videoHeaders = GetBufferedVideoHeadersPacket();

    bool bSentFirstPacket, bSentSEI;

    void AppendFLVPacket(const BYTE *lpData, UINT size, BYTE type, DWORD timestamp)
    {
        if (!bSentSEI && type == 9 && lpData[0] == 0x17 && lpData[1] == 0x1) { //send SEI with first keyframe packet
            UINT networkDataSize  = fastHtonl(size+sei.size);
            UINT networkTimestamp = fastHtonl(timestamp);
            UINT streamID = 0;
            fileOut.OutputByte(type);
            fileOut.Serialize(((LPBYTE)(&networkDataSize))+1,  3);
            fileOut.Serialize(((LPBYTE)(&networkTimestamp))+1, 3);
            fileOut.Serialize(&networkTimestamp, 1);
            fileOut.Serialize(&streamID, 3);
            fileOut.Serialize(lpData, 5);
            fileOut.Serialize(sei.lpPacket, sei.size);
            fileOut.Serialize(lpData+5, size-5);
            fileOut.OutputDword(fastHtonl(size+sei.size+11/*14*/));

            bSentSEI = true;
        } else {
            UINT networkDataSize  = fastHtonl(size);
            UINT networkTimestamp = fastHtonl(timestamp);
            UINT streamID = 0;
            fileOut.OutputByte(type);
            fileOut.Serialize(((LPBYTE)(&networkDataSize))+1,  3);
            fileOut.Serialize(((LPBYTE)(&networkTimestamp))+1, 3);
            fileOut.Serialize(&networkTimestamp, 1);
            fileOut.Serialize(&streamID, 3);
            fileOut.Serialize(lpData, size);
            fileOut.OutputDword(fastHtonl(size+11/*14*/));
        }

        lastTimeStamp = timestamp;
    }

public:
	bool Init(CTSTR lpFile, bool bBack)
    {
		m_bBack = bBack;
		bSentFirstPacket = false;
		bSentSEI = false;
		if (m_bBack)
		{
			videoHeaders = GetBufferedVideoHeadersPacket_Back();
			sei = GetBufferedSEIPacket_back();
		}		
        strFile = lpFile;
        initialTimestamp = -1;

        if(!fileOut.Open(lpFile, XFILE_CREATEALWAYS, 1024*1024))
            return false;

        fileOut.OutputByte('F');
        fileOut.OutputByte('L');
        fileOut.OutputByte('V');
        fileOut.OutputByte(1);
        fileOut.OutputByte(5); //bit 0 = (hasAudio), bit 2 = (hasAudio)
        fileOut.OutputDword(DWORD_BE(9));
        fileOut.OutputDword(0);

        metaDataPos = fileOut.GetPos();

        char  metaDataBuffer[2048];
        char *enc = metaDataBuffer;
        char *pend = metaDataBuffer+sizeof(metaDataBuffer);

        enc = AMF_EncodeString(enc, pend, &av_onMetaData);
		char *endMetaData = G_LiveInstance->EncMetaData(enc, pend, true, m_bBack);
        UINT  metaDataSize = endMetaData-metaDataBuffer;

        AppendFLVPacket((LPBYTE)metaDataBuffer, metaDataSize, 18, 0);
        return true;
    }

    ~FLVFileStream()
    {
        UINT64 fileSize = fileOut.GetPos();
        fileOut.Close();

        XFile file;
        if(file.Open(strFile, XFILE_WRITE, XFILE_OPENEXISTING))
        {
            double doubleFileSize = double(fileSize);
            double doubleDuration = double(lastTimeStamp/1000);

            file.SetPos(metaDataPos+0x28, XFILE_BEGIN);
            QWORD outputVal = *reinterpret_cast<QWORD*>(&doubleDuration);
            outputVal = fastHtonll(outputVal);
            file.Write(&outputVal, 8);

            file.SetPos(metaDataPos+0x3B, XFILE_BEGIN);
            outputVal = *reinterpret_cast<QWORD*>(&doubleFileSize);
            outputVal = fastHtonll(outputVal);
            file.Write(&outputVal, 8);

            file.Close();
        }
    }

    void InitBufferedPackets()
    {
        sei.InitBuffer();
        audioHeaders.InitBuffer();
        videoHeaders.InitBuffer();
    }

    virtual void AddPacket(const BYTE *data, UINT size, DWORD timestamp, DWORD /*pts*/, PacketType type) override
    {
        InitBufferedPackets();

        if(!bSentFirstPacket)
        {
            bSentFirstPacket = true;

            AppendFLVPacket(audioHeaders.lpPacket, audioHeaders.size, 8, 0);
            AppendFLVPacket(videoHeaders.lpPacket, videoHeaders.size, 9, 0);
        }

        if(initialTimestamp == -1 && data[0] != 0x17)
            return;
        else if(initialTimestamp == -1 && data[0] == 0x17) {
            initialTimestamp = timestamp;
        }

        AppendFLVPacket(data, size, (type == PacketType_Audio) ? 8 : 9, timestamp-initialTimestamp);
    }
};


VideoFileStream* CreateFLVFileStream(CTSTR lpFile,bool bBack)
{
    FLVFileStream *fileStream = new FLVFileStream;
	if (fileStream->Init(lpFile, bBack))
        return fileStream;

    delete fileStream;
    return NULL;
}
