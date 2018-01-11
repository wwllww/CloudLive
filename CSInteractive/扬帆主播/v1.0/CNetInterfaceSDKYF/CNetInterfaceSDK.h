#ifndef CNETINTERFACESDK_H
#define CNETINTERFACESDK_H
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include "CHttpInterfaceSDK.h"
struct ChannelInfo;
struct NodeInfo;
typedef struct tagLogInInfo
{
    QString          number;                   //      登录号
    QString          uid;
    QString          channelId;
    QString          oauth_token;
    QString          nickname;                 //      用户昵称
    QString          nubernumber;              //      用户手机号
    QString          user_email;               //      邮箱
    QString          enterprises_name;         //      单位名
    QString          enterprises_appid;        //      appid
}StLogInInfo;

typedef struct tagChannelInfo
{
    QString          id;                       //      id
    QString          name;                     //      频道名字
    QString          pushstreamurl;            //      推流地址
    QString          flv_playurl;              //      flv播放地址
    QString          m3u8_playurl;             //      m3u8播放地址
    QString          channelnumber;            //      频道号
    QString          interactivenumber;        //      互动号
    QString          appKey;
    int              defaultAnchorFlag;
}StChannelInfo;

typedef struct tagProgramInfo
{
    QString          id;                       //      id
    QString          name;                     //      节目名字
    QString          channelId;
    QString          channelVerticalUrl;
    QString          stills;
}StProgramlInfo;

typedef struct tagServerInfo
{
    QString          name;
    QString          ip;
    unsigned int              port;
}StServerInfo;

class CNetInterfaceSDK : public QObject
{
    Q_OBJECT

public:
    QString                     m_strUrlPre;
    QString                     m_strUploadUrl;
    tagLogInInfo                m_LoginInfo;
    tagProgramInfo              m_ProgramInfo;
    QList<tagChannelInfo>       m_channelList;
    QList<tagServerInfo*>       m_serverList;
    int                         m_Index = 1;
    QString                     m_StrToken;
    QString                     m_strAppId;

    ChannelInfo *m_pChannelInfo = NULL;
    ChannelInfo *m_pCurrentChannelInfo = NULL;
    int         m_channeCount = 0;
    NodeInfo    *m_pNodeInfo = NULL;
    int         m_serverCount = 0;
    QString     m_CurChannelProgramId;
    QString     m_strAppkey;
    char        m_strAppkey_value[64];
    char        m_strChatUrl[256];
    int         m_StopOperType = 0;

public:
    CNetInterfaceSDK();
    virtual ~CNetInterfaceSDK();
    static CNetInterfaceSDK *GetInstance();
    static void DestroyInstance();
    int SetServerAddr(const QString& strUrl,const QString& strUploadUrl,const QString& strAppId);
    int Login(const QString& strName, const QString& strPwd,int iUsrType);
    int Authorize();
    int GetChannelList(ChannelInfo** pChannelInfo, int* pChannelCount);
    int FreeChannelList();
    int GetPushStreamServerList(NodeInfo** pNodeInfo, int* pNodeCount);
    int FreePushStreamServerList();
    int StartLive();
    int CloseLive();
    int PauseLive();
    int LoginReplyAnalysis(QNetworkReply* reply);
    int AuthorizeReplynalysis(QNetworkReply* reply);
    int GetChannelListReplyAnalysis(QNetworkReply* reply);
    int GetServerListReplyAnalysis(QNetworkReply* reply);
    int StartLiveReplyAnalysis(QNetworkReply* reply);
    int CloseLiveReplyAnalysis(QNetworkReply* reply);
    int PauseLiveReplyAnalysis(QNetworkReply* reply);
    int ChannelListCopy(ChannelInfo** pChannelInfo, int* pChannelCount);
    int ServerListCopy(NodeInfo** pNodeInfo, int* pServerCount);

    int GetCurChannelProgram();
    int CreatProgram();
    int GetCurChannelProgramReplyAnalysis(QNetworkReply* reply);
    int CreatProgramReplyAnalysis(QNetworkReply* reply);

    int GetChannelLiveStreamInfo();
    int SetChannelLiveStreamInfo();
    int GetChannelLiveStreamInfoReplyAnalysis(QNetworkReply* reply);
    int SetChannelLiveStreamInfoReplyAnalysis(QNetworkReply* reply);

    char* GetParameterList(const QString& strAppkey);
    char* GetParameterListReplyAnalysis(QNetworkReply* reply);

protected:
    QNetworkRequest             m_pPostRequest;
    QNetworkRequest             m_pGetRequest;
    QNetworkAccessManager*      m_pNetManager;
    QNetworkAccessManager*      m_pGetListNetManager;
    static CNetInterfaceSDK*    m_pInstance;
};

#endif // CNETINTERFACESDK_H
