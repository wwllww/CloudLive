#if !defined(EXCPROCESS_H)
#define EXCPROCESS_H
#include "BaseProcess.h"

class ExcProcess;
typedef void (ExcProcess::*FUNC_PROCESS)(CRequse &Req,CRespond &Res);
class ExcProcess : public BaseProcess
{
public:
    virtual ~ExcProcess(){}
	struct RealData
	{
        char Fun_Name[200];
        FUNC_PROCESS Func_ID;
	};

    FUNC_PROCESS Find(const char * Fuc_Name,const RealData * ActulData, int Size);

protected:
    void ImplementProcess(const RealData * ActulData, int TotalNum,CRequse &Req,CRespond &Res);
	FUNC_PROCESS Fun;


};
typedef struct ExcProcess::RealData TData;
void AddFuncToArry(TData *RealDate, const TData & Data, int Size);


#define  BEGIN_REGIST_CLASS(CLASS_NAME,NUM)\
	TData g_##CLASS_NAME[NUM]; \
	int CLASS_NAME##TemNum = 0; \
	int CLASS_NAME##Total = NUM; \
	TData CLASS_NAME##TemData; \
	int CLASS_NAME::CLASS_NAME##initFunc(const char * ClassName, int Num){

#define  F1_REGIST_CLASS(FUNC_NAME,CLASS_NAME,FUNC_ID) \
if (CLASS_NAME##TemNum >= CLASS_NAME##Total || !FUNC_NAME || strcmp(FUNC_NAME, "") == 0)\
	{\
		abort();\
	}\
	memset(&CLASS_NAME##TemData,0,sizeof(CLASS_NAME##TemData));\
    strcpy_s(CLASS_NAME##TemData.Fun_Name,FUNC_NAME);\
	CLASS_NAME##TemData.Func_ID = static_cast<FUNC_PROCESS>(&FUNC_ID);\
    AddFuncToArry(g_##CLASS_NAME,CLASS_NAME##TemData,CLASS_NAME##TemNum);\
	++CLASS_NAME##TemNum;

#define END_REGIST_CLASS(CLASS_NAME) return 1;}\
     static int i = CLASS_NAME::CLASS_NAME##initFunc(#CLASS_NAME, CLASS_NAME##TemNum);\
                void CLASS_NAME::Excuse(CRequse &Req,CRespond &Res)\
						{\
							ImplementProcess(g_##CLASS_NAME, CLASS_NAME##TemNum,Req,Res);\
						}
						

#endif
