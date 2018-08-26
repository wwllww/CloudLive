#ifndef HTTPSOCKETCONTROL_H
#define HTTPSOCKETCONTROL_H
#include "AsynIoPub.h"
#define  SEND_OK      0
#define  SEND_NOFOUND 1
#define  SEND_DATA    2

typedef struct _DATAINFO{
	PBYTE	 buf;
	DWORD	 len;
	bool     CanWrite;
	int      SendStatus;
	int      BufRealLen;
	_DATAINFO()
	{
		buf = NULL;
		len = 0;
		CanWrite = false;
		SendStatus = -1;
		BufRealLen = 0;
	}

}__DATAINFO, *__PDATAINFO;

typedef struct _MsgInfo{
	AIOID id;
	char *Msgbuf;
	int MsgLen;
	ULL64 Context;
	bool bPost;
	bool bOptions;
	_MsgInfo()
	{
		id = -1;
		Msgbuf = NULL;
		Context = 0;
		bPost = false;
		bOptions = false;
	}

}__MsgInfo, *__pMsgInfo;

class CTCPSocketControl : public ITcpControlCallback2
{
public:
	CTCPSocketControl();
	virtual ~CTCPSocketControl();

	virtual void on_tcp_accept(AsynIoErr st, AIOID id, const AioAddrPair &addr);

	virtual void on_tcp_connect(AsynIoErr st, AIOID id, const AioAddrPair &addr, ULL64 ctx1, ULL64 ctx2);

	virtual void on_tcp_write(AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

	virtual void on_tcp_read(AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

};
#endif
