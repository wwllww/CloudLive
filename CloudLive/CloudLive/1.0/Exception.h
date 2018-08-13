#if !defined(EXCEPTION_H)
#define EXCEPTION_H
#include "ErrorBase.h"
#include <string>
#pragma warning(disable:4100)
class CException : public CErrorBase  
{
public:
	CException(const char *,int);
	virtual ~CException();

    void ThrowErrorMsg(const char* Fmt, ...);
    CErrorBase m_BaseThow;
private:
    std::string esPath;
	int LineNum;

};

#endif
