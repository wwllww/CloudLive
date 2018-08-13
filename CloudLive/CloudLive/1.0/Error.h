#ifndef ERROR_H
#define ERROR_H
#include "Exception.h"
#include <iostream>

#pragma warning(disable:4005)

#ifndef BUTEL_THORWERROR
#define BUTEL_THORWERROR \
CException ex(__FILE__,__LINE__);\
    ex.ThrowErrorMsg
#endif
#ifndef BUTEL_IFNULLRETURNERROR
#define BUTEL_IFNULLRETURNERROR(InParam) \
    if(!InParam)\
    {\
        CException ex(__FILE__,__LINE__);\
        ex.ThrowErrorMsg("InParam %s is NULL",#InParam);\
    }
#endif


#ifndef DEBUG_PRINT
#define DEBUG_PRINT
#if defined(QT_DEBUG)
#define DEBUG_PRINT(Msg) qDebug() << Msg;
#else
#define DEBUG_PRINT(Msg) /*std::cout << Msg << std::endl;*/Log::writeError(LOG_RTSPSERV,1,Msg);
#endif
#endif
#endif // ERROR_H

