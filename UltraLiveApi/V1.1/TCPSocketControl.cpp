#include "TCPSocketControl.h"
#include "HttpLive.h"
#include "LogDeliver.h"

CTCPSocketControl::CTCPSocketControl()
{

}

CTCPSocketControl::~CTCPSocketControl()
{

}

void CTCPSocketControl::on_tcp_accept(AsynIoErr st, AIOID id, const AioAddrPair &addr)
{
	if(st == AIO_SUC)
	{
		char *Read = new char[4096];
		HttpLive::TcpControl->asyn_read(id,Read,4096,0,1000,false);
	}
	else
	{
		HttpLive::TcpControl->close_connect(id);
	}
}

void CTCPSocketControl::on_tcp_write( AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2 )
{
	__PDATAINFO DataInfo = (__PDATAINFO)ctx2;
	if(st == AIO_SUC)
	{ 
		if(DataInfo)
		{
			if(DataInfo->SendStatus == SEND_OK)
			{
				HttpLive *Live = (HttpLive*)ctx1;
				DataInfo->CanWrite = true;
				if(!Live->DoAccept(id))
				{
					HttpLive::TcpControl->close_connect(id);
				}
			}
			else if(DataInfo->SendStatus == SEND_DATA)
			{
				DataInfo->CanWrite = true;

			}
			else if(DataInfo->SendStatus == SEND_NOFOUND)
			{
				HttpLive::TcpControl->close_connect(id);

				if(DataInfo->buf)
					delete [] DataInfo->buf;

				delete [] DataInfo;
				
			}
		}
	}
	else
	{
		Log::writeMessage(562949953421312,1," %s on_tcp_write Failed! LastError: %d, st = %d",__FUNCTION__,WSAGetLastError(),st);

		if(st == AIO_FAILED || st == AIO_TIMEOUT)
		{
			if(DataInfo)
			{
				if(DataInfo->SendStatus == SEND_OK || DataInfo->SendStatus == SEND_DATA)
				{
					DataInfo->CanWrite = true;

					HttpLive *Live = (HttpLive*)ctx1;

					Live->m_ListSocket.erase(id);

				}
				else if(DataInfo->SendStatus == SEND_NOFOUND)
				{
					HttpLive::TcpControl->close_connect(id);

					if(DataInfo->buf)
						delete [] DataInfo->buf;

					delete [] DataInfo;

				}
			}

			if(st == AIO_FAILED)
				HttpLive::TcpControl->close_connect(id);
		}
	}

	
}

void CTCPSocketControl::on_tcp_read( AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2 )
{
	if(st == AIO_SUC)
	{
		if(ctx2 == 1000)
			HttpLive::DoAcceptProcess(id,buf,retlen);
	}
	else
	{
		Log::writeMessage(562949953421312,1," %s on_tcp_read Failed! LastError: %d",__FUNCTION__,WSAGetLastError());
		HttpLive::TcpControl->close_connect(id);
	}

	delete [] buf;
}
