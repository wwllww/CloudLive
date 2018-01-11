#if !defined(EXCEPTION_H)
#define EXCEPTION_H
#include "ErrorBase.h"
#include <string>
#pragma warning(disable:4100)
class API_EXPORT CException : public CErrorBase  
{
public:
	CException(const char *,int,const char*);
	virtual ~CException();

    void ThrowErrorMsg(const char* Fmt, ...);
    CErrorBase m_BaseThow;
private:
    std::string esPath;
	std::string Function;
	int LineNum;

};

#endif
