#include "HttpSocketControl.h"
#include "CHttpNetWork.h"
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
		__PDATAINFO Data;
		CHttpNetWork::GetInstance()->m_DataQueue.try_pop(Data);
		if (Data && Data->CanWrite)
		{
			Data->CanWrite = false;
			CHttpNetWork::GetInstance()->m_DataQueue.push(Data);
			CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)Data->buf, Data->len, (ULL64)Data, 0, false);
		}
		else
		{
			if (Data)
				CHttpNetWork::GetInstance()->m_DataQueue.push(Data);

			Log::writeError(((long long)1 << 49), 1, "当时队列不可写 %s %d", __FUNCTION__, __LINE__);
		}
	}
	else
	{
		CHttpNetWork::GetInstance()->TcpControl->close_connect(id);
	}
}

void CTCPSocketControl::on_tcp_connect(AsynIoErr st, AIOID id, const AioAddrPair &addr, ULL64 ctx1, ULL64 ctx2)
{
	__PDATAINFO DataInfo = (__PDATAINFO)ctx2;
	CHttpNetWork *NetWork = (CHttpNetWork*)ctx1;

	if (st == AIO_SUC)
	{
		NetWork->TcpControl->asyn_write(id, (char*)DataInfo->buf, strlen((char*)DataInfo->buf), (ULL64)NetWork, (ULL64)DataInfo);

		if (addr.m_raddr.m_ip == ntohl(inet_addr(NetWork->ServerIp.c_str())))
		{
			NetWork->CenterManagerId = id;
		}
	}
	else
	{
		NetWork->ResetDataBuf(DataInfo);
		Log::writeMessage(562949953421312, 1, " %s on_tcp_connect Failed! LastError: %d, st = %d", __FUNCTION__, WSAGetLastError(), st);
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
				CHttpNetWork *NetWork = (CHttpNetWork*)ctx1;
				CHttpNetWork::GetInstance()->ResetDataBuf(DataInfo);
				if (!NetWork->DoAccept(id))
				{
					NetWork->TcpControl->close_connect(id);
				}

				//在投递一次
				__PDATAINFO Data;
				CHttpNetWork::GetInstance()->m_DataQueue.try_pop(Data);
				if (Data && Data->CanWrite)
				{
					Data->CanWrite = false;
					CHttpNetWork::GetInstance()->m_DataQueue.push(Data);
					CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)Data->buf, Data->len, (ULL64)Data, 0, false);
				}
				else
				{
					if (Data)
						CHttpNetWork::GetInstance()->m_DataQueue.push(Data);

					Log::writeError(((long long)1 << 49), 1, "当时队列不可写 %s %d", __FUNCTION__, __LINE__);
				}
			}
			else if(DataInfo->SendStatus == SEND_DATA)
			{
				CHttpNetWork::GetInstance()->ResetDataBuf(DataInfo);

				//在投递一次

				//if (id != CHttpNetWork::GetInstance()->CenterManagerId)
				{
					__PDATAINFO Data;
					CHttpNetWork::GetInstance()->m_DataQueue.try_pop(Data);
					if (Data && Data->CanWrite)
					{
						Data->CanWrite = false;
						CHttpNetWork::GetInstance()->m_DataQueue.push(Data);
						CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)Data->buf, Data->len, (ULL64)Data, 0, false);
					}
					else
					{
						if (Data)
							CHttpNetWork::GetInstance()->m_DataQueue.push(Data);

						Log::writeError(((long long)1 << 49), 1, "当时队列不可写 %s %d", __FUNCTION__, __LINE__);
					}
				}

			}
			else if(DataInfo->SendStatus == SEND_NOFOUND)
			{
				CHttpNetWork::GetInstance()->TcpControl->close_connect(id);

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
					CHttpNetWork *NetWork = (CHttpNetWork*)ctx1;
					NetWork->ResetDataBuf(DataInfo);
					NetWork->m_ListSocket.erase(id);

					if (id == NetWork->CenterManagerId)
					{
						NetWork->CenterManagerId = -1;
					}

				}
				else if(DataInfo->SendStatus == SEND_NOFOUND)
				{
					if(DataInfo->buf)
						delete [] DataInfo->buf;

					delete [] DataInfo;

				}
			}

			CHttpNetWork::GetInstance()->TcpControl->close_connect(id);
		}
	}

	
}

void CTCPSocketControl::on_tcp_read( AsynIoErr st, AIOID id, const AioAddrPair &addr, char *buf, UI32 buflen, UI32 retlen, ULL64 ctx1, ULL64 ctx2 )
{
	__PDATAINFO DataInfo = (__PDATAINFO)ctx1;
	if(st == AIO_SUC)
	{
		const char *pFind = strstr((char*)DataInfo->buf, "\r\n\r\n");

		if (strncmp((char*)DataInfo->buf, "GET", 3) == 0 || strncmp((char*)DataInfo->buf, "OPTIONS", 7) == 0)
		{
			if (pFind)
			{
				DataInfo->BufRealLen += retlen;
				CHttpNetWork::GetInstance()->DoAcceptProcess(id, (char*)DataInfo->buf, DataInfo->BufRealLen, ctx1);
			}
			else
			{
				DataInfo->BufRealLen += retlen;
				CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)DataInfo->buf + DataInfo->BufRealLen, DataInfo->len - DataInfo->BufRealLen, (ULL64)DataInfo, 0, false);
			}

			
		}
		else if (strncmp((char*)DataInfo->buf, "POST", 4) == 0)
		{
			if (pFind)
			{
				DataInfo->BufRealLen += retlen;

				char *pJsonLen = strstr((char*)DataInfo->buf, "Content-Length");
				if (pJsonLen)
				{
					char Len[10] = { 0 };
					int i = 0;
					for (pJsonLen += strlen("Content-Length:"); *pJsonLen != '\r'; pJsonLen++)
					{
						if (*pJsonLen != ':' && *pJsonLen != ' ')
						{
							Len[i++] = *pJsonLen;
						}
					}

					int TotalLen = pFind - (char*)DataInfo->buf + 4 + atoi(Len);

					if (TotalLen == DataInfo->BufRealLen)
					{
						CHttpNetWork::GetInstance()->DoAcceptProcess(id, (char*)DataInfo->buf, DataInfo->BufRealLen, ctx1);
					}
					else
					{
						CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)DataInfo->buf + DataInfo->BufRealLen, DataInfo->len - DataInfo->BufRealLen, (ULL64)DataInfo, 0, false);
					}
				}
				else
				{
					CHttpNetWork::GetInstance()->WaitAndCloseSocket(id);
				}
			}
			else
			{
				DataInfo->BufRealLen += retlen;
				CHttpNetWork::GetInstance()->TcpControl->asyn_read(id, (char*)DataInfo->buf + DataInfo->BufRealLen, DataInfo->len - DataInfo->BufRealLen, (ULL64)DataInfo, 0, false);
			}
		}
		else
		{
			DataInfo->buf[retlen] = 0;
			CHttpNetWork::GetInstance()->ResetDataBuf(DataInfo);
		}
	}
	else
	{
		CHttpNetWork::GetInstance()->m_ListSocket.erase(id);
		CHttpNetWork::GetInstance()->ResetDataBuf(DataInfo);
		Log::writeMessage(562949953421312,1," %s on_tcp_read Failed! LastError: %d,st = %d",__FUNCTION__,WSAGetLastError(),st);
		CHttpNetWork::GetInstance()->TcpControl->close_connect(id);

		if (id == CHttpNetWork::GetInstance()->CenterManagerId)
		{
			CHttpNetWork::GetInstance()->CenterManagerId = -1;
		}
	}
}

