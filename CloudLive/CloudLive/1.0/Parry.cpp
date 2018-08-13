#include "PArry.h"
#include "Error.h"

CPArry::CPArry(int DynamicFree):m_DynamicFree(DynamicFree)
{
    m_Data = new void *[100];
    size = 0;
}
CPArry::CPArry():m_DynamicFree(0)
{
    m_Data = new void *[100];
    size = 0;
}
CPArry::~CPArry()
{
    clear();
}

bool CPArry::Add(COPject *Data)
{
    if(!Data)
        return false;
    m_Data[size] = Data;
    ++size;
    return true;
}

COPject* CPArry::GetAt(int Index) const
{
    if(Index >= size)
    {
        return NULL;
    }

    if(m_Data[Index])
        return (COPject*)m_Data[Index];

    return NULL;
}

int  CPArry::GetSize() const
{
    return size;
}
bool  CPArry::insert(COPject *Data,int Index)
{
    if(Index > size || !Data)
        return false;
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
            delete m_Data[i];
    }
    delete []  m_Data;
}

COPject* CPArry::operator[](unsigned int Index )
{
    if(Index < 0 || Index >= size)
    {
        BUTEL_THORWERROR("The Arry is out of range at %d",Index);
    }

    return (COPject*)m_Data[Index];
}

bool CPArry::erase(COPject *Data, bool bDelete /*= false*/)
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
		delete (COPject*)m_Data[Index];
	for (int i = Index + 1; i < size; ++i)
	{
		m_Data[i - 1] = m_Data[i];
	}
	--size;

	return true;
}
