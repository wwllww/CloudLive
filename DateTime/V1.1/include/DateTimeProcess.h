#ifndef DATETIMEPROESS_H
#define DATETIMEPROESS_H
#include <STRING>
using namespace std;
class CDateTime  
{
public:
	CDateTime();
	CDateTime(const char *cTime,const char *cFormat);
	virtual ~CDateTime();
	bool AnalyseTime(const char *cTime);
	void AnalyseFormat(const char *cFormat);
	void Format(const char *cFormat, ...);
	int GetYear()const {return m_iYear;}
	int GetMonth()const {return m_iMonth;}
	int GetDay() const {return m_iDay;}
	int GetHour() const {return m_iHour;}
	int GetMin()const {return m_iMin;}
	int GetSec()const {return m_iSec;}
	string GetTimeString()const { return m_csTime; }
	void LoadFormatTime(const char *cTime,const char *cFormat);
	inline CDateTime& operator = (const char* cTime)
	{
		m_csTime = cTime;
		return *this;
	}
private:
	string m_csTime;
	int m_iYear;
	int m_iMonth;
	int m_iDay;
	int m_iHour;
	int m_iMin;
	int m_iSec;
	bool bIsHour_12;
};
#endif // !DATETIMEPROESS_H
