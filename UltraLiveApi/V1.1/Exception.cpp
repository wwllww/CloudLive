#include "BaseAfx.h"
#include "Exception.h"
#include <stdarg.h>
#include <stdio.h>
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


CException::CException(const char * File,int Line,const char* Fun)
{
    esPath = File;
	LineNum = Line;
	Function = Fun;
}

CException::~CException()
{

}

void CException::ThrowErrorMsg(const char* Fmt, ...)
{
    va_list var;
    char Msg[4096] = {0};
    va_start(var,Fmt);
	vsnprintf(Msg, sizeof Msg, Fmt, var);
    va_end(var);
	char Line[4096] = { 0 };
	_snprintf(Line,sizeof Line, "LiveSDK_Log:%s %d ERR: %s %s", esPath.c_str(), LineNum, Function.c_str(), Msg);
    m_BaseThow.m_Error = Line;
	Log::writeError(LOG_RTSPSERV, 1, Line);
    throw m_BaseThow;
	
}
