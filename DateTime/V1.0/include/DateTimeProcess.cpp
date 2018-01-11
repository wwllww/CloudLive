// DateTime.cpp: implementation of the CDateTime class.
//
//////////////////////////////////////////////////////////////////////

#include "DateTimeProcess.h"
#include <STDARG.H>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDateTime::CDateTime():m_csTime(""),
					   m_iYear(0),
                       m_iMonth(0),
					   m_iDay(0),
                       m_iHour(0),
                       m_iMin(0),
                       m_iSec(0),bIsHour_12(false)
{

}

CDateTime::~CDateTime()
{

}

CDateTime::CDateTime(const char *cTime, const char *cFormat)
{
	if(!AnalyseTime(cTime))
	{
		m_iYear = 0;
		m_iMonth = 0;
		m_iDay = 0;
		m_iHour = 0;
		m_iMin = 0;
		m_iSec = 0;
		m_csTime = "";
		bIsHour_12 = false;
	}
	else
	{
		AnalyseFormat(cFormat);
	}


}

void CDateTime::Format(const char *cFormat, ...)
{
	va_list ap;
	char cTFormat[1024] = {0};
	va_start(ap,cFormat);
	vsprintf(cTFormat,cFormat,ap);
	va_end(ap);

	m_csTime = cTFormat;
	
	if(bIsHour_12)
	{
		m_csTime += m_iHour > 12 ? " pm":" am";
	}

}

bool CDateTime::AnalyseTime(const char *cTime)
{
	if(!cTime || 0 == strlen(cTime))
			return false;

	char cYear[5] = {0};
	char cMonth[3] = {0};
	char cDay[3] = {0};
	char cHour[3] = {0};
	char cMin[3] = {0};
	char cSec[3] = {0};

	char *pTime = (char*) cTime;
	int iSwith = 0;
	int iLen = 0;
	bool bHasAdd = false;
	while('\0' != *pTime)
	{
		if(0x20 == *pTime && (*(pTime - 1) < '0' || *(pTime - 1) > '9') && 0x20 != *(pTime -1))
		{
			bHasAdd = true;
		}

		if(0x20 == *pTime)
		{	
			iLen = 0;
			if(*(pTime + 1) >= '0' && *(pTime + 1) <= '9')
			{
				if(!bHasAdd)
				{
					++iSwith;
				}
			}
			pTime ++;
			continue;
		}
		if(*pTime >= '0' && *pTime <= '9')
		{	
			switch(iSwith)
			{
			case 0:
				memcpy(cYear + iLen,pTime,sizeof(char));
				break;
			case 1:
				memcpy(cMonth + iLen,pTime,sizeof(char));
				break;
			case 2:
				memcpy(cDay + iLen,pTime,sizeof(char));
				break;
			case 3:
				memcpy(cHour + iLen,pTime,sizeof(char));
				break;
			case 4:
				memcpy(cMin + iLen,pTime,sizeof(char));
				break;
			case 5:
				memcpy(cSec + iLen,pTime,sizeof(char));
				break;
			default:
				break;
			}

			bHasAdd = false;
		}

		

		iLen ++;
		if(*pTime < '0' || *pTime > '9')
		{

			++iSwith;
			iLen = 0;
		}
		
		pTime++;
	}

	m_iYear = atoi(cYear);
	m_iMonth = atoi(cMonth);
	m_iDay = atoi(cDay);
	m_iHour = atoi(cHour);
	m_iMin = atoi(cMin);
	m_iSec = atoi(cSec);

	return true;
}


void CDateTime::AnalyseFormat(const char *cFormat)
{
	
	if(!cFormat || 0 == strlen(cFormat))
		return;

	char cYear[10] = {0};
	char cMonth[10] = {0};
	char cDay[10] = {0};
	char cHour_24[10] = {0};
	char cHour_12[10] = {0};
	char cMin[10] = {0};
	char cSec[10] = {0};

	char *pFormat = (char *)cFormat;

	char *pFirst = NULL;
	char *pSec = NULL;
	char *pThird = NULL;
	char *pFour = NULL;
	char *pFive = NULL;
	char *pSix = NULL;

	char cFirstSymbol[5] = {0};
	char cSecSymbol[5] = {0};
	char cThirdSymbol[5] = {0};
	char cFourSymbol[5] = {0};
	char cFiveSymbol[5] = {0};

	
	int iFirst = 0;
	int iSec = 0;
	int iThird = 0;
	int iFour = 0;
	int iFive = 0;
	int iSix = 0;

	bool bHasInvoke[10] = {false};
	int iPerPos[10] = {0};
	int iPos = 0;
	int iSymbol = 0;
	bool bFirstInvoke = true;
	while('\0' != *pFormat)
	{
		switch(*pFormat)
		{
		case 'y':
		case 'Y'://年份
			{
				if(0 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pFirst = cYear;
						iFirst = m_iYear;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pSec = cYear;
						iSec = m_iYear;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pThird = cYear;
						iThird = m_iYear;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pFour = cYear;
						iFour = m_iYear;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pFive = cYear;
						iFive = m_iYear;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[0])
					{
						pSix = cYear;
						iSix = m_iYear;
					}
				}
				if(!bHasInvoke[0])
				{
					iPos ++;
					bHasInvoke[0] = true;
				}
				memcpy(cYear + iPerPos[0],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[0] ++;
			}

			break;
		case 'm'://分钟
			{
			
				if(0 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pFirst = cMin;
						iFirst = m_iMin;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pSec = cMin;
						iSec = m_iMin;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pThird = cMin;
						iThird = m_iMin;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pFour = cMin;
						iFour = m_iMin;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pFive = cMin;
						iFive = m_iMin;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[1])
					{
						pSix = cMin;
						iSix = m_iMin;
					}
				}
				if(!bHasInvoke[1])
				{
					iPos ++;
					bHasInvoke[1] = true;
				}
				memcpy(cMin + iPerPos[1],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[1] ++;

			}
			break;
		case 'M'://月份
			{
				if(0 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pFirst = cMonth;
						iFirst = m_iMonth;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pSec = cMonth;
						iSec = m_iMonth;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pThird = cMonth;
						iThird = m_iMonth;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pFour = cMonth;
						iFour = m_iMonth;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pFive = cMonth;
						iFive = m_iMonth;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[2])
					{
						pSix = cMonth;
						iSix = m_iMonth;
					}
				}
				if(!bHasInvoke[2])
					{
						iPos ++;
						bHasInvoke[2] = true;
					}
				memcpy(cMonth + iPerPos[2],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[2] ++;

			}
			break;
		case 'd':
		case 'D'://天数
			{
				if(0 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pFirst = cDay;
						iFirst = m_iDay;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pSec = cDay;
						iSec = m_iDay;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pThird = cDay;
						iThird = m_iDay;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pFour = cDay;
						iFour = m_iDay;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pFive = cDay;
						iFive = m_iDay;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[3])
					{
						pSix = cDay;
						iSix = m_iDay;
					}
				}
				if(!bHasInvoke[3])
				{
					iPos ++;
					bHasInvoke[3] = true;
				}
				memcpy(cDay + iPerPos[3],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[3] ++;
			}
			break;
		case 'h'://12小时制
			{
				bIsHour_12 = true;
				if(0 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pFirst = cHour_12;
						iFirst = m_iHour;
						if(iFirst > 12)
							iFirst -= 12;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pSec = cHour_12;
						iSec = m_iHour;
						if(iSec > 12)
							iSec -= 12;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pThird = cHour_12;
						iThird = m_iHour;
						if(iThird > 12)
							iThird -= 12;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pFour = cHour_12;
						iFour = m_iHour;
						if(iFour > 12)
							iFour -= 12;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pFive = cHour_12;
						iFive = m_iHour;
						if(iFive > 12)
							iFive -= 12;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[4])
					{
						pSix = cHour_12;
						iSix = m_iHour;
						if(iSix > 12)
							iSix -= 12;
					}
				}
				if(!bHasInvoke[4])
				{
					iPos ++;
					bHasInvoke[4] = true;
				}
				memcpy(cHour_12 + iPerPos[4],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[4] ++;
			}
			break;
		case 'H'://24小时制
			{
				bIsHour_12 = false;
				if(0 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pFirst = cHour_24;
						iFirst = m_iHour;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pSec = cHour_24;
						iSec = m_iHour;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pThird = cHour_24;
						iThird = m_iHour;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pFour = cHour_24;
						iFour = m_iHour;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pFive = cHour_24;
						iFive = m_iHour;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[5])
					{
						pSix = cHour_24;
						iSix = m_iHour;
					}
				}
				if(!bHasInvoke[5])
				{
					iPos ++;
					bHasInvoke[5] = true;
				}
				memcpy(cHour_24 + iPerPos[5],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[5] ++;
			}
			break;
		case 's'://秒数
		case 'S':
			{
				static int i = 0;
				if(0 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pFirst = cSec;
						iFirst = m_iSec;
					}
				}
				else if(1 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pSec = cSec;
						iSec = m_iSec;
					}
				}
				else if(2 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pThird = cSec;
						iThird = m_iSec;
					}
				}
				else if(3 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pFour = cSec;
						iFour = m_iSec;
					}
				}
				else if(4 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pFive = cSec;
						iFive = m_iSec;
					}
				}
				else if(5 == iPos)
				{
					if(!bHasInvoke[6])
					{
						pSix = cSec;
						iSix = m_iSec;
					}
				}
				if(!bHasInvoke[6])
				{
					iPos ++;
					bHasInvoke[6] = true;
				}
				memcpy(cSec + iPerPos[6],pFormat,sizeof(char) );
				pFormat ++;
				iPerPos[6] ++;
			}
			break;
		default:
			{
				
				bool IsFind = false;
				while(!bFirstInvoke && 0x20 == *pFormat)
				{
					pFormat ++;
					IsFind = true;
				}

				if(IsFind)
					break;

				if(0 == iSymbol)
				{
					memcpy(cFirstSymbol,pFormat,sizeof(char));
				}
				else if(1 == iSymbol)
				{
					memcpy(cSecSymbol,pFormat,sizeof(char));
				}
				else if(2 == iSymbol)
				{
					memcpy(cThirdSymbol,pFormat,sizeof(char));
				}
				else if(3 == iSymbol)
				{
					memcpy(cFourSymbol,pFormat,sizeof(char));
				}
				else if(4 == iSymbol)
				{
					memcpy(cFiveSymbol,pFormat,sizeof(char));
	
				}
				if(0x20 == *pFormat)
					bFirstInvoke = false;

				iSymbol ++;
				pFormat ++;
			}
			break;
		}
	}

	string sFormat;
	if(pFirst && 4 == strlen(pFirst))
	{
		sFormat = "%04d";
	}
	else if(pFirst && 2 == strlen(pFirst))
	{
		sFormat = "%02d";
	}
	else if(pFirst && (1 == strlen(pFirst)|| *pFirst == 'y' || *pFirst == 'Y'))
	{
		sFormat = "%d";
	}

	if (pSec)
		sFormat += cFirstSymbol;

	if(pSec && 4 == strlen(pSec))
	{
		sFormat += "%04d";
	}
	else if(pSec && 2 == strlen(pSec))
	{
		sFormat += "%02d";
	}
	else if(pSec && (1 == strlen(pSec)|| *pSec == 'y' || *pSec == 'Y'))
	{
		sFormat += "%d";
	}

	if (pThird)
		sFormat += cSecSymbol;

	if(pThird && 4 == strlen(pThird))
	{
		sFormat += "%04d";
	}
	else if(pThird && 2 == strlen(pThird))
	{
		sFormat += "%02d";
	}
	else if(pThird && (1 == strlen(pThird)|| *pThird == 'y' || *pThird == 'Y'))
	{
		sFormat += "%d";
	}

	if (pFour)
		sFormat += cThirdSymbol;

	if(pFour && 4 == strlen(pFour))
	{
		sFormat += "%04d";
	}
	else if(pFour && 2 == strlen(pFour))
	{
		sFormat += "%02d";
	}
	else if(pFour && (1 == strlen(pFour)|| *pFour == 'y' || *pFour == 'Y'))
	{
		sFormat += "%d";
	}

	if (pFive)
		sFormat += cFourSymbol;

	if(pFive && 4 == strlen(pFive))
	{
		sFormat += "%04d";
	}
	else if(pFive && 2 == strlen(pFive))
	{
		sFormat += "%02d";
	}
	else if(pFive && (1 == strlen(pFive)|| *pFive == 'y' || *pFive == 'Y'))
	{
		sFormat += "%d";
	}

	if (pSix)
		sFormat += cFiveSymbol;
	
	if(pSix && 4 == strlen(pSix))
	{
		sFormat += "%04d";
	}
	else if(pSix && 2 == strlen(pSix))
	{
		sFormat += "%02d";
	}
	else if(pSix && (1 == strlen(pSix)|| *pSix == 'y' || *pSix == 'Y'))
	{
		sFormat += "%d";
	}

	Format(sFormat.c_str(),iFirst,iSec,iThird,iFour,iFive,iSix);

}

void CDateTime::LoadFormatTime(const char *cTime,const char *cFormat)
{
	if(!cTime || 0 == strlen (cTime))
	{
		if(!m_csTime.empty())
		{
			if(!AnalyseTime(m_csTime.c_str()))
			{
				m_iYear = 0;
				m_iMonth = 0;
				m_iDay = 0;
				m_iHour = 0;
				m_iMin = 0;
				m_iSec = 0;
				m_csTime = "";
			}
			else
			{
				AnalyseFormat(cFormat);
			}
		}
	}
	else
	{

		if(!AnalyseTime(cTime))
		{
			m_iYear = 0;
			m_iMonth = 0;
			m_iDay = 0;
			m_iHour = 0;
			m_iMin = 0;
			m_iSec = 0;
			m_csTime = "";
		}
		else
		{
			AnalyseFormat(cFormat);
		}
	}
}