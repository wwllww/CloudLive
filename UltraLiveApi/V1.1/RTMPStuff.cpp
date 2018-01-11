#include "RTMPStuff.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

void InitSockets()
{
    WSADATA wsad;
    WSAStartup(MAKEWORD( 2, 2 ), &wsad);
}

void TerminateSockets()
{
    WSACleanup();
}

int SendResultNumber(RTMP *r, double txn, double ID)
{
    RTMPPacket packet;
    char pbuf[256], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_NULL;
    enc = AMF_EncodeNumber(enc, pend, ID);

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendConnectResult(RTMP *r, double txn)
{
    RTMPPacket packet;
    char pbuf[384], *pend = pbuf+sizeof(pbuf);
    AMFObject obj;
    AMFObjectProperty p, op;
    AVal av;

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "FMS/3,5,1,525");
    enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "status");
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
    STR2AVAL(av, "NetConnection.Connect.Success");
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
    STR2AVAL(av, "Connection succeeded.");
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);

    STR2AVAL(p.p_name, "version");
    STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
    p.p_type = AMF_STRING;
    obj.o_num = 1;
    obj.o_props = &p;
    op.p_type = AMF_OBJECT;
    STR2AVAL(op.p_name, "data");
    op.p_vu.p_object = obj;
    enc = AMFProp_Encode(&op, enc, pend);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
}

void AVreplace(AVal *src, const AVal *orig, const AVal *repl)
{
    char *srcbeg = src->av_val;
    char *srcend = src->av_val + src->av_len;
    char *dest, *sptr, *dptr;
    int n = 0;

    /* count occurrences of orig in src */
    sptr = src->av_val;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
        n++;
        sptr += orig->av_len;
    }
    if (!n)
        return;

    dest = (char*)malloc(src->av_len + 1 + (repl->av_len - orig->av_len) * n);

    sptr = src->av_val;
    dptr = dest;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
        n = sptr - srcbeg;
        memcpy(dptr, srcbeg, n);
        dptr += n;
        memcpy(dptr, repl->av_val, repl->av_len);
        dptr += repl->av_len;
        sptr += orig->av_len;
        srcbeg = sptr;
    }
    n = srcend - srcbeg;
    memcpy(dptr, srcbeg, n);
    dptr += n;
    *dptr = '\0';
    src->av_val = dest;
    src->av_len = dptr - dest;
}

int SendPlayStart(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Start);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Started_playing);
    enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
    enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

int SendPlayStop(RTMP *r)
{
    RTMPPacket packet;
    char pbuf[512], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av_onStatus);
    enc = AMF_EncodeNumber(enc, pend, 0);
    *enc++ = AMF_NULL;
    *enc++ = AMF_OBJECT;

    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av_status);
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av_NetStream_Play_Stop);
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av_Stopped_playing);
    enc = AMF_EncodeNamedString(enc, pend, &av_details, &r->Link.playpath);
    enc = AMF_EncodeNamedString(enc, pend, &av_clientid, &av_clientid);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;
    return RTMP_SendPacket(r, &packet, FALSE);
}

