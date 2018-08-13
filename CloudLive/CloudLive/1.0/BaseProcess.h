#if !defined(BASEPROCESS_H)
#define BASEPROCESS_H
enum ExeResult {
    RES_OK = 0,
    RES_FAILED = 0xFFFF
};
#include "Respond.h"
#include "Requse.h"

class BaseProcess :public COPject
{
public:
    BaseProcess();
	virtual ~BaseProcess();
    virtual void Excuse(CRequse &Req,CRespond &Res) = 0;

};

#endif
