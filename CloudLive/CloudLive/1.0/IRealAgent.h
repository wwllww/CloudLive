#if !defined(IREALIB_H)
#define IREALIB_H
#include "IExeAgent.h"
#include "BMap.h"
class CRequse;
class CRespond;
class BaseProcess;
class IRealAgent : public IExeAgent
{
public:
    IRealAgent(){}
    ~IRealAgent();
    void ExecuseInvoke(CRequse &Req,CRespond &Res);
	void DeleteClass(const char *ClassName);
private:
    CBMap<BaseProcess*> m_ListProcess = 1;
};

#endif
