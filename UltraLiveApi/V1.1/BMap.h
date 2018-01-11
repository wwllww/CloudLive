#ifndef BMAP_H
#define BMAP_H
#include <memory>
#include "PArry.h"
#include "OperatNew.h"

#pragma warning(disable:4996)


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#endif

//非线程安全
template <class T >
class CBMap
{
private:
    CPArry *m_Data;
    bool m_Dynamic;
public:
	CBMap(bool Dynamic = false) : m_Dynamic(Dynamic)
	{
		m_Data = new CPArry(1);
	}
	~CBMap()
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始析构", __FUNCTION__);
		if (m_Data)
		{
			if (m_Dynamic)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始动态删除 size = %d", __FUNCTION__,m_Data->GetSize());
				for (int i = 0; i < m_Data->GetSize(); ++i)
				{
					Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 找到并删除", __FUNCTION__);
					Data *TemDate = (Data*)m_Data->GetAt(i);
					if (((CBMap<CPObject*>::Data*)TemDate)->ExeFun)
						delete ((CBMap<CPObject*>::Data*)TemDate)->ExeFun;
					((CBMap<CPObject*>::Data*)TemDate)->ExeFun = NULL;
					
				}
			}
			delete m_Data;
		}
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 析构完成", __FUNCTION__);
	}
	T &operator [] (const char * pszStr)
	{
		int index = -1;
		if ((index = Exist(pszStr)) != -1)
		{
			Data * Tem = (Data*)m_Data->GetAt(index);
			return Tem->ExeFun;
		}

		Data * Tem = new Data;
		strcpy(Tem->Name, pszStr);
		insert(Tem->Name, Tem);
		Tem->ExeFun = NULL;
		return Tem->ExeFun;
	}

	T &operator [] (uint64_t u64)
	{
		char Tem[50] = { 0 };
		sprintf(Tem, "%llu", u64);

		return operator [](Tem);
	}

	T & GetAt(int index)
	{
		Data * Tem = (Data*)m_Data->GetAt(index);
		return Tem->ExeFun;
	}

	CBMap &operator = (CBMap<T> &Value)
	{
		return Value;
	}

	int Exist(const char * pszStr)
	{
		if (!pszStr || strcmp("", pszStr) == 0)
			return -1;
		Data *TemData = NULL;
		int l = 0;
		int h = m_Data->GetSize() - 1;
		int ret = 0;
		int m = 0;
		while (l <= h)
		{
			m = (l + h) >> 1;
			TemData = (Data*)m_Data->GetAt(m);
			ret = strcmp(pszStr, TemData->Name);

			if (ret > 0)
			{
				l = m + 1;
			}
			else if (ret < 0)
			{
				h = m - 1;
			}
			else
			{

				return m;
			}
		}

		return -1;
	}
	int GetSize() const
	{
		return m_Data->GetSize();
	}

	bool earse(uint64_t u64)
	{
		char Tem[50] = { 0 };
		sprintf(Tem, "%llu", u64);

		return earse(Tem);
	}


	bool earse(const char* pzKey)
	{
		int index = -1;
		if ((index = Exist(pzKey)) != -1)
		{
			Data * Tem = (Data*)m_Data->GetAt(index);

			if (m_Dynamic)
			{
				delete ((CBMap<CPObject*>::Data*)Tem)->ExeFun;

			}
			delete Tem;
			return m_Data->erase(index);
		}
		return false;
	}

	bool IsHave(const char *pzKey)
	{
		return Exist(pzKey) != -1;
	}

	bool IsHave(uint64_t u64)
	{
		char Tem[50] = { 0 };
		sprintf(Tem, "%llu", u64);

		return IsHave(Tem);
	}


	bool Remove(const char* pzKey)
	{
		int index = -1;
		if ((index = Exist(pzKey)) != -1)
		{
			Data * Tem = (Data*)m_Data->GetAt(index);
			delete Tem;
			return m_Data->erase(index);
		}
		return false;
	}

	struct Data : public CPObject
	{
		char Name[100];
		T ExeFun;
		Data()
		{
			memset(Name, 0, sizeof Name);
		}
		const char*GainClassName()
		{
			return "Data";
		}
	};

private:
	void insert(const char *pszStr, T& Value)
	{
		if (!pszStr || strmcp(pszStr, "") == 0)
			return;
		Data * TemData = new Data;
		strcpy(TemData->Name, pszStr);
		TemData->ExeFun = Value;
		m_Data->Add(TemData);
	}
	void insert(const char *pszStr, CPObject *data)
	{
		if (!pszStr || strcmp(pszStr, "") == 0 || !data)
			return;
		Data * TemData = NULL;
		if (0 == m_Data->GetSize())
		{
			m_Data->Add(data);
		}
		else
		{
			int l = 0;
			int h = m_Data->GetSize() - 1;
			int ret = 0;
			int m = 0;
			while (l <= h)
			{
				m = (l + h) >> 1;
				TemData = (Data*)m_Data->GetAt(m);
				ret = strcmp(pszStr, TemData->Name);

				if (ret > 0)
				{
					l = m + 1;
				}
				else if (ret < 0)
				{
					h = m - 1;
				}
				else
				{
					delete TemData;
					m_Data->insert(data, m);
					return;
				}
			}

			if (ret > 0)
			{
				m_Data->insert(data, m + 1);
			}
			else
			{
				m_Data->insert(data, m);
			}
		}
	}

};

#endif //BMAP_H

