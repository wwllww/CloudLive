#include "BaseAfx.h"
#include "ExcProcess.h"

FUNC_PROCESS ExcProcess::Find(const char * Fuc_Name,const RealData * ActulData, int Size)
{
    if( 0 == Size || !Fuc_Name || !ActulData)
		return NULL;
	int l = 0;
	int h = Size - 1;
	while(l <= h)
	{
		int m = (l + h) >> 1;
		int ret = strcmp(Fuc_Name,ActulData[m].Fun_Name);

		if(ret > 0)
		{
			l = m + 1;
			
		}
		else if(ret < 0)
		{
			h = m - 1;
		}
		else 
		{
			return ActulData[m].Func_ID;
		}
	}

	return NULL;
}

void ExcProcess::ImplementProcess(const RealData * ActulData, int TotalSize,CRequse &Req,CRespond &Res)
{

	Fun = Find(Req.GetCmd(),ActulData,TotalSize);
	if(Fun)
		(this->*Fun)(Req,Res);
	else
	{
		BUTEL_THORWERROR("ExcProcess::ImplementProcess can't find Execuse Function %s!", Req.GetCmd());
	}

}

void AddFuncToArry(TData *RealDate,const TData & Data,int Size)
{
    if(!Data.Fun_Name || !RealDate)
        return;

    if(Size == 0)
	{
        strcpy_s(RealDate[0].Fun_Name ,Data.Fun_Name);
		RealDate[0].Func_ID = Data.Func_ID;
	}
	else
	{
		
		for(int i = 0; i < Size; ++i)
		{
            int ret = strcmp(Data.Fun_Name,RealDate[i].Fun_Name);
			if(ret > 0)
			{
				if(i == Size - 1)
				{
					strcpy_s(RealDate[i + 1].Fun_Name, Data.Fun_Name);
					RealDate[i + 1].Func_ID = Data.Func_ID;
					break;
				}
                else
					continue;
			}
			else if(ret < 0)
			{
				for(int j = Size - 1;j >= i; --j)	
				{
					RealDate[j + 1] = RealDate[j];
				}
				strcpy_s(RealDate[i].Fun_Name, Data.Fun_Name);
				RealDate[i].Func_ID = Data.Func_ID;
				break;
			}
			else
			{
				RealDate[i].Func_ID = Data.Func_ID;
				break;
			}
		}
		
	}


}
