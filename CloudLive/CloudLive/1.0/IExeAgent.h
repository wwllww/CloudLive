#if !defined(IEXELIB_H)
#define IEXELIB_H
#include "IAgent.h"

IAgent *QueryInterface();
class IExeAgent : public IAgent
{
public:
    IExeAgent();
    virtual ~IExeAgent();
    virtual void SetName(const char *Name);
    static  IAgent * G_LibManage;
protected:
    const char* m_Name;

};
#endif
