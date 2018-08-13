#if !defined(REQUSE_H)
#define REQUSE_H
#include "OPject.h"
#include "PArry.h"
class CRequse : public COPject
{
public:
	CRequse();
    ~CRequse();
	const char *GainClassName();
    void SetCmd(const char *CMD);
	void SetParam(const char* cParam);
    void SetProcessClassName(const char *ProcessClass);
    const char* GetProcessClassName()const;
    const char* GetCmd();
	const char* GetParam() const;
private:
    const char* m_Cmd;
    const char* m_ProcessClass;
	const char* m_Param;
};

#endif
