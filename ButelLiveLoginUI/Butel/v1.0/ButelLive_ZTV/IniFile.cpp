// TTIniFile_Imp.cpp: implementation of the TTIniFile_Imp class.
// version: 2009-11-6 1.1
//////////////////////////////////////////////////////////////////////
#ifdef DEBUG
#include "comLib/IniFile.h"
#else
#include "IniFile.h"
#endif

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

#pragma warning(disable:4996)

#ifdef _ANDROID_
#include <stdlib.h>
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
namespace BUTELCONNECT
{

IniFile::IniFile()
{

}

IniFile::~IniFile()
{
    clear();
}

int IniFile::GetInteger(const WCHAR* szSection, const WCHAR* szKey, int nDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    TSection* p = getSection(szSection);
    if(!p)
        return nDefault;
    const WCHAR* szValue = getValue(p, szKey);
    if(!szValue)
        return nDefault;
    else if ('0' == szValue[0]
        &&('x'==szValue[1]||'X'==szValue[1]))
    {
        int nRet=0;
        wscanf(szValue,L"%x",&nRet);
        return nRet;
    }
    return _wtoi(szValue);
}

std::wstring IniFile::GetString(const WCHAR* szSection, const WCHAR* szKey, const WCHAR* szDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    TSection* p = getSection(szSection);
    if(!p)
        return szDefault;
    const WCHAR* szValue = getValue(p, szKey);
    if(!szValue)
        return szDefault;
    return szValue;
}

std::wstring IniFile::GetStringLowercase(const WCHAR* szSection, const WCHAR* szKey, const WCHAR* szDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    TSection* p = getSection(szSection);
    if(!p)
        return szDefault;
    const WCHAR* szValue = getValue(p, szKey);
    if(!szValue)
        return szDefault;
    std::wstring str;
    while(*szValue)
    {
        str += getLowercase(*szValue);
        ++szValue;
    }
    return str;
}

bool IniFile::GetBool(const WCHAR* szSection, const WCHAR* szKey, bool bDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::wstring str = GetStringLowercase(szSection, szKey, bDefault?L"yes":L"no");
    return L"yes"==str || L"true"==str;
}

int	IniFile::GetIntegerEx(const WCHAR* szSection, const WCHAR* keyPrefix, int index, int nDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    return GetInteger(szSection, makeSec(keyPrefix, index).c_str(), nDefault);
}

std::wstring IniFile::GetStringEx(const WCHAR* szSection, const WCHAR* keyPrefix, int index, const TCHAR* szDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    return GetString(szSection, makeSec(keyPrefix, index).c_str(), szDefault);
}

std::wstring IniFile::GetStringLowercaseEx(const WCHAR* szSection, const WCHAR* keyPrefix, int index, const TCHAR* szDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    return GetStringLowercase(szSection, makeSec(keyPrefix, index).c_str(), szDefault);
}

bool IniFile::GetBoolEx(const WCHAR* szSection, const WCHAR* keyPrefix, int index, bool bDefault)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    return GetBool(szSection, makeSec(keyPrefix, index).c_str(), bDefault);
}

wstring IniFile::makeSec(const WCHAR* keyPrefix, int index) const
{
    std::wostringstream s;
    s<<keyPrefix<<index;
    return s.str();
}

inline void IniFile::clear()
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::map<std::wstring, TSection*>::iterator iter = m_Inifile.begin();
    for(; iter != m_Inifile.end(); ++iter)
    {
        TSection* p = iter->second;
        delete p;
    }
    m_Inifile.clear();
}

inline IniFile::TSection* IniFile::getSection(const WCHAR* szSection, bool is_need_create)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::wstring strSec;
    while(*szSection)
    {
        strSec += getLowercase(*szSection);
        szSection++;
    }

    std::map<std::wstring, TSection*>::iterator iter = m_Inifile.find(strSec);
    if(iter == m_Inifile.end())
    {
        if (!is_need_create)
            return 0;

        std::pair<std::map<std::wstring, TSection*>::iterator, bool> result = m_Inifile.insert(make_pair(strSec,new TSection));
        if (result.second)
            return result.first->second;
        else
            return 0;
    }
    else
    {
        return iter->second;
    }
}

inline const WCHAR* IniFile::getValue(const TSection* pSec, const WCHAR* szP)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::wstring strP;
    while(*szP)
    {
        strP += getLowercase(*szP);
        szP++;
    }

    TSection::const_iterator iter = pSec->find(strP);
    if(iter == pSec->end())
        return 0;
    const std::wstring& str = iter->second;
    return str.c_str();
}

inline WCHAR IniFile::getLowercase(WCHAR in)
{
    if(in>='A' && in<='Z')
        return in - 'A' + 'a';
    return in;
}

inline void IniFile::addASection(const std::wstring& strSection)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    //only add the first one
    if(m_Inifile.find(strSection) != m_Inifile.end())
        return;

    m_Inifile[strSection] = new TSection;
}

inline void IniFile::addAValue(const std::wstring& strSection, const std::wstring& strP, const std::wstring& strV)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::map<std::wstring, TSection*>::iterator iter =  m_Inifile.find(strSection);
    if(iter == m_Inifile.end())
        return;
    TSection* pSec = iter->second;

    //only do add action on the 1st time
    if(pSec->find(strP) != pSec->end())
        return;
    (*pSec)[strP] = strV;
//	std::cout<<strSection << " "<< strP<< "="<<strV<<"<"<<std::endl;
}

void CharToWchar(const char* c,wstring &RetStr)
{
    int len = MultiByteToWideChar(CP_ACP,0,c,strlen(c),NULL,0);
    WCHAR* m_wchar = new WCHAR[len+1];
    MultiByteToWideChar(CP_ACP,0,c,strlen(c),m_wchar,len);
    m_wchar[len]='\0';
    RetStr = m_wchar;
}

bool IniFile::LoadIniFile(const WCHAR* szFileName)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    clear();
    std::ifstream inif(szFileName);
//	if(!inif.good())	//AIX5.3�²���ʹ�ø��жϣ�Ҳ����ʹ��if(!inif)����������C++���п��汾����
    if(!inif.is_open())
        return false;
    m_strFilePath = szFileName;
    const int CFG_LINE_LEN = 1024;
    CHAR line[CFG_LINE_LEN];
    std::wstring strCurrSection;
    std::wstring strLine;
    while(!inif.eof())
    {
        inif.getline(line, CFG_LINE_LEN);
        CharToWchar(line,strLine);
        if(wcslen(strLine.c_str()) == 0)
            continue;
        if(!analyzeLine(strCurrSection, strLine.c_str()))
            break;
    }
    return true;
}

//special marks in a inifile:
//[ ] # ; ' " = space
inline bool IniFile::analyzeLine(std::wstring& strCurrSec, const WCHAR* szLine)
{
    const WCHAR* pEqMark = wcschr(szLine, L'=');
    if(!pEqMark)	//may be a section name
    {
        std::wstring strSec;
        bool bSection = false;
        for(int i=0; ;++i)
        {
            switch(szLine[i])
            {
            case '\t':
                break;				//spaces in section name will be skipped;
            case '[':
                if(bSection)		//like: "[x["
                    return true;
                bSection = true;
                break;
            case ']':
                if(strSec.length())
                {
                    strCurrSec = strSec;	//set a new current section
                    addASection(strSec);
                }
                return true;		//like: "]"
            //finish marks.
            case '#':
            case ';':
            case '\r':				//on linux \r is the last character in a line
            case 0:
                return true;
            default:
                if(bSection)
                    strSec += getLowercase(szLine[i]);
                else				//like: "a["
                    return true;
                break;
            }
        }
    }
    else
    {
        if(!strCurrSec.length())
            return true;

        const TCHAR* p = szLine;
        //get parameter 1st
        std::wstring strPara;
        while(p != pEqMark)
        {
            switch(*p)
            {
            //case ' ':
            case '\t':
                break;
            case '#':
            case ';':
            case '\r':
            case 0:
                return true;
            //invalid characters.
            case '[':
            case ']':
            case '\"':
            case '\'':
                return true;
            default:
                {
                    if (strCurrSec == L"domain_isp_map")
                        strPara += (*p);
                    else
                        strPara += getLowercase(*p);
                }
            }
            ++p;
        }
        if(!strPara.length())
            return true;

        ++p;	//skip the '=' mark
        std::wstring strValue;
        bool bInBarcket = true;	//is there a ' or a "?
        for(;;)
        {
            switch(*p)
            {
            case ' ':
                if(bInBarcket)
                    strValue += *p;
                break;
            case '\'':
            case '\"':
                if(bInBarcket)		//finished
                {
                    addAValue(strCurrSec, strPara, strValue);
                    return true;
                }
                else
                    bInBarcket = true;
                break;
            case '#':
            case ';':
            case '\r':
            case 0:
                addAValue(strCurrSec, strPara, strValue);
                return true;
            default:
                strValue += *p;
            }
            ++p;
        }
        return true;
    }
}

void IniFile::flush_to_file(const std::string& strFileName)
{
// 	BUTELCONNECT::SingleMutex s(m_mutex);
//
// 	int nKeyLength = 21;
// 	std::string strEndOfLine = "\n";
//
// 	std::string strFlushFileName = strFileName;
// 	if (strFlushFileName.empty())
// 	{
// 		strFlushFileName = "Host.ini";
// 	}
//
// 	FILE *pFile = fopen(strFlushFileName.c_str(),"w+");
// 	fputs("#Host configure file",pFile);
// 	fputs(strEndOfLine.c_str(),pFile);
// 	fputs(strEndOfLine.c_str(),pFile);
//
// 	std::map<std::wstring, TSection*>::iterator iter1 = m_Inifile.begin();
// 	for (;iter1 != m_Inifile.end();iter1++)
// 	{
// 		std::wstring strSection = L"[";
// 		strSection += iter1->first.c_str();
// 		strSection += "]";
// 		fputs(strSection.c_str(),pFile);
// 		fputs(strEndOfLine.c_str(),pFile);
// 		std::map<std::wstring, std::wstring>::iterator iter2 = iter1->second->begin();
// 		for (;iter2 != iter1->second->end();iter2++)
// 		{
// 			std::wstring strLine = iter2->first.c_str();
// 			int nSpaceNum = (int)(nKeyLength-strLine.length());
// 			for (int i = 0;i < nSpaceNum;i++)
// 			{
// 				strLine += L" ";
// 			}
//
// 			strLine += L"= ";
// 			strLine += iter2->second.c_str();
// 			fputs(reinterpret_cast<const char *>(strLine.c_str()),pFile);
// 			fputs(strEndOfLine.c_str(),pFile);
// 		}
//
// 		fputs(strEndOfLine.c_str(),pFile);
// 	}
// 	fclose(pFile);

}

void IniFile::flush_to_string(std::wstring& strConfigure)
{
// 	BUTELCONNECT::SingleMutex s(m_mutex);
//
// 	int nKeyLength = 21;
// 	std::wstring strEndOfLine = L"\n";
//
// 	strConfigure += L"#Host version 1.0.0.1";
// 	strConfigure += strEndOfLine;
// 	strConfigure += L"#2014.04.18";;
// 	strConfigure += strEndOfLine;
// 	strConfigure += strEndOfLine;
//
// 	std::map<std::string, TSection*>::iterator iter1 = m_Inifile.begin();
// 	for (;iter1 != m_Inifile.end();iter1++)
// 	{
// 		std::wstring strSection = L"[";
// 		strSection += iter1->first.c_str();
// 		strSection += L"]";
// 		strConfigure += strSection;
// 		strConfigure += strEndOfLine;
// 		std::map<std::wstring, std::wstring>::iterator iter2 = iter1->second->begin();
// 		for (;iter2 != iter1->second->end();iter2++)
// 		{
// 			std::wstring strLine = iter2->first.c_str();
// 			int nSpaceNum = (int)(nKeyLength-strLine.length());
// 			for (int i = 0;i < nSpaceNum;i++)
// 			{
// 				strLine += L" ";
// 			}
//
// 			strLine += L"= ";
// 			strLine += iter2->second.c_str();
// 			strConfigure += strLine;
// 			strConfigure += strEndOfLine.c_str();
// 		}
//
// 		strConfigure += strEndOfLine;
// 	}
}

//void IniFile::SetInteger(const WCHAR* szSection, const WCHAR* szKey, int value)
//{
//	BUTELCONNECT::SingleMutex s(m_mutex);
//	int nRet = 0;
//	do
//	{
//		if (szSection == NULL || szKey == NULL)
//		{
//			nRet = 1;
//			break;
//		}

//		TSection* p = getSection(szSection,true);
//		if (p == NULL)
//		{
//			nRet = 2;
//			break;
//		}

//		//TSection::iterator iter = p->find(szKey);
//		//if (iter == p->end())
//		//{
//		//	nRet = 3;
//		//	break;
//		//}

//		TCHAR szValue[1024] = {0};
//		wsprintf(szValue,L"%d",value);
//		(*p)[szKey] = szValue;

//	} while (0);
//}

void IniFile::SetString(const WCHAR* szSection, const WCHAR* szKey, const WCHAR* value)
{
// 	BUTELCONNECT::SingleMutex s(m_mutex);
// 	int nRet = 0;
// 	do
// 	{
// 		if (szSection == NULL || szKey == NULL || value == NULL || strlen(value) == 0)
// 		{
// 			nRet = 1;
// 			break;
// 		}
//
// 		TSection* p = getSection(szSection, true);
// 		if (p == NULL)
// 		{
// 			nRet = 2;
// 			break;
// 		}
//
// // 		TSection::iterator iter = p->find(szKey);
// // 		if (iter == p->end())
// // 		{
// // 			nRet = 3;
// // 			break;
// // 		}
//
// 		(*p)[szKey] = value;
//
// 	} while (0);

    WritePrivateProfileString(szSection, szKey, value,m_strFilePath.c_str());
}

void IniFile::SetBool(const WCHAR* szSection, const WCHAR* szKey, bool bValue)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    int nRet = 0;
    do
    {
        if (szSection == NULL || szKey == NULL)
        {
            nRet = 1;
            break;
        }

        TSection* p = getSection(szSection,true);
        if (p == NULL)
        {
            nRet = 2;
            break;
        }

        //TSection::iterator iter = p->find(szKey);
        //if (iter == p->end())
        //{
        //	nRet = 3;
        //	break;
        //}

        (*p)[szKey] = bValue?L"yes":L"no";

    } while (0);
}

bool IniFile::get_a_section(const WCHAR* szSection,std::map<std::wstring, std::wstring> &section)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    TSection* p = getSection(szSection);
    if(!p)
        return false;
    section = *p;
    return true;
}

bool IniFile::add_a_key_value(const std::wstring& strSection, const std::wstring& strP, const std::wstring& strV)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::map<std::wstring, TSection*>::iterator iter =  m_Inifile.find(strSection);
    if(iter == m_Inifile.end())
        return false;
    TSection* pSec = iter->second;

    (*pSec)[strP] = strV;
    return true;
}

bool IniFile::delete_a_key_value(const std::wstring& strSection, const std::wstring& strP)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    std::map<std::wstring, TSection*>::iterator iter =  m_Inifile.find(strSection);
    if(iter == m_Inifile.end())
        return false;
    TSection* pSec = iter->second;
    std::map<std::wstring,std::wstring>::iterator iter1 = pSec->find(strP);
    if (iter1 != pSec->end())
    {
        pSec->erase(iter1);
    }
    return true;
}

bool IniFile::is_section_key_exist(const WCHAR* szSection, const WCHAR* szKey)
{
    BUTELCONNECT::SingleMutex s(m_mutex);
    bool bRet = true;
    do
    {
        if (szSection == NULL || szKey == NULL)
        {
            bRet = false;
            break;
        }

        TSection* p = getSection(szSection);
        if (p == NULL)
        {
            bRet = false;
            break;
        }

        TSection::iterator iter = p->find(szKey);
        if (iter == p->end())
        {
            bRet = false;
            break;
        }
    }while(0);
    return bRet;
}

} //namespace BUTELCONNECT
