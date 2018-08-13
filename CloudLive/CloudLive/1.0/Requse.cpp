#include "Requse.h"

CRequse::CRequse():m_Cmd(NULL),m_ProcessClass(NULL)
{
}

CRequse::~CRequse()
{

}
const char *CRequse::GainClassName()
{
	return "CRequse";
}

void CRequse::SetCmd(const char *CMD)
{
    m_Cmd = CMD;
}

const char *CRequse::GetCmd()
{
    return m_Cmd;
}

void CRequse::SetProcessClassName(const char *ProcessClass)
{
    m_ProcessClass = ProcessClass;
}

const char *CRequse::GetProcessClassName() const
{
    return m_ProcessClass;
}

void CRequse::SetParam(const char* cParam)
{
	m_Param = cParam;
}

const char* CRequse::GetParam() const
{
	return m_Param;
}
