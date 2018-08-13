#include "BaseAfx.h"
#include "IRealAgent.h"
#include "Requse.h"
#include "Respond.h"
#include <windows.h>
IRealAgent *EXE = NULL;
IRealAgent::~IRealAgent()
{
}

uint64_t GetSysTickCount64()
{
	static LARGE_INTEGER clockFre = { 0 };

	if (!clockFre.QuadPart)
	{
		QueryPerformanceFrequency(&clockFre);
	}

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	uint64_t timeVal = currentTime.QuadPart;
	timeVal *= 1000;
	timeVal /= clockFre.QuadPart;

	return timeVal;
}

void IRealAgent::ExecuseInvoke(CRequse &Req,CRespond &Res)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!",__FUNCTION__);
    BaseProcess *BP = NULL;

	uint64_t StartTime = GetSysTickCount64();

    try
    {
		BP = m_ListProcess[Req.GetProcessClassName()];
		if (!BP)
		{
			BP = CREATEOBJECT(Req.GetProcessClassName());
			if (BP)
			{
				m_ListProcess[Req.GetProcessClassName()] = BP;
			}
		}

		if (BP)
		{
			BP->Excuse(Req, Res);
		}
		else
		{
			BUTEL_THORWERROR("Can't Find the Prcoss Class %s", Req.GetProcessClassName());
		}
    }
    catch(CErrorBase &Error)
    {
       DEBUG_PRINT( Error.m_Error.c_str());

		Json::Value jVaule;
		jVaule["rc"] = -1;
		jVaule["msg"] = Error.m_Error.c_str();
		Res.SetRespond(jVaule);
    }
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end 函数 %s 调用用时 %llu ms!", __FUNCTION__,Req.GetCmd(),GetSysTickCount64() - StartTime);
}

void IRealAgent::DeleteClass(const char *ClassName)
{
	m_ListProcess.earse(ClassName);
}

