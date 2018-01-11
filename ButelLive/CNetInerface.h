#ifndef CNETINERFACE_H
#define CNETINERFACE_H
#include <string>
#include <vector>
#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QString>
#include <Login.h>
#include "ChannelAndProgram.h"
class LoginUI;
class ChannelAndProgramUI;
using namespace std;

typedef struct tagLogInInfo
{
    QString          nickname;               //     用户昵称
    QString          nubernumber;         //     用户手机号
    QString          user_email;             //      邮箱
    QString          enterprises_name;  //      单位名
    QString          enterprises_appid;  //      appid
}StLogInInfo;

typedef struct tagChannelInfo
{
    QString          id;                            //     id
    QString          name;                      //     频道名字
    QString          pushstreamurl;             //     推流地址
    QString          flv_playurl;              //      flv播放地址
    QString          m3u8_playurl;             //     m3u8播放地址
    QString          channelnumber;            //      频道号
    QString          interactivenumber;        //      互动号
    QString          appKey;
}StChannelInfo;

typedef struct tagServerInfo
{
    QString          name;
    QString          ip;
    int              port;
}StServerInfo;
/**************************网络访问接口*******************************/
class CNetInerface : public QObject
{
    Q_OBJECT
public:
    CNetInerface(QObject* parent = 0);
    virtual ~CNetInerface();
    // 单例接口
    static CNetInerface *GetInstance();
    // 释放
    static void Destroy();
    // 设置服务器地址
    int SetSerAddr(QString strHttpPre);
    // 登陆操作
    int Login(QString strName, QString strPwd);
    // 鉴权
    int Authorize(QString strAppId);
    // 获取频道列表
    int GetChannelList(QString token,QString creator,int index,int size);
    //获取推流服务器列表
    int GetPushStreamServerList(const QString& token);

    //开始直播
    int StartLive(QString token,int channelId);
    //关闭直播
    int CloseLive(QString token,int channelId);
    //临时退出直播
    int PauseLive(QString token,int channelId);
private:
    static CNetInerface               *m_pInstance;


public:
    LoginUI*                      m_pLoginUI;
    ChannelAndProgramUI*          m_pChannelAndProgramUI;
    ButelLive*                    m_pButelLiveUI;
public:
    QNetworkAccessManager*      m_pLoginNetManager;
    QNetworkAccessManager*      m_pAuthorizeNetManager;
    QNetworkAccessManager*      m_pChannelNetManager;
    QNetworkAccessManager*      m_pServerNetManager;
    QNetworkAccessManager*      m_pStartLiveNetManager;
    QNetworkAccessManager*      m_pCloseLiveNetManager;
    QNetworkAccessManager*      m_pPauseLiveNetManager;
    QNetworkRequest                    m_pPostRequest;
    QNetworkRequest                    m_pGetRequest;
    QNetworkReply*                      m_pReply;
    QString                                     m_strUrlPre;
    QString                                     m_StrToken;
    tagLogInInfo                             m_LoginInfo;                             //登录信息
    QList<tagChannelInfo*>          m_channelList;                          //频道信息结构体列表
    int                                              m_iCurrentChannelIndex;         //记录当前选中的频道在频道列表中的索引
    int                             m_Index;
    QList<tagServerInfo*>           m_serverList;
public slots:
    void LoginReplyFinished(QNetworkReply* replay);
    void AuthorizeReplyFinished(QNetworkReply* replay);
    void ChannelReplyFinished(QNetworkReply* replay);
    void StartLiveReplyFinished(QNetworkReply* reply);
    void CloseLiveReplyFinished(QNetworkReply* reply);
    void PauseLiveReplyFinished(QNetworkReply* reply);
    void GetPushStreamServerListReplyFinished(QNetworkReply* replay);
};

#endif // CNETINERFACE_H
