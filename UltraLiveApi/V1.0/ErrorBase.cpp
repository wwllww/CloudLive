#include "BaseAfx.h"
#include "ErrorBase.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

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

