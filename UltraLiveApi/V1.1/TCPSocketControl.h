#ifndef TCPSOCKETCONTROL_H
#define TCPSOCKETCONTROL_H
#include "AsynIoPub.h"
#define  SEND_OK      0
#define  SEND_NOFOUND 1
#define  SEND_DATA    2

typedef struct _DATAINFO{
	PBYTE	 buf;
	DWORD	 len;
	bool     CanWrite;
	int      SendStatus;
	_DATAINFO()
	{
		buf = NULL;
		len = 0;
		CanWrite = false;
		SendStatus = -1;
	}

}__DATAINFO, *__PDATAINFO;

class CTCPSocketControl : public ITcpControlCallback2
{
public:
	CTCPSocketControl();
	virtual ~CTCPSocketControl();

	virtual void on_tcp_accept(AsynIoErr st, AIOID id, const AioAddrPair &addr);

	virtual void on_tcp_write(AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

	virtual void on_tcp_read(AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2);

};
#endif
