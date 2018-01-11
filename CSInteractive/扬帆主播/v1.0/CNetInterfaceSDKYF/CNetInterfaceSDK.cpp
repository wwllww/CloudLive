#include "CNetInterfaceSDK.h"
#include "LogDeliver.h"
#include "qdebug.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QThread>
#include <QDateTime>

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

CNetInterfaceSDK *CNetInterfaceSDK::m_pInstance = NULL;

CNetInterfaceSDK::CNetInterfaceSDK()
{
    m_pNetManager = NULL;
    m_pGetListNetManager = NULL;
}

CNetInterfaceSDK * CNetInterfaceSDK::GetInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CNetInterfaceSDK;
    }
    return m_pInstance;
}
void CNetInterfaceSDK::DestroyInstance()
{
    if (NULL != m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

CNetInterfaceSDK::~CNetInterfaceSDK()
{

}

int CNetInterfaceSDK::SetServerAddr(const QString& strUrl,const QString& strUploadUrl,const QString& strAppId)
{
    if(strUrl.isEmpty() || strUploadUrl.isEmpty())
    {
        return -1;
    }
    else
    {
        m_strUrlPre = strUrl;
        m_strUploadUrl = strUploadUrl;
        m_strAppId = strAppId;
        return 0;
    }
}


int CNetInterfaceSDK::Login(const QString& strName,const QString& strPwd,int iUsrType)
{
    if (strName.isEmpty() || strPwd.isEmpty()) return -2;
    m_LoginInfo.number = strName;

    QString url = "%1/setsail/external/orginalService/login?login=%2&password=%3&pushToken=%4&userType=%5&imei=%6&terminalType=%7";
    QString qstrUrl = url.arg(m_strUrlPre).arg(strName).arg(strPwd).arg("1234567890").arg(2).arg("imei").arg("pc");
    qDebug() << qstrUrl;
    m_pPostRequest.setUrl(QUrl(qstrUrl));

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());

    if(!m_pNetManager)
        m_pNetManager = new QNetworkAccessManager;
    QNetworkReply* reply = m_pNetManager->get(m_pPostRequest/*,bytePara*/);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    return LoginReplyAnalysis(reply);
    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return LoginReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -1;
    }
}
int CNetInterfaceSDK::Authorize()
{
    if (m_strAppId.isEmpty()) return -1;
    QString para ="{\"appid\": \"%1\"}" ;
    QString qstrPara = para.arg(m_strAppId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/account/authorize";
    QString qstrUrl = url.arg(m_strUploadUrl);
    m_pPostRequest.setUrl(QUrl(qstrUrl));

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());

    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    return AuthorizeReplynalysis(reply);
    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return AuthorizeReplynalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::GetChannelList(ChannelInfo** pChannelInfo, int* pChannelCount)
{

    m_pGetListNetManager = new QNetworkAccessManager;

    qDebug() << "CNetInterfaceSDK:" << QThread::currentThreadId();
    if(m_strUrlPre.isEmpty()/* || m_StrToken.isEmpty()*/)return -1;
    QString url = "%1/setsail/external/externalService?service=getAnchorChannelList&params={\"idx\" :%2,\"size\" :%3,\"token\":\"%4\",\"type\" :\"%5\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(0).arg(10).arg(m_LoginInfo.oauth_token).arg(2);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pGetListNetManager->get(m_pGetRequest);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit,Qt::QueuedConnection);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    int iRet = GetChannelListReplyAnalysis(reply);
//    if(0 == iRet)
//    {
//        return ChannelListCopy(pChannelInfo,pChannelCount);
//    }
//    else
//    {
//        return iRet;
//    }
    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            int iRet = GetChannelListReplyAnalysis(reply);
            if(0 == iRet)
            {
                return ChannelListCopy(pChannelInfo,pChannelCount);
            }
            else
            {
                return iRet;
            }
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::FreeChannelList()
{
    if(!m_pChannelInfo) return 0;
    ChannelInfo* pTmpChannelInfo = m_pChannelInfo;
    for(int i = 0; i < m_channeCount; i++,pTmpChannelInfo++)
    {
        free(pTmpChannelInfo->pChannelName);
        free(pTmpChannelInfo->pChannelNumber);
        free(pTmpChannelInfo->pInteractionNube);
        free(pTmpChannelInfo->pAppKey);
        free(pTmpChannelInfo->pPushUrl);
        free(pTmpChannelInfo->pPlayUrl_flv);
        free(pTmpChannelInfo->pPlayUrl_m3u8);
    }
    m_channeCount = 0;
    free(m_pChannelInfo);
    m_pChannelInfo = NULL;
    qDebug() << "ChannelList free over";
    return 0;
}
int CNetInterfaceSDK::GetPushStreamServerList(NodeInfo** pNodeInfo, int* pNodeCount)
{
    if(m_strUploadUrl.isEmpty() || m_StrToken.isEmpty())return -1;
    QString url = "%1/webapi/LiveChannel/GetPushStreamServer?token=%2";
    QString qstrUrl = url.arg(m_strUploadUrl).arg(m_StrToken);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            int iRet = GetServerListReplyAnalysis(reply);
            if(0 == iRet)
            {
                return ServerListCopy(pNodeInfo,pNodeCount);
            }
            else
            {
                return iRet;
            }
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::FreePushStreamServerList()
{
    if(!m_pNodeInfo) return 0;
    NodeInfo* pTmpNodeInfo = m_pNodeInfo;
    for(int i = 0; i < m_serverCount; i++,pTmpNodeInfo++)
    {
        free(pTmpNodeInfo->pdescribe);
    }
    m_serverCount = 0;
    free(m_pNodeInfo);
    m_pNodeInfo = NULL;
    qDebug() << "ServerList free over";
    return 0;
}
int CNetInterfaceSDK::StartLive()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url = "%1/setsail/external/externalService?service=onLive&params={\"channelId\" : \"%2\",\"token\" :\"%3\",\"programId\" :\"%4\",\"userType\" :\"%5\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_pCurrentChannelInfo->nChannelId).arg(m_LoginInfo.oauth_token).arg(m_ProgramInfo.id).arg(2);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return StartLiveReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::CloseLive()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url = "%1/setsail/external/externalService?service=overLive&params={\"channelId\" : \"%2\",\"token\" :\"%3\",\"programId\" :\"%4\",\"userType\" :\"%5\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_ProgramInfo.channelId).arg(m_LoginInfo.oauth_token).arg(m_ProgramInfo.id).arg(2);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return CloseLiveReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::PauseLive()
{
    if(!m_pCurrentChannelInfo || m_strUrlPre.isEmpty() || m_StrToken.isEmpty()) return -1;
    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(m_StrToken).arg(m_pCurrentChannelInfo->nChannelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/pause";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec(QEventLoop::ExcludeUserInputEvents);
    return PauseLiveReplyAnalysis(reply);
}

int CNetInterfaceSDK::GetCurChannelProgram()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url = "%1/setsail/external/externalService?service=getChannelCurProgram&params={\"channelId\" : \"%2\",\"token\" :\"%3\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_pCurrentChannelInfo->nChannelId).arg(m_LoginInfo.oauth_token);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return GetCurChannelProgramReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::CreatProgram()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url,qstrUrl;
    QDateTime curDataTime = QDateTime::currentDateTime();
    QString strDateTime = curDataTime.toString("yyyyMMddhhmmss");
    if(m_ProgramInfo.id.isEmpty())
    {
        url = "%1/setsail/external/externalService?service=initiateLive&params={\"channelId\" : \"%2\",\"liveType\" :%3,\"picUrl\" :\"%4\",\"programName\" :\"%5\",\"startTime\" :\"%6\",\"token\" :\"%7\",\"userType\" :\"%8\"}";
        qstrUrl = url.arg(m_strUrlPre).arg(m_pCurrentChannelInfo->nChannelId).arg(0).arg(m_ProgramInfo.stills).arg(m_ProgramInfo.name).arg(strDateTime).arg(m_LoginInfo.oauth_token).arg(2);
    }
    else
    {
        url = "%1/setsail/external/externalService?service=initiateLive&params={\"channelId\" : \"%2\",\"joinProgramId\" :\"%3\",\"liveType\" :%4,\"picUrl\" :\"%5\",\"programName\" :\"%6\",\"startTime\" :\"%7\",\"token\" :\"%8\",\"userType\" :\"%9\"}";
        qstrUrl = url.arg(m_strUrlPre).arg(m_ProgramInfo.channelId).arg(m_ProgramInfo.id).arg(0).arg(m_ProgramInfo.stills).arg(m_ProgramInfo.name).arg(strDateTime).arg(m_LoginInfo.oauth_token).arg(2);
    }
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return CreatProgramReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}

int CNetInterfaceSDK::GetCurChannelProgramReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {
                        QJsonObject data_obj = data_value.toObject();
                        QJsonValue id_value = data_obj.take("id");
                        m_ProgramInfo.id = id_value.toString();
                        QJsonValue programName_value = data_obj.take("programName");
                        m_ProgramInfo.name = programName_value.toString();
                        QJsonValue channelId_value = data_obj.take("channelId");
                        m_ProgramInfo.channelId = channelId_value.toString();
                        QJsonValue channelVerticalUrl_value = data_obj.take("channelVerticalUrl");
                        m_ProgramInfo.channelVerticalUrl = channelVerticalUrl_value.toString();
                        QJsonValue stills_value = data_obj.take("stills");
                        m_ProgramInfo.stills = stills_value.toString();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}
char* CNetInterfaceSDK::GetParameterList(const QString& strAppkey)
{
    if(m_strUrlPre.isEmpty() || strAppkey.isEmpty())return NULL;
    m_strAppkey = strAppkey;
    QString url = "%1/setsail/external/externalService?service=getParameterList&params={\"keys\" : \"%2\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(strAppkey);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return NULL;
        } else {
            return GetParameterListReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return NULL;
    }

}
char* CNetInterfaceSDK::GetParameterListReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {
                        QJsonObject data_obj = data_value.toObject();
                        QJsonValue rows_value = data_obj.take("rows");
                        if(rows_value.isArray())
                        {
                            QJsonArray rows_arr = rows_value.toArray();
                            for(int i = 0; i < rows_arr.count(); i++)
                            {
                                QJsonValue element_value = rows_arr.at(i);
                                if(element_value.isObject())
                                {
                                    QJsonObject elementObj = element_value.toObject();
                                    if(elementObj.contains("name"))
                                    {
                                        QJsonValue name_value = elementObj.take("name");
                                        QString name = name_value.toString();
                                        if(m_strAppkey == name)
                                        {
                                            if(elementObj.contains("value"))
                                            {
                                                QJsonValue value_value = elementObj.take("value");
//                                                m_strAppkey_value = value_value.toString();
                                                QString value = value_value.toString();
                                                strcpy(m_strAppkey_value,value.toLocal8Bit().data());
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    if(iRet == 1000)
    {
//        char* pAppkey = (char*)malloc(m_strAppkey_value.length() + 1);
//        strcpy(pAppkey,m_strAppkey_value.toLocal8Bit().data());
//        return pAppkey;

        return m_strAppkey_value;
    }
    else
        return NULL;
}
int CNetInterfaceSDK::GetChannelLiveStreamInfo()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url = "%1/setsail/external/externalService?service=getAnchorChannelLiveStreamInfo&params={\"channelId\" : \"%2\",\"token\" :\"%3\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_pCurrentChannelInfo->nChannelId).arg(m_LoginInfo.oauth_token);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return GetChannelLiveStreamInfoReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::SetChannelLiveStreamInfo()
{
    if(m_strUrlPre.isEmpty())return -1;
    QString url = "%1/setsail/external/externalService?service=setAnchorChannelLiveStreamInfo&params={\"channelId\" : \"%2\",\"id\" :\"%3\",\"liveName\" :\"%4\",\"stills\" :\"%5\",\"token\" :\"%6\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_pCurrentChannelInfo->nChannelId).arg(m_ProgramInfo.id).arg(m_ProgramInfo.name).arg(m_ProgramInfo.stills).arg(m_LoginInfo.oauth_token);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);

    QTimer timer;
    timer.setInterval(5000);  // 设置超时时间 5 秒
    timer.setSingleShot(true);  // 单次触发

    QEventLoop loop;
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    timer.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    if (timer.isActive()) {  // 处理响应
        timer.stop();
        if (reply->error() != QNetworkReply::NoError) {
            return -1;
        } else {
            return SetChannelLiveStreamInfoReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
}
int CNetInterfaceSDK::GetChannelLiveStreamInfoReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {

                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}
int CNetInterfaceSDK::SetChannelLiveStreamInfoReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {
                        QJsonObject data_obj = data_value.toObject();
                        QJsonValue liveName_value = data_obj.take("liveName");
                        m_ProgramInfo.name = liveName_value.toString();
                        QJsonValue stills_value = data_obj.take("stills");
                        m_ProgramInfo.stills = stills_value.toString();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}


int CNetInterfaceSDK::CreatProgramReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {
                        QJsonObject data_obj = data_value.toObject();
                        QJsonValue id_value = data_obj.take("id");
                        m_ProgramInfo.id = id_value.toString();
                        QJsonValue programName_value = data_obj.take("programName");
                        m_ProgramInfo.name = programName_value.toString();
                        QJsonValue channelId_value = data_obj.take("channelId");
                        m_ProgramInfo.channelId = channelId_value.toString();
                        QJsonValue channelVerticalUrl_value = data_obj.take("channelVerticalUrl");
                        m_ProgramInfo.channelVerticalUrl = channelVerticalUrl_value.toString();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}

int CNetInterfaceSDK::LoginReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    int ret = reply->error();
    if(ret > 0 )
    {
        if(iRet <= 99)
            return -1;
        else if(iRet > 99)
            return -2;
    }
    m_LoginInfo.enterprises_appid = "";
    m_LoginInfo.enterprises_name = "";
    m_LoginInfo.nickname = "";
    m_LoginInfo.nubernumber = "";
    m_LoginInfo.user_email = "";
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue result_value = jsonObj.take("data");
                    if(result_value.isObject())
                    {
                        QJsonObject result_obj = result_value.toObject();
                        QJsonValue nickname_value = result_obj.take("user_nickname");
                        m_LoginInfo.nickname = nickname_value.toString();
                        QJsonValue nubernum_value = result_obj.take("nube");
                        m_LoginInfo.nubernumber = nubernum_value.toString();
                        QJsonValue email_value = result_obj.take("user_email");
                        m_LoginInfo.user_email = email_value.toString();
                        QJsonValue oauth_token_value = result_obj.take("oauth_token");
                        m_LoginInfo.oauth_token = oauth_token_value.toString();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -2;
    }
    reply->deleteLater();
    if(1000 == iRet)
        return 0;
    else
        return -2;
}
int CNetInterfaceSDK::AuthorizeReplynalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if(jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("result"))
                {
                    QJsonValue result_value = jsonObj.take("result");
                    if(result_value.isObject())
                    {
                        QJsonObject result_obj = result_value.toObject();
                        QJsonValue token_value = result_obj.take("token");
                        m_StrToken = token_value.toString();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    if(state_value.isObject())
                    {
                        QJsonObject state_obj = state_value.toObject();
                        QJsonValue message_value = state_obj.take("msg");
                        if(message_value.isString())
                        {
                            qDebug() << message_value.toString();
                        }
                        QJsonValue rc_value = state_obj.take("rc");
                        iRet =  rc_value.toInt();
                    }
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}
int CNetInterfaceSDK::GetChannelListReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    int iCount;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue data_value = jsonObj.take("data");
                    if(data_value.isObject())
                    {
                        QJsonObject dataObj;
                        dataObj = data_value.toObject();
                        QJsonValue totalCount_value = dataObj.take("totalcount");
                        iCount = totalCount_value.toInt();
                        QJsonValue rows_value = dataObj.take("rows");
                        if(rows_value.isArray())
                        {
                            QJsonArray jsonArry = rows_value.toArray();
                            for(int i = 0; i < jsonArry.count(); i++)
                            {
                                QJsonValue element_value = jsonArry.at(i);
                                if(element_value.isObject())
                                {
                                    tagChannelInfo channelInfo;
                                    QJsonObject elementObj = element_value.toObject();
                                    if(elementObj.contains("name"))
                                    {
                                        QJsonValue name_value = elementObj.take("name");
                                        QString ChannelName = name_value.toString();
                                        channelInfo.name = ChannelName;
                                    }
                                    if(elementObj.contains("id"))
                                    {
                                        QJsonValue id_value = elementObj.take("id");
                                        QString idValue = id_value.toString();
                                        channelInfo.id = idValue;
                                    }
                                    if(elementObj.contains("streamUrl"))
                                    {
                                        QJsonValue pushstreamurl_value = elementObj.take("streamUrl");
                                        QString pushstreamurlValue = pushstreamurl_value.toString();
                                        channelInfo.pushstreamurl = pushstreamurlValue;
                                    }
                                    if(elementObj.contains("defaultAnchorFlag"))
                                    {
                                        QJsonValue defaultAnchorFlag_value = elementObj.take("defaultAnchorFlag");
                                        channelInfo.defaultAnchorFlag = defaultAnchorFlag_value.toInt();
                                    }
                                    if(elementObj.contains("channelnumber"))
                                    {
                                        QJsonValue channelnumber_value = elementObj.take("channelnumber");
                                        QString channelnumberValue = channelnumber_value.toString();
                                        channelInfo.channelnumber= channelnumberValue;
                                    }
                                    if(elementObj.contains("appKey"))
                                    {
                                        QJsonValue appKey_value = elementObj.take("appKey");
                                        QString appKeyValue = appKey_value.toString();
                                        channelInfo.appKey= appKeyValue;
                                    }
                                    if(elementObj.contains("interactiveNumber"))
                                    {
                                        QJsonValue interactivenumber_value = elementObj.take("interactiveNumber");
                                        QString interactivenumberValue = interactivenumber_value.toString();
                                        channelInfo.interactivenumber= interactivenumberValue;
                                    }
                                    if(elementObj.contains("playUrl"))
                                    {
                                        QJsonValue playurls_value = elementObj.take("playUrl");
                                        QString playlurls = playurls_value.toString();
                                        int iIndex = playlurls.indexOf(',');
                                        channelInfo.flv_playurl = playlurls.mid(0,iIndex);
                                        channelInfo.m3u8_playurl = playlurls.mid(iIndex + 1);
                                        qDebug() << channelInfo.flv_playurl;
                                        qDebug() << channelInfo.m3u8_playurl;
                                    }
                                    m_channelList.append(channelInfo);
                                }
                            }
                        }
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    reply->deleteLater();
    m_Index++;
    if((m_Index <= iCount/10 && 0 == iCount%10) || (m_Index <= iCount/10+1 &&iCount%10 > 0))
    {
        QString url = "%1/webapi/livechannel/getlist?token=%2&creator=%3&startindex=%4&pagecount=%5";
        QString qstrUrl = url.arg(m_strUrlPre).arg(m_StrToken).arg(m_LoginInfo.number).arg(m_Index).arg(10);
        qDebug() << qstrUrl;
        m_pGetRequest.setUrl(QUrl(qstrUrl));
        Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
        QNetworkReply* reply = m_pGetListNetManager->get(m_pGetRequest);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec(QEventLoop::ExcludeUserInputEvents);
        iRet = GetChannelListReplyAnalysis(reply);
    }
    m_Index = 1;
    if(1000 == iRet)
        return 0;
    return iRet;
}
int CNetInterfaceSDK::GetServerListReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("result"))
                {
                    QJsonValue result_value = jsonObj.take("result");
                    if(result_value.isArray())
                    {
                        QJsonArray result_arr = result_value.toArray();
                        for(int i = 0; i < result_arr.count(); i++)
                        {
                            QJsonValue element_value = result_arr.at(i);
                            if(element_value.isObject())
                            {
                                tagServerInfo* serverInfo = new tagServerInfo;
                                QJsonObject elementObj = element_value.toObject();
                                QJsonValue name_val = elementObj.take("name");
                                serverInfo->name = name_val.toString();
                                QJsonValue ip_val = elementObj.take("ip");
                                serverInfo->ip = ip_val.toString();
                                QJsonValue port_val = elementObj.take("port");
                                serverInfo->port = port_val.toInt();
                                m_serverList.append(serverInfo);
                            }
                        }
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    reply->deleteLater();
    return iRet;
}
int CNetInterfaceSDK::ChannelListCopy(ChannelInfo** pChannelInfo, int* pChannelCount)
{
    m_channeCount = m_channelList.count();
    ChannelInfo *pTmpChannelInfo = (ChannelInfo*)malloc(m_channeCount * sizeof(ChannelInfo));
    m_pChannelInfo = pTmpChannelInfo;
    *pChannelInfo = pTmpChannelInfo;
    *pChannelCount = m_channeCount;
    for(int i = 0; i < m_channeCount; i++,pTmpChannelInfo++)
    {
        unsigned int channelId = m_channelList[i].id.toUInt();
        pTmpChannelInfo->nChannelId = channelId;
        unsigned int AnchorFlag = m_channelList[i].defaultAnchorFlag;
        pTmpChannelInfo->nDefaultAnchorFlag = AnchorFlag;
        pTmpChannelInfo->pChannelName = (char*)malloc(strlen(m_channelList[i].name.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pChannelName,m_channelList[i].name.toUtf8().data());
        pTmpChannelInfo->pChannelNumber = (char*)malloc(strlen(m_channelList[i].channelnumber.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pChannelNumber,m_channelList[i].channelnumber.toUtf8().data());
        pTmpChannelInfo->pInteractionNube = (char*)malloc(strlen(m_channelList[i].interactivenumber.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pInteractionNube,m_channelList[i].interactivenumber.toUtf8().data());
        pTmpChannelInfo->pAppKey = (char*)malloc(strlen(m_channelList[i].appKey.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pAppKey,m_channelList[i].appKey.toUtf8().data());
        pTmpChannelInfo->pPushUrl = (char*)malloc(strlen(m_channelList[i].pushstreamurl.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pPushUrl,m_channelList[i].pushstreamurl.toUtf8().data());
        pTmpChannelInfo->pPlayUrl_flv = (char*)malloc(strlen(m_channelList[i].flv_playurl.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pPlayUrl_flv,m_channelList[i].flv_playurl.toUtf8().data());
        pTmpChannelInfo->pPlayUrl_m3u8 = (char*)malloc(strlen(m_channelList[i].m3u8_playurl.toUtf8().data())+1);
        strcpy(pTmpChannelInfo->pPlayUrl_m3u8,m_channelList[i].m3u8_playurl.toUtf8().data());
    }
    m_channelList.clear();
    return 0;
}
int CNetInterfaceSDK::ServerListCopy(NodeInfo** pNodeInfo, int* pServerCount)
{
    m_serverCount = m_serverList.count();
    NodeInfo *pTmpNodeInfo = (NodeInfo*)malloc(m_serverCount * sizeof(NodeInfo));
    m_pNodeInfo = pTmpNodeInfo;
    *pNodeInfo = pTmpNodeInfo;
    *pServerCount = m_serverCount;
    for(int i = 0; i < m_serverCount; i++,pTmpNodeInfo++)
    {
        pTmpNodeInfo->port = m_serverList[i]->port;
        pTmpNodeInfo->pdescribe = (char*)malloc(strlen(m_serverList[i]->name.toUtf8().data())+1);
        strcpy(pTmpNodeInfo->pdescribe,m_serverList[i]->name.toUtf8().data());
        strcpy(pTmpNodeInfo->ip,m_serverList[i]->ip.toUtf8().data());
    }
    return 0;
}
int CNetInterfaceSDK::StartLiveReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}
int CNetInterfaceSDK::CloseLiveReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    iRet = state_value.toInt();
                }
            }
        }
    }
    else
    {
        iRet = -1;
    }
    reply->deleteLater();
    return iRet;
}
int CNetInterfaceSDK::PauseLiveReplyAnalysis(QNetworkReply* reply)
{
    int iRet = -1;
    QByteArray data = reply->readAll();
    qDebug() << data;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,QString(data).toLocal8Bit().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    if(state_value.isObject())
                    {
                        QJsonObject state_obj = state_value.toObject();
                        QJsonValue message_value = state_obj.take("msg");
                        if(message_value.isString())
                        {
                            qDebug() << message_value.toString();
                        }
                        QJsonValue rc_value = state_obj.take("rc");
                        iRet = rc_value.toInt();
                    }
                }
            }
        }
    }
    reply->deleteLater();
    return iRet;
}

