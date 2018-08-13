#if !defined(BASEAFX_H)
#define BASEAFX_H
#include "BaseProcess.h"
#include "Error.h"
#include "LogDeliver.h"


#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif // !LOG_RTSPSERV

typedef  BaseProcess* (*CREATEOBJ)();

typedef struct CREATOBJ
{
	const char *Class_Name;
	CREATEOBJ fp;
}CreateOBJ,*pCreateOBJ;

 int AddCreateOBject2Array(const char * ClassName,CREATEOBJ Fp);
 BaseProcess* CREATEOBJECT(const char *Classname);

#define DYNIC_DECLARE(CLASS_NAME)\
public:\
	static BaseProcess * CreateOBject();\
    void   Excuse(CRequse &Req,CRespond &Res);\
    static int CLASS_NAME::CLASS_NAME##initFunc(const char * ClassName,int Num);


#define IMPLEMENT_DYNIC(CLASS_NAME)\
	BaseProcess * CLASS_NAME::CreateOBject()\
		{\
			return new CLASS_NAME;\
		}\
  static int  j = AddCreateOBject2Array(#CLASS_NAME, CLASS_NAME::CreateOBject);

#endif
