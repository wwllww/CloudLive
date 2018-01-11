#ifndef ERROR_H
#define ERROR_H
#include "Exception.h"

#pragma warning(disable:4005)

#ifndef BUTEL_THORWERROR
#define BUTEL_THORWERROR \
	CException ex(__FILE__, __LINE__, __FUNCTION__); \
	ex.ThrowErrorMsg
#endif
#ifndef BUTEL_IFNULLRETURNERROR
#define BUTEL_IFNULLRETURNERROR(InParam) \
    if(!InParam)\
    {\
        CException ex(__FILE__,__LINE__,__FUNCTION__);\
		ex.ThrowErrorMsg("InParam %s is NULL",#InParam); \
    }
#endif

#endif // ERROR_H

