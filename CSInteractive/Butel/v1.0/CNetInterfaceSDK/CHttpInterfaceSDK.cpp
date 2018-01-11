#include "CHttpInterfaceSDK.h"
#include "StopLiveConfirm.h"
#include <QString>
#include <QTranslator>

int BLiveAppInit(char* pServerUrl, char* pUploadUrl, char* pStrAppId)
{
    QTranslator* translator = new QTranslator;
    translator->load("CNetInterfaceSDK_CN.qm");
    qApp->installTranslator(translator);

    if(pServerUrl == NULL)
        return -1;
    return CNetInterfaceSDK::GetInstance()->SetServerAddr(QString::fromLocal8Bit(pServerUrl));
}
int BLiveAppLogin(char* pUserName, char* pPassword, int nUserType)
{
    if(pUserName == NULL || pPassword == NULL)
        return -2;
    return CNetInterfaceSDK::GetInstance()->Login(QString::fromLocal8Bit(pUserName),QString::fromLocal8Bit(pPassword),nUserType);
}
int BLiveAppAuthorize()
{
    return CNetInterfaceSDK::GetInstance()->Authorize();
}
int BLiveAppGetChannelList(ChannelInfo** pChannelInfo, int* pChannelCount)
{
    return CNetInterfaceSDK::GetInstance()->GetChannelList(pChannelInfo,pChannelCount);
}
int BLiveAppReleaseChannelList()
{
    return CNetInterfaceSDK::GetInstance()->FreeChannelList();
}
int BLiveAppSetChannel(ChannelInfo* pCurrentChannel)
{
    if(!pCurrentChannel) return -1;
    CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo = pCurrentChannel;
    return 0;
}
int BLiveAppGetSeverNodeList(NodeInfo** pNodeInfo, int* pNodeCount)
{
    return CNetInterfaceSDK::GetInstance()->GetPushStreamServerList(pNodeInfo,pNodeCount);
}
int BLiveAppReleaseSeverNodeList()
{
    return CNetInterfaceSDK::GetInstance()->FreePushStreamServerList();
}

int BLiveAppBeforeStartPush()
{
    return CNetInterfaceSDK::GetInstance()->StartLive();
}
int BLiveAppBeforeStopPushConfirm(bool& bChannelClosed)
{
    StopLiveConfirmUI * pStopLiveUI = new StopLiveConfirmUI(NULL);
    int ret = pStopLiveUI->exec();
    bChannelClosed = CNetInterfaceSDK::GetInstance()->m_StopOperType;
    if(10 == ret)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
int BLiveAppAfterStopPush()
{
    if(CNetInterfaceSDK::GetInstance()->m_StopOperType == OperPause)
    {
        return CNetInterfaceSDK::GetInstance()->PauseLive();
    }
    else if(CNetInterfaceSDK::GetInstance()->m_StopOperType == OperClose)
    {
        return CNetInterfaceSDK::GetInstance()->CloseLive();
    }
}
char* BLiveAppGetChatUrl()
{
    return NULL;
}
char* BLiveAppGetTitle()
{
    const char* pTitle = "Butel导播台";
    return (char*)pTitle;
}
char* BLiveAppGetAbout()
{
//    char* about = "ButelLive";
    const char* pVersion = "版本号：3.0.0.1\n\n版权所有：北京红云融通技术有限公司";
    return (char*)pVersion;
}
char* BLiveAppGetAppkey(char* key)
{
    return NULL;
}
