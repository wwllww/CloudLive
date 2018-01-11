#if !defined(ERRORBASE_H)
#define ERRORBASE_H
#include <string>
#pragma warning(disable:4100)
class API_EXPORT CErrorBase
{
public:
	CErrorBase();
	CErrorBase(CErrorBase&Err);
	virtual ~CErrorBase();

    std::string m_Error;
	

};

#endif
