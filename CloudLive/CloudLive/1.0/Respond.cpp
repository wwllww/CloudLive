#include "BaseAfx.h"
#include "Respond.h"

CRespond::CRespond():m_res(RES_FAILED),m_ErrCode(0)
{
}

CRespond::~CRespond()
{

}

const char *CRespond::GainClassName()
{
	return "CRespond";
}

void CRespond::SetResType(ExeResult Type)
{
    m_res = Type;
}

ExeResult CRespond::GetResType() const
{
    return m_res;
}
void CRespond::SetErrCode(int ErrCode)
{
    m_ErrCode = ErrCode;
}

int CRespond::GetErrCode() const
{
    return m_ErrCode;
}

void CRespond::SetRespond(const Json::Value &Res)
{
	jRespond = Res;
}

Json::Value CRespond::GetRespond() const
{
	return jRespond;
}
