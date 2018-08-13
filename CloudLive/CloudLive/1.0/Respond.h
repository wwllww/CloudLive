#if !defined(RESPON_H)
#define RESPON_H
#include "OPject.h"
#include "PArry.h"
#include "BaseProcess.h"


class CRespond : public COPject
{
public:
	CRespond();
    ~CRespond();
	const char *GainClassName();
    void SetResType(ExeResult Type);
    ExeResult GetResType() const;
    void SetErrCode(int ErrCode);
    int GetErrCode() const;
	void SetRespond(const Json::Value &Res);
	Json::Value GetRespond() const;
private:
    ExeResult  m_res;
    int m_ErrCode;
	Json::Value jRespond;
};

#endif
