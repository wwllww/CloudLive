#ifndef BUTELLIVESDK_H
#define BUTELLIVESDK_H

#ifdef CBUTELLIVEDLLAPI_EXPORTS
#include "CHttpInterfaceSDK.h"
#endif

#ifdef CBUTELLIVEDLLAPI_EXPORTS
#define API_EXPORT  __declspec(dllexport)
#else
#define API_EXPORT  __declspec(dllimport)
#endif

#ifdef  __cplusplus
extern "C" {
#endif //  __cplusplus

        API_EXPORT  int     CreatButelLiveInstance(bool bChat,ChannelInfo* channelInfoList,int channelCount,int currentChannelIndex);

#ifdef  __cplusplus
}
#endif //  __cplusplus


#endif // BUTELLIVESDK_H
