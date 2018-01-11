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

    if(strUrl.isEmpty())
    {
        return -1;
    }
    else
    {
        m_strUrlPre = strUrl;
        return 0;
    }
}

int CNetInterfaceSDK::Login(const QString& strName,const QString& strPwd,int iUsrType)
{
    if (strName.isEmpty() || strPwd.isEmpty()) return -2;
    m_LoginInfo.number = strName;
    ///////////////////////////密码使用MD5加密/////////////////////////////////////
    QString qstrPwdMD5;
    QByteArray byteFront,byteBehind;
    QCryptographicHash hashMD5(QCryptographicHash::Md5);
    byteFront.append(strPwd.toLocal8Bit());
    hashMD5.addData(byteFront);
    byteBehind = hashMD5.result();
    qstrPwdMD5.append(byteBehind.toHex());
    //////////////////////////////////////////////////////////////////////////////

    QString para ="{\"account\": \"%1\",\"pwd\":\"%2\"}" ;
    QString qstrPara = para.arg(strName).arg(qstrPwdMD5);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/internal/usergroup/login";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrPara = %s", __FUNCTION__,qstrPara.toStdString().c_str());

    if(!m_pNetManager)
        m_pNetManager = new QNetworkAccessManager;
    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);
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
int CNetInterfaceSDK::Authorize(/*const QString& strAppId*/)
{
    QString strAppId = m_LoginInfo.enterprises_appid;
    if (strAppId.isEmpty()) return -1;
    QString para ="{\"appid\": \"%1\"}" ;
    QString qstrPara = para.arg(strAppId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/account/authorize";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrPara = %s", __FUNCTION__,qstrPara.toStdString().c_str());

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
int CNetInterfaceSDK::GetChannelList(ChannelInfo** pChannelInfo, int* pChannelCount/*const QString& token,int index,int size,const QString& creator*/)
{

    m_pGetListNetManager = new QNetworkAccessManager;

    qDebug() << "CNetInterfaceSDK:" << QThread::currentThreadId();
    if(m_strUrlPre.isEmpty() || m_StrToken.isEmpty())return -1;
    QString url = "%1/webapi/livechannel/getlist?token=%2&creator=%3&startindex=%4&pagecount=%5";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_StrToken).arg(m_LoginInfo.number).arg(1).arg(10);
    qDebug() << qstrUrl;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());

    m_pGetRequest.setUrl(QUrl(qstrUrl));
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
    if(m_strUrlPre.isEmpty() || m_StrToken.isEmpty())return -1;
    QString url = "%1/webapi/LiveChannel/GetPushStreamServer?token=%2";
    QString qstrUrl = url.arg(m_strUrlPre).arg(m_StrToken);
    qDebug() << qstrUrl;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    QNetworkReply* reply = m_pNetManager->get(m_pGetRequest);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    int iRet = GetServerListReplyAnalysis(reply);
//    if(0 == iRet)
//    {
//        return ServerListCopy(pNodeInfo,pNodeCount);
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
    if(!m_pCurrentChannelInfo || m_strUrlPre.isEmpty() || m_StrToken.isEmpty()) return -1;
    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(m_StrToken).arg(m_pCurrentChannelInfo->nChannelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/start";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrPara = %s", __FUNCTION__,qstrPara.toStdString().c_str());

    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);

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
    if(!m_pCurrentChannelInfo || m_strUrlPre.isEmpty() || m_StrToken.isEmpty()) return -1;
    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(m_StrToken).arg(m_pCurrentChannelInfo->nChannelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/close";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrPara = %s", __FUNCTION__,qstrPara.toStdString().c_str());

    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    return CloseLiveReplyAnalysis(reply);
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
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);

    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrPara = %s", __FUNCTION__,qstrPara.toStdString().c_str());

    QNetworkReply* reply = m_pNetManager->post(m_pPostRequest,bytePara);
//    QEventLoop loop;
//    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
//    loop.exec(QEventLoop::ExcludeUserInputEvents);
//    return PauseLiveReplyAnalysis(reply);
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
            return PauseLiveReplyAnalysis(reply);
        }
    } else {  // 处理超时
        disconnect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        reply->abort();
        reply->deleteLater();
        return -2;
    }
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
                    if(result_value.isObject())
                    {
                        QJsonObject result_obj = result_value.toObject();
                        QJsonValue nickname_value = result_obj.take("nickname");
                        m_LoginInfo.nickname = nickname_value.toString();
                        QJsonValue nubernum_value = result_obj.take("nubernum");
                        m_LoginInfo.nubernumber = nubernum_value.toString();
                        QJsonValue email_value = result_obj.take("email");
                        m_LoginInfo.user_email = email_value.toString();
                        QJsonValue enterprises_value = result_obj.take("enterprises");
                        QJsonArray enterprises_arr = enterprises_value.toArray();
                        for(int i = 0; i < enterprises_arr.count(); i++)
                        {
                            QJsonValue enterprises_obj_value =  enterprises_arr.at(i);
                            QJsonObject enterprises_obj_value_obj = enterprises_obj_value.toObject();
                            QJsonValue enterprises_name_value = enterprises_obj_value_obj.take("name");
                            m_LoginInfo.enterprises_name = enterprises_name_value.toString();
                            QJsonValue enterprises_appid_value = enterprises_obj_value_obj.take("appid");
                            m_LoginInfo.enterprises_appid = enterprises_appid_value.toString();
                        }
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
                        iRet = rc_value.toInt();
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
    if(0 == iRet)
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
                if(jsonObj.contains("pagingrows"))
                {
                    QJsonValue pagingrows_value = jsonObj.take("pagingrows");
                    if(pagingrows_value.isObject())
                    {
                        QJsonObject jsonPagingRowsObj;
                        jsonPagingRowsObj = pagingrows_value.toObject();
                        QJsonValue totalCount_value = jsonPagingRowsObj.take("totalcount");
                        iCount = totalCount_value.toInt();
                        QJsonValue rows_value = jsonPagingRowsObj.take("rows");
                        if(rows_value.isArray())
                        {
                            QJsonArray jsonArry = rows_value.toArray();
                            for(int i = 0; i < jsonArry.count(); i++)
                            {
                                QJsonValue element_value = jsonArry.at(i);
                                if(element_value.isObject())
                                {
//                                    tagChannelInfo* channelInfo = new tagChannelInfo;
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
                                        int idValue = id_value.toInt();
                                        channelInfo.id = QString::number(idValue);
                                    }
                                    if(elementObj.contains("pushstreamurl"))
                                    {
                                        QJsonValue pushstreamurl_value = elementObj.take("pushstreamurl");
                                        QString pushstreamurlValue = pushstreamurl_value.toString();
                                        channelInfo.pushstreamurl = pushstreamurlValue;
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
                                    if(elementObj.contains("interactivenumber"))
                                    {
                                        QJsonValue interactivenumber_value = elementObj.take("interactivenumber");
                                        QString interactivenumberValue = interactivenumber_value.toString();
                                        channelInfo.interactivenumber= interactivenumberValue;
                                    }
                                    if(elementObj.contains("playurls"))
                                    {
                                        QJsonValue playurls_value = elementObj.take("playurls");
                                        QJsonArray playurls_arr = playurls_value.toArray();
                                        for(int i = 0; i < playurls_arr.count(); i++)
                                        {
                                            QJsonValue playurls_obj_value =  playurls_arr.at(i);
                                            QJsonObject playurls_obj_value_obj = playurls_obj_value.toObject();
                                            QJsonValue playurls_streamtype_value = playurls_obj_value_obj.take("streamtype");
                                            QString streamType = playurls_streamtype_value.toString();
                                            QJsonValue url_value = playurls_obj_value_obj.take("url");
                                            if(streamType == "flv")
                                            {
                                                channelInfo.flv_playurl = url_value.toString();
                                            }
                                            else if (streamType == "m3u8")
                                            {
                                                channelInfo.m3u8_playurl = url_value.toString();
                                            }
                                        }
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
int CNetInterfaceSDK::CloseLiveReplyAnalysis(QNetworkReply* reply)
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
/*
int CNetInterfaceSDK::Login(const QString& strName,const QString& strPwd)
{
    QString url = "%1/external/orginalService/login?login=%2&password=%3&userType=%4";
    QString qstrUrl = url.arg(m_strUrlPre).arg(strName).arg(strPwd).arg(2);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    qDebug() << qstrUrl;
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pReply = m_pLoginNetManager->get(m_pGetRequest);
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s m_pLoginNetManager == NULL", __FUNCTION__);
    return 0;
}

void CNetInterfaceSDK::GetChannelList(const QString& token,int index,int size,const QString& creator)
{
    if (token.isEmpty())return;
    QString url = "%1/external/externalService?service=getAnchorChannelList&params={\"token\":\"%2\",\"type\":\"%3\",\"idx\":%4,\"size\":%5}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token).arg(2).arg(index).arg(size);
    m_pGetRequest.setUrl(QUrl(qstrUrl));
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pReply = m_pChannelNetManager->get(m_pGetRequest);
}

void CNetInterfaceSDK::StartLive(QString token,int channelId)
{
    if (token.isEmpty())return;
    QString url = "%1/external/externalService?service=onLive&params={\"token\":\"%2\",\"userType\":%3,\"channelId\":\"%4\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token).arg(2).arg(channelId);
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    qDebug() << qstrUrl;
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pReply = m_pStartLiveNetManager->get(m_pGetRequest);
}
void CNetInterfaceSDK::CloseLive(QString token,int channelId)
{
    if (token.isEmpty()) return;
    QString url = "%1/external/externalService?service=overLive&params={\"token\":\"%2\",\"userType\":%3,\"channelId\":\"%4\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token).arg(2).arg(channelId);
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    qDebug() << qstrUrl;
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pReply = m_pCloseLiveNetManager->get(m_pGetRequest);
}

void CNetInterfaceSDK::PauseLive(QString token,int channelId)
{
    if (token.isEmpty()) return;
    QString url = "%1/external/externalService?service=overLive&params={\"token\":\"%2\",\"userType\":%3,\"channelId\":\"%4\"}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token).arg(2).arg(channelId);
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    qDebug() << qstrUrl;
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s qstrUrl = %s", __FUNCTION__,qstrUrl.toStdString().c_str());
    m_pReply = m_pPauseLiveNetManager->get(m_pGetRequest);
}

void CNetInterfaceSDK::LoginReplyFinished(QNetworkReply* reply)
{
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s", __FUNCTION__);
    m_LoginInfo.uid = "";
    m_LoginInfo.channelId = "";
    m_LoginInfo.enterprises_appid = "";
    m_LoginInfo.enterprises_name = "";
    m_LoginInfo.nickname = "";
    m_LoginInfo.nubernumber = "";
    m_LoginInfo.user_email = "";
    QByteArray data = reply->readAll();
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,data.toStdString().data());
    qDebug() << "Login info:"<<data;
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        LoginCB(true,data.data());
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
                        QJsonValue uid_value = result_obj.take("uid");
                        m_LoginInfo.uid = uid_value.toString();
                        QJsonValue channelId_value = result_obj.take("channelId");
                        m_LoginInfo.channelId = channelId_value.toString();
                        QJsonValue nickname_value = result_obj.take("user_nickname");
                        m_LoginInfo.nickname = nickname_value.toString();
                        QJsonValue oauth_token_value = result_obj.take("oauth_token");
                        m_LoginInfo.oauth_token = oauth_token_value.toString();
                        m_StrToken = m_LoginInfo.oauth_token;
                    }
                }
                QJsonValue message_value;
                if(jsonObj.contains("message"))
                {
                    message_value = jsonObj.take("message");
                    qDebug() << message_value.toString();
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                }
            }
        }
    }
    else
    {
        LoginCB(false,data.data());
    }
    reply->deleteLater();
    qDebug() << data;
}

void CNetInterfaceSDK::ChannelReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    int iCount;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        GetChannelListCB(true,data.data());
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                if(jsonObj.contains("data"))
                {
                    QJsonValue pagingrows_value = jsonObj.take("data");
                    if(pagingrows_value.isObject())
                    {
                        QJsonObject jsonPagingRowsObj;
                        jsonPagingRowsObj = pagingrows_value.toObject();
                        QJsonValue totalCount_value = jsonPagingRowsObj.take("totalCount");
                        int iCount = totalCount_value.toInt();
                        QJsonValue rows_value = jsonPagingRowsObj.take("rows");
                        if(rows_value.isArray())
                        {
                            QJsonArray jsonArry = rows_value.toArray();
                            for(int i = 0; i < jsonArry.count(); i++)
                            {
                                QJsonValue element_value = jsonArry.at(i);
                                if(element_value.isObject())
                                {
                                    tagChannelInfo* channelInfo = new tagChannelInfo;
                                    QJsonObject elementObj = element_value.toObject();
                                    if(elementObj.contains("name"))
                                    {
                                        QJsonValue name_value = elementObj.take("name");
                                        QString ChannelName = name_value.toString();
                                        channelInfo->name = ChannelName;
                                    }
                                    if(elementObj.contains("id"))
                                    {
                                        QJsonValue id_value = elementObj.take("id");
                                        channelInfo->id = id_value.toString();
                                    }
                                    if(elementObj.contains("streamUrl"))
                                    {
                                        QJsonValue pushstreamurl_value = elementObj.take("streamUrl");
                                        QString pushstreamurlValue = pushstreamurl_value.toString();
                                        channelInfo->pushstreamurl = pushstreamurlValue;
                                    }
                                    if(elementObj.contains("channelnumber"))
                                    {
                                        QJsonValue channelnumber_value = elementObj.take("channelnumber");
                                        QString channelnumberValue = channelnumber_value.toString();
                                        channelInfo->channelnumber= channelnumberValue;
                                    }
                                    if(elementObj.contains("interactiveNumber"))
                                    {
                                        QJsonValue interactivenumber_value = elementObj.take("interactiveNumber");
                                        QString interactivenumberValue = interactivenumber_value.toString();
                                        channelInfo->interactivenumber= interactivenumberValue;
                                    }
                                    m_channelList.append(channelInfo);
                                }
                            }
                        }
                    }
                }
                QJsonValue message_value;
                if(jsonObj.contains("message"))
                {
                    message_value = jsonObj.take("message");
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                }
            }
        }
    }
    else
    {
        GetChannelListCB(false,data.data());
    }
    reply->deleteLater();
    m_Index++;
    if((m_Index <= iCount/10 && 0 == iCount%10) || (m_Index <= iCount/10+1 &&iCount%10 > 0))
    {
        GetChannelList(m_StrToken,10,m_Index,m_LoginInfo.nubernumber);
    }
}

void CNetInterfaceSDK::StartLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,data.toStdString().data());
    if(reply->error() > 0)
    {
        return;
    }
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        StartLiveCB(true,data.data());
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();
                QJsonValue message_value;
                if(jsonObj.contains("message"))
                {
                    message_value = jsonObj.take("message");
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    if(1000 == state_value.toInt())
                    {
//                        if(m_pPoleLive)
//                            m_pPoleLive->StartLive();

                    }
                    else
                    {
//                        if(m_pPoleLive)
//                        {
//                            CMessageBoxUI* msgBox = new CMessageBoxUI(m_pPoleLive,QStringLiteral("提示信息"),QStringLiteral("开启直播失败！"));
//                            msgBox->show();
//                        }
                    }
                }
            }
        }
    }
    else
    {
        StartLiveCB(false,data.data());
    }
    reply->deleteLater();
    qDebug() << data;
}

void CNetInterfaceSDK::CloseLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,data.toStdString().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        CloseLiveCB(true,data.data());
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();

                QJsonValue message_value;
                if(jsonObj.contains("message"))
                {
                    message_value = jsonObj.take("message");
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                }
            }
        }
    }
    else
    {
        CloseLiveCB(false,data.data());
    }
    reply->deleteLater();
    qDebug() << data;

}
void CNetInterfaceSDK::PauseLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s data = %s", __FUNCTION__,data.toStdString().data());
    QJsonParseError  jsonError;
    QJsonObject jsonObj;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(data, &jsonError);
    if (jsonError.error == QJsonParseError::NoError)
    {
        PauseLiveCB(true,data.data());
        if (!(jsonDocument.isNull() || jsonDocument.isEmpty()))
        {
            if (jsonDocument.isObject())
            {
                jsonObj = jsonDocument.object();

                QJsonValue message_value;
                if(jsonObj.contains("message"))
                {
                    message_value = jsonObj.take("message");
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                }
            }
        }
    }
    else
    {
        PauseLiveCB(false,data.data());
    }
    reply->deleteLater();
    qDebug() << data;
}
*/


