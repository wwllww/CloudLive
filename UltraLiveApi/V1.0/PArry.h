#if !defined(PARRY_H)
#define PARRY_H

#include "BaseAfx.h"

#pragma warning(disable:4018)
class CPArry : public CPObject
{
public:
    CPArry(int DynamicFree);
	CPArry(const CPObject&) = delete;
	CPArry();
    ~CPArry();
	bool Add(CPObject *Data);
	CPObject* GetAt(int Index) const;
	int   GetSize() const;
	bool  insert(CPObject *Data, int Index = 0);
	bool  exist(const char * pszStr);
	bool  erase(CPObject *Data,bool bDelete = false);
	bool  erase(int Index, bool bDelete = false);

	void SwapValues(int ValA, int ValB);

	CPObject *operator [](unsigned int Index);
	CPArry *operator = (const CPArry &) = delete;
	const char* GainClassName() { return "CPArry"; }
private:
	void  clear();
	void **m_Data;
	unsigned int  size;
    int m_DynamicFree;
};

#endif
