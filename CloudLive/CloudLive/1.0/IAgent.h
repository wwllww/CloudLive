#if !defined(ILIB_H)
#define ILIB_H

class CRequse;
class CRespond;
class CUIManager;
class IAgent
{
public:
    IAgent();
    virtual ~IAgent();
    virtual void SetName(const char *) = 0;
    virtual void ExecuseInvoke(CRequse &Req,CRespond &Res) = 0;
	virtual void DeleteClass(const char *ClassName){};
};

#endif
