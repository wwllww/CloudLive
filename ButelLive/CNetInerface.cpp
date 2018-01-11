#include "CNetInerface.h"
#include <QObject>
#include <QDebug>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QEventLoop>
#include "ConfigOper.h"
CNetInerface *CNetInerface::m_pInstance = NULL;
CNetInerface::CNetInerface(QObject* parent):QObject(parent)
{
    m_Index = 1;
    m_strUrlPre = ConfigOper::instance()->m_StrUrl;
    m_pLoginNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pLoginNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(LoginReplyFinished(QNetworkReply*)));
    m_pAuthorizeNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pAuthorizeNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(AuthorizeReplyFinished(QNetworkReply*)));
    m_pChannelNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pChannelNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(ChannelReplyFinished(QNetworkReply*)));
    m_pStartLiveNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pStartLiveNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(StartLiveReplyFinished(QNetworkReply*)));
    m_pCloseLiveNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pCloseLiveNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(CloseLiveReplyFinished(QNetworkReply*)));
    m_pPauseLiveNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pPauseLiveNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(PauseLiveReplyFinished(QNetworkReply*)));
    m_pServerNetManager = new QNetworkAccessManager(this);
    QObject::connect(m_pServerNetManager,SIGNAL(finished(QNetworkReply*)),this,SLOT(GetPushStreamServerListReplyFinished(QNetworkReply*)));
}

CNetInerface::~CNetInerface()
{
    qDeleteAll(m_channelList);
}
CNetInerface * CNetInerface::GetInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CNetInerface;
    }
    return m_pInstance;
}

void CNetInerface::Destroy()
{
    if (NULL != m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}
//设置服务器地址
int CNetInerface::SetSerAddr(QString strHttpPre)
{
    if (strHttpPre.isEmpty())
    {
        return -1;
    }
    m_strUrlPre = strHttpPre;
    return 0;
}

int CNetInerface::Login(QString strName, QString strPwd)
{
    if (strName.isEmpty() || strPwd.isEmpty()) return -1;

    ///////////////////////////密码使用MD5加密/////////////////////////////////////
    QString qstrPwdMD5;
    QByteArray byteFront,byteBehind;
    QCryptographicHash hashMD5(QCryptographicHash::Md5);
    byteFront.append(strPwd.toLocal8Bit());
    hashMD5.addData(byteFront);
    byteBehind = hashMD5.result();
    qstrPwdMD5.append(byteBehind.toHex());
    ////////////////////////////////////////////////////////////////////////////////////

    QString para ="{\"account\": \"%1\",\"pwd\":\"%2\"}" ;
    QString qstrPara = para.arg(strName).arg(qstrPwdMD5);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/internal/usergroup/login";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    m_pReply = m_pLoginNetManager->post(m_pPostRequest,bytePara);
    return 0;
}

int CNetInerface::Authorize(QString strAppId)
{
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
    m_pReply= m_pAuthorizeNetManager->post(m_pPostRequest,bytePara);
    return 0;
}

int CNetInerface::GetChannelList(QString token,QString creator,int index,int size)
{
    if (token.isEmpty())return -1;
    QString url = "%1/webapi/livechannel/getlist?token=%2&creator=%3&startindex=%4&pagecount=%5}";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token).arg(creator).arg(index).arg(size);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    m_pReply = m_pChannelNetManager->get(m_pGetRequest);
    return 0;
}

int CNetInerface::GetPushStreamServerList(const QString& token)
{
    if (token.isEmpty())return -1;
    QString url = "%1/webapi/LiveChannel/GetPushStreamServer?token=%2";
    QString qstrUrl = url.arg(m_strUrlPre).arg(token);
    qDebug() << qstrUrl;
    m_pGetRequest.setUrl(QUrl(qstrUrl));
    m_pReply = m_pServerNetManager->get(m_pGetRequest);
    return 0;
}

int CNetInerface::StartLive(QString token,int channelId)
{
    if (token.isEmpty()) return -1;

    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(token).arg(channelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/start";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    m_pReply = m_pStartLiveNetManager->post(m_pPostRequest,bytePara);
    return 0;

}
int CNetInerface::CloseLive(QString token,int channelId)
{
    if (token.isEmpty()) return -1;

    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(token).arg(channelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/close";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    m_pReply = m_pCloseLiveNetManager->post(m_pPostRequest,bytePara);
    return 0;

}
int CNetInerface::PauseLive(QString token,int channelId)
{
    if (token.isEmpty()) return -1;
    QString para ="{\"token\": \"%1\",\"channelid\":\"%2\"}" ;
    QString qstrPara = para.arg(token).arg(channelId);
    QByteArray bytePara = qstrPara.toLocal8Bit();
    int paraLength = bytePara.length();
    QString url = "%1/webapi/livechannel/pause";
    QString qstrUrl = url.arg(m_strUrlPre);
    m_pPostRequest.setUrl(QUrl(qstrUrl));
    m_pPostRequest.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    m_pPostRequest.setHeader(QNetworkRequest::ContentLengthHeader,paraLength);
    m_pReply = m_pPauseLiveNetManager->post(m_pPostRequest,bytePara);
    return 0;

}
/***************************登录返回报文解析*****************************************/
void CNetInerface::LoginReplyFinished(QNetworkReply* reply)
{
    m_LoginInfo.enterprises_appid = "";
    m_LoginInfo.enterprises_name = "";
    m_LoginInfo.nickname = "";
    m_LoginInfo.nubernumber = "";
    m_LoginInfo.user_email = "";
    QByteArray data = reply->readAll();
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
                        qDebug() << rc_value.toInt();
                        m_pLoginUI->LoginStateInfo(rc_value.toInt()); //登录信息显示到登录界面
                        if(rc_value.toInt() != 0) return;
                    }
                }
            }
        }
    }
    qDebug() << "**********************************";
    qDebug() << m_LoginInfo.nickname <<m_LoginInfo.nubernumber << m_LoginInfo.user_email << m_LoginInfo.enterprises_name << m_LoginInfo.enterprises_appid;
    qDebug() << "**********************************";
    reply->deleteLater();
    qDebug() << data;
    Authorize(m_LoginInfo.enterprises_appid);
}

/***************************鉴权返回报文解析*****************************************/
void CNetInerface::AuthorizeReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
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
                        qDebug() << rc_value.toInt();
                        m_pLoginUI->AuthorizeStateInfo(rc_value.toInt()); //鉴权信息显示到登录界面
                    }
                }
            }
        }
    }
    qDebug() << "**********************************";
    qDebug() << m_StrToken;
    qDebug() << "**********************************";
    reply->deleteLater();
    qDebug() << data;
}

/***************************获取频道列表返回报文解析********************************/
void CNetInerface::ChannelReplyFinished(QNetworkReply* reply)
{
//    m_pChannelAndProgramUI->ClearAllChannelItem();
//    qDeleteAll(m_channelList);
//    m_channelList.clear();
    QByteArray data = reply->readAll();
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
                        if(iCount <=0)
                            m_pChannelAndProgramUI->SetInformation(tr("The channel list is empty!"));
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
                                        double idValue = id_value.toDouble();
                                        channelInfo->id = QString::number(idValue);
                                    }
                                    if(elementObj.contains("pushstreamurl"))
                                    {
                                        QJsonValue pushstreamurl_value = elementObj.take("pushstreamurl");
                                        QString pushstreamurlValue = pushstreamurl_value.toString();
                                        channelInfo->pushstreamurl = pushstreamurlValue;
                                    }
                                    if(elementObj.contains("channelnumber"))
                                    {
                                        QJsonValue channelnumber_value = elementObj.take("channelnumber");
                                        QString channelnumberValue = channelnumber_value.toString();
                                        channelInfo->channelnumber= channelnumberValue;
                                    }
                                    if(elementObj.contains("appKey"))
                                    {
                                        QJsonValue appKey_value = elementObj.take("appKey");
                                        QString appKeyValue = appKey_value.toString();
                                        channelInfo->appKey= appKeyValue;
                                    }
                                    if(elementObj.contains("interactivenumber"))
                                    {
                                        QJsonValue interactivenumber_value = elementObj.take("interactivenumber");
                                        QString interactivenumberValue = interactivenumber_value.toString();
                                        channelInfo->interactivenumber= interactivenumberValue;
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
                                                channelInfo->flv_playurl = url_value.toString();
                                            }
                                            else if (streamType == "m3u8")
                                            {
                                                channelInfo->m3u8_playurl = url_value.toString();
                                            }
                                        }
                                    }
                                    m_pChannelAndProgramUI->AddChannelItem(channelInfo->name);
                                    m_channelList.append(channelInfo);
                                    qDebug() << "**********************************";
                                    qDebug() << channelInfo->id << channelInfo->name << channelInfo->channelnumber << channelInfo->pushstreamurl << channelInfo->flv_playurl << channelInfo->m3u8_playurl << channelInfo->appKey << channelInfo->interactivenumber;
                                    qDebug() << "**********************************";
                                }
                            }
                        }
                        ConfigOper::instance()->WriteCfgFile();
                    }
                }
                if(jsonObj.contains("state"))
                {
                    QJsonValue state_value = jsonObj.take("state");
                    qDebug() << state_value.toInt();
                    if(state_value.toInt() != 0)
                        m_pChannelAndProgramUI->SetInformation(tr("Failed to get the list of channels!"));
                }
            }
        }
    }
    reply->deleteLater();
    qDebug() << data;
    m_Index++;
    if((m_Index <= iCount/10 && 0 == iCount%10) || (m_Index <= iCount/10+1 &&iCount%10 > 0))
    {
        GetChannelList(m_StrToken,m_LoginInfo.nubernumber,m_Index,10);
    }
}
void CNetInerface::GetPushStreamServerListReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
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
            }
        }
    }
}
void CNetInerface::StartLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
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
                        qDebug() << rc_value.toInt();
                        if(0 == rc_value.toInt())
                        {

                        }
                        else
                        {

                        }
                    }
                }
            }
        }
    }
    reply->deleteLater();
    qDebug() << data;
}

void CNetInerface::CloseLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
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
                        if(rc_value.toInt() == 0)
                        {
                        }
                    }
                }
            }
        }
    }
    reply->deleteLater();
    qDebug() << data;
}
void CNetInerface::PauseLiveReplyFinished(QNetworkReply* reply)
{
    QByteArray data = reply->readAll();
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
                        qDebug() << rc_value.toInt();
                    }
                }
            }
        }
    }
    reply->deleteLater();
    qDebug() << data;
}
