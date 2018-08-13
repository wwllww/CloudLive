#include "BaseAfx.h"
#include "ErrorBase.h"

CErrorBase::CErrorBase()
{

}

CErrorBase::CErrorBase(CErrorBase&Err)
{
    m_Error = Err.m_Error;
}


CErrorBase::~CErrorBase()
{

}

