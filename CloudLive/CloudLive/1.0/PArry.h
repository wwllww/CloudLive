#if !defined(PARRY_H)
#define PARRY_H

#include "OPject.h"

#pragma warning(disable:4018)
class CPArry : public COPject  
{
public:
    CPArry(int DynamicFree);
	CPArry();
    ~CPArry();
	bool Add(COPject *Data);
	COPject* GetAt(int Index) const;
	int   GetSize() const;
	bool  insert(COPject *Data,int Index = 0);
	bool  exist(const char * pszStr);
	bool  erase(COPject *Data, bool bDelete = false);
	bool  erase(int Index, bool bDelete = false);
	COPject *operator [](unsigned int Index);
private:
	void  clear();
	void **m_Data;
	unsigned int  size;
    int m_DynamicFree;


};

#endif
