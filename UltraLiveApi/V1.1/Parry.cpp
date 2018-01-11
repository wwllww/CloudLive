#include "PArry.h"
#include "Error.h"
#include "OperatNew.h"


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


#ifndef MAX_SIZE
#define MAX_SIZE 2000
#endif

CPArry::CPArry(int DynamicFree):m_DynamicFree(DynamicFree)
{
	m_Data = new void *[MAX_SIZE];
    size = 0;
}
CPArry::CPArry():m_DynamicFree(0)
{
	m_Data = new void *[MAX_SIZE];
    size = 0;
}
CPArry::~CPArry()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 开始析构", __FUNCTION__);
    clear();
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 析构完成", __FUNCTION__);
}

bool CPArry::Add(CPObject *Data)
{
    if(!Data || size >= MAX_SIZE)
        return false;
    m_Data[size] = Data;
    ++size;
    return true;
}

CPObject* CPArry::GetAt(int Index) const
{
    if(Index >= size)
    {
        return NULL;
    }

    if(m_Data[Index])
		return (CPObject*)m_Data[Index];

    return NULL;
}

int  CPArry::GetSize() const
{
    return size;
}
bool  CPArry::insert(CPObject *Data, int Index)
{
	if (Index > size || !Data || size >= MAX_SIZE)
	{
		BUTEL_THORWERROR("数据不合法Index = %d,size = %d,Data = %p", Index,size,Data);
	}
    void *TemData = NULL;
    if(1 == size)
    {
        ++size;
        if(0 == Index)
        {
            TemData = m_Data[0];
            m_Data[0] = Data;
            m_Data[1] = TemData;
            return true;
        }
        m_Data[Index] = Data;
        return true;
    }
    else
    {
        for(int i = size - 1; i >= Index; --i )
        {
            m_Data[i + 1] = m_Data[i];
        }
        m_Data[Index] = Data;

        ++size;
    }


    return true;
}

void CPArry::clear()
{
    if(m_DynamicFree)
    {
        for(int i = 0;i < size; ++i)
            delete (CPObject*)m_Data[i];
    }
    delete []  m_Data;
}

bool CPArry::erase(CPObject *Data, bool bDelete)
{
	if (!Data)
		return false;

	bool bFind = false;
	for (int i = 0; i < size; ++i)
	{
		if (m_Data[i] == Data)
		{
			bFind = true;
			for (int j = i + 1; j < size; ++j)
			{
				m_Data[j - 1] = m_Data[j];
			}
			--size;
			break;
		}
	}

	if (!bFind)
		return false;

	if (bDelete)
		delete Data;
	return true;
}

bool CPArry::erase(int Index, bool bDelete)
{
	if (Index < 0 || Index >= size)
		return false;

	if (bDelete)
		delete (CPObject*)m_Data[Index];
	for (int i = Index + 1; i < size; ++i)
	{
		m_Data[i - 1] = m_Data[i];
	}
	--size;

	return true;
}

CPObject* CPArry::operator[](unsigned int Index)
{
    if(Index < 0 || Index >= size)
    {
        BUTEL_THORWERROR("The Arry is out of range at %d",Index);
    }

	return (CPObject*)m_Data[Index];
}


void CPArry::SwapValues(int ValA, int ValB)
{
	if (ValA < 0 || ValA >= size || ValB < 0 || ValB >= size)
	{
		BUTEL_THORWERROR("The Arry is out of range at %d,%d", ValA, ValB);
	}
	void *Tem = m_Data[ValA];
	m_Data[ValA] = m_Data[ValB];
	m_Data[ValB] = Tem;
}


