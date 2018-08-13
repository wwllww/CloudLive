#include "BaseAfx.h"
#include "Exception.h"
#include <stdarg.h>
#include <stdio.h>

CException::CException(const char * File,int Line)
{
    esPath = File;
	LineNum = Line;
}

CException::~CException()
{

}

void CException::ThrowErrorMsg(const char* Fmt, ...)
{
    va_list var;
    char Msg[4096] = {0};
    va_start(var,Fmt);
    vsnprintf_s(Msg,sizeof Msg,Fmt,var);
    va_end(var);
    char Line[5120] = {0};
	sprintf_s(Line, sizeof Line, "%s %d ERR: %s", esPath.c_str(), LineNum, Msg);
    m_BaseThow.m_Error = Line;
	
    throw m_BaseThow;
	
}
