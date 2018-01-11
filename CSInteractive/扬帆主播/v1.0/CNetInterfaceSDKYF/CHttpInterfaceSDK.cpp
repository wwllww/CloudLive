#include "CHttpInterfaceSDK.h"
#include "CreateLiveUI.h"
#include "CurLiveUI.h"
#include "StopLiveConfirm.h"
#include <QString>
#include <QTranslator>

int BLiveAppInit(char* pServerUrl, char* pUploadUrl, char* pStrAppId)
{
    QTranslator* translator = new QTranslator;
    translator->load("CNetInterfaceSDK_CN.qm");
    qApp->installTranslator(translator);

    if(pServerUrl == NULL || pUploadUrl == NULL || pStrAppId == NULL)
        return -1;
    return CNetInterfaceSDK::GetInstance()->SetServerAddr(QString::fromLocal8Bit(pServerUrl),QString::fromLocal8Bit(pUploadUrl),QString::fromLocal8Bit(pStrAppId));
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
    if(CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->nDefaultAnchorFlag == 1)
    {
        int ret = CNetInterfaceSDK::GetInstance()->GetCurChannelProgram();
        CreateLiveUI *pCreateLiveUI = new CreateLiveUI(NULL,QObject::tr("Live broadcast"),CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->pChannelName,CNetInterfaceSDK::GetInstance()->m_ProgramInfo.name);
        int userRet =  pCreateLiveUI->exec();
        if(10 == userRet)
        {
            if(1000 == CNetInterfaceSDK::GetInstance()->SetChannelLiveStreamInfo())
            {
                ret = CNetInterfaceSDK::GetInstance()->StartLive();
                if(1000 == ret)
                {
                    return 0;
                }
            }
            else
            {
                return -2;
            }
        }
        else
        {
            return -4;
        }
    }
    else if(CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->nDefaultAnchorFlag == 2)
    {
        int ret = CNetInterfaceSDK::GetInstance()->GetCurChannelProgram();
        if(1000 == ret)
        {
            CurLiveUI *pCurLiveUI = new CurLiveUI(NULL,QObject::tr("information"),QObject::tr("Currently live:%1").arg(CNetInterfaceSDK::GetInstance()->m_ProgramInfo.name));
            int userRet =  pCurLiveUI->exec();
            if(10 == userRet)
            {
                if(1000 == CNetInterfaceSDK::GetInstance()->CreatProgram())
                {
                    ret = CNetInterfaceSDK::GetInstance()->StartLive();
                    if(1000 == ret)
                    {
                        return 0;
                    }
                }
            }
            else if(20 == userRet)
            {
                CreateLiveUI *pCreateLiveUI = new CreateLiveUI(NULL,QObject::tr("Live broadcast"),CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->pChannelName,CNetInterfaceSDK::GetInstance()->m_ProgramInfo.name);
                int userRet =  pCreateLiveUI->exec();
                if(10 == userRet)
                {
                    if(1000 == CNetInterfaceSDK::GetInstance()->CreatProgram())
                    {
                        ret = CNetInterfaceSDK::GetInstance()->StartLive();
                        if(1000 == ret)
                        {
                            return 0;
                        }
                    }
                    else
                    {
                        return -2;
                    }
                }
                else
                {
                    return -4;
                }
            }
            else
            {
                return -1;
            }
        }
        else if(2048 == ret)
        {
            CreateLiveUI *pCreateLiveUI = new CreateLiveUI(NULL,QObject::tr("Live broadcast"),CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->pChannelName,CNetInterfaceSDK::GetInstance()->m_ProgramInfo.name);
            int userRet =  pCreateLiveUI->exec();
            if(10 == userRet)
            {
                if(1000 == CNetInterfaceSDK::GetInstance()->CreatProgram())
                {
                    ret = CNetInterfaceSDK::GetInstance()->StartLive();
                    if(1000 == ret)
                    {
                        return 0;
                    }
                }
                else
                {
                    return -2;
                }
            }
            else
            {
                return -4;
            }
        }
        else
        {
            return -3;
        }
    }

}

int BLiveAppBeforeStopPushConfirm(bool& bChannelClosed)
{
    bChannelClosed = false;
    if(CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->nDefaultAnchorFlag == 1)
    {
        return 0;
    }
    else if(CNetInterfaceSDK::GetInstance()->m_pCurrentChannelInfo->nDefaultAnchorFlag == 2)
    {
        StopLiveConfirmUI * pStopLiveUI = new StopLiveConfirmUI(NULL);
        int ret = pStopLiveUI->exec();
        if(10 == ret)
        {
            return 0;
        }
        else
        {
            return -1;
        }
    }
}
int BLiveAppAfterStopPush()
{
    if(CNetInterfaceSDK::GetInstance()->m_StopOperType == OperPause)
    {
//        return CNetInterfaceSDK::GetInstance()->PauseLive();
        return 0;
    }
    else if(CNetInterfaceSDK::GetInstance()->m_StopOperType == OperClose)
    {
        return CNetInterfaceSDK::GetInstance()->CloseLive();
    }
}
char* BLiveAppGetChatUrl()
{
    strcpy(CNetInterfaceSDK::GetInstance()->m_strChatUrl,CNetInterfaceSDK::GetInstance()->m_ProgramInfo.channelVerticalUrl.toLocal8Bit().data());
    return CNetInterfaceSDK::GetInstance()->m_strChatUrl;
//    return NULL;
}
char* BLiveAppGetTitle()
{
    const char* pTitle = "扬帆主播";
    return (char*)pTitle;
}
char* BLiveAppGetAbout()
{
    const char* pVersion = "版本号：3.0.0.1\n\n版权所有：扬州广播电视总台";
    return (char*)pVersion;
}
char* BLiveAppGetAppkey(char* key)
{
    return CNetInterfaceSDK::GetInstance()->GetParameterList(QString::fromLocal8Bit(key));
}
