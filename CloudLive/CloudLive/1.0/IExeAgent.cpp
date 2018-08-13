#include "BaseAfx.h"
#include "IExeAgent.h"
#include <string>
#ifndef NULL
#define NULL 0
#endif
IAgent * IExeAgent::G_LibManage = NULL;

IAgent * QueryInterface()
{
    return IExeAgent::G_LibManage;
}


IExeAgent::IExeAgent()
{
    G_LibManage = this;
}

IExeAgent::~IExeAgent()
{

}

void IExeAgent::SetName(const char *Name)
{
    m_Name = Name;
}

