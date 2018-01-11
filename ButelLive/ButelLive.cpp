#include "ButelLive.h"
#include "ui_ButelLive.h"
#include "InputName.h"
#include "SceneItem.h"
#include "MyMessageBox.h"
#include "MaterialItem.h"
#include "SLiveApi.h"
#include "configoper.h"
#include "LogDeliver.h"
#include "PreviewItem.h"
#include "PlayControl.h"
#include "SettingUI.h"
#include "SceneMateralItem.h"
#include "PositionSizeSet.h"
#include "CNetInerface.h"
#include "SetShortcutKey.h"

#include <Windows.h>
#include <windowsx.h>

#include <QAction>
#include <QMessageBox>
#include <QMouseEvent>
#include <QRect>
#include <QPainter>
#include <QListWidgetItem>
#include <QScrollBar>
#include <QFileDialog>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QFile>
#include <QMap>

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

class CNetInerface;
//int g_BitRate = 0;
//int g_FPS = 0;

//QStringList g_MateralDataList;
QMap<int,QString> g_MateralDataMap;   //关闭回调素材映射
QMap<int,QString> g_AgentDataMap;   //关闭回调区域站位源映射

ButelLive * g_This;

void ConvertGBKToUtf8(std::string& utf8, const char *strGBK)
{
    int len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)strGBK, -1, NULL,0);
    unsigned short * wszUtf8 = new unsigned short[len+1];
    memset(wszUtf8, 0, len * 2 + 2);
    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)strGBK, -1, (LPWSTR)wszUtf8, len);
    len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL);
    char *szUtf8 = new char[len + 1];
    memset(szUtf8, 0, len + 1);
    WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, szUtf8, len, NULL,NULL);
    utf8 = szUtf8;
    delete[] szUtf8;
    delete[] wszUtf8;
}

QJsonObject getJsonObjectFromString(const QString jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString.toStdString().c_str());
    if( jsonDocument.isNull() ){
        qDebug()<< "===> please check the string "<< jsonString.toStdString().c_str();
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}
QString getStringFromJsonObject(const QJsonObject& jsonObject){
    return QString(QJsonDocument(jsonObject).toJson());
}

QJsonObject getJsonObjectFromByteArray(const QByteArray jsonString)
{
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonString);
    if( jsonDocument.isNull() ){
        qDebug()<< "===> QJsonDocument："<< jsonString;
    }
    QJsonObject jsonObject = jsonDocument.object();
    return jsonObject;
}

//推流回调

void PushStatusCallBacFunc(uint64_t InstanceID, int BitRate, int FPS, int iStatus,bool IsReject)
{

}
void PushStatusCallBack2Func(uint64_t InstanceID, int FPS, PushStauts *Status)
{
   // g_BitRate = BitRate;
   // g_FPS = FPS;
    emit g_This->PushStreamFPSChanged(FPS,Status);
}
void ConfigCallBackFunc(uint64_t InstanceID,uint64_t StreamID,const char *ConfigParam)
{
    for(int i = 0; i < g_This->m_VideoInstanceVec.count(); i++)
    {
        if(g_This->m_VideoInstanceVec[i] == InstanceID)
        {
            std::string utf;
            ConvertGBKToUtf8(utf,ConfigParam);
            QString val(utf.c_str());
            g_MateralDataMap.insert(i,val);
            return;
        }
    }
    for(int i = 0; i < g_This->m_AgentInstanceVec.count(); i++)
    {
        if(g_This->m_AgentInstanceVec[i] == InstanceID)
        {
            std::string utf;
            ConvertGBKToUtf8(utf,ConfigParam);
            QString val(utf.c_str());
            g_AgentDataMap.insert(i,val);
            return;
        }
    }
    return;
}

void LiveAudioCallBackFunc(float LeftDb,float RightDb)
{
    emit g_This->LiveAudioDBValueChanged(LeftDb,RightDb);
}

bool /*ButelLive::*/SaveConfig(const QString& jsonFile);

ButelLive::ButelLive(tagChannelInfo* channelInfo,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ButelLive)
{
    ui->setupUi(this);
    g_This = this;
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_DeleteOnClose);

    m_pChannelInfo = channelInfo;
    qDebug() << m_pChannelInfo->pushstreamurl;

//    setAttribute(Qt::WA_TranslucentBackground, true);
//    this->setSizeGripEnabled(true);
    m_bAltKeyPressed = false;
    m_IsLeftPressDown = false;
    m_bIsListen = true;
    m_bIsPreview = false;
    m_bIsLiving = false;
    m_bIsEditing = false;
    m_bPlayLocal = true;
    m_iSceneNum = 0;
    m_iMateralNum = 0;
    m_iDianboNum = 0;
    m_iHudongNum = 0;
    m_iShexiangNum = 0;
    m_iTupianNum = 0;
    m_iDateTimeNum = 0;
    m_iTextNum = 0;
    m_iLiveNum = 0;
    m_iAudioCaptureNum = 0;
    m_iAgentNum = 0;
    m_iMonitorCaptureNum = 0;
    m_iWindowCaptureNum = 0;
    m_iProcTopWindowNum = 0;
    m_CurrentSceneNo = -1;
    m_bIsFullScreenPreview = false;
    this->m_Dir = NONE;
    this->setMinimumHeight(100);
    this->setMinimumWidth(150);
    this->setMouseTracking(true);
    ui->m_pTitleBarLabel->m_IsChildWnd = false;
    MenuInit();
    UiInit();
    InitDeviceList();
    connect(this,SIGNAL(PushStreamFPSChanged(int,PushStauts *)),this,SLOT(OnPushStreamFPSChanged(int,PushStauts *)));
    connect(this,SIGNAL(ConfigResourceInit()),this,SLOT(OnConfigResourceInit()));
    connect(this,SIGNAL(LiveAudioDBValueChanged(float,float)),this,SLOT(OnLiveAudioDBValueChanged(float,float)));

    CNetInerface* netInterface = CNetInerface::GetInstance();
    netInterface->GetPushStreamServerList(netInterface->m_StrToken);

}

ButelLive::~ButelLive()
{
    SLiveRelese();
    SaveConfig("resource.cfg");
    delete ui;
}

void ButelLive::OnPushStreamFPSChanged(int FPS,PushStauts * pPushStatus)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    QString strFPS = "FPS:" + QString::number(FPS);
    ui->m_pLbFPS->setText(strFPS);

    QString strBitRate0 = QString::number(pPushStatus[0].BitRate) + "kb/s";
    ui->m_pLbStatus0BitRate->setText(strBitRate0);
    QString strLost0 = "Lost:" + QString::number(pPushStatus[0].LostTotalNum);
    ui->m_pLbStatus0Lost->setText(strLost0);
    QString strLostPercent0= "(" + QString::number(pPushStatus[0].LostPercent,'f',2) + "%)";
    ui->m_pLbStatus0Percent->setText(strLostPercent0);

    QString colorSheet = "QLabel{background-color:rgb(%1,%2,%3);}";

    int R0 = GetRValue(pPushStatus[0].Color);
    int G0 = GetGValue(pPushStatus[0].Color);
    int B0 = GetBValue(pPushStatus[0].Color);
    QString RGB0Sheet = colorSheet.arg(R0).arg(G0).arg(B0);
    ui->m_pLbStatusRGB0->setStyleSheet(RGB0Sheet);

    QString strBitRate1 = QString::number(pPushStatus[1].BitRate) + "kb/s";
    ui->m_pLbStatus1BitRate->setText(strBitRate1);
    QString strLost1 = "Lost:" + QString::number(pPushStatus[1].LostTotalNum);
    ui->m_pLbStatus1Lost->setText(strLost1);
    QString strLostPercent1= "(" + QString::number(pPushStatus[1].LostPercent,'f',2) + "%)";
    ui->m_pLbStatus1Percent->setText(strLostPercent1);

    int R1 = GetRValue(pPushStatus[1].Color);
    int G1 = GetGValue(pPushStatus[1].Color);
    int B1 = GetBValue(pPushStatus[1].Color);
    QString RGB1Sheet = colorSheet.arg(R1).arg(G1).arg(B1);
    ui->m_pLbStatusRGB1->setStyleSheet(RGB1Sheet);

    QString strBitRate2 = QString::number(pPushStatus[2].BitRate) + "kb/s";
    ui->m_pLbStatus2BitRate->setText(strBitRate2);
    QString strLost2 = "Lost:" + QString::number(pPushStatus[2].LostTotalNum);
    ui->m_pLbStatus2Lost->setText(strLost2);
    QString strLostPercent2= "(" + QString::number(pPushStatus[2].LostPercent,'f',2) + "%)";
    ui->m_pLbStatus2Percent->setText(strLostPercent2);

    int R2 = GetRValue(pPushStatus[2].Color);
    int G2 = GetGValue(pPushStatus[2].Color);
    int B2 = GetBValue(pPushStatus[2].Color);
    QString RGB2Sheet = colorSheet.arg(R2).arg(G2).arg(B2);
    ui->m_pLbStatusRGB2->setStyleSheet(RGB2Sheet);

    QString strBitRate3 = QString::number(pPushStatus[3].BitRate) + "kb/s";
    ui->m_pLbStatus3BitRate->setText(strBitRate3);
    QString strLost3 = "Lost:" + QString::number(pPushStatus[3].LostTotalNum);
    ui->m_pLbStatus3Lost->setText(strLost3);
    QString strLostPercent3= "(" + QString::number(pPushStatus[3].LostPercent,'f',2) + "%)";
    ui->m_pLbStatus3Percent->setText(strLostPercent3);

    int R3 = GetRValue(pPushStatus[3].Color);
    int G3 = GetGValue(pPushStatus[3].Color);
    int B3 = GetBValue(pPushStatus[3].Color);
    QString RGB3Sheet = colorSheet.arg(R3).arg(G3).arg(B3);
    ui->m_pLbStatusRGB3->setStyleSheet(RGB3Sheet);
}

//根据json格式配置文件初始化场景，源等信息
bool ButelLive::InitConfig(const QString& jsonFile)
{
    QFile file(jsonFile);
    if (!file.open(QFile::ReadWrite | QFile::Text)) {
        qDebug() << "Error: Cannot open file: "
                 << qPrintable(file.errorString());
        return false;
    }
    QByteArray data = file.readAll();
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
                if(jsonObj.contains("agents"))
                {
                    m_AgentSourceList.clear();
                    QJsonArray agentArr = jsonObj.take("agents").toArray();
                    for(int i = 0; i < agentArr.count(); i++)
                    {
                        MateralInfo* pAgentInfo = new MateralInfo;
                        pAgentInfo->name = agentArr[i].toObject().take("name").toString();
                        pAgentInfo->type = agentArr[i].toObject().take("type").toInt();
                        pAgentInfo->data = getStringFromJsonObject(agentArr[i].toObject().take("data").toObject());
                        m_AgentSourceList.append(pAgentInfo);
                    }
                }
                if(jsonObj.contains("materals"))
                {
                    m_MateralList.clear();
                    QJsonArray matArr = jsonObj.take("materals").toArray();
                    for(int i = 0; i < matArr.count(); i++)
                    {
                        MateralInfo* pMatInfo = new MateralInfo;
                        pMatInfo->name = matArr[i].toObject().take("name").toString();
                        pMatInfo->type = matArr[i].toObject().take("type").toInt();
                        //pMatInfo->data = matArr[i].toObject().take("data").toString();
                        pMatInfo->data = getStringFromJsonObject(matArr[i].toObject().take("data").toObject());
                        m_MateralList.append(pMatInfo);
                    }
                }
                if(jsonObj.contains("scenes"))
                {
                    m_SceneList.clear();
                    QJsonArray scnArr = jsonObj.take("scenes").toArray();
                    for(int i = 0; i < scnArr.count(); i++)
                    {
                        SceneInfo* pScnInfo = new SceneInfo;
                        pScnInfo->name = scnArr[i].toObject().take("name").toString();
                        pScnInfo->bIsReserve = scnArr[i].toObject().take("bIsReserve").toBool();
                        pScnInfo->shortcutKeys = scnArr[i].toObject().take("shortcutKeys").toString();
                        QJsonArray linkMatArr = scnArr[i].toObject().take("sources").toArray();
                        for(int j = 0; j < linkMatArr.count(); j++)
                        {
//                            pScnInfo->m_LinkMateralVec.append(linkMatArr[j].toInt());
                            LinkSourceInfo sourceInfo;
                            sourceInfo.id = linkMatArr[j].toObject().take("id").toInt();
                            sourceInfo.left = linkMatArr[j].toObject().take("left").toInt();
                            sourceInfo.top = linkMatArr[j].toObject().take("top").toInt();
                            sourceInfo.width = linkMatArr[j].toObject().take("width").toInt();
                            sourceInfo.height = linkMatArr[j].toObject().take("height").toInt();
                            sourceInfo.bRender = linkMatArr[j].toObject().take("bRender").toBool();
                            sourceInfo.bIsAgent = linkMatArr[j].toObject().take("bIsAgent").toBool();

                            sourceInfo.cutLeft = linkMatArr[j].toObject().take("cutLeft").toInt();
                            sourceInfo.cutTop = linkMatArr[j].toObject().take("cutTop").toInt();
                            sourceInfo.cutWidth = linkMatArr[j].toObject().take("cutWidth").toInt();
                            sourceInfo.cutHeight = linkMatArr[j].toObject().take("cutHeight").toInt();
                            sourceInfo.bIsCut = linkMatArr[j].toObject().take("bIsCut").toBool();

                            pScnInfo->m_LinkMateralVec.append(sourceInfo);

                        }
                        m_SceneList.append(pScnInfo);
                    }
                }
            }
        }
    }
    file.close();

    Log::writeMessage(LOG_RTSPSERV, 1, "%s materals init!", __FUNCTION__);


    for(int i = 0; i < m_MateralList.count(); i++)
    {
        int iType = m_MateralList[i]->type;
        if(iType == Dianbo || iType == Hudong || iType == Shexiang || iType == Live)
        {
            CPreviewItem* preItem = new CPreviewItem(this,m_MateralList[i]->name,iType,ui->m_pLWPreview->count());
            QListWidgetItem* item  = new QListWidgetItem;
            item->setSizeHint(QSize(210,150));
            ui->m_pLWPreview->addItem(item);
            ui->m_pLWPreview->setItemWidget(item,preItem);
            uint64_t tmpInstance;
            uint64_t hwnd = preItem->m_PreviewArea->winId();
            if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
                delete preItem;
                delete item;
                return false;
            }
            m_VideoInstanceVec.append(tmpInstance);
            QString strJson_obj;
            if(iType == Dianbo)
            {
                strJson_obj = "{\"ClassName\":\"VideoSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType == Shexiang)
            {
                strJson_obj = "{\"ClassName\":\"DeviceSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType == Hudong)
            {
                strJson_obj = "{\"ClassName\":\"PipeVideo\",\"NubeNum\":\"%1\",\"AppKey\":\"%2\",\"NickName\":\"%3\",\"data\":\%4\}";
                strJson_obj = strJson_obj.arg(m_pChannelInfo->interactivenumber).arg(m_pChannelInfo->appKey).arg("testname").arg(m_MateralList.at(i)->data);
            }
            else if(iType == Live)
            {
                QJsonArray channelArr;
                CNetInerface* pNetInstance = CNetInerface::GetInstance();
                for(int i = 0; i < pNetInstance->m_channelList.count(); i++)
                {
                    QJsonObject channel_item_obj;
                    channel_item_obj.insert("name", pNetInstance->m_channelList[i]->name);
                    channel_item_obj.insert("flv", pNetInstance->m_channelList[i]->flv_playurl);
                    channel_item_obj.insert("m3u8", pNetInstance->m_channelList[i]->m3u8_playurl);
                    channelArr.append(channel_item_obj);
                }
                QByteArray byte_array = QJsonDocument(channelArr).toJson();
                QString playList(byte_array);
                strJson_obj = "{\"ClassName\":\"VideoLiveSource\",\"data\":\%1\,\"PlayList\":%2}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data).arg(playList);
            }
            uint64_t StreamID1,StreamID2;
            VideoArea Area;
            Area.left = 0;
            Area.top = 0;
            Area.width = preItem->m_PreviewArea->width();
            Area.height = preItem->m_PreviewArea->height();

            Area.CropLeft = 0;
            Area.CropTop = 0;
            Area.CropRight = 0;
            Area.CropBottom = 0;

            if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&StreamID1, &StreamID2) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
                m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
                delete preItem;
                delete item;
                return false;
            }
            else
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s m_MateralList[i] init!", __FUNCTION__);
                m_MateralList[i]->streamID1 = StreamID1;
                m_MateralList[i]->streamID2 = StreamID2;
                Log::writeMessage(LOG_RTSPSERV, 1, "%s new CMaterialItem!", __FUNCTION__);
                SLiveStartResize(tmpInstance,false);

            }
        }
        else
        {
            uint64_t tmpInstance;
            uint64_t hwnd = 0;
            if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
            }
            m_VideoInstanceVec.append(tmpInstance);
            VideoArea Area;
            Area.left = 0;
            Area.top = 0;
            Area.width = m_pixMiniWidth;
            Area.height = m_pixMiniHeight;

            Area.CropLeft = 0;
            Area.CropTop = 0;
            Area.CropRight = 0;
            Area.CropBottom = 0;

            QString strJson_obj;
            if(iType == Datetime)
            {
                strJson_obj = "{\"ClassName\":\"ProcessDateTime\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            if(iType == Tupian)
            {
                strJson_obj = "{\"ClassName\":\"BitmapImageSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType == AudioCapture)
            {
                strJson_obj = "{\"ClassName\":\"DSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType == Text)
            {
                strJson_obj = "{\"ClassName\":\"TextOutputSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType == MonitorCapture)
            {
                strJson_obj = "{\"ClassName\":\"MonitorCaptureSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType ==  WindowCapture)
            {
                strJson_obj = "{\"ClassName\":\"WindowCaptureSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            else if(iType ==  ProcTopWindow)
            {
                strJson_obj = "{\"ClassName\":\"ProcTopWindowSource\",\"data\":\%1\}";
                strJson_obj = strJson_obj.arg(m_MateralList.at(i)->data);
            }
            if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&m_MateralList[i]->streamID1, &m_MateralList[i]->streamID2) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
                m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
                return false;
            }
            else
            {
                SLiveStartResize(tmpInstance,false);
            }
        }
        QString strType;
        if(iType == Dianbo)
        {
            strType = tr("VOD source");
            m_iDianboNum++;
        }
        else if(iType == Shexiang)
        {
            strType = tr("Video capture equipment");
            m_iShexiangNum++;
        }
        else if(iType == Hudong)
        {
            strType = tr("Interactive link source");
            m_iHudongNum++;
        }
        else if(iType == Tupian)
        {
            strType = tr("Picture equipment");
            m_iTupianNum++;
        }
        else if(iType == AudioCapture)
        {
            strType = tr("Audio capture device");
            m_iAudioCaptureNum++;
        }
        else if(iType == Datetime)
        {
            strType = tr("Date time source");
            m_iDateTimeNum++;
        }
        else if(iType == Text)
        {
            strType = tr("Text source");
            m_iTextNum++;
        }
        else if(iType == Live)
        {
            strType = tr("Live source");
            m_iLiveNum++;
        }
        else if(iType == MonitorCapture)
        {
            strType = tr("MonitorCapture source");
            m_iMonitorCaptureNum++;
        }
        else if(iType == WindowCapture)
        {
            strType = tr("WindowCapture source");
            m_iWindowCaptureNum++;
        }
        else if(iType == ProcTopWindow)
        {
            strType = tr("WindowCapture source");
            m_iProcTopWindowNum++;
        }

        CMaterialItem *MateralItem = new CMaterialItem(this,m_MateralList[i]->name,strType,ui->m_pLWMaterial->count());
        m_pLWMaterialItem = new QListWidgetItem;
        m_pLWMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
        ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MateralItem);
        m_MaterialItemIsSelectedList.append(false);
    }
    for(int i = 0; i < m_AgentSourceList.count(); i++)
    {
        uint64_t tmpInstance;
        uint64_t hwnd = 0;
        if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
        {
            Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        }
        m_AgentInstanceVec.append(tmpInstance);
        VideoArea Area;
        Area.left = 0;
        Area.top = 0;
        Area.width = m_pixMiniWidth;
        Area.height = m_pixMiniHeight;

        Area.CropLeft = 0;
        Area.CropTop = 0;
        Area.CropRight = 0;
        Area.CropBottom = 0;

        QString strJson_obj = "{\"ClassName\":\"AgentSource\",\"data\":\%1\}";
        strJson_obj = strJson_obj.arg(m_AgentSourceList.at(i)->data);
        if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&m_AgentSourceList[i]->streamID1, &m_AgentSourceList[i]->streamID2) < 0)
        {
            Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
            m_AgentInstanceVec.removeAt(m_AgentInstanceVec.count()-1);
            return false;
        }
        else
        {
            SLiveStartResize(tmpInstance,false);
            m_iAgentNum++;
        }
    }
    Log::writeMessage(LOG_RTSPSERV, 1, "%s scenes init!", __FUNCTION__);
    for(int i = 0; i < m_SceneList.count(); i++)
    {
        QString name = m_SceneList[i]->name;
        if(m_SceneList[i]->bIsReserve)
            name = name + tr("[reserve](Shift + F2)");
        else if(!m_SceneList[i]->shortcutKeys.isEmpty())
        {
            name = name + "(" + m_SceneList[i]->shortcutKeys +")";
            QAction* pActShortcut = new QAction(this);
            pActShortcut->setShortcut(m_SceneList[i]->shortcutKeys);
            QObject::connect(pActShortcut, &QAction::triggered,[=](bool b){
                for(int i = 0; i < m_SceneList.count(); i++)
                {
                    if(m_SceneList[i]->shortcutKeys == pActShortcut->shortcut().toString())
                    {
                        m_CurrentSceneNo = i;
                        CSceneItem* pSceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(i));
                        pSceneItem->SetItemInfo();
                    }
                }
            });
            this->addAction(pActShortcut);
        }

        CSceneItem *SceneItem = new CSceneItem(this,name,i);
        m_pLWSceneItem = new QListWidgetItem;
        m_pLWSceneItem->setSizeHint(QSize(0,50));
        ui->m_pLWScene->addItem(m_pLWSceneItem);
        ui->m_pLWScene->setItemWidget(m_pLWSceneItem,SceneItem);
        if(0 == i)
        {
            m_ItemIsSelectedList.append(true);
            SceneItem->SetItemInfo();
        }
        else
        {
            m_ItemIsSelectedList.append(false);
        }
    }

    if(0 == m_SceneList.count())
    {
        m_iSceneNum++;
        QString strName = QString(tr("scene%1")).arg(m_iSceneNum);
        SceneInfo* sceneInfo = new SceneInfo;
        sceneInfo->name =  strName;
        sceneInfo->bIsReserve = false;
        m_SceneList.append(sceneInfo);

        CSceneItem *SceneItem = new CSceneItem(this,strName,0);
        m_pLWSceneItem = new QListWidgetItem;
        m_pLWSceneItem->setSizeHint(QSize(0,50));
        ui->m_pLWScene->addItem(m_pLWSceneItem);
        ui->m_pLWScene->setItemWidget(m_pLWSceneItem,SceneItem);
        m_ItemIsSelectedList.append(true);
        SceneItem->SetItemInfo();
    }

    for(int i = 0; i < ui->m_pLWPreview->count(); i++)
    {
        int iMatNo = GetPreviewSourceLinkMateralNo(i);
        if(iMatNo == -1) return false;
        if(m_MateralList[iMatNo]->type == Dianbo)
        {
            CPreviewItem* preItem = (CPreviewItem*)(ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(i)));
            preItem->VideoInit();
        }
    }
    m_iSceneNum = m_SceneList.count();
    m_iMateralNum = m_MateralList.count();
    return true;
}

//保存场景，源等信息
bool /*ButelLive::*/SaveConfig(const QString& jsonFile)
{
    QFile file(jsonFile);
    if (!file.open(QFile::ReadWrite | QFile::Text | QFile::Truncate)) {
        qDebug() << "Error: Cannot open file: "
                 << qPrintable(file.errorString());
        return false;
    }
    QJsonDocument jsonDocument;

    //写场景信息
    QJsonArray scenesArr;
    for(int i = 0; i < g_This->m_SceneList.count(); i++)
    {
        QJsonObject scenes_item_obj;
        scenes_item_obj.insert("name", g_This->m_SceneList.at(i)->name);
        scenes_item_obj.insert("bIsReserve", g_This->m_SceneList.at(i)->bIsReserve);
        scenes_item_obj.insert("shortcutKeys", g_This->m_SceneList.at(i)->shortcutKeys);
        QJsonArray linkArr;
        for(int j = 0; j < g_This->m_SceneList.at(i)->m_LinkMateralVec.count(); j++)
        {
//            linkArr.append(g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j));

            QJsonObject source_info_obj;
            source_info_obj.insert("id", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).id);
            source_info_obj.insert("left", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).left);
            source_info_obj.insert("top", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).top);
            source_info_obj.insert("width", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).width);
            source_info_obj.insert("height", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).height);
            source_info_obj.insert("bRender", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).bRender);
            source_info_obj.insert("bIsAgent", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).bIsAgent);

            source_info_obj.insert("cutLeft", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).cutLeft);
            source_info_obj.insert("cutTop", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).cutTop);
            source_info_obj.insert("cutWidth", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).cutWidth);
            source_info_obj.insert("cutHeight", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).cutHeight);
            source_info_obj.insert("bIsCut", g_This->m_SceneList.at(i)->m_LinkMateralVec.at(j).bIsCut);

            linkArr.append(source_info_obj);

        }
        scenes_item_obj.insert("sources", linkArr);
        scenesArr.append(scenes_item_obj);
    }

    //写素材信息
    QJsonArray materalArr;
    for(int i = 0; i < g_This->m_MateralList.count(); i++)
    {
        QJsonObject mats_item_obj;
        mats_item_obj.insert("name", g_This->m_MateralList.at(i)->name);
        mats_item_obj.insert("type", g_This->m_MateralList.at(i)->type);
        QJsonObject data_obj = getJsonObjectFromString(g_MateralDataMap.value(i));
        mats_item_obj.insert("data", data_obj);
//        mats_item_obj.insert("data", QJsonValue(g_MateralDataList.at(i)));
        materalArr.append(mats_item_obj);
    }
    //写区域占位源信息
    QJsonArray agentArr;
    for(int i = 0; i < g_This->m_AgentSourceList.count(); i++)
    {
        QJsonObject agents_item_obj;
        agents_item_obj.insert("name", g_This->m_AgentSourceList.at(i)->name);
        agents_item_obj.insert("type", g_This->m_AgentSourceList.at(i)->type);
        QJsonObject data_obj = getJsonObjectFromString(g_AgentDataMap.value(i));
        agents_item_obj.insert("data", data_obj);
        agentArr.append(agents_item_obj);
    }
    QJsonObject root_obj;
    root_obj.insert("scenes", scenesArr);
    root_obj.insert("materals", materalArr);
    root_obj.insert("agents", agentArr);
    jsonDocument.setObject(root_obj);

    QByteArray byte_array = jsonDocument.toJson(QJsonDocument::Indented);
    file.write(byte_array);
    file.close();
    return true;
}

void ButelLive::closeEvent(QCloseEvent *event)
{
    //SaveConfig("cfg.txt");
    return QDialog::closeEvent(event);
}

//主界面初始化
void ButelLive::UiInit()
{
    setMinimumSize(870,550);
    setWindowIcon(QIcon(":images/nav_logo.png"));
    ui->m_pTitleBarLabel->m_pTitleLabel->setText(tr("ButelDirector-") + m_pChannelInfo->name);
    ui->m_pTitleBarLabel->m_pIconLabel->setStyleSheet("QLabel{border-image:url(:images/nav_logo.png);color:#ffffff;font:14pt;}");
    ui->m_pTitleBarLabel->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleBarLabel->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleBarLabel->m_pMaximizeButton->setStyleSheet("QPushButton{border-image:url(:images/nav_enlarge.png);}"
                                "QPushButton:hover{border-image:url(:images/nav_enlarge_hover.png);}");
    ui->m_pTitleBarLabel->m_pMinimizeButton->setStyleSheet("QPushButton{border-image:url(:images/nav_mini.png);}"
                                "QPushButton:hover{border-image:url(:images/nav_mini_hover.png);}");
    ui->m_pTitleBarLabel->m_pMenuButton->setStyleSheet("QPushButton{border-image:url(:images/nav_menu.png);}"
                                "QPushButton:hover{border-image:url(:images/nav_menu_hover.png);}");

    ui->m_pLblLocalPreview->setStyleSheet("QLabel{border-image:url(:images/video_sacn.png);}"
                                          "QLabel:hover{border-image:url(:images/video_sacn_hover&selected.png);}");
    ui->m_pLblEditMode->setStyleSheet("QLabel{border-image:url(:images/video_editmode.png);}"
                                          "QLabel:hover{border-image:url(:images/video_editmode_hover&selected.png);}");
    ui->m_pLblStartLive->setStyleSheet("QLabel{border-image:url(:images/video_live.png);}"
                                          "QLabel:hover{border-image:url(:images/video_live_hover&selected.png);}");
    ui->m_pBtnListen->setStyleSheet("QPushButton"
                                    "{"
                                        "border-image:url(:images/tyt_listen.png);"
                                    "}"
                                    "QPushButton:hover"
                                    "{"
                                        "border-image:url(:images/tyt_listen_hover.png);"
                                    "}"
                                    "QPushButton:pressed"
                                    "{"
                                        "border-image:url(:images/tyt_listen_close.png);"
                                    "}");
    ui->m_pSldLeftCtr->setStyleSheet(
                                     "QSlider::add-page:vertical"
                                     "{"
                                        "background-color: rgb(79, 79, 83);"
                                        "width:4px;"
                                     "}"
                                     "QSlider::sub-page:vertical"
                                     "{"
                                         "background-color: rgb(14, 14, 15);"
                                         "width:4px;"
                                     "}"
                                     "QSlider::groove:vertical"
                                     "{"
                                         "background:transparent;"
                                         "width:8px;"
                                     "}"
                                     "QSlider::handle:vertical"
                                     "{"
                                        "height:30px;"
                                        "width:20px;"
                                        "border-image: url(:/images/tyt_huakuai.png);"
                                        "margin: -9 -10px;"
                                     "}"
                                     );
    ui->m_pSldRightCtr->setStyleSheet(
                                     "QSlider::add-page:vertical"
                                     "{"
                                        "background-color: rgb(79, 79, 83);"
                                        "width:4px;"
                                     "}"
                                     "QSlider::sub-page:vertical"
                                     "{"
                                         "background-color: rgb(14, 14, 15);"
                                         "width:4px;"
                                     "}"
                                     "QSlider::groove:vertical"
                                     "{"
                                         "background:transparent;"
                                         "width:8px;"
                                     "}"
                                     "QSlider::handle:vertical"
                                     "{"
                                        "height:30px;"
                                        "width:20px;"
                                        "border-image: url(:/images/tyt_huakuai.png);"
                                        "margin: -9 -10px;"
                                     "}"
                                     );
    ui->m_pSldPGMCtr->setStyleSheet("QSlider::add-page:vertical"
                                     "{"
                                        "background-color: rgb(79, 79, 83);"
                                        "width:4px;"
                                     "}"
                                     "QSlider::sub-page:vertical"
                                     "{"
                                         "background-color: rgb(14, 14, 15);"
                                         "width:4px;"
                                     "}"
                                     "QSlider::groove:vertical"
                                     "{"
                                         "background:transparent;"
                                         "width:8px;"
                                     "}"
                                     "QSlider::handle:vertical"
                                     "{"
                                        "height:30px;"
                                        "width:20px;"
                                        "border-image: url(:/images/tyt_huakuai.png);"
                                        "margin: -9 -10px;"
                                     "}"
                                     );
    QString menuStyle = "QMenu"
                        "{"
                         "color: rgb(255,255,255);"
                         "background: rgb(35,35,37);"
                        "}"
                        "QMenu::item"
                        "{"
                        "color: #7f7f7f;"
                        "height: 40px;"
                        "width: 200px;"
                        "padding: 0px 25px 0px 20px;"
                        "}"
                        "QMenu::item:enabled:selected"
                        "{"
                        "color: rgb(230, 230, 230);"
                        "background: rgba(38, 161, 192, 255);"
                        "}"
                        "QMenu::indicator"
                        "{"
                        "margin: 180px;"
                        "image: url(:/images/edit_chose.png);"
                        "}"
                        "QMenu::indicator:selected"
                        "{"
                        "image: url(:/images/edit_chose_hover.png);"
                        "}"
                        "QMenu::separator"
                        "{"
                        "height: 1px;"
                        "background: rgb(80, 80, 80);"
                        "}"
                         "QMenu::right-arrow"
                         "{"
                         "margin: 25px;"
                         "image: url(:/images/edit_arrow.png);"
                         "}"
                         "QMenu::right-arrow:selected"
                         "{"
                         "image: url(:/images/edit_arrow_hover.png);"
                         "}";
    m_PreviewMenu.setStyleSheet(menuStyle);
    m_MainMenu.setStyleSheet(menuStyle);
    m_pMenuFile->setStyleSheet(menuStyle);
    m_pMenuLive->setStyleSheet(menuStyle);
    m_pMenuAbout->setStyleSheet(menuStyle);
    m_MenuSceneEdit.setStyleSheet(menuStyle);
    m_pMenuQuence->setStyleSheet(menuStyle);
    m_MenuMaterialAdd.setStyleSheet(menuStyle);
    m_MenuMaterialEdit.setStyleSheet(menuStyle);
    m_MenuSceneMaterialEdit.setStyleSheet(menuStyle);
    m_pMenuManagerSquence->setStyleSheet(menuStyle);
    m_pMenuManagerPosSize->setStyleSheet(menuStyle);
    m_PreviewInSourceMenu.setStyleSheet(menuStyle);
    m_pMenuPreview->setStyleSheet(menuStyle);
    ui->m_pLWScene->verticalScrollBar()->setStyleSheet(
                                  "QScrollBar"
                                  "{"
                                    "width:8px;"
                                  "}"
                                  "QScrollBar::handle:vertical"
                                  "{"
                                    "width:8px;"
                                    "background:rgb(79,79,83);"
                                    "border-radius:3px;"
                                  "}"
                                  "QScrollBar::sub-page::vertical"
                                  "{"
                                    "width:8px;"
                                    "background:#232325;"
                                  "}"
                                  "QScrollBar::add-page::vertical"
                                  "{"
                                    "width:8px;"
                                    "background:#232325;"
                                  "}"
                                  "QScrollBar::sub-line::vertical"
                                  "{"
                                    "width:8px;"
                                    "height:8px;"
                                    "background:rgb(79,79,83);"
                                  "}"
                                  );
    ui->m_pLWMaterial->verticalScrollBar()->setStyleSheet(
                                  "QScrollBar"
                                  "{"
                                    "width:8px;"
                                  "}"
                                  "QScrollBar::handle:vertical"
                                  "{"
                                    "width:8px;"
                                    "background:rgb(79,79,83);"
                                    "border-radius:3px;"
                                  "}"
                                  "QScrollBar::sub-page::vertical"
                                  "{"
                                    "width:8px;"
                                    "background:#232325;"
                                  "}"
                                  "QScrollBar::add-page::vertical"
                                  "{"
                                    "width:8px;"
                                    "background:#232325;"
                                  "}"
                                  "QScrollBar::sub-line::vertical"
                                  "{"
                                    "width:8px;"
                                    "height:8px;"
                                    "background:rgb(79,79,83);"
                                  "}"
                                  );
    ui->m_pLWScene->setStyleSheet("QListWidget::item:selected{background:rgb(47,160,190);}");

    ui->m_pBtnReset->setStyleSheet("QPushButton"
                                    "{"
                                        "border-image:url(:images/tyt_reset.png);"
                                    "}"
                                    "QPushButton:hover"
                                    "{"
                                        "border-image:url(:images/tyt_reset_hover.png);"
                                    "}");

    ui->m_pSldLeftCtr->setRange(0,100);
    ui->m_pSldLeftCtr->setSliderPosition(50);
    ui->m_pSldRightCtr->setRange(0,100);
    ui->m_pSldRightCtr->setSliderPosition(50);
    ui->m_pSldPGMCtr->setRange(0,100);
    ui->m_pSldPGMCtr->setSliderPosition(50);
    ui->m_pStckWdgt->setCurrentIndex(0);
    ui->m_pLWScene->setCurrentRow(0);
    if(ui->m_pBtnLocalPreview->isEnabled())
        ui->m_pBtnLocalPreview->setCursor(Qt::PointingHandCursor);
    if(ui->m_pBtnEditMode->isEnabled())
        ui->m_pBtnEditMode->setCursor(Qt::PointingHandCursor);
    if(ui->m_pBtnStartLive->isEnabled())
        ui->m_pBtnStartLive->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnScene->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnMaterial->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnAddScene->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnDelScene->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnMoveUp->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnMoveDown->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnAddMaterial->setCursor(Qt::PointingHandCursor);
    ui->m_pBtnDelMaterial->setCursor(Qt::PointingHandCursor);
    ui->m_pLWPreview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->m_pLWPreview->setFocusPolicy(Qt::NoFocus);
}



void ButelLive::OnConfigResourceInit()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    InitConfig("resource.cfg");
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    emit g_This->ConfigResourceInit();
    return 0;
}

void ButelLive::SLiveApiInit()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
//    float sccalX ,sccaly;
//    int iBitRate;
//    if(ConfigOper::instance()->m_EncodeFormat == 32)
//    {
//        sccalX = (float)1280/m_pixWidth;
//        sccaly = (float)720/m_pixHeight;

//        m_pixWidth = 1280;
//        m_pixHeight = 720;

//        m_pixMiniWidth *=  sccalX;
//        m_pixMiniHeight *= sccaly;
//        iBitRate = 1200;
//    }
//    else if(ConfigOper::instance()->m_EncodeFormat == 16)
//    {
//        sccalX = (float)960/m_pixWidth;
//        sccaly = (float)540/m_pixHeight;

//        m_pixWidth = 960;
//        m_pixHeight = 540;

//        m_pixMiniWidth *=  sccalX;
//        m_pixMiniHeight *= sccaly;
//        iBitRate = 800;
//    }
//    else if(ConfigOper::instance()->m_EncodeFormat == 4)
//    {

//        sccalX = (float)640/m_pixWidth;
//        sccaly = (float)360/m_pixHeight;

//        m_pixWidth = 640;
//        m_pixHeight = 360;

//        m_pixMiniWidth *=  sccalX;
//        m_pixMiniHeight *= sccaly;
//        iBitRate = 500;
//    }


    m_pixWidth = ConfigOper::instance()->m_Width;
    m_pixHeight = ConfigOper::instance()->m_Height;

    EncoderParam Encoder = {0};
    LiveSettingParam LiveSetting = {0};
    VideoSettingParam VideoSetting = {0};
    AudioSettingParam AudioSetting = {0};
    AdvancedParam Advanced = {0};

    Encoder.bChange = true;
    Encoder.bUseCBR = ConfigOper::instance()->m_bUseCBR;
    Encoder.bUseVBR = ConfigOper::instance()->m_bUseVBR;
    Encoder.bUsePadding = ConfigOper::instance()->m_bUsePadding;
    Encoder.VideEncoderType = ConfigOper::instance()->m_VideEncoderType;
    Encoder.Quality = ConfigOper::instance()->m_Quality;
    Encoder.AudioEncoderType = ConfigOper::instance()->m_AudioEncoderType;
    Encoder.AudioBitRate = ConfigOper::instance()->m_AudioBitRate;
    Encoder.AudioSampleRate = ConfigOper::instance()->m_AudioSampleRate;
    Encoder.AudioChannel = ConfigOper::instance()->m_AudioChannel;

    LiveSetting.bChange = true;
    LiveSetting.bUsePush = ConfigOper::instance()->m_bUsePush;
    LiveSetting.bUseBackPush = ConfigOper::instance()->m_bUseBackPush;
    LiveSetting.bRecoder = ConfigOper::instance()->m_bRecoder;
    LiveSetting.Width = ConfigOper::instance()->m_Width;
    LiveSetting.Height = ConfigOper::instance()->m_Height;
    LiveSetting.VideoBitRate = ConfigOper::instance()->m_VideoBitRate;
    strcpy(LiveSetting.LivePushUrl,ConfigOper::instance()->m_LivePushUrl.toLocal8Bit().data());
    strcpy(LiveSetting.LiveBackPushUrl,QString(ConfigOper::instance()->m_LiveBackPushUrl).toLocal8Bit().data());
    strcpy(LiveSetting.RecoderPath,ConfigOper::instance()->m_RecoderPath.toLocal8Bit().data());
    LiveSetting.FileType = ConfigOper::instance()->m_FileType;

    LiveSetting.bUseLiveSec = ConfigOper::instance()->m_bUseLiveSec;
    LiveSetting.bUsePushSec = ConfigOper::instance()->m_bUsePushSec;
    LiveSetting.bUseBackPushSec = ConfigOper::instance()->m_bUseBackPushSec;
    LiveSetting.bRecoderSec = ConfigOper::instance()->m_bRecoderSec;
    LiveSetting.WidthSec = ConfigOper::instance()->m_WidthSec;
    LiveSetting.HeightSec = ConfigOper::instance()->m_HeightSec;
    LiveSetting.VideoBitRateSec = ConfigOper::instance()->m_VideoBitRateSec;
    strcpy(LiveSetting.LivePushUrlSec,ConfigOper::instance()->m_LivePushUrlSec.toLocal8Bit().data());
    strcpy(LiveSetting.LiveBackPushUrlSec,ConfigOper::instance()->m_LiveBackPushUrlSec.toLocal8Bit().data());
    strcpy(LiveSetting.RecoderPathSec,ConfigOper::instance()->m_RecoderPathSec.toLocal8Bit().data());
    LiveSetting.FileTypeSec = ConfigOper::instance()->m_FileTypeSec;
    LiveSetting.AutoConnect = ConfigOper::instance()->m_AutoConnect;
    LiveSetting.DelayTime = ConfigOper::instance()->m_DelayTime;

    VideoSetting.AdpterID = ConfigOper::instance()->m_AdpterID;
    VideoSetting.bChange = true;
    VideoSetting.FPS = ConfigOper::instance()->m_FPS;

    AudioSetting.bChange = true;

    if(!ConfigOper::instance()->m_MonitorDevice.isEmpty())
    {
        strcpy(AudioSetting.MonitorDevice,ConfigOper::instance()->m_MonitorDevice.toLocal8Bit().data());
    }
    else
    {
        if(AudioRenderList.count() > 0)
        {
           strcpy(AudioSetting.MonitorDevice,AudioRenderList.at(0).toLocal8Bit().data());
           ConfigOper::instance()->m_MonitorDevice = AudioRenderList.at(0);
        }
    }
    if(!ConfigOper::instance()->m_ScrProDevice.isEmpty())
    {
        strcpy(AudioSetting.ScrProDevice,ConfigOper::instance()->m_ScrProDevice.toLocal8Bit().data());
    }
    else
    {
        if(AudioRenderList.count() > 0)
        {
            strcpy(AudioSetting.ScrProDevice,AudioRenderList.at(0).toLocal8Bit().data());
            ConfigOper::instance()->m_ScrProDevice = AudioRenderList.at(0);
        }
    }

    Advanced.BFrameCount = ConfigOper::instance()->m_BFrameCount;
    Advanced.BufferTime = ConfigOper::instance()->m_BufferTime;
    Advanced.bUseMultiThread = ConfigOper::instance()->m_bUseMultiThread;
    Advanced.KeyFrame = ConfigOper::instance()->m_KeyFrame;
    Advanced.PriorityID = ConfigOper::instance()->m_PriorityID;
    strcpy(Advanced.X264Preset,ConfigOper::instance()->m_X264Preset.toLocal8Bit().data());
    strcpy(Advanced.X264Profile,ConfigOper::instance()->m_X264Profile.toLocal8Bit().data());
    Advanced.bChange = true;

    SLiveParam Param = { 0 };
    Param.Encoder = Encoder;
    Param.LiveSetting = LiveSetting;
    Param.VideoSetting = VideoSetting;
    Param.AudioSetting = AudioSetting;
    Param.Advanced = Advanced;

    outputCX = m_pixWidth;
    outputCY = m_pixHeight;
    Param.PushStatus = PushStatusCallBacFunc;
    Param.PushStatus2 = PushStatusCallBack2Func;
    Param.ConfigCB = ConfigCallBackFunc;
    Param.MainHwnd = (uint64_t)winId();
    Param.LiveAudioCb = LiveAudioCallBackFunc;
    if (SLiveInit(&Param) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveInit failed!", __FUNCTION__);
        return;
    }
    uint64_t hwnd = ui->m_pDisArea->winId();
    if (SLiveCreateInstance(&Instance,hwnd) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }

    uint64_t hwnd_2 = ui->m_pDisArea_2->winId();
    if (SLiveCreateInstance(&Instance_2,hwnd_2,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }

    DWORD threadID;
    HANDLE hThread;
    hThread = CreateThread(NULL,0,ThreadProc,NULL,0,&threadID);
    WaitForSingleObject(hThread,INFINITE);
    CloseHandle(hThread);

    UINT monitorNum = 0;
    if(SLiveGetMinitorNum(&monitorNum) < 0)
    {

    }
    else
    {
        for(int i = 0; i < monitorNum; i++)
        {
            ui->m_pCmbMonitor->addItem(tr("Monitor") + QString::number(i+1));
        }
    }
}

//场景点击
void ButelLive::on_m_pBtnScene_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pStckWdgt->setCurrentIndex(0);
}

//素材点击
void ButelLive::on_m_pBtnMaterial_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pStckWdgt->setCurrentIndex(1);
}

//菜单初始化
void ButelLive::MenuInit()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    //文件子菜单初始化
//    m_pMenuFile = new QMenu(QStringLiteral("文件(F)"),this);
    m_pMenuFile = new QMenu(tr("flie(F)"),this);
    m_pMenuFile->clear();
//    QAction* pImportScene = new QAction(QStringLiteral("导入场景(I)"),this);
//    QAction* pExportScene = new QAction(QStringLiteral("导出场景(E)"),this);
//    QAction* pActExit = new QAction(QStringLiteral("退出(S)"),this);
    QAction* pImportScene = new QAction(tr("import scene(I)"),this);
    QAction* pExportScene = new QAction(tr("export scene(E)"),this);
    QAction* pActExit = new QAction(tr("quit(S)"),this);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(pImportScene);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(pExportScene);
    m_pMenuFile->addSeparator();
    m_pMenuFile->addAction(pActExit);
    connect(pImportScene,SIGNAL(triggered(bool)),this,SLOT(OnImportScene()));
    connect(pExportScene,SIGNAL(triggered(bool)),this,SLOT(OnExportScene()));

    //直播子菜单初始化
//    m_pMenuLive = new QMenu(QStringLiteral("直播(L)"),this);
    m_pMenuLive = new QMenu(tr("live(L)"),this);
    m_pMenuLive->clear();
//    QAction* pStartLive = new QAction(QStringLiteral("开始直播(R)"),this);
//    QAction* pStopPreview = new QAction(QStringLiteral("停止预览(B)"),this);
//    QAction* pQuickChange = new QAction(QStringLiteral("紧急切播(T)"),this);
//    QAction* pLivePreview = new QAction(QStringLiteral("直播预览(L)"),this);
    QAction* pStartLive = new QAction(tr("start live(R)"),this);
    QAction* pStopPreview = new QAction(tr("stop preview(B)"),this);
    QAction* pQuickChange = new QAction(tr("emergency cut(T)"),this);
    QAction* pLivePreview = new QAction(tr("live preview(L)"),this);
    this->addAction(pQuickChange);
    pQuickChange->setShortcut(tr("Shift+F2"));
    connect(pQuickChange,SIGNAL(triggered (bool)),this,SLOT(OnQuickChange(bool)));

    m_pMenuLive->addSeparator();
    m_pMenuLive->addAction(pStartLive);
    m_pMenuLive->addSeparator();
    m_pMenuLive->addAction(pStopPreview);
    m_pMenuLive->addSeparator();
    m_pMenuLive->addAction(pQuickChange);
    m_pMenuLive->addSeparator();
    m_pMenuLive->addAction(pLivePreview);

    //关于子菜单初始化
//    m_pMenuAbout = new QMenu(QStringLiteral("关于(L)"),this);
    m_pMenuAbout = new QMenu(tr("about(L)"),this);
    m_pMenuAbout->clear();
//    QAction* pAboutBLive = new QAction(QStringLiteral("关于BLive(A)"),this);
//    QAction* pDisplayLog = new QAction(QStringLiteral("显示日志(L)"),this);
//    QAction* pCheckUpdate = new QAction(QStringLiteral("检查软件更新"),this);
    QAction* pAboutBLive = new QAction(tr("about ButelLive(A)"),this);
    QAction* pDisplayLog = new QAction(tr("display log(L)"),this);
    QAction* pCheckUpdate = new QAction(tr("check software update"),this);
    m_pMenuAbout->addSeparator();
    m_pMenuAbout->addAction(pAboutBLive);
    m_pMenuAbout->addSeparator();
    m_pMenuAbout->addAction(pDisplayLog);
    m_pMenuAbout->addSeparator();
    m_pMenuAbout->addAction(pCheckUpdate);

    //主菜单初始化
    m_MainMenu.clear();
//    QAction* pActSet = new QAction(QStringLiteral("全局设置(C)"),this);
//    QAction* pActHelp = new QAction(QStringLiteral("帮助(H)"),this);
    QAction* pActSet = new QAction(tr("global settings(C)"),this);
    QAction* pActHelp = new QAction(tr("help(H)"),this);
    m_MainMenu.addSeparator();
    m_MainMenu.addMenu(m_pMenuFile);
    m_MainMenu.addSeparator();
    m_MainMenu.addMenu(m_pMenuLive);
    m_MainMenu.addSeparator();
    m_MainMenu.addAction(pActSet);
    m_MainMenu.addSeparator();
    m_MainMenu.addMenu(m_pMenuAbout);
    m_MainMenu.addSeparator();
    m_MainMenu.addAction(pActHelp);

    QObject::connect(pActSet, &QAction::triggered,[=](bool b){
        SettingUI* pSetUI = new SettingUI(this);
        pSetUI->show();
    });

    //投影区右击菜单初始化
    m_PreviewMenu.clear();
//    QAction* pMaxPreviewMode = new QAction(QStringLiteral("全屏预览模式(F)"),this);
//    QAction* pZoom = new QAction(QStringLiteral("缩放到适合画面"),this);
//    QAction* pEqualMode = new QAction(QStringLiteral("1:1模式"),this);
//    QAction* pUseLook = new QAction(QStringLiteral("启用查看"),this);
//    QAction* pUseCtronl = new QAction(QStringLiteral("启用控制面板"),this);
    QMenu* pMenuPreviewTo = new QMenu(tr("preview to"),this);
    QAction* pMaxPreviewMode = new QAction(tr("full screen preview mode(F)"),this);
    QAction* pZoom = new QAction(tr("zoom to fit screen"),this);
    QAction* pEqualMode = new QAction(tr("1:1 mode"),this);
    QAction* pUseLook = new QAction(tr("enable view"),this);
    QAction* pUseCtronl = new QAction(tr("enable control panel"),this);
    m_PreviewMenu.addMenu(pMenuPreviewTo);
    m_PreviewMenu.addSeparator();
    m_PreviewMenu.addAction(pMaxPreviewMode);
    m_PreviewMenu.addSeparator();
    m_PreviewMenu.addAction(pZoom);
    m_PreviewMenu.addSeparator();
    m_PreviewMenu.addAction(pEqualMode);
    m_PreviewMenu.addSeparator();
    m_PreviewMenu.addAction(pUseLook);
    m_PreviewMenu.addSeparator();
    m_PreviewMenu.addAction(pUseCtronl);
    connect(pMaxPreviewMode,SIGNAL(triggered (bool)),this,SLOT(OnPreiveFullScreen()));



    //顺序子菜单初始化
//    m_pMenuQuence = new QMenu(QStringLiteral("顺序"),this);
    m_pMenuQuence = new QMenu(tr("order"),this);
    m_pMenuQuence->clear();
//    QAction* pMoveUp = new QAction(QStringLiteral("上移"),this);
//    QAction* pMoveDown = new QAction(QStringLiteral("下移"),this);
//    QAction* pMoveToTop = new QAction(QStringLiteral("移至顶部"),this);
//    QAction* pMoveToLow = new QAction(QStringLiteral("移至底部"),this);
    QAction* pMoveUp = new QAction(tr("move up"),this);
    QAction* pMoveDown = new QAction(tr("move down"),this);
    QAction* pMoveToTop = new QAction(tr("move to top"),this);
    QAction* pMoveToLow = new QAction(tr("move to bottom"),this);
    m_pMenuQuence->addAction(pMoveUp);
    m_pMenuQuence->addSeparator();
    m_pMenuQuence->addAction(pMoveDown);
    m_pMenuQuence->addSeparator();
    m_pMenuQuence->addAction(pMoveToTop);
    m_pMenuQuence->addSeparator();
    m_pMenuQuence->addAction(pMoveToLow);
    connect(pMoveUp,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnMoveUp_clicked()));
    connect(pMoveDown,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnMoveDown_clicked()));
    connect(pMoveToTop,SIGNAL(triggered (bool)),this,SLOT(OnMoveToTop()));
    connect(pMoveToLow,SIGNAL(triggered (bool)),this,SLOT(OnMoveToLow()));

    //场景编辑菜单初始化
    m_MenuSceneEdit.clear();
//    QAction* pAddScene = new QAction(QStringLiteral("添加场景"),this);
//    QAction* pRemove = new QAction(QStringLiteral("移除"),this);
//    QAction* pRename = new QAction(QStringLiteral("重命名"),this);
//    QAction* pCopy = new QAction(QStringLiteral("复制"),this);
//    QAction* pSetCopy = new QAction(QStringLiteral("设置为备用场"),this);
//    QAction* pSetShotcut = new QAction(QStringLiteral("设定快捷键"),this);
    QAction* pAddScene = new QAction(tr("add scene"),this);
    QAction* pRemove = new QAction(tr("remove"),this);
    QAction* pRename = new QAction(tr("rename"),this);
    QAction* pCopy = new QAction(tr("copy"),this);
    QAction* pSetReserve = new QAction(tr("set as reserve"),this);
    QAction* pSetShotcut = new QAction(tr("set shortcut keys"),this);
    m_MenuSceneEdit.addAction(pAddScene);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addMenu(m_pMenuQuence);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addAction(pRemove);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addAction(pRename);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addAction(pCopy);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addAction(pSetReserve);
    m_MenuSceneEdit.addSeparator();
    m_MenuSceneEdit.addAction(pSetShotcut);
    connect(pAddScene,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnAddScene_clicked()));
    connect(pRemove,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnDelScene_clicked()));
    connect(pRename,SIGNAL(triggered (bool)),this,SLOT(OnActRename()));
    connect(pSetReserve,SIGNAL(triggered (bool)),this,SLOT(OnActReserve()));
    connect(pCopy,SIGNAL(triggered (bool)),this,SLOT(OnActSceneCopy()));
    connect(pSetShotcut,SIGNAL(triggered (bool)),this,SLOT(OnActSetSceneShortcut()));

    //素材添加菜单初始化
    m_MenuMaterialAdd.clear();

//    QAction* pGetProcTopLevelWnd = new QAction(QStringLiteral("添加 获取进程顶层窗口"),this);
//    QAction* pGetWnd = new QAction(QStringLiteral("添加 获取窗口"),this);
//    QAction* pDisplayDevGet = new QAction(QStringLiteral("添加 显示器获取"),this);
//    QAction* pPictureSource = new QAction(QStringLiteral("添加 图片源"),this);
//    QAction* pScreenPlay = new QAction(QStringLiteral("添加 投影片放映"),this);
//    QAction* pCharactersSource = new QAction(QStringLiteral("添加 文字来源"),this);
//    QAction* pAudioConnectCtronl = new QAction(QStringLiteral("添加 音频连接控制"),this);
//    QAction* pAudioCatchDevice = new QAction(QStringLiteral("添加 音频捕捉设备"),this);
//    QAction* pTimeSource = new QAction(QStringLiteral("添加 时间源"),this);
//    QAction* pVideoCatchDevice = new QAction(QStringLiteral("添加 视频捕捉设备"),this);
//    QAction* pInteractConnectSource = new QAction(QStringLiteral("添加 互动连接源"),this);
//    QAction* pRequestSource = new QAction(QStringLiteral("添加 点播来源"),this);

    QAction* pGetProcTopLevelWnd = new QAction(tr("add get the top level window of the process"),this);
    QAction* pGetWnd = new QAction(tr("add get window"),this);
    QAction* pDisplayDevGet = new QAction(tr("add display acquisition"),this);
    QAction* pPictureSource = new QAction(tr("add picture sourece"),this);
    QAction* pScreenPlay = new QAction(tr("add screen sheet"),this);
    QAction* pCharactersSource = new QAction(tr("add text source"),this);
    QAction* pAudioConnectCtronl = new QAction(tr("add audio link control"),this);
    QAction* pAudioCatchDevice = new QAction(tr("add audio capture device"),this);
    QAction* pTimeSource = new QAction(tr("add time source"),this);
    QAction* pVideoCatchDevice = new QAction(tr("add video capture device"),this);
    QAction* pInteractConnectSource = new QAction(tr("add interaction link source"),this);
    QAction* pRequestSource = new QAction(tr("add VOD source"),this);
    QAction* pLiveSource = new QAction(tr("add Live source"),this);

    m_MenuMaterialAdd.addAction(pGetProcTopLevelWnd);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pGetWnd);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pDisplayDevGet);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pPictureSource);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pScreenPlay);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pCharactersSource);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pAudioConnectCtronl);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pAudioCatchDevice);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pTimeSource);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pVideoCatchDevice);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pInteractConnectSource);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pRequestSource);
    m_MenuMaterialAdd.addSeparator();
    m_MenuMaterialAdd.addAction(pLiveSource);
    connect(pInteractConnectSource,SIGNAL(triggered (bool)),this,SLOT(OnInteractConnectSource()));
    connect(pRequestSource,SIGNAL(triggered (bool)),this,SLOT(OnRequestSource()));
    connect(pPictureSource,SIGNAL(triggered (bool)),this,SLOT(OnPictureSource()));
    connect(pVideoCatchDevice,SIGNAL(triggered (bool)),this,SLOT(OnVideoCatchSource()));
    connect(pTimeSource,SIGNAL(triggered (bool)),this,SLOT(OnDateTimeSource()));
    connect(pAudioCatchDevice,SIGNAL(triggered (bool)),this,SLOT(OnAudioCatchSource()));
    connect(pCharactersSource,SIGNAL(triggered (bool)),this,SLOT(OnTextSource()));
    connect(pLiveSource,SIGNAL(triggered (bool)),this,SLOT(OnLiveSource()));
    connect(pDisplayDevGet,SIGNAL(triggered (bool)),this,SLOT(OnMonitorCapture()));
    connect(pGetWnd,SIGNAL(triggered (bool)),this,SLOT(OnWindowCapture()));
    connect(pGetProcTopLevelWnd,SIGNAL(triggered (bool)),this,SLOT(OnProcTopWindow()));

    //素材编辑菜单初始化
    m_MenuMaterialEdit.clear();
//    QAction* pAddToCurrentScene = new QAction(QStringLiteral("添加至当前场景"),this);
//    QAction* pMaterialRename = new QAction(QStringLiteral("重命名"),this);
//    QAction* pMaterialDelete = new QAction(QStringLiteral("删除"),this);
//    QAction* pMaterialSet = new QAction(QStringLiteral("设置"),this);
    QAction* pAddToCurrentScene = new QAction(tr("add to current scene"),this);
    QAction* pMaterialRename = new QAction(tr("rename"),this);
    QAction* pMaterialDelete = new QAction(tr("delete"),this);
    QAction* pMaterialSet = new QAction(tr("setting"),this);
    m_MenuMaterialEdit.addAction(pAddToCurrentScene);
    m_MenuMaterialEdit.addSeparator();
    m_MenuMaterialEdit.addAction(pMaterialRename);
    m_MenuMaterialEdit.addSeparator();
    m_MenuMaterialEdit.addAction(pMaterialDelete);
    m_MenuMaterialEdit.addSeparator();
    m_MenuMaterialEdit.addAction(pMaterialSet);    
    connect(pAddToCurrentScene,SIGNAL(triggered (bool)),this,SLOT(OnMaterialAddToCurrentScene()));
    connect(pMaterialRename,SIGNAL(triggered (bool)),this,SLOT(OnMaterialRename()));
    connect(pMaterialDelete,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnDelMaterial_clicked()));
    connect(pMaterialSet,SIGNAL(triggered (bool)),this,SLOT(OnMaterialSet()));

    //场景素材编辑菜单初始化管理
    m_MenuSceneMaterialEdit.clear();
    m_pMenuManagerSquence = new QMenu(tr("sequence"),this);
    QAction* pManagerMoveUp = new QAction(tr("move up"),this);
    QAction* pManagerMoveDown = new QAction(tr("move down"),this);
    QAction* pManagerMoveTop = new QAction(tr("move top"),this);
    QAction* pManagerMoveBottom = new QAction(tr("move bottom"),this);
    connect(pManagerMoveUp,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnScnMtrlMoveUp_clicked()));
    connect(pManagerMoveDown,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnScnMtrlMoveDown_clicked()));
    connect(pManagerMoveTop,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnScnMtrlMoveTop_clicked()));
    connect(pManagerMoveBottom,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnScnMtrlMoveBottom_clicked()));


    m_pMenuManagerSquence->addAction(pManagerMoveUp);
    m_pMenuManagerSquence->addSeparator();
    m_pMenuManagerSquence->addAction(pManagerMoveDown);
    m_pMenuManagerSquence->addSeparator();
    m_pMenuManagerSquence->addAction(pManagerMoveTop);
    m_pMenuManagerSquence->addSeparator();
    m_pMenuManagerSquence->addAction(pManagerMoveBottom);

    m_pMenuManagerPosSize = new QMenu(tr("position size"),this);

    QAction* pManagerFitScreen = new QAction(tr("fit screen"),this);
    QAction* pManagerResetSize = new QAction(tr("reset size"),this);
    QAction* pManagerSetPosSize = new QAction(tr("set position size"),this);
    QAction* pManagerResetCut = new QAction(tr("reset cut"),this);
    QAction* pManagerCenter = new QAction(tr("center"),this);
    QAction* pManagerHCenter = new QAction(tr("horizontal center"),this);
    QAction* pManagerVCenter = new QAction(tr("vertical center"),this);
    QAction* pManagerMoveLeftEdge = new QAction(tr("left edge"),this);
    QAction* pManagerMoveTopEdge = new QAction(tr("top edge"),this);
    QAction* pManagerMoveRightEdge = new QAction(tr("right edge"),this);
    QAction* pManagerMoveBottomEdge = new QAction(tr("bottom edge"),this);

    m_pMenuManagerPosSize->addAction(pManagerFitScreen);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerResetSize);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerSetPosSize);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerResetCut);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerCenter);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerHCenter);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerVCenter);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerMoveLeftEdge);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerMoveTopEdge);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerMoveRightEdge);
    m_pMenuManagerPosSize->addSeparator();
    m_pMenuManagerPosSize->addAction(pManagerMoveBottomEdge);


    connect(pManagerFitScreen,SIGNAL(triggered (bool)),this,SLOT(OnActFitScreen()));
    connect(pManagerResetSize,SIGNAL(triggered (bool)),this,SLOT(OnActResetSize()));
    connect(pManagerSetPosSize,SIGNAL(triggered (bool)),this,SLOT(OnActSetPosSize()));
    connect(pManagerResetCut,SIGNAL(triggered (bool)),this,SLOT(OnActResetCut()));

    connect(pManagerCenter,SIGNAL(triggered (bool)),this,SLOT(OnActCenter()));
    connect(pManagerHCenter,SIGNAL(triggered (bool)),this,SLOT(OnActHCenter()));
    connect(pManagerVCenter,SIGNAL(triggered (bool)),this,SLOT(OnActVCenter()));
    connect(pManagerMoveLeftEdge,SIGNAL(triggered (bool)),this,SLOT(OnActMoveLeftEdge()));
    connect(pManagerMoveTopEdge,SIGNAL(triggered (bool)),this,SLOT(OnActMoveTopEdge()));
    connect(pManagerMoveRightEdge,SIGNAL(triggered (bool)),this,SLOT(OnActMoveRightEdge()));
    connect(pManagerMoveBottomEdge,SIGNAL(triggered (bool)),this,SLOT(OnActMoveBottomEdge()));


    QAction* pManagerRemove = new QAction(tr("remove"),this);
    QAction* pManagerRename = new QAction(tr("rename"),this);
    QAction* pManagerSet = new QAction(tr("setting"),this);
    m_MenuSceneMaterialEdit.addMenu(m_pMenuManagerSquence);
    m_MenuSceneMaterialEdit.addSeparator();
    m_MenuSceneMaterialEdit.addMenu(m_pMenuManagerPosSize);
    m_MenuSceneMaterialEdit.addSeparator();
    m_MenuSceneMaterialEdit.addAction(pManagerRemove);
    m_MenuSceneMaterialEdit.addSeparator();
    m_MenuSceneMaterialEdit.addAction(pManagerRename);
    m_MenuSceneMaterialEdit.addSeparator();
    m_MenuSceneMaterialEdit.addAction(pManagerSet);
    connect(pManagerRemove,SIGNAL(triggered (bool)),this,SLOT(on_m_pBtnScnMtrlDel_clicked()));
    connect(pManagerRename,SIGNAL(triggered (bool)),this,SLOT(OnManagerRename()));
    connect(pManagerSet,SIGNAL(triggered (bool)),this,SLOT(OnManagerSet()));

    m_PreviewInSourceMenu.clear();

    m_pMenuPreview = new QMenu(tr("preview"),this);

    m_pMenuPreview->addAction(pZoom);
    m_pMenuPreview->addSeparator();
    m_pMenuPreview->addAction(pEqualMode);
    m_pMenuPreview->addSeparator();
    m_pMenuPreview->addAction(pUseLook);
    m_pMenuPreview->addSeparator();
    m_pMenuPreview->addAction(pUseCtronl);

    m_PreviewInSourceMenu.addMenu(pMenuPreviewTo);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addAction(pMaxPreviewMode);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addMenu(m_pMenuPreview);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addMenu(m_pMenuManagerSquence);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addMenu(m_pMenuManagerPosSize);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addAction(pManagerRemove);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addAction(pManagerRename);
    m_PreviewInSourceMenu.addSeparator();
    m_PreviewInSourceMenu.addAction(pManagerSet);

}
//标题栏主菜单显示，TitleBar中触发
void ButelLive::ShowMenu()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_MainMenu.exec(QCursor::pos()-QPoint(245,0));
}

//紧急切播
void ButelLive::OnQuickChange(bool)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
//    QMessageBox::information(this,"info","123","OK");
    if(!m_bIsPreview)
    {
        return;
    }

    int iReserveScene = -1;
    for(int i = 0; i < m_SceneList.count(); i++)
    {
        if(m_SceneList[i]->bIsReserve)
        {
            iReserveScene = i;
            break;
        }
    }

    if(m_bIsPreview && m_CurrentSceneNo >= 0 && m_CurrentSceneNo < m_SceneList.count() && iReserveScene >= 0)
    {
        SLiveClearIntances(Instance);
        //开始预览现在场景中源

        SceneInfo * sceneInfo = m_SceneList.at(iReserveScene);
        for(int i = 0; i < sceneInfo->m_LinkMateralVec.count(); i++)
        {
            int iMateralIndex = sceneInfo->m_LinkMateralVec.at(i).id;
            bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
            if(sceneInfo->m_LinkMateralVec[i].bRender)
                AddPreviewFromMateralNo(iMateralIndex,bIsAgent);
        }
        SLiveSetSenceToBackUp();
        m_CurrentSceneNo = iReserveScene;
        CSceneItem* pSceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iReserveScene));
        pSceneItem->SetItemInfo();
    }
}


//判断鼠标位置，设置鼠标形状
void ButelLive::region(const QPoint &cursorGlobalPoint)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);

    QRect rect = this->rect();
    QPoint tl = mapToGlobal(rect.topLeft());
    QPoint rb = mapToGlobal(rect.bottomRight());

    int x = cursorGlobalPoint.x();
    int y = cursorGlobalPoint.y();

    if(tl.x() + PADDING >= x && tl.x() <= x && tl.y() + PADDING >= y && tl.y() <= y) {
        // 左上角
        m_Dir = LEFTTOP;
        this->setCursor(QCursor(Qt::SizeFDiagCursor));  // 设置鼠标形状
    } else if(x >= rb.x() - PADDING && x <= rb.x() && y >= rb.y() - PADDING && y <= rb.y()) {
        // 右下角
        m_Dir = RIGHTBOTTOM;
        this->setCursor(QCursor(Qt::SizeFDiagCursor));
    } else if(x <= tl.x() + PADDING && x >= tl.x() && y >= rb.y() - PADDING && y <= rb.y()) {
        //左下角
        m_Dir = LEFTBOTTOM;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
    } else if(x <= rb.x() && x >= rb.x() - PADDING && y >= tl.y() && y <= tl.y() + PADDING) {
        // 右上角
        m_Dir = RIGHTTOP;
        this->setCursor(QCursor(Qt::SizeBDiagCursor));
    } else if(x <= tl.x() + PADDING && x >= tl.x()) {
        // 左边
        m_Dir = LEFT;
        this->setCursor(QCursor(Qt::SizeHorCursor));
    } else if( x <= rb.x() && x >= rb.x() - PADDING) {
        // 右边
        m_Dir = RIGHT;
        this->setCursor(QCursor(Qt::SizeHorCursor));
    }else if(y >= tl.y() && y <= tl.y() + PADDING){
        // 上边
        m_Dir = UP;
        this->setCursor(QCursor(Qt::SizeVerCursor));
    } else if(y <= rb.y() && y >= rb.y() - PADDING) {
        // 下边
        m_Dir = DOWN;
        this->setCursor(QCursor(Qt::SizeVerCursor));
    }else {
        // 默认
        m_Dir = NONE;
        this->setCursor(QCursor(Qt::ArrowCursor));
    }
}

void ButelLive::mouseReleaseEvent(QMouseEvent *event)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    if(event->button() == Qt::LeftButton) {
        m_IsLeftPressDown = false;
        if(m_Dir != NONE) {
            this->releaseMouse();
            this->setCursor(QCursor(Qt::ArrowCursor));
        }
    }    
    QDialog::mouseReleaseEvent(event);
}

void ButelLive::mousePressEvent(QMouseEvent *event)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    switch(event->button()) {
    case Qt::LeftButton:
        m_IsLeftPressDown = true;
        if(m_Dir != NONE) {
            this->mouseGrabber();
        } else {
            m_DragPosition = event->globalPos() - this->frameGeometry().topLeft();
        }
        break;
    default:
        QDialog::mousePressEvent(event);
    }
}

//根据鼠标按下移动位置设置窗口缩放大小
void ButelLive::mouseMoveEvent(QMouseEvent *event)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    HWND hWnd = (HWND)this->winId();
    if(::IsZoomed(hWnd))
    {
        return;
    }
    QPoint gloPoint = event->globalPos();
    QRect rect = this->rect();
    QPoint tl = mapToGlobal(rect.topLeft());
    QPoint rb = mapToGlobal(rect.bottomRight());
    if(!m_IsLeftPressDown) {
        this->region(gloPoint);
    } else {
        if(m_Dir != NONE) {
            QRect rMove(tl, rb);
            switch(m_Dir) {
            case LEFT:
                if(rb.x() - gloPoint.x() <= this->minimumWidth())
                    rMove.setX(tl.x());
                else
                    rMove.setX(gloPoint.x());
                break;
            case RIGHT:
                rMove.setWidth(gloPoint.x() - tl.x());
                break;
            case UP:
                if(rb.y() - gloPoint.y() <= this->minimumHeight())
                    rMove.setY(tl.y());
                else
                    rMove.setY(gloPoint.y());
                break;
            case DOWN:
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            case LEFTTOP:
                if(rb.x() - gloPoint.x() <= this->minimumWidth())
                    rMove.setX(tl.x());
                else
                    rMove.setX(gloPoint.x());
                if(rb.y() - gloPoint.y() <= this->minimumHeight())
                    rMove.setY(tl.y());
                else
                    rMove.setY(gloPoint.y());
                break;
            case RIGHTTOP:
                rMove.setWidth(gloPoint.x() - tl.x());
                rMove.setY(gloPoint.y());
                break;
            case LEFTBOTTOM:
                rMove.setX(gloPoint.x());
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            case RIGHTBOTTOM:
                rMove.setWidth(gloPoint.x() - tl.x());
                rMove.setHeight(gloPoint.y() - tl.y());
                break;
            default:
                break;
            }
            QPoint oldPos = this->pos();
            if(rMove.width() <= 870)
            {
               rMove.setLeft(oldPos.x());
            }
            if(rMove.height() <= 550)
            {
               rMove.setTop(oldPos.y());
            }
            this->setGeometry(rMove);

        }
    }
    QDialog::mouseMoveEvent(event);
}

void ButelLive::paintEvent(QPaintEvent *event)
{
//    QPainter painter(this);
//    painter.setBrush(QColor(qRgb(35, 35, 37)));
//    painter.setPen(QColor(qRgb(35, 35, 37)));
//    painter.setRenderHint(QPainter::Antialiasing);//抗锯齿
//    HWND hWnd = (HWND)this->winId();
//    if(::IsZoomed(hWnd))
//    {
//        painter.drawRoundedRect(0, 0, width(), height(), 0, 0);
//        ui->m_pGridLayout->setContentsMargins(0,0,0,0);
//    }
//    else
//    {
//        painter.drawRoundedRect(0, 0, width(), height(), 7, 7);
//        ui->m_pGridLayout->setContentsMargins(8,8,8,8);
//    }
    QDialog::paintEvent(event);
}

//素材添加按钮响应
void ButelLive::on_m_pBtnAddMaterial_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_MenuMaterialAdd.exec(QCursor::pos());
}
//场景添加按钮响应
void ButelLive::on_m_pBtnAddScene_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iSceneNum++;
    int iSceneIndex = ui->m_pLWScene->count();
//    QString strName = QString(QStringLiteral("场景%1")).arg(m_iSceneNum);
    QString strName = QString(tr("scene%1")).arg(m_iSceneNum);
    InputNameUI* dlgInName = new InputNameUI(this,Scene);
    dlgInName->setInputText(strName);
//    if(ON_BUTTON_OK == dlgInName->exec())
//    {
        int ret = dlgInName->exec();
        if(ret == ON_BUTTON_CANCEL)
        {
          m_iSceneNum--;
          return;
        }
        QString qstrSceneName = dlgInName->getInputText();
//        for(int i = 0; i < iSceneIndex; i++)
//        {
//            CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(i));
//            QString strSceneialName = sceneItem->m_LabelName.text();
//            if(strSceneialName == qstrSceneName)
//            {
//                MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrSceneName),BUTTON_YES);
//                message.exec();
//                m_iSceneNum--;
//                return;
//            }
//        }
        if(qstrSceneName.isEmpty())
        {
            MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
            message.exec();
            m_iSceneNum--;
            return;
        }
        CSceneItem *SceneItem = new CSceneItem(this,qstrSceneName,iSceneIndex);
        m_pLWSceneItem = new QListWidgetItem;
        m_pLWSceneItem->setSizeHint(QSize(0,50));
        ui->m_pLWScene->addItem(m_pLWSceneItem);
        ui->m_pLWScene->setItemWidget(m_pLWSceneItem,SceneItem);
        m_ItemIsSelectedList.append(false);
        SceneInfo* sceneInfo = new SceneInfo;
        sceneInfo->name =  qstrSceneName;
        sceneInfo->bIsReserve = false;
        m_SceneList.append(sceneInfo);
//    }
//    else
//    {
//        m_iSceneNum--;
//    }
    return;
}
//场景编辑菜单显示
void ButelLive::OnSceneEditClicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex < 0) return;
    if(m_SceneList[iIndex]->bIsReserve)
    {
        m_MenuSceneEdit.actions().at(10)->setText(tr("cancel set as reserve"));
    }
    else
    {
        m_MenuSceneEdit.actions().at(10)->setText(tr("set as reserve"));
    }
    m_MenuSceneEdit.exec(QCursor::pos());
}
//素材编辑菜单显示
void ButelLive::OnMaterialEditClicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_MenuMaterialEdit.exec(QCursor::pos());
}

void ButelLive::OnSceneArrowClicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pStckWdgt->setCurrentIndex(2);
    ui->m_pLWSceneMateralManage->clear();
    m_SceneMaterialItemIsSelectedList.clear();
    int iCurScene = GetSceneCurrentRow();
    Log::writeMessage(LOG_RTSPSERV, 1, "%s iCurScene = %d,m_SceneMaterialItemIsSelectedList.count = %d", __FUNCTION__,iCurScene,m_SceneMaterialItemIsSelectedList.count());
    SceneInfo* pSceneInfo = m_SceneList.at(iCurScene);
//    for(int i = 0; i < pSceneInfo->m_LinkMateralVec.count(); i++)
    for(int i = pSceneInfo->m_LinkMateralVec.count() - 1; i >= 0; i--)
    {
//        ui->m_pLWSceneMateralManage->addItem(m_MateralList.at(pSceneInfo->m_LinkMateralVec.at(i).id)->name);
        int materalIndex = pSceneInfo->m_LinkMateralVec.at(i).id;
        bool bIsAgent = pSceneInfo->m_LinkMateralVec.at(i).bIsAgent;
        SceneMaterialItem* pSceneMateralItem;
        if(bIsAgent)
        {
            pSceneMateralItem = new SceneMaterialItem(this,m_AgentSourceList[materalIndex]->name,m_AgentSourceList[materalIndex]->type,ui->m_pLWSceneMateralManage->count());
        }
        else
        {
            pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[materalIndex]->name,m_MateralList[materalIndex]->type,ui->m_pLWSceneMateralManage->count());
        }
//        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[materalIndex]->name,m_MateralList[materalIndex]->type,ui->m_pLWSceneMateralManage->count());
        if(pSceneInfo->m_LinkMateralVec[i].bRender)
            pSceneMateralItem->m_CheckBox.setChecked(true);
        else
            pSceneMateralItem->m_CheckBox.setChecked(false);
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->addItem(m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.append(false);
    }
}


//鼠标点击场景项,设置其他项目的颜色为不选中
void ButelLive::SetItemColor(int iIndex,bool bIsSelected)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));
    setAutoFillBackground(true);
    QPalette palette;
    if(!bIsSelected)
    {
        palette.setColor(QPalette::Base, QColor(35,35,37));
        sceneItem->setPalette(palette);
        sceneItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        sceneItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
        sceneItem->m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
    }
    else
    {
        palette.setColor(QPalette::Base, QColor(47,160,190));
        sceneItem->setPalette(palette);
        sceneItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        sceneItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
        sceneItem->m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
    }
}
void ButelLive::SetSceneMaterialItemColor(int iIndex, bool bIsSelected)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    SceneMaterialItem* materialItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(iIndex));
    setAutoFillBackground(true);
    QPalette palette;
    if(!bIsSelected)
    {
        palette.setColor(QPalette::Base, QColor(35,35,37));
        materialItem->setPalette(palette);
        materialItem->m_CheckBox.setStyleSheet("QCheckBox{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        materialItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        materialItem->m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        materialItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");

    }
    else
    {
        palette.setColor(QPalette::Base, QColor(47,160,190));
        materialItem->setPalette(palette);
        materialItem->m_CheckBox.setStyleSheet("QCheckBox{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        materialItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        materialItem->m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        materialItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    }
}
//鼠标点击素材项,设置其他项目的颜色为不选中
void ButelLive::SetMaterialItemColor(int iIndex,bool bIsSelected)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(iIndex));
    setAutoFillBackground(true);
    QPalette palette;
    if(!bIsSelected)
    {
        palette.setColor(QPalette::Base, QColor(35,35,37));
        materialItem->setPalette(palette);
        materialItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        materialItem->m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
        materialItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");

    }
    else
    {
        palette.setColor(QPalette::Base, QColor(47,160,190));
        materialItem->setPalette(palette);
        materialItem->m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        materialItem->m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
        materialItem->m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    }
}

//删除场景项
void ButelLive::on_m_pBtnDelScene_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iCurrentRow = GetSceneCurrentRow();
    if(iCurrentRow < 0) return;
    MyMessageBox message(this, tr("information"), tr("Are you sure you want to delete the existing project?"),BUTTON_YES|BUTTON_NO);
    if(message.exec() == ON_BUTTON_OK)
    {
        if(iCurrentRow-1>=0)
        {
            m_CurrentSceneNo -= 1;
        }
        else if(iCurrentRow-1<0 && iCurrentRow < ui->m_pLWScene->count())
        {
            m_CurrentSceneNo -= 1;
        }
        QListWidgetItem * item = ui->m_pLWScene->takeItem(iCurrentRow);
        CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
        delete sceneItem;
        delete item;
        m_ItemIsSelectedList.removeAt(iCurrentRow);
        setAutoFillBackground(true);
        QPalette palette;
        palette.setColor(QPalette::Base, QColor(35,35,37));
        SetSceneIndexNumber();
        if(iCurrentRow-1>=0)
        {
            SetSceneCurrentRow(iCurrentRow-1);
            SetItemColor(iCurrentRow-1,true);
            m_ItemIsSelectedList.replace(iCurrentRow-1,true);
        }
        else if(iCurrentRow-1<0 && iCurrentRow < ui->m_pLWScene->count())
        {
            SetSceneCurrentRow(iCurrentRow);
            SetItemColor(iCurrentRow,true);
            m_ItemIsSelectedList.replace(iCurrentRow,true);
        }

        int MatCount = m_SceneList.at(iCurrentRow)->m_LinkMateralVec.count();
        //删除正在预览的场景中的源
        if(m_bIsPreview || m_bIsLiving)
        {
             if(SLiveClearIntances(Instance) < 0)
             {
                 return;
             }
             else
             {

             }
        }

        SceneInfo * sceneInfo = m_SceneList.at(iCurrentRow);
        for(int i = 0; i < MatCount; i++)
        {
            bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
            if(bIsAgent)
            {
                int iLinkMatIndex = sceneInfo->m_LinkMateralVec.at(i).id;
                SLiveDestroyInstance(m_AgentInstanceVec[iLinkMatIndex]);
                m_AgentSourceList.removeAt(iLinkMatIndex);
                m_AgentInstanceVec.removeAt(iLinkMatIndex);
                for(int i = 0; i < m_SceneList.count(); i++)
                {
                    for(int j = 0; j < m_SceneList[i]->m_LinkMateralVec.count(); j++)
                    {
                        bool bIsAgent = m_SceneList[i]->m_LinkMateralVec[j].bIsAgent;
                        if(bIsAgent)
                        {
                            if(m_SceneList[i]->m_LinkMateralVec[j].id > iLinkMatIndex)
                                m_SceneList[i]->m_LinkMateralVec[j].id--;
                        }
                    }
                }
            }
        }
        for(int i = 0; i < this->actions().count(); i++)
        {
            if(this->actions().at(i)->shortcut().toString() == m_SceneList[iCurrentRow]->shortcutKeys)
            {
                this->removeAction(this->actions().at(i));
                break;
            }
        }
        m_SceneList.removeAt(iCurrentRow);
    }
    else
        return;
}

//设置当前场景索引号
void ButelLive::SetSceneCurrentRow(int iIndex)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pLWScene->setCurrentRow(iIndex);
    m_CurrentSceneNo = iIndex;
}
//获取当前场景索引号
int ButelLive::GetSceneCurrentRow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    return ui->m_pLWScene->currentRow();
}
//整体设置场景项中的编号为当前索引号
void ButelLive::SetSceneIndexNumber()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    for(int index = 0; index < ui->m_pLWScene->count(); index++)
    {
        CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(index));
        sceneItem->m_Index = index;
    }
}
//场景项上移
void ButelLive::on_m_pBtnMoveUp_clicked()
{  
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex <= 0) return;
    //场景项上下移操作，主预览区不需要发生改变，先断开场景切换信号连接
    QObject::disconnect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
    QListWidgetItem * item = ui->m_pLWScene->item(iIndex);
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
    m_pLWSceneItem = new QListWidgetItem;
    m_pLWSceneItem->setSizeHint(QSize(0,50));
    ui->m_pLWScene->insertItem(iIndex - 1,m_pLWSceneItem);
    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,sceneItem);
    m_ItemIsSelectedList.insert(iIndex - 1,true);
    delete item;
    m_ItemIsSelectedList.removeAt(iIndex + 1);
    m_SceneList.swap(iIndex,iIndex-1);
    SetSceneCurrentRow(iIndex - 1);
    SetSceneIndexNumber();
    QObject::connect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
}
//场景项下移
void ButelLive::on_m_pBtnMoveDown_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    int iCount = ui->m_pLWScene->count();
    if( iIndex >= iCount - 1) return;

    QObject::disconnect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
    QListWidgetItem * item = ui->m_pLWScene->item(iIndex + 1);
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
    m_pLWSceneItem = new QListWidgetItem;
    m_pLWSceneItem->setSizeHint(QSize(0,50));
    ui->m_pLWScene->insertItem(iIndex,m_pLWSceneItem);
    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,sceneItem);
    m_ItemIsSelectedList.insert(iIndex,false);
    delete item;
    m_ItemIsSelectedList.removeAt(iIndex + 2);
    m_SceneList.swap(iIndex,iIndex+1);
    SetSceneCurrentRow(iIndex + 1);
    SetSceneIndexNumber();
    QObject::connect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
}

//返回到场景窗口
void ButelLive::on_m_pBtnReturnScene_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pStckWdgt->setCurrentIndex(0);
}

//导入场景
void ButelLive::OnImportScene()
{
    QString strFileName = QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("*.cfg"));
    if(!strFileName.isEmpty())
    {
        if(m_bIsPreview || m_bIsLiving)
        {
            SLiveClearIntances(Instance);
        }
        ui->m_pLWScene->clear();
        ui->m_pLWPreview->clear();
        ui->m_pLWMaterial->clear();
        ui->m_pLWSceneMateralManage->clear();

        m_SceneList.clear();
        m_MateralList.clear();
        m_AgentSourceList.clear();
        m_ItemIsSelectedList.clear();
        m_MaterialItemIsSelectedList.clear();
        m_SceneMaterialItemIsSelectedList.clear();
        for(int i = 0; i < m_VideoInstanceVec.count(); i++)
        {
            SLiveDestroyInstance(m_VideoInstanceVec.at(i));
        }
        m_VideoInstanceVec.clear();
        for(int i = 0; i < m_AgentInstanceVec.count(); i++)
        {
            SLiveDestroyInstance(m_AgentInstanceVec.at(i));
        }
        m_AgentInstanceVec.clear();

        InitConfig(strFileName);
    }
}

//导出场景
void ButelLive::OnExportScene()
{
    char* ConfigList;
    if(0 == SLiveGetPreConfig(&ConfigList))
    {
        qDebug() << ConfigList;
        std::string utf;
        ConvertGBKToUtf8(utf,ConfigList);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();
        QJsonValue cfglist_value = json_root_obj.value(QString("ConfigList"));
        if(cfglist_value.isArray())
        {
            QJsonArray cfglist_arr = cfglist_value.toArray();
            for(int i = 0; i < cfglist_arr.count(); i++)
            {
                ConfigureInfo cfgInfo;
                QJsonValue cfglist_arr_item =  cfglist_arr.at(i);
                QJsonObject cfglist_arr_item_obj = cfglist_arr_item.toObject();
                QJsonValue cfglist_arr_item_instanceID_value = cfglist_arr_item_obj.take("InstanceID");
                uint64_t instanceID = cfglist_arr_item_instanceID_value.toString().toULongLong();
                QJsonValue cfglist_arr_item_streamID_value = cfglist_arr_item_obj.take("StreamID");
                uint64_t streamID = cfglist_arr_item_streamID_value.toString().toULongLong();
                QJsonValue cfglist_arr_item_data_value = cfglist_arr_item_obj.take("data");
                if(cfglist_arr_item_data_value.isObject())
                {
                    QJsonObject data_obj = cfglist_arr_item_data_value.toObject();
                    QString data = getStringFromJsonObject(data_obj);

                    cfgInfo.InstanceID = instanceID;
                    cfgInfo.StreamID = streamID;
                    cfgInfo.data = data;
                    m_ConfigVec.append(cfgInfo);
                }

            }
        }
        SLiveFreeMemory(ConfigList);
    }

    for(int i = 0; i < m_VideoInstanceVec.count(); i++)
    {
        for(int j = 0; j < m_ConfigVec.count(); j++)
        {
            if(m_VideoInstanceVec[i] == m_ConfigVec[j].InstanceID)
            {
                std::string utf;
                ConvertGBKToUtf8(utf,m_ConfigVec[j].data.toLocal8Bit().data());
                QString val(utf.c_str());
                g_MateralDataMap.insert(i,val);
            }
        }
    }
    for(int i = 0; i < m_AgentInstanceVec.count(); i++)
    {
        for(int j = 0; j < m_ConfigVec.count(); j++)
        {
            if(m_AgentInstanceVec[i] == m_ConfigVec[j].InstanceID)
            {
                std::string utf;
                ConvertGBKToUtf8(utf,m_ConfigVec[j].data.toLocal8Bit().data());
                QString val(utf.c_str());
                g_AgentDataMap.insert(i,val);
            }
        }
    }

    QString strSaveName = QFileDialog::getSaveFileName(this,tr("Save as"),"",tr("*.cfg"));
    if(!strSaveName.isEmpty())
    {
        SaveConfig(strSaveName);
    }
    g_MateralDataMap.clear();
    g_AgentDataMap.clear();
    return;
}
//设置备用场景
void ButelLive::OnActReserve()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex < 0) return;

    if(m_SceneList[iIndex]->bIsReserve)
    {
//        m_MenuSceneEdit.actions().at(10)->setText(tr("cancel set as reserve"));
        CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));
        sceneItem->m_LabelName.setText(m_SceneList[iIndex]->name);
        m_SceneList[iIndex]->bIsReserve = false;

    }
    else
    {
        for(int i = 0; i < m_SceneList.count(); i++)
        {
            if(m_SceneList[i]->bIsReserve)
            {
                CSceneItem* pItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(i));
                pItem->m_LabelName.setText(m_SceneList[i]->name);
                m_SceneList[i]->bIsReserve = false;
                break;
            }
        }
        CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));
        sceneItem->m_LabelName.setText(sceneItem->m_LabelName.text() + tr("[reserve](Shift + F2)"));
        m_SceneList[iIndex]->bIsReserve = true;
    }
}

//设置场景快捷键
void ButelLive::OnActSetSceneShortcut()
{
    int iIndex = GetSceneCurrentRow();
    if( iIndex < 0) return;
    if(m_SceneList[iIndex]->bIsReserve) return;
    SetShortcutKeyUI* pSetShortcutUI = new SetShortcutKeyUI(this,m_SceneList[iIndex]->shortcutKeys);
    if(ON_BUTTON_OK == pSetShortcutUI->exec())
    {
        QString qstrShortKey = pSetShortcutUI->getInputText();
        if(qstrShortKey.isEmpty())
        {
            for(int i = 0; i < this->actions().count(); i++)
            {
                if(this->actions().at(i)->shortcut().toString() == m_SceneList[iIndex]->shortcutKeys)
                {
                    this->removeAction(this->actions().at(i));
                    break;
                }
            }
            CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));
            sceneItem->m_LabelName.setText(m_SceneList[iIndex]->name);
            m_SceneList[iIndex]->shortcutKeys = qstrShortKey;
        }
        else
        {
            for(int i = 0; i < this->actions().count(); i++)
            {
                if((i == iIndex) && (this->actions().at(i)->shortcut().toString() == qstrShortKey))
                {
                    return;
                }
                else if((i != iIndex) && (this->actions().at(i)->shortcut().toString() == qstrShortKey))
                {
                    MyMessageBox message(this, tr("error"), tr("The shortcut key is already occupied!"),BUTTON_YES);
                    message.exec();
                    return;
                }
            }
            QAction* pActShortcut = new QAction(this);
            QObject::connect(pActShortcut, &QAction::triggered,[=](bool b){
                for(int i = 0; i < m_SceneList.count(); i++)
                {
                    if(m_SceneList[i]->shortcutKeys == pActShortcut->shortcut().toString())
                    {
                        m_CurrentSceneNo = i;
                        CSceneItem* pSceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(i));
                        pSceneItem->SetItemInfo();
                    }
                }
            });
            pActShortcut->setShortcut(qstrShortKey);
            CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));
            sceneItem->m_LabelName.setText(m_SceneList[iIndex]->name + "(" + qstrShortKey + ")");
            m_SceneList[iIndex]->shortcutKeys = qstrShortKey;
            this->addAction(pActShortcut);
        }
    }
}
//场景复制
void ButelLive::OnActSceneCopy()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex < 0) return;

    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iSceneNum++;
    int iSceneIndex = ui->m_pLWScene->count();
    QString strName = QString(tr("scene%1")).arg(m_iSceneNum);
    InputNameUI* dlgInName = new InputNameUI(this,Scene);
    dlgInName->setInputText(strName);

    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
      m_iSceneNum--;
      return;
    }
    QString qstrSceneName = dlgInName->getInputText();

    if(qstrSceneName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
        message.exec();
        m_iSceneNum--;
        return;
    }
    CSceneItem *SceneItem = new CSceneItem(this,qstrSceneName,iSceneIndex);
    m_pLWSceneItem = new QListWidgetItem;
    m_pLWSceneItem->setSizeHint(QSize(0,50));
    ui->m_pLWScene->addItem(m_pLWSceneItem);
    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,SceneItem);
    m_ItemIsSelectedList.append(false);
    SceneInfo* sceneInfo = new SceneInfo;
    sceneInfo->name =  qstrSceneName;
    sceneInfo->bIsReserve = false;
    for(int i = 0; i < m_SceneList[iIndex]->m_LinkMateralVec.count(); i++)
    {
        LinkSourceInfo sourceInfo = m_SceneList[iIndex]->m_LinkMateralVec.at(i);
        sceneInfo->m_LinkMateralVec.append(sourceInfo);
    }
    m_SceneList.append(sceneInfo);

    return;
}

//场景重命名
void ButelLive::OnActRename()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex < 0) return;
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(ui->m_pLWScene->item(iIndex));

    QString strName = m_SceneList[iIndex]->name;
    InputNameUI* dlgInName = new InputNameUI(this,Scene);
    dlgInName->setInputText(strName);
//    if(ON_BUTTON_OK == dlgInName->exec())
//    {
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL) return;
        QString qstrSceneName = dlgInName->getInputText();
        m_SceneList[iIndex]->name = qstrSceneName;
        if(m_SceneList[iIndex]->bIsReserve)
        {
            qstrSceneName = qstrSceneName + tr("[reserve](Shift + F2)");
        }
        else if(!m_SceneList[iIndex]->shortcutKeys.isEmpty())
        {
            qstrSceneName += "(" + m_SceneList[iIndex]->shortcutKeys + ")";
        }
        sceneItem->m_LabelName.setText(qstrSceneName);

//    }
}
//场景置顶
void ButelLive::OnMoveToTop()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    if( iIndex <= 0) return;
    //场景项上下移操作，主预览区不需要发生改变，先断开场景切换信号连接
    QObject::disconnect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
    QListWidgetItem * item = ui->m_pLWScene->item(iIndex);
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
    m_pLWSceneItem = new QListWidgetItem;
    m_pLWSceneItem->setSizeHint(QSize(0,50));
    ui->m_pLWScene->insertItem(0,m_pLWSceneItem);
    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,sceneItem);
    m_ItemIsSelectedList.insert(0,true);
    delete item;
    m_ItemIsSelectedList.removeAt(iIndex + 1);
    m_SceneList.swap(iIndex,0);
    SetSceneCurrentRow(0);
    SetSceneIndexNumber();
    QObject::connect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
}

//场景置底
void ButelLive::OnMoveToLow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetSceneCurrentRow();
    int iCount = ui->m_pLWScene->count();
    if( iIndex >= iCount - 1) return;
    //场景项上下移操作，主预览区不需要发生改变，先断开场景切换信号连接
    QObject::disconnect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
    QListWidgetItem * item = ui->m_pLWScene->item(iIndex);
    CSceneItem* sceneItem = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
    m_pLWSceneItem = new QListWidgetItem;
    m_pLWSceneItem->setSizeHint(QSize(0,50));
    ui->m_pLWScene->addItem(m_pLWSceneItem);
    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,sceneItem);
    m_ItemIsSelectedList.append(true);
    delete item;
    m_ItemIsSelectedList.removeAt(iIndex);
    m_SceneList.swap(iIndex,iCount - 1);
    SetSceneCurrentRow(iCount - 1);
    SetSceneIndexNumber();
    QObject::connect(ui->m_pLWScene,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWScene_currentRowChanged(int)));
}

void ButelLive::OnPictureSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iTupianNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("Picture source%1")).arg(m_iTupianNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
        m_iTupianNum--;
        return;
    }
    QString qstrMateralName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materalItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMateralName = materalItem->m_LabelName.text();
        if(strMateralName == qstrMateralName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMateralName),BUTTON_YES);
            message.exec();
            m_iTupianNum--;
            return;
        }
    }
    if(qstrMateralName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
        message.exec();
        m_iTupianNum--;
        return;
    }
    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMateralName;
    matInfo->type = Tupian;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"BitmapImageSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMateralName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixWidth;
    Area.height = m_pixHeight;

    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;

    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,qstrMateralName,tr("Picture source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = 20;
        sourceInfo.height = 20;
        sourceInfo.bRender = true;

        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,strName,Tupian,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}

//添加视频捕捉源
void ButelLive::OnVideoCatchSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    if(VideoList.count() < 0) return;

//    m_iMateralNum++;
    m_iShexiangNum++;
    int iMateralIndex = ui->m_pLWMaterial->count();
//    QString strName = QString(QStringLiteral("视频捕捉设备%1")).arg(m_iShexiangNum);
    QString strName = QString(tr("Video capture equipment%1")).arg(m_iShexiangNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
//    if(ON_BUTTON_OK == dlgInName->exec())
//    {
        int ret = dlgInName->exec();
        if(ret == ON_BUTTON_CANCEL)
        {
             m_iShexiangNum--;
             return;
        }

        QString qstrMateralName = dlgInName->getInputText();
        for(int i = 0; i < iMateralIndex; i++)
        {
            CMaterialItem* materalItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
            QString strMateralName = materalItem->m_LabelName.text();
            if(strMateralName == qstrMateralName)
            {
                MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMateralName),BUTTON_YES);
                message.exec();
                m_iShexiangNum--;
                return;
            }
        }
        if(qstrMateralName.isEmpty())
        {
            MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
            message.exec();
            m_iShexiangNum--;
            return;
        }



        QString strDShow = "{\"ClassName\":\"DeviceSource\",\"Name\":\"%1\"}";
        strDShow = strDShow.arg(qstrMateralName);
//        if(/*m_DeviceInitInfo.m_InputVideo == */VideoList.count() > 0)
        {
//            int w,h;
//            if(ConfigOper::instance()->m_EncodeFormat == 32)
//            {
//                w = 1280;
//                h = 720;
//            }
//            else if(ConfigOper::instance()->m_EncodeFormat == 16)
//            {
//                w = 960;
//                h = 540;
//            }
//            else if(ConfigOper::instance()->m_EncodeFormat == 4)
//            {
//                w = 640;
//                h = 360;
//            }

            CPreviewItem* preItem = new CPreviewItem(this,qstrMateralName,Shexiang,ui->m_pLWPreview->count());
            QListWidgetItem* item  = new QListWidgetItem;
            item->setSizeHint(QSize(210,150));
            ui->m_pLWPreview->addItem(item);
            ui->m_pLWPreview->setItemWidget(item,preItem);

            uint64_t tmpInstance;
            uint64_t hwnd = preItem->m_PreviewArea->winId();
            if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
                delete preItem;
                delete item;
                return;
            }

            m_VideoInstanceVec.append(tmpInstance);

            uint64_t StreamID1,StreamID2;
            VideoArea Area;
            Area.left = 0;
            Area.top = 0;
            Area.width = preItem->m_PreviewArea->width();
            Area.height = preItem->m_PreviewArea->height();
            Area.CropLeft = 0;
            Area.CropTop = 0;
            Area.CropRight = 0;
            Area.CropBottom = 0;
//            strDShow = strDShow.arg(VideoList.at(0).ShowName).arg(VideoList.at(0).DeviceID).arg(w).arg(h);
            if (SLiveAddStream(tmpInstance, strDShow.toLocal8Bit().data(), &Area, &StreamID1,&StreamID2) < 0)
            {
                m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
                delete preItem;
                delete item;
                return;
            }
            else
            {
                MateralInfo* matInfo = new MateralInfo;
                matInfo->name = qstrMateralName;
                matInfo->type = Shexiang;
                matInfo->source = "Shexiang";
                matInfo->streamID1 = StreamID1;
                matInfo->streamID2 = StreamID2;
                m_MateralList.append(matInfo);
            }

            CMaterialItem *MateralItem = new CMaterialItem(this,qstrMateralName,tr("Video capture equipment"),iMateralIndex);
            m_pLWMaterialItem = new QListWidgetItem;
            m_pLWMaterialItem->setSizeHint(QSize(0,50));
            ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
            ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MateralItem);
            m_MaterialItemIsSelectedList.append(false);
            SLiveStartResize(tmpInstance,false);
        }
        if(2 == ui->m_pStckWdgt->currentIndex())
        {
            int iCurScene = ui->m_pLWScene->currentRow();
            SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
            LinkSourceInfo sourceInfo;
            sourceInfo.id = m_MateralList.count() - 1;
            sourceInfo.left = 0;
            sourceInfo.top = 0;
            sourceInfo.width = m_pixWidth;
            sourceInfo.height = m_pixHeight;
            sourceInfo.bRender = true;
            sceneInfo->m_LinkMateralVec.append(sourceInfo);

            m_SceneList.replace(iCurScene,sceneInfo);
    //        ui->m_pLWSceneMateralManage->addItem(m_MateralList[sourceInfo.id]->name);

            SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
            m_pLWSceneMaterialItem = new QListWidgetItem;
            m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
            ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
            ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
            m_SceneMaterialItemIsSelectedList.insert(0,false);
            for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
            {
                SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
                pSceneMateralItem->m_Index = ii;
            }

            if(m_bIsPreview || m_bIsLiving)
                AddPreviewFromMateralNo(sourceInfo.id);
        }
//    }
//    else
//    {
//        m_iShexiangNum--;
//    }
    return;
}

//添加点播源
void ButelLive::OnRequestSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
//    m_iMateralNum++;
    m_iDianboNum++;
    int iMateralIndex = ui->m_pLWMaterial->count();
//    QString strName = QString(QStringLiteral("点播源%1")).arg(m_iDianboNum);
    QString strName = QString(tr("VOD source%1")).arg(m_iDianboNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
//    if(ON_BUTTON_OK == dlgInName->exec())
//    {
        int ret = dlgInName->exec();
        if(ret == ON_BUTTON_CANCEL)
        {
             m_iDianboNum--;
             return;
        }
        QString qstrMateralName = dlgInName->getInputText();
        for(int i = 0; i < iMateralIndex; i++)
        {
            CMaterialItem* materalItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
            QString strMateralName = materalItem->m_LabelName.text();
            if(strMateralName == qstrMateralName)
            {
                MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMateralName),BUTTON_YES);
                message.exec();
                m_iDianboNum--;
                return;
            }
        }
        if(qstrMateralName.isEmpty())
        {
            MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
            message.exec();
            m_iDianboNum--;
            return;
        }

//        QString name = QFileDialog::getOpenFileName(this,QStringLiteral("选择文件"),"",tr("*.mp4 *.avi *.flv *.ts *.mkv *.mov"));
//        if(!name.isEmpty())
//        {


            CPreviewItem* preItem = new CPreviewItem(this,qstrMateralName,Dianbo,ui->m_pLWPreview->count());
            QListWidgetItem* item  = new QListWidgetItem;
            item->setSizeHint(QSize(210,150));
            ui->m_pLWPreview->addItem(item);
            ui->m_pLWPreview->setItemWidget(item,preItem);

            uint64_t tmpInstance;
            uint64_t hwnd = preItem->m_PreviewArea->winId();
            if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
                delete preItem;
                delete item;
                return;
            }

            m_VideoInstanceVec.append(tmpInstance);

//            QString strJson_obj = "{\"ClassName\":\"VideoSource\",\"DemandFilePath\":\"%1\",\"Volume\":%2}";
//            strJson_obj = strJson_obj.arg(name).arg(1);
            QString strJson_obj = "{\"ClassName\":\"VideoSource\",\"Name\":\"%1\"}";
            strJson_obj = strJson_obj.arg(qstrMateralName);
            uint64_t StreamID1,StreamID2;
            VideoArea Area;
            Area.left = 0;
            Area.top = 0;
            Area.width = preItem->m_PreviewArea->width();
            Area.height = preItem->m_PreviewArea->height();
            Area.CropLeft = 0;
            Area.CropTop = 0;
            Area.CropRight = 0;
            Area.CropBottom = 0;
            if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&StreamID1, &StreamID2) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);

                m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
                delete preItem;
                delete item;

                return;
            }
            else
            {
                MateralInfo* matInfo = new MateralInfo;
                matInfo->name = qstrMateralName;
                matInfo->type = Dianbo;
                matInfo->source = "name";
                matInfo->streamID1 = StreamID1;
                matInfo->streamID2 = StreamID2;
                m_MateralList.append(matInfo);
                preItem->VideoInit();

            }
            SLiveStartResize(tmpInstance,false);
//        }
//        else
//        {
//            return;
//        }
            CMaterialItem *MateralItem = new CMaterialItem(this,qstrMateralName,tr("VOD source"),iMateralIndex);
            m_pLWMaterialItem = new QListWidgetItem;
            m_pLWMaterialItem->setSizeHint(QSize(0,50));
            ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
            ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MateralItem);
            m_MaterialItemIsSelectedList.append(false);

            if(2 == ui->m_pStckWdgt->currentIndex())
            {
                int iCurScene = ui->m_pLWScene->currentRow();
                SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
                LinkSourceInfo sourceInfo;
                sourceInfo.id = m_MateralList.count() - 1;
                sourceInfo.left = 0;
                sourceInfo.top = 0;
                sourceInfo.width = m_pixWidth;
                sourceInfo.height = m_pixHeight;
                sourceInfo.bRender = true;
                sceneInfo->m_LinkMateralVec.append(sourceInfo);

                m_SceneList.replace(iCurScene,sceneInfo);
        //        ui->m_pLWSceneMateralManage->addItem(m_MateralList[sourceInfo.id]->name);

                SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
                m_pLWSceneMaterialItem = new QListWidgetItem;
                m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
                ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
                ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
                m_SceneMaterialItemIsSelectedList.insert(0,false);
                for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
                {
                    SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
                    pSceneMateralItem->m_Index = ii;
                }

                if(m_bIsPreview || m_bIsLiving)
                    AddPreviewFromMateralNo(sourceInfo.id);
            }
//    }
//    else
//    {
//        m_iDianboNum--;
//    }
    return;
}


//添加互动连接源
void ButelLive::OnInteractConnectSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
//    int iMaterialIndex = ui->m_pLWMaterial->count();


//    InputNameUI* dlgInName = new InputNameUI(this);

//    m_iMateralNum++;
    m_iHudongNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
//    QString strName = QString(QStringLiteral("互动连接源%1")).arg(m_iHudongNum);
    QString strName = QString(tr("Interactive link source%1")).arg(m_iHudongNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);

//    if(ON_BUTTON_OK == dlgInName->exec())
//    {
        int ret = dlgInName->exec();
        if(ret == ON_BUTTON_CANCEL)
        {
             m_iHudongNum--;
             return;
        }
        QString qstrMaterialName = dlgInName->getInputText();
        for(int i = 0; i < iMaterialIndex; i++)
        {
            CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
            QString strMaterialName = materialItem->m_LabelName.text();
            if(strMaterialName == qstrMaterialName)
            {
                MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
                message.exec();
                m_iHudongNum--;
                return;
            }
        }
        if(qstrMaterialName.isEmpty())
        {
            MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
            message.exec();
            m_iHudongNum--;
            return;
        }
//        PipeDataCollectPropertyUI* pipeDataCollect = new PipeDataCollectPropertyUI(this);
//        if(pipeDataCollect->exec() == ON_BUTTON_OK)
//        {


            CPreviewItem* preItem = new CPreviewItem(this,strName,Hudong,ui->m_pLWPreview->count());
            QListWidgetItem* item  = new QListWidgetItem;
            item->setSizeHint(QSize(210,150));
            ui->m_pLWPreview->addItem(item);
            ui->m_pLWPreview->setItemWidget(item,preItem);

            uint64_t tmpInstance;
            uint64_t hwnd = preItem->m_PreviewArea->winId();
            if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
                delete preItem;
                delete item;

                return;
            }

            m_VideoInstanceVec.append(tmpInstance);

            QString strJson_obj = "{\"ClassName\":\"PipeVideo\",\"NubeNum\":\"%1\",\"AppKey\":\"%2\",\"NickName\":\"%3\",\"Name\":\"%4\"}";
            strJson_obj = strJson_obj.arg(m_pChannelInfo->interactivenumber).arg(m_pChannelInfo->appKey).arg("testname").arg(qstrMaterialName);
            uint64_t StreamID1,StreamID2;
            VideoArea Area;
            Area.left = 0;
            Area.top = 0;
            Area.width = preItem->m_PreviewArea->width();
            Area.height = preItem->m_PreviewArea->height();
            Area.CropLeft = 0;
            Area.CropTop = 0;
            Area.CropRight = 0;
            Area.CropBottom = 0;
            if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&StreamID1, &StreamID2) < 0)
            {
                Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);

                m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
                delete preItem;
                delete item;

                return;
            }
            else
            {
                MateralInfo* matInfo = new MateralInfo;
                matInfo->name = qstrMaterialName;
                matInfo->type = Hudong;
                matInfo->source = "name";
                matInfo->streamID1 = StreamID1;
                matInfo->streamID2 = StreamID2;
                m_MateralList.append(matInfo);

                CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("Interactive link source"),iMaterialIndex);
                m_pLWMaterialItem = new QListWidgetItem;
                m_pLWMaterialItem->setSizeHint(QSize(0,50));
                ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
                ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
                m_MaterialItemIsSelectedList.append(false);

            }
            SLiveStartResize(tmpInstance,false);
//        }
            if(2 == ui->m_pStckWdgt->currentIndex())
            {
                int iCurScene = ui->m_pLWScene->currentRow();
                SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
                LinkSourceInfo sourceInfo;
                sourceInfo.id = m_MateralList.count() - 1;
                sourceInfo.left = 0;
                sourceInfo.top = 0;
                sourceInfo.width = m_pixWidth;
                sourceInfo.height = m_pixHeight;
                sourceInfo.bRender = true;
                sceneInfo->m_LinkMateralVec.append(sourceInfo);

                m_SceneList.replace(iCurScene,sceneInfo);
        //        ui->m_pLWSceneMateralManage->addItem(m_MateralList[sourceInfo.id]->name);

                SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
                m_pLWSceneMaterialItem = new QListWidgetItem;
                m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
                ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
                ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
                m_SceneMaterialItemIsSelectedList.insert(0,false);
                for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
                {
                    SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
                    pSceneMateralItem->m_Index = ii;
                }

                if(m_bIsPreview || m_bIsLiving)
                    AddPreviewFromMateralNo(sourceInfo.id);
            }
//    }
//    else
//    {
//        m_iHudongNum--;
//    }
    return;
}

void ButelLive::OnMonitorCapture()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iMonitorCaptureNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("MonitorCapture source%1")).arg(m_iMonitorCaptureNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iMonitorCaptureNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iMonitorCaptureNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iMonitorCaptureNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = MonitorCapture;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"MonitorCaptureSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixMiniWidth;
    Area.height = m_pixMiniHeight;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("MonitoCapture source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = m_pixWidth;
        sourceInfo.height = m_pixHeight;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
void ButelLive::OnWindowCapture()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iWindowCaptureNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("WindowCapture source%1")).arg(m_iWindowCaptureNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iWindowCaptureNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iWindowCaptureNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iWindowCaptureNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = MonitorCapture;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"WindowCaptureSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixMiniWidth;
    Area.height = m_pixMiniHeight;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("WindowCapture source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = m_pixWidth;
        sourceInfo.height = m_pixHeight;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
void ButelLive::OnProcTopWindow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iProcTopWindowNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("ProcTopWindow source%1")).arg(m_iProcTopWindowNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iProcTopWindowNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iProcTopWindowNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iProcTopWindowNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = MonitorCapture;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"ProcTopWindowSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixMiniWidth;
    Area.height = m_pixMiniHeight;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("ProcTopWindowSource source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = m_pixWidth;
        sourceInfo.height = m_pixHeight;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
void ButelLive::OnTextSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iTextNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("Text source%1")).arg(m_iTextNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iTextNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iTextNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iTextNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = Text;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"TextOutputSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixMiniWidth;
    Area.height = m_pixMiniHeight;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("Text source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = m_pixWidth;
        sourceInfo.height = m_pixHeight;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
void ButelLive::OnLiveSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iLiveNum++;
    int iMateralIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("Live source%1")).arg(m_iLiveNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iLiveNum--;
         return;
    }
    QString qstrMateralName = dlgInName->getInputText();
    for(int i = 0; i < iMateralIndex; i++)
    {
        CMaterialItem* materalItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMateralName = materalItem->m_LabelName.text();
        if(strMateralName == qstrMateralName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMateralName),BUTTON_YES);
            message.exec();
            m_iLiveNum--;
            return;
        }
    }
    if(qstrMateralName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("Please enter a name!"),BUTTON_YES);
        message.exec();
        m_iLiveNum--;
        return;
    }
    CPreviewItem* preItem = new CPreviewItem(this,qstrMateralName,Live,ui->m_pLWPreview->count());
    QListWidgetItem* item  = new QListWidgetItem;
    item->setSizeHint(QSize(210,150));
    ui->m_pLWPreview->addItem(item);
    ui->m_pLWPreview->setItemWidget(item,preItem);

    uint64_t tmpInstance;
    uint64_t hwnd = preItem->m_PreviewArea->winId();
    if (SLiveCreateInstance(&tmpInstance,hwnd,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        delete preItem;
        delete item;
        return;
    }
    m_VideoInstanceVec.append(tmpInstance);

    QJsonArray channelArr;
    CNetInerface* pNetInstance = CNetInerface::GetInstance();
    for(int i = 0; i < pNetInstance->m_channelList.count(); i++)
    {
        QJsonObject channel_item_obj;
        channel_item_obj.insert("name", pNetInstance->m_channelList[i]->name);
        channel_item_obj.insert("flv", pNetInstance->m_channelList[i]->flv_playurl);
        channel_item_obj.insert("m3u8", pNetInstance->m_channelList[i]->m3u8_playurl);
        channelArr.append(channel_item_obj);
    }

    QByteArray byte_array = QJsonDocument(channelArr).toJson();
    QString playList(byte_array);

//    QJsonObject channelList_obj;
//    channelList_obj.insert("PlayList", channelArr);
//    QString playList = getStringFromJsonObject(channelList_obj);

//    QString playList = "[\"%1\",\"%2\"]";
//    playList = playList.arg(m_pChannelInfo->flv_playurl,m_pChannelInfo->m3u8_playurl);

    QString strJson_obj = "{\"ClassName\":\"VideoLiveSource\",\"Name\":\"%1\",\"PlayList\":%2}";
    strJson_obj = strJson_obj.arg(qstrMateralName).arg(playList);
    qDebug() << strJson_obj;

    uint64_t StreamID1,StreamID2;
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = preItem->m_PreviewArea->width();
    Area.height = preItem->m_PreviewArea->height();
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&StreamID1, &StreamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);

        m_VideoInstanceVec.removeAt(m_VideoInstanceVec.count()-1);
        delete preItem;
        delete item;

        return;
    }
    else
    {
        MateralInfo* matInfo = new MateralInfo;
        matInfo->name = qstrMateralName;
        matInfo->type = Live;
        matInfo->streamID1 = StreamID1;
        matInfo->streamID2 = StreamID2;
        m_MateralList.append(matInfo);

    }
    SLiveStartResize(tmpInstance,false);
    CMaterialItem *MateralItem = new CMaterialItem(this,qstrMateralName,tr("Live source"),iMateralIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MateralItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = m_pixWidth;
        sourceInfo.height = m_pixHeight;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);
        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }
        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
}
// 添加时间源菜单项响应
void ButelLive::OnDateTimeSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iDateTimeNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("Date time source%1")).arg(m_iDateTimeNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iDateTimeNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iDateTimeNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iDateTimeNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = Datetime;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"ProcessDateTime\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = 200;
    Area.height = 48;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("Date time source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = 200;
        sourceInfo.height = 48;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
// 添加音频捕捉设备菜单项响应
void ButelLive::OnAudioCatchSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iAudioCaptureNum++;
    int iMaterialIndex = ui->m_pLWMaterial->count();
    QString strName = QString(tr("Audio capture device%1")).arg(m_iAudioCaptureNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iAudioCaptureNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(i));
        QString strMaterialName = materialItem->m_LabelName.text();
        if(strMaterialName == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iAudioCaptureNum--;
            return;
        }
    }
    if(qstrMaterialName.isEmpty())
    {
        MyMessageBox message(this, tr("error"), tr("please enter a name!"),BUTTON_YES);
        message.exec();
        m_iAudioCaptureNum--;
        return;
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = AudioCapture;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"DSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = 20;
    Area.height = 20;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_VideoInstanceVec.append(tmpInstance);
        m_MateralList.append(matInfo);
    }

    CMaterialItem *MatertialItem = new CMaterialItem(this,strName,tr("Date time source"),iMaterialIndex);
    m_pLWMaterialItem = new QListWidgetItem;
    m_pLWMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWMaterial->addItem(m_pLWMaterialItem);
    ui->m_pLWMaterial->setItemWidget(m_pLWMaterialItem,MatertialItem);
    m_MaterialItemIsSelectedList.append(false);

    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iCurScene = ui->m_pLWScene->currentRow();
        SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
        LinkSourceInfo sourceInfo;
        sourceInfo.id = m_MateralList.count() - 1;
        sourceInfo.left = 0;
        sourceInfo.top = 0;
        sourceInfo.width = 20;
        sourceInfo.height = 20;
        sourceInfo.bRender = true;
        sceneInfo->m_LinkMateralVec.append(sourceInfo);

        m_SceneList.replace(iCurScene,sceneInfo);

        SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[sourceInfo.id]->name,m_MateralList[sourceInfo.id]->type,ui->m_pLWSceneMateralManage->count());
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
        m_SceneMaterialItemIsSelectedList.insert(0,false);
        for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
            pSceneMateralItem->m_Index = ii;
        }

        if(m_bIsPreview || m_bIsLiving)
            AddPreviewFromMateralNo(sourceInfo.id);
    }
    return;
}
void ButelLive::OnAgentSource()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_iAgentNum++;
    int iMaterialIndex = m_AgentSourceList.count();
    QString strName = QString(tr("Agent source%1")).arg(m_iAgentNum);
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL)
    {
         m_iAgentNum--;
         return;
    }
    QString qstrMaterialName = dlgInName->getInputText();
    for(int i = 0; i < iMaterialIndex; i++)
    {
        if(m_AgentSourceList[i]->name == qstrMaterialName)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'already exists. Please enter a different name!")).arg(qstrMaterialName),BUTTON_YES);
            message.exec();
            m_iAudioCaptureNum--;
            return;
        }
    }

    MateralInfo* matInfo = new MateralInfo;
    matInfo->name = qstrMaterialName;
    matInfo->type = AgentSource;

    uint64_t tmpInstance;
    if (SLiveCreateInstance(&tmpInstance,NULL,false,true) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveCreateInstance failed!", __FUNCTION__);
        return;
    }
    QString strJson_obj = "{\"ClassName\":\"AgentSource\",\"Name\":\"%1\"}";
    strJson_obj = strJson_obj.arg(qstrMaterialName);
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixWidth;
    Area.height = m_pixHeight;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if (SLiveAddStream(tmpInstance,strJson_obj.toLocal8Bit().data(),&Area,&matInfo->streamID1, &matInfo->streamID2) < 0)
    {
        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveAddStream failed!", __FUNCTION__);
        return;
    }
    else
    {
        m_AgentInstanceVec.append(tmpInstance);
        m_AgentSourceList.append(matInfo);
    }

    int iCurScene = ui->m_pLWScene->currentRow();
    SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
    LinkSourceInfo sourceInfo;
    sourceInfo.id = m_AgentSourceList.count() - 1;
    sourceInfo.left = 0;
    sourceInfo.top = 0;
    sourceInfo.width = m_pixWidth;
    sourceInfo.height = m_pixHeight;
    sourceInfo.bRender = true;
    sourceInfo.cutLeft = 0;
    sourceInfo.cutTop = 0;
    sourceInfo.cutWidth = m_pixWidth;
    sourceInfo.cutHeight = m_pixHeight;
    sourceInfo.bIsCut = false;
    sourceInfo.bIsAgent = true;
    sceneInfo->m_LinkMateralVec.append(sourceInfo);

    m_SceneList.replace(iCurScene,sceneInfo);

    SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,qstrMaterialName,AgentSource,ui->m_pLWSceneMateralManage->count());
    m_pLWSceneMaterialItem = new QListWidgetItem;
    m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
    ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
    ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
    m_SceneMaterialItemIsSelectedList.insert(0,false);
    for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
    {
        SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
        pSceneMateralItem->m_Index = ii;
    }

    if(m_bIsPreview || m_bIsLiving)
        AddPreviewFromMateralNo(sourceInfo.id,true);

    return;
}
void ButelLive::SetSelectSourceIndex(int index)
{
    for(int i = 0; i < ui->m_pLWSceneMateralManage->count(); i++)
    {
        SceneMaterialItem* pItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(i));
        pItem->m_Index = i;
    }
//    int iSel = ui->m_pLWSceneMateralManage->count() - index - 1;
    if(2 == ui->m_pStckWdgt->currentIndex() && index < ui->m_pLWSceneMateralManage->count())
    {
        //ui->m_pLWSceneMateralManage->setCurrentRow(iSel);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(index));
        pSMItem->SetItemInfo(index);
    }
    qDebug() << "ui->m_pLWSceneMateralManage->currentRow() = " << ui->m_pLWSceneMateralManage->currentRow();
}

void ButelLive::SetSelectIndexSource(int index)
{
    int iSel = ui->m_pLWSceneMateralManage->count() - index - 1;
    if(2 == ui->m_pStckWdgt->currentIndex() && iSel < ui->m_pLWSceneMateralManage->count())
    {
        //ui->m_pLWSceneMateralManage->setCurrentRow(iSel);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(iSel));
        pSMItem->SetItemInfo(iSel);
    }
    qDebug() << "ui->m_pLWSceneMateralManage->currentRow() = " << ui->m_pLWSceneMateralManage->currentRow();
}
//设置当前场景管理素材索引号
void ButelLive::SetSceneMaterialCurrentRow(int iIndex)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pLWSceneMateralManage->setCurrentRow(iIndex);
}
//获取当前场景管理素材索引号
int ButelLive::GetSceneMaterialCurrentRow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    return ui->m_pLWSceneMateralManage->currentRow();
}

//设置当前素材索引号
void ButelLive::SetMaterialCurrentRow(int iIndex)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pLWMaterial->setCurrentRow(iIndex);
}
//获取当前素材索引号
int ButelLive::GetMaterialCurrentRow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    return ui->m_pLWMaterial->currentRow();
}
//整体设置素材项中的编号为当前索引号
void ButelLive::SetMaterialIndexNumber()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    for(int index = 0; index < ui->m_pLWMaterial->count(); index++)
    {
        CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(index));
        materialItem->m_Index = index;
    }
}

//删除素材
void ButelLive::on_m_pBtnDelMaterial_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iCurrentRow = GetMaterialCurrentRow();
    if(iCurrentRow < 0) return;
    MyMessageBox message(this, tr("information"), tr("Are you sure you want to delete the existing project?"),BUTTON_YES|BUTTON_NO);
    if(message.exec() == ON_BUTTON_OK)
    {
        MateralInfo* matInfo = m_MateralList.at(iCurrentRow);
        for(int i = 0; i < m_SceneList.count(); i++)
        {
            SceneInfo* scnInfo = m_SceneList.at(i);
            for(int j = 0;  j < scnInfo->m_LinkMateralVec.count(); j++)
            {
                bool bIsAgent = scnInfo->m_LinkMateralVec.at(j).bIsAgent;
                if(!bIsAgent)
                {
                    if(scnInfo->m_LinkMateralVec.at(j).id == iCurrentRow)
                    {
                        if(m_bIsPreview || m_bIsLiving)
                        {
                            if(SLiveDelStream(Instance,matInfo->streamID1) < 0)
                            {

                            }
                        }
                        scnInfo->m_LinkMateralVec.removeAt(j);
                    }
                }
            }
        }
        if(matInfo->type == Dianbo || matInfo->type == Shexiang || matInfo->type == Hudong || matInfo->type == Live)
        {

            int iPrewNo = GetMateralSourceLinkPreviewNo(iCurrentRow);
            if(iPrewNo == -1) return;
            QListWidgetItem * item = ui->m_pLWPreview->takeItem(iPrewNo);
            CPreviewItem* PreItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(item);
            delete PreItem;
            delete item;
//            SLiveDestroyInstance(m_VideoInstanceVec.at(iPrewNo));
//            m_VideoInstanceVec.removeAt(iPrewNo);
//            SLiveDestroyInstance(m_VideoInstanceVec.at(iCurrentRow));
//            m_VideoInstanceVec.removeAt(iCurrentRow);

            //删除预览项以后，将剩下的序号置为当前索引
            for(int i = 0; i < ui->m_pLWPreview->count(); i++)
            {
                CPreviewItem* pPreItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(i));
                pPreItem->m_Index = i;
            }
        }
        SLiveDestroyInstance(m_VideoInstanceVec.at(iCurrentRow));
        m_VideoInstanceVec.removeAt(iCurrentRow);
        m_MateralList.removeAt(iCurrentRow);

        //移除素材列表项后，将全部场景中所关联素材编号大于当前要删除的素材号减1

        for(int ii = 0; ii < m_SceneList.count(); ii++)
        {
            SceneInfo* scnInfo = m_SceneList.at(ii);
            for(int jj = 0;  jj < scnInfo->m_LinkMateralVec.count(); jj++)
            {
//                int iMatIndex = scnInfo->m_LinkMateralVec.at(jj);
                int iMatIndex = scnInfo->m_LinkMateralVec.at(jj).id;
                bool bIsAgent = scnInfo->m_LinkMateralVec.at(jj).bIsAgent;
                if(!bIsAgent)
                {
                    if(iMatIndex > iCurrentRow)
                    {
                        iMatIndex--;
                        LinkSourceInfo sourceInfo = scnInfo->m_LinkMateralVec.at(jj);
                        sourceInfo.id = iMatIndex;
                        scnInfo->m_LinkMateralVec.replace(jj,sourceInfo);
    //                    scnInfo->m_LinkMateralVec.replace(jj,iMatIndex);
                    }
                }

            }
        }

        QListWidgetItem * item = ui->m_pLWMaterial->takeItem(iCurrentRow);
        CMaterialItem* MateriaItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(item);
        delete MateriaItem;
        delete item;
        m_MaterialItemIsSelectedList.removeAt(iCurrentRow);
        setAutoFillBackground(true);
        QPalette palette;
        palette.setColor(QPalette::Base, QColor(35,35,37));
        SetMaterialIndexNumber();

        if(iCurrentRow-1>=0)
        {
            SetMaterialCurrentRow(iCurrentRow-1);
            SetMaterialItemColor(iCurrentRow-1,true);
            m_MaterialItemIsSelectedList.replace(iCurrentRow-1,true);
        }
        else if(iCurrentRow-1<0 && iCurrentRow < ui->m_pLWMaterial->count())
        {
            SetMaterialCurrentRow(iCurrentRow);
            SetMaterialItemColor(iCurrentRow,true);
            m_MaterialItemIsSelectedList.replace(iCurrentRow,true);
        }
    }
    else
        return;

}

//素材编辑 添加至当前场景 菜单项 响应
void ButelLive::OnMaterialAddToCurrentScene()
{
    int iMatIndex = GetMaterialCurrentRow();
    int iCurScene = GetSceneCurrentRow();
    if(iCurScene < 0) return;
    for(int j = 0; j < m_SceneList[iCurScene]->m_LinkMateralVec.count(); j++)
    {
        if(m_SceneList[iCurScene]->m_LinkMateralVec[j].id == iMatIndex)
        {
            MyMessageBox message(this, tr("error"), QString(tr("'%1'in the current scene, cannot be added again！")).arg(m_MateralList.at(iMatIndex)->name),BUTTON_YES);
            message.exec();
            return;
        }
    }

//    SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[i]->name,m_MateralList[i]->type,ui->m_pLWSceneMateralManage->count());
//    m_pLWSceneMaterialItem = new QListWidgetItem;
//    m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
//    ui->m_pLWSceneMateralManage->addItem(m_pLWSceneMaterialItem);
//    ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
//    m_SceneMaterialItemIsSelectedList.append(false);

    SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
    LinkSourceInfo sourceInfo;
    sourceInfo.id = iMatIndex;
    sourceInfo.left = 0;
    sourceInfo.top = 0;
    sourceInfo.width = m_pixWidth;
    sourceInfo.height = m_pixHeight;
    sourceInfo.bRender = true;
    sourceInfo.cutLeft = 0;
    sourceInfo.cutTop = 0;
    sourceInfo.cutWidth = m_pixWidth;
    sourceInfo.cutHeight = m_pixHeight;
    sourceInfo.bIsCut = false;
    sceneInfo->m_LinkMateralVec.append(sourceInfo);

    m_SceneList.replace(iCurScene,sceneInfo);
    if(m_bIsPreview || m_bIsLiving)
        AddPreviewFromMateralNo(iMatIndex);
}
//素材重命名
void ButelLive::OnMaterialRename()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = GetMaterialCurrentRow();
    if( iIndex < 0) return;

    CMaterialItem* materialItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(iIndex));

    QString strName = materialItem->m_LabelName.text();
    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL) return;
    QString qstrSceneName = dlgInName->getInputText();
    if(SLiveReNameStream(m_VideoInstanceVec[iIndex],m_MateralList[iIndex]->streamID1,qstrSceneName.toLocal8Bit().data()) < 0)
    {
        return;
    }
    else
    {
        materialItem->m_LabelName.setText(qstrSceneName);
        m_MateralList[iIndex]->name = qstrSceneName;
        if(m_MateralList[iIndex]->type == Dianbo || m_MateralList[iIndex]->type == Shexiang || m_MateralList[iIndex]->type == Hudong || m_MateralList[iIndex]->type == Live)
        {
            CPreviewItem* pPreviewItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(GetMateralSourceLinkPreviewNo(iIndex)));
            pPreviewItem->m_LbSourceName.setText(qstrSceneName);
        }
    }
}
//素材设置
void ButelLive::OnMaterialSet()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iLinkMatIndex = GetMaterialCurrentRow();
    if(iLinkMatIndex < 0) return;
    if(SLiveConfigStream(m_VideoInstanceVec.at(iLinkMatIndex),m_MateralList.at(iLinkMatIndex)->streamID1) < 0)
    {
        return;
    }
    else
    {
        if(m_MateralList[iLinkMatIndex]->type == Dianbo)
        {
            int iPreviewIndex = GetMateralSourceLinkPreviewNo(iLinkMatIndex);
            if(iPreviewIndex == -1) return;
            CPreviewItem* pPreviewItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(iPreviewIndex));
            pPreviewItem->VideoDataFresh();
        }
    }
}

//打开或关闭声音操作
void ButelLive::on_m_pBtnListen_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    m_bPlayLocal = !m_bPlayLocal;
    SetAudioPara();
    if(m_bPlayLocal)
    {
        ui->m_pBtnListen->setStyleSheet("QPushButton"
                                        "{"
                                            "border-image:url(:images/tyt_listen.png);"
                                        "}"
                                        "QPushButton:hover"
                                        "{"
                                            "border-image:url(:images/tyt_listen_hover.png);"
                                        "}"
                                        "QPushButton:pressed"
                                        "{"
                                            "border-image:url(:images/tyt_listen_close.png);"
                                        "}");
    }
    else
    {
        ui->m_pBtnListen->setStyleSheet("QPushButton"
                                        "{"
                                            "border-image:url(:images/tyt_listen_close.png);"
                                        "}"
                                        "QPushButton:hover"
                                        "{"
                                            "border-image:url(:images/tyt_listen_close_hover.png);"
                                        "}"
                                        "QPushButton:pressed"
                                        "{"
                                            "border-image:url(:images/tyt_listen.png);"
                                        "}");
    }
}
//2016.10.12 修改鼠标滚动出现item中颜色不一致的问题
void ButelLive::on_m_pLWScene_itemEntered(QListWidgetItem *item)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    CSceneItem* pSceneItem  = (CSceneItem*)ui->m_pLWScene->itemWidget(item);
    pSceneItem->MouseScrollEnterItem();
}

//场景管理中添加素材
void ButelLive::on_m_pBtnScnMtrlAdd_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    QMenu addMateralMenu;
    QMenu* materalMenu= new QMenu(tr("add existing material"),this);
    QAction* pAreaSource = new QAction(tr("add area source"),this);\
    connect(pAreaSource,SIGNAL(triggered (bool)),this,SLOT(OnAgentSource()));
    for(int i = 0; i < m_MateralList.count(); i++)
    {
        QAction* pAct = new QAction(m_MateralList.at(i)->name,this);
        materalMenu->addAction(pAct);
        QObject::connect(pAct, &QAction::triggered,[=](bool b){

//            ui->m_pLWSceneMateralManage->addItem(m_MateralList.at(i)->name);

            for(int j = 0; j < m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec.count(); j++)
            {
                int iMatIndex = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[j].id;
                bool bIsAgent = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[j].bIsAgent;
                if(!bIsAgent)
                {
                    if(iMatIndex == i)
                    {
                        MyMessageBox message(this, tr("error"), QString(tr("'%1'in the current scene, cannot be added again!")).arg(m_MateralList.at(i)->name),BUTTON_YES);
                        message.exec();
                        return;
                    }
                }
            }
            SceneMaterialItem* pSceneMateralItem = new SceneMaterialItem(this,m_MateralList[i]->name,m_MateralList[i]->type,ui->m_pLWSceneMateralManage->count());
            m_pLWSceneMaterialItem = new QListWidgetItem;
            m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
            ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
            ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSceneMateralItem);
            m_SceneMaterialItemIsSelectedList.insert(0,false);

            for(int ii = 0; ii < ui->m_pLWSceneMateralManage->count(); ii++)
            {
                SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(ii));
                pSceneMateralItem->m_Index = ii;
            }

            int iCurScene = ui->m_pLWScene->currentRow();
            SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
            LinkSourceInfo sourceInfo;
            sourceInfo.id = i;
            sourceInfo.left = 0;
            sourceInfo.top = 0;
            sourceInfo.bIsCut = false;
            sourceInfo.cutLeft = 0;
            sourceInfo.cutTop = 0;
            int iType = m_MateralList[i]->type;
            if(iType == Datetime)
            {
                sourceInfo.width = 200;
                sourceInfo.height = 48;
                sourceInfo.cutWidth = 200;
                sourceInfo.cutHeight = 48;
            }
            else if(iType == AudioCapture)
            {
                sourceInfo.width = 20;
                sourceInfo.height = 20;
                sourceInfo.cutWidth = 20;
                sourceInfo.cutHeight = 20;
            }
            else
            {
                sourceInfo.width = m_pixWidth;
                sourceInfo.height = m_pixHeight;
                sourceInfo.cutWidth = m_pixWidth;
                sourceInfo.cutHeight = m_pixHeight;
            }
            sourceInfo.bRender = true;
            sceneInfo->m_LinkMateralVec.append(sourceInfo);
            m_SceneList.replace(iCurScene,sceneInfo);
            if(m_bIsPreview || m_bIsLiving)
                AddPreviewFromMateralNo(i);
        });
        if(i != m_MateralList.count() -1)
            materalMenu->addSeparator();
    }
    addMateralMenu.addMenu(materalMenu);
    addMateralMenu.addSeparator();
    addMateralMenu.addAction(pAreaSource);
    addMateralMenu.addSeparator();
    addMateralMenu.addActions(m_MenuMaterialAdd.actions());
    materalMenu->setStyleSheet(m_MenuMaterialAdd.styleSheet());
    addMateralMenu.setStyleSheet(m_MenuMaterialAdd.styleSheet());
    addMateralMenu.exec(QCursor::pos());
}

//本地预览
void ButelLive::on_m_pBtnLocalPreview_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iCurScene = ui->m_pLWScene->currentRow();
    if(iCurScene < 0) return;
    SceneInfo * sceneInfo = m_SceneList.at(iCurScene);

    if(!m_bIsPreview)
    {
        for(int i = 0; i < sceneInfo->m_LinkMateralVec.count(); i++)
        {
            int iMateralIndex = sceneInfo->m_LinkMateralVec.at(i).id;
            bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
//            if(sceneInfo->m_LinkMateralVec[i].bRender)
                AddPreviewFromMateralNo(iMateralIndex,bIsAgent,sceneInfo->m_LinkMateralVec[i].bRender);
        }
        m_bIsPreview = true;
        ui->m_pBtnLocalPreview->setText(tr("StopPreview"));
        ui->m_pBtnStartLive->setEnabled(false);
        ui->m_pBtnEditMode->setEnabled(true);
    }
    else
    {
        SLiveClearIntances(Instance);
        SLiveClearIntances(Instance_2);
        m_bIsPreview = false;
        ui->m_pBtnLocalPreview->setText(tr("LocalPreview"));
        ui->m_pBtnStartLive->setEnabled(true);
        ui->m_pBtnEditMode->setEnabled(false);
        if(m_bIsEditing)
        {
            if(m_SelLevel >= 0)
            {
                int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
                bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
                if(bIsAgent)
                {
                    SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,false);
                }
                else
                {
                  SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,false);
                }
            }
            m_bIsEditing = false;
            ui->m_pBtnEditMode->setStyleSheet("QPushButton{color: rgba(255, 255, 255, 255);background-color: rgba(57, 57, 59, 0);}");
        }
    }
    m_CurrentSceneNo = iCurScene;
}

//切入直播
void ButelLive::on_m_pBtnToLive_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    UINT disTime = ui->m_pLdtToLiveTime->text().toUInt();
    int iDx = ui->m_pCmbToLive->currentIndex();
    switch (iDx) {
    case 0:
        if(SLiveSwitchInstance(Instance,Instance_2,Cut) < 0)
        {
            return;
        }
        break;
    case 1:
        if(SLiveSwitchInstance(Instance,Instance_2,DisSolve,disTime) < 0)
        {
            return;
        }
        break;
    case 2:
        if(SLiveSwitchInstance(Instance,Instance_2,UpDown) < 0)
        {
            return;
        }
        break;
    case 3:
        if(SLiveSwitchInstance(Instance,Instance_2,Diffuse) < 0)
        {
            return;
        }
        break;
    case 4:
        if(SLiveSwitchInstance(Instance,Instance_2,Radius) < 0)
        {
            return;
        }
        break;
    default:
        break;
    }
}

void ButelLive::resizeEvent(QResizeEvent *event)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    SLiveStartResize(Instance,false);
    SLiveStartResize(Instance_2,false);
    ResizeRenderFrame();
    ResizeRenderFrame_2();
}

void ButelLive::ResizeRenderFrame()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    float mainAspect;
    mainAspect = float(outputCX) / float(outputCY);
    int x, y;
    UINT controlWidth = ui->m_pDisArea->width();
    UINT controlHeight = ui->m_pDisArea->height();
    // Scale to fit，等比例缩放
    float RenderSizeX = float(controlWidth);
    float RenderSizeY = float(controlHeight);
    float renderAspect = RenderSizeX / RenderSizeY;

    if (renderAspect > mainAspect)
    {
        RenderSizeX = RenderSizeY*mainAspect;
        x = int((float(controlWidth) - RenderSizeX)*0.5f);
        y = 0;
    }
    else
    {
        RenderSizeY = RenderSizeX / mainAspect;
        x = 0;
        y = int((float(controlHeight) - RenderSizeY)*0.5f);
    }
    renderFrameOffsetx = x;
    renderFrameOffsety = y;
    renderFrameSizex = int(RenderSizeX + 0.5f) & 0xFFFFFFFE;
    renderFrameSizey = int(RenderSizeY + 0.5f) & 0xFFFFFFFE;
}

void ButelLive::ResizeRenderFrame_2()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    float mainAspect;
    mainAspect = float(outputCX) / float(outputCY);
    int x, y;
    UINT controlWidth = ui->m_pDisArea_2->width();
    UINT controlHeight = ui->m_pDisArea_2->height();
    // Scale to fit，等比例缩放
    float RenderSizeX = float(controlWidth);
    float RenderSizeY = float(controlHeight);
    float renderAspect = RenderSizeX / RenderSizeY;

    if (renderAspect > mainAspect)
    {
        RenderSizeX = RenderSizeY*mainAspect;
        x = int((float(controlWidth) - RenderSizeX)*0.5f);
        y = 0;
    }
    else
    {
        RenderSizeY = RenderSizeX / mainAspect;
        x = 0;
        y = int((float(controlHeight) - RenderSizeY)*0.5f);
    }
    renderFrameOffsetx = x;
    renderFrameOffsety = y;
    renderFrameSizex = int(RenderSizeX + 0.5f) & 0xFFFFFFFE;
    renderFrameSizey = int(RenderSizeY + 0.5f) & 0xFFFFFFFE;
}

void ButelLive::OnBtnToPreview()
{

}


void ButelLive::SetPreviewCurrentRow(int index)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    ui->m_pLWPreview->setCurrentRow(index);
}
int ButelLive::GetPreviewCurrentRow()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    return ui->m_pLWPreview->currentRow();
}

//根据预览窗的索引查出对应的素材索引
int  ButelLive::GetPreviewSourceLinkMateralNo(int iPreNo)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = -1;
    for(int i = 0; i < m_MateralList.count(); i++)
    {
        if(m_MateralList.at(i)->type == Dianbo || m_MateralList.at(i)->type == Shexiang || m_MateralList.at(i)->type == Hudong || m_MateralList[i]->type == Live) //后续添加其他源判断
        {
            iIndex++;
            if(iPreNo == iIndex)
                return i;
        }
    }
    return -1;
}
//根据素材索引查出对应的预览窗的索引
int  ButelLive::GetMateralSourceLinkPreviewNo(int iMatNo)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iIndex = -1;
    for(int i = 0; i <= iMatNo; i++)
    {
        if(m_MateralList.at(i)->type == Dianbo || m_MateralList.at(i)->type == Shexiang || m_MateralList.at(i)->type == Hudong || m_MateralList.at(i)->type == Live) //后续添加其他源判断
        {
            iIndex++;
        }
    }
    return iIndex;
}

void ButelLive::AddMateralToScene(int materalIndex,int sceneIndex)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iCurScene = ui->m_pLWScene->currentRow();
    if(iCurScene < 0) return;
    if(m_bIsPreview || m_bIsLiving)
    {
        if(SLiveAdd2Agent(m_MateralList[materalIndex]->name.toLocal8Bit().data()) < 0)
        {
            return;
        }
        else
        {

        }
    }
}

void ButelLive::on_m_pLWScene_currentRowChanged(int currentRow)
{
    m_SceneMaterialItemIsSelectedList.clear();
    m_SelLevel = -1;
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    if(!m_bIsPreview && !m_bIsLiving)
    {
        m_CurrentSceneNo = currentRow;
        return;
    }

    if((m_bIsPreview || m_bIsLiving) && m_CurrentSceneNo >= 0 && m_CurrentSceneNo < m_SceneList.count() && currentRow >= 0)
    {
        SLiveClearIntances(Instance);
        //开始预览现在场景中源
        SceneInfo * sceneInfo = m_SceneList.at(currentRow);
        for(int i = 0; i < sceneInfo->m_LinkMateralVec.count(); i++)
        {
            int iMateralIndex = sceneInfo->m_LinkMateralVec.at(i).id;
            bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
            bool bRender = sceneInfo->m_LinkMateralVec[i].bRender;
//            if(sceneInfo->m_LinkMateralVec[i].bRender)
                AddPreviewFromMateralNo(iMateralIndex,bIsAgent,bRender);

        }
        m_CurrentSceneNo = currentRow;
    }
    else if((m_bIsPreview || m_bIsLiving) && m_CurrentSceneNo < 0 && currentRow >= 0 && currentRow >= 0)
    {
        //开始预览现在场景中源
        SceneInfo * sceneInfo = m_SceneList.at(currentRow);
        for(int i = 0; i < sceneInfo->m_LinkMateralVec.count(); i++)
        {           
            int iMateralIndex = sceneInfo->m_LinkMateralVec.at(i).id;
            bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
            bool bRender = sceneInfo->m_LinkMateralVec[i].bRender;
//            if(sceneInfo->m_LinkMateralVec[i].bRender)
                AddPreviewFromMateralNo(iMateralIndex,bIsAgent,bRender);

        }
        m_CurrentSceneNo = currentRow;
    }
}

//删除场景关联素材
void ButelLive::on_m_pBtnScnMtrlDel_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iCurScene = ui->m_pLWScene->currentRow();
//    int index = ui->m_pLWSceneMateralManage->currentRow();
//    if(index < 0) return;
//    int iCurScnMat = ui->m_pLWSceneMateralManage->count() - index - 1;
    int iCurScnMat = m_SelLevel;
    if(iCurScnMat < 0) return;
    uint64_t streamID1;
    int iLinkMatIndex = m_SceneList.at(iCurScene)->m_LinkMateralVec.at(iCurScnMat).id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[iCurScnMat].bIsAgent;
    if(bIsAgent)
    {
        streamID1 = m_AgentSourceList.at(iLinkMatIndex)->streamID1;
    }
    else
    {
        streamID1 = m_MateralList.at(iLinkMatIndex)->streamID1;
    }
    if(m_bIsPreview || m_bIsLiving)
    {
        if(SLiveDelStream(Instance,streamID1) < 0)
        {
            return;
        }
    }
    if(bIsAgent)
    {
        SLiveDestroyInstance(m_AgentInstanceVec[iLinkMatIndex]);
        m_AgentSourceList.removeAt(iLinkMatIndex);
        m_AgentInstanceVec.removeAt(iLinkMatIndex);
        for(int i = 0; i < m_SceneList.count(); i++)
        {
            for(int j = 0; j < m_SceneList[i]->m_LinkMateralVec.count(); j++)
            {
                bool bIsAgent = m_SceneList[i]->m_LinkMateralVec[j].bIsAgent;
                if(bIsAgent)
                {
                    if(m_SceneList[i]->m_LinkMateralVec[j].id > iLinkMatIndex)
                        m_SceneList[i]->m_LinkMateralVec[j].id--;
                }
            }
        }
    }
    m_SceneList.at(iCurScene)->m_LinkMateralVec.removeAt(iCurScnMat);
    m_SelLevel = -1;
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        disconnect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        int index = ui->m_pLWSceneMateralManage->currentRow();
        QListWidgetItem * item = ui->m_pLWSceneMateralManage->takeItem(index);
        delete item;
        m_SceneMaterialItemIsSelectedList.removeAt(iCurScnMat);
        ui->m_pLWSceneMateralManage->setCurrentRow(-1);
        for(int i = 0; i < ui->m_pLWSceneMateralManage->count(); i++)
        {
            SceneMaterialItem* pSceneMateralItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(i));
            pSceneMateralItem->m_Index = i;
        }
        connect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
    }
}


void ButelLive::InitDeviceList()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    //获取视频捕捉设备
    char* ListVideo;
    VideoList.clear();
    if(0 == SLiveGetVideoCaptureList(&ListVideo))
    {
        qDebug() << ListVideo;
        std::string utf;
        ConvertGBKToUtf8(utf,ListVideo);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();
        QJsonValue devicelist_value = json_root_obj.value(QString("DeviceList"));
        if(devicelist_value.isArray())
        {
            QJsonArray devicelist_arr = devicelist_value.toArray();
            for(int i = 0; i < devicelist_arr.count(); i++)
            {
                DeviceInfo deviceInfo;
                QJsonValue devicelist_arr_item =  devicelist_arr.at(i);
                QJsonObject devicelist_arr_item_obj = devicelist_arr_item.toObject();
                QJsonValue devicelist_arr_item_ShowName_value = devicelist_arr_item_obj.take("ShowName");
                QString ShowName = devicelist_arr_item_ShowName_value.toString();
                QJsonValue devicelist_arr_item_DeviceName_value = devicelist_arr_item_obj.take("DeviceName");
                QString DeviceName = devicelist_arr_item_DeviceName_value.toString();
                QJsonValue devicelist_arr_item_DeviceID_value = devicelist_arr_item_obj.take("DeviceID");
                QString DeviceID = devicelist_arr_item_DeviceID_value.toString();
                DeviceID = DeviceID.replace(QChar('\\'),QString("\\\\"));
                deviceInfo.ShowName = ShowName;
                deviceInfo.DeviceName = DeviceName;
                deviceInfo.DeviceID = DeviceID;
                VideoList.push_back(deviceInfo);
            }
        }
        SLiveFreeMemory(ListVideo);
    }

    //获取音频捕捉设备
    char* AudioVideo;
    AudioCaptureList.clear();
    if(0 == SLiveGetAudioCaptureList(&AudioVideo))
    {
        qDebug() << AudioVideo;
        std::string utf;
        ConvertGBKToUtf8(utf,AudioVideo);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();
        QJsonValue devicelist_value = json_root_obj.value(QString("DeviceList"));
        if(devicelist_value.isArray())
        {
            QJsonArray devicelist_arr = devicelist_value.toArray();
            for(int i = 0; i < devicelist_arr.count(); i++)
            {
                QJsonValue devicelist_arr_item =  devicelist_arr.at(i);
                QJsonObject devicelist_arr_item_obj = devicelist_arr_item.toObject();
                QJsonValue devicelist_arr_item_obj_value = devicelist_arr_item_obj.take("DeviceName");
                QString deviceName = devicelist_arr_item_obj_value.toString();
                AudioCaptureList.append(deviceName);
            }
        }
        SLiveFreeMemory(AudioVideo);
    }

    //获取音频渲染设备
    char* AudioRender;
    AudioRenderList.clear();
    if(0 == SLiveGetAudioRenderList(&AudioRender))
    {
        qDebug() << AudioRender;
        std::string utf;
        ConvertGBKToUtf8(utf,AudioRender);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();
        QJsonValue devicelist_value = json_root_obj.value(QString("DeviceList"));
        if(devicelist_value.isArray())
        {
            QJsonArray devicelist_arr = devicelist_value.toArray();
            for(int i = 0; i < devicelist_arr.count(); i++)
            {
                QJsonValue devicelist_arr_item =  devicelist_arr.at(i);
                QString deviceName = devicelist_arr_item.toString();
                AudioRenderList.append(deviceName);
            }
        }
    }
    SLiveFreeMemory(AudioRender);

    //获取显卡列表
    char* DisplayDevices;
    DisplayDeviceList.clear();
    if(0 == SLiveDisplayDevices(&DisplayDevices))
    {
        qDebug() << DisplayDevices;
        std::string utf;
        ConvertGBKToUtf8(utf,DisplayDevices);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();
        QJsonValue devicelist_value = json_root_obj.value(QString("NameList"));
        if(devicelist_value.isArray())
        {
            QJsonArray devicelist_arr = devicelist_value.toArray();
            for(int i = 0; i < devicelist_arr.count(); i++)
            {
                QJsonValue devicelist_arr_item =  devicelist_arr.at(i);
                QString deviceName = devicelist_arr_item.toString();
                DisplayDeviceList.append(deviceName);
            }
        }
    }
    SLiveFreeMemory(DisplayDevices);


}

//鼠标按下调用
Vect ButelLive::MapWindowToFramePos(Vect& mousePos)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    Vect ReVect;
    ReVect.x = (mousePos.x - renderFrameOffsetx) * (outputCX / renderFrameSizex);
    ReVect.y = (mousePos.y - renderFrameOffsety) * (outputCY / renderFrameSizey);
    return ReVect;
}

// 鼠标按下移动调用
Vect ButelLive::MoveMapWindowToFramePos(Vect& mousePos)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    Vect ReVect;
    ReVect.x = (mousePos.x) * (outputCX / renderFrameSizex);
    ReVect.y = (mousePos.y) * (outputCY / renderFrameSizey);
    return ReVect;
}

//将对应素材添加至预览
void ButelLive::AddPreviewFromMateralNo(int iMatNo,bool bIsAgent,bool bIsRender)
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    int iType;
    int streamW,streamH,left,top;
    int streamCutW,streamCutH,cutLeft,cutTop;
    bool bIsCut = false;
    if(bIsAgent)
    {
        MateralInfo* pMatInfo = m_AgentSourceList.at(iMatNo);
        iType = pMatInfo->type;
        int iCurScene = GetSceneCurrentRow();
        for(int i = 0; i < m_SceneList[iCurScene]->m_LinkMateralVec.count(); i++)
        {
            bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsAgent;
            if(bIsAgent)
            {
                if(m_SceneList[iCurScene]->m_LinkMateralVec[i].id == iMatNo)
                {
                    streamW = m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
                    streamH = m_SceneList[iCurScene]->m_LinkMateralVec[i].height;
                    left = m_SceneList[iCurScene]->m_LinkMateralVec[i].left;
                    top = m_SceneList[iCurScene]->m_LinkMateralVec[i].top;

                    streamCutW = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutWidth;
                    streamCutH = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutHeight;
                    cutLeft = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutLeft;
                    cutTop = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutTop;

                    bIsCut = m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsCut;

                    break;
                }
            }
        }
    }
    else
    {
        MateralInfo* pMatInfo = m_MateralList.at(iMatNo);
        iType = pMatInfo->type;
        int iCurScene = GetSceneCurrentRow();
        for(int i = 0; i < m_SceneList[iCurScene]->m_LinkMateralVec.count(); i++)
        {
            bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsAgent;
            if(!bIsAgent)
            {
                if(m_SceneList[iCurScene]->m_LinkMateralVec[i].id == iMatNo)
                {
                    streamW = m_SceneList[iCurScene]->m_LinkMateralVec[i].width;
                    streamH = m_SceneList[iCurScene]->m_LinkMateralVec[i].height;
                    left = m_SceneList[iCurScene]->m_LinkMateralVec[i].left;
                    top = m_SceneList[iCurScene]->m_LinkMateralVec[i].top;

                    streamCutW = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutWidth;
                    streamCutH = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutHeight;
                    cutLeft = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutLeft;
                    cutTop = m_SceneList[iCurScene]->m_LinkMateralVec[i].cutTop;

                    bIsCut = m_SceneList[iCurScene]->m_LinkMateralVec[i].bIsCut;

                    break;
                }
            }
        }
    }
    VideoArea Area;
    Area.left = left;
    Area.top = top;
    Area.width = streamCutW;
    Area.height = streamCutH;

    Area.CropLeft = left - cutLeft;
    Area.CropTop = top - cutTop;
    Area.CropRight = (streamCutW-streamW)-(left-cutLeft);
    Area.CropBottom = (streamCutH-streamH)-(top-cutTop);

//    VideoArea AreaCut;
//    AreaCut.left = cutLeft;
//    AreaCut.top = cutTop;
//    AreaCut.width = streamCutW;
//    AreaCut.height = streamCutH;

    qDebug() << "***************************************************";
    qDebug() << Area.left << Area.top << Area.width << Area.height;
    qDebug() << Area.CropLeft << Area.CropTop << Area.CropRight << Area.CropBottom;

    if(iType == AgentSource)
    {

//        if(!bIsCut)
//        {
//            if(SLiveAdd2Intance(m_AgentInstanceVec.at(iMatNo),Instance,&Area) < 0)
//            {
//                return;
//            }
//        }
//        else
//        {
//            if(SLiveAdd2Intance(m_AgentInstanceVec.at(iMatNo),Instance,&AreaCut) < 0)
//            {
//                return;
//            }
//        }
        if(SLiveAdd2Intance(m_AgentInstanceVec.at(iMatNo),Instance,&Area,bIsRender) < 0)
        {
            return;
        }
    }
    else
    {
//        if(!bIsCut)
//        {
//            if(SLiveAdd2Intance(m_VideoInstanceVec.at(iMatNo),Instance,&Area) < 0)
//            {
//                return;
//            }
//        }
//        else
//        {
//            if(SLiveAdd2Intance(m_VideoInstanceVec.at(iMatNo),Instance,&AreaCut) < 0)
//            {
//                return;
//            }
//        }
        if(SLiveAdd2Intance(m_VideoInstanceVec.at(iMatNo),Instance,&Area,bIsRender) < 0)
        {
            return;
        }
    }
}


void ButelLive::on_m_pBtnStartLive_clicked()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
    if(m_bIsLiving)
    {
        if(SLiveStopLive(Instance_2) < 0)
        {
            return;
        }
        else
        {
            m_bIsLiving = false;
            ui->m_pBtnStartLive->setText(tr("start live"));
            ui->m_pBtnLocalPreview->setEnabled(true);
            SLiveClearIntances(Instance);
            SLiveClearIntances(Instance_2);
            ui->m_pBtnEditMode->setEnabled(false);
            int iCurScene = ui->m_pLWScene->currentRow();
            if(iCurScene < 0) return;
            if(m_bIsEditing)
            {
                if(m_SelLevel >= 0)
                {
                    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
                    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
                    if(bIsAgent)
                    {
                        SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,false);
                    }
                    else
                    {
                      SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,false);
                    }
                }
                m_bIsEditing = false;
                ui->m_pBtnEditMode->setStyleSheet("QPushButton{color: rgba(255, 255, 255, 255);background-color: rgba(57, 57, 59, 0);}");
            }
        }
    }
    else
    {
        QString strPushStreamAdd = "{\"PushUrl\":\"%1\"}";
        strPushStreamAdd = strPushStreamAdd.arg(m_pChannelInfo->pushstreamurl);
        qDebug() << "strPushStreamAdd =" << strPushStreamAdd;
        if (SLiveStartLive(Instance_2,strPushStreamAdd.toStdString().c_str(),true) < 0)
        {
            return;
        }
        else
        {
            m_bIsLiving = true;
            ui->m_pBtnStartLive->setText(tr("stop live"));
            ui->m_pBtnLocalPreview->setEnabled(false);

            ui->m_pBtnEditMode->setEnabled(true);

            int iCurScene = ui->m_pLWScene->currentRow();
            if(iCurScene < 0) return;
            SceneInfo * sceneInfo = m_SceneList.at(iCurScene);
            for(int i = 0; i < sceneInfo->m_LinkMateralVec.count(); i++)
            {
                int iMateralIndex = sceneInfo->m_LinkMateralVec.at(i).id;
                bool bIsAgent = sceneInfo->m_LinkMateralVec.at(i).bIsAgent;
//                if(sceneInfo->m_LinkMateralVec[i].bRender)
                    AddPreviewFromMateralNo(iMateralIndex,bIsAgent,sceneInfo->m_LinkMateralVec[i].bRender);
            }
            m_CurrentSceneNo = iCurScene;
            SLiveSwitchInstance(Instance,Instance_2,Cut);
        }
    }
}

void ButelLive::on_m_pBtnEditMode_clicked()
{
    if(!m_bIsPreview && !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
    if(iCurScene < 0) return;
    if(!m_bIsEditing)
    {
        m_SelLevel = m_SceneList[iCurScene]->m_LinkMateralVec.count() - 1;
        int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
        bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
        if(bIsAgent)
        {
            SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,true);
        }
        else
        {
          SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,true);
        }
        m_bIsEditing = true;
        ui->m_pBtnEditMode->setStyleSheet("QPushButton{color: rgba(0, 255, 0, 255);background-color: rgba(57, 57, 59, 0);}");
    }
    else
    {
        if(m_SelLevel >= 0)
        {
            int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
            bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
            if(bIsAgent)
            {
                SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,false);
            }
            else
            {
              SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,false);
            }
        }
        m_bIsEditing = false;
        ui->m_pBtnEditMode->setStyleSheet("QPushButton{color: rgba(255, 255, 255, 255);background-color: rgba(57, 57, 59, 0);}");
    }
}

//下移
void ButelLive::OnMoveStreamDowm(bool)
{
    int iCurScene = GetSceneCurrentRow();
    if(m_SelLevel <= 0 || m_SelLevel > m_SceneList[iCurScene]->m_LinkMateralVec.count()-1)
        return;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
    if(bIsAgent)
    {
        if(SLiveMoveStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,MoveDown) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.insert(m_SelLevel-1,streamInfo);
            m_SelLevel--;
        }
    }
    else
    {
        if(SLiveMoveStream(Instance,m_MateralList[iMatIndex]->streamID1,MoveDown) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.insert(m_SelLevel-1,streamInfo);
            m_SelLevel--;
        }
    }
}

//上移
void ButelLive::OnMoveStreamUp(bool)
{
    int iCurScene = GetSceneCurrentRow();
    if(m_SelLevel >= m_SceneList[iCurScene]->m_LinkMateralVec.count()-1)
        return;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
    if(bIsAgent)
    {
        if(SLiveMoveStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,MoveUp) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            if(m_SelLevel+1 < m_SceneList[iCurScene]->m_LinkMateralVec.count())
                m_SceneList[iCurScene]->m_LinkMateralVec.insert(m_SelLevel+1,streamInfo);
            else if(m_SelLevel+1 >= m_SceneList[iCurScene]->m_LinkMateralVec.count())
                m_SceneList[iCurScene]->m_LinkMateralVec.append(streamInfo);
            m_SelLevel++;
        }
    }
    else
    {
        if(SLiveMoveStream(Instance,m_MateralList[iMatIndex]->streamID1,MoveUp) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            if(m_SelLevel+1 < m_SceneList[iCurScene]->m_LinkMateralVec.count())
                m_SceneList[iCurScene]->m_LinkMateralVec.insert(m_SelLevel+1,streamInfo);
            else if(m_SelLevel+1 >= m_SceneList[iCurScene]->m_LinkMateralVec.count())
                m_SceneList[iCurScene]->m_LinkMateralVec.append(streamInfo);
            m_SelLevel++;
        }
    }
}

//置顶
void ButelLive::OnMoveStreamTop(bool)
{
    int iCurScene = GetSceneCurrentRow();
    if(m_SelLevel >=  m_SceneList[iCurScene]->m_LinkMateralVec.count()-1)
        return;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
    if(bIsAgent)
    {
        if(SLiveMoveStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,MoveTop) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.append(streamInfo);
            m_SelLevel = m_SceneList[iCurScene]->m_LinkMateralVec.count()-1;
        }
    }
    else
    {
        if(SLiveMoveStream(Instance,m_MateralList[iMatIndex]->streamID1,MoveTop) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.append(streamInfo);
            m_SelLevel = m_SceneList[iCurScene]->m_LinkMateralVec.count()-1;
        }
    }
}

//置底
void ButelLive::OnMoveStreamBottom(bool)
{
    int iCurScene = GetSceneCurrentRow();
    if(m_SelLevel <= 0)
        return;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
    if(bIsAgent)
    {
        if(SLiveMoveStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,MoveBottom) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.insert(0,streamInfo);
            m_SelLevel = 0;
        }
    }
    else
    {
        if(SLiveMoveStream(Instance,m_MateralList[iMatIndex]->streamID1,MoveBottom) < 0)
        {
            return;
        }
        else
        {
            LinkSourceInfo streamInfo = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel];
            m_SceneList[iCurScene]->m_LinkMateralVec.removeAt(m_SelLevel);
            m_SceneList[iCurScene]->m_LinkMateralVec.insert(0,streamInfo);
            m_SelLevel = 0;
        }
    }
}

bool ButelLive::SceneIsExist(const QString& name)
{
    for(int i = 0; i < m_SceneList.count(); i++)
    {
        if(m_SceneList[i]->name ==  name)
            return true;
    }
    return false;
}
bool ButelLive::MateralIsExist(const QString& name)
{
    for(int i = 0; i < m_MateralList.count(); i++)
    {
        if(m_MateralList[i]->name ==  name)
            return true;
    }
    return false;
}

void ButelLive::OnSceneMaterialEditClicked()
{
    m_MenuSceneMaterialEdit.exec(QCursor::pos());
}

//场景素材管理素材重命名
void ButelLive::OnManagerRename()
{
    Log::writeMessage(LOG_RTSPSERV, 1, "%s ", __FUNCTION__);
//    int iCount = ui->m_pLWSceneMateralManage->count();
//    int index = iCount - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iIndex = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[index].bIsAgent;
    if( iIndex < 0) return;

    QString strName;
    if(bIsAgent)
    {
        strName = m_AgentSourceList[iIndex]->name;
    }
    else
    {
        strName = m_MateralList[iIndex]->name;
    }

    InputNameUI* dlgInName = new InputNameUI(this,Materal);
    dlgInName->setInputText(strName);
    int ret = dlgInName->exec();
    if(ret == ON_BUTTON_CANCEL) return;
    QString qstrSceneName = dlgInName->getInputText();


    if(bIsAgent)
    {
        if(SLiveReNameStream(m_AgentInstanceVec[iIndex],m_AgentSourceList[iIndex]->streamID1,qstrSceneName.toLocal8Bit().data()) < 0)
        {
            return;
        }
        else
        {
           m_AgentSourceList[iIndex]->name = qstrSceneName;
        }

    }
    else
    {
        if(SLiveReNameStream(m_VideoInstanceVec[iIndex],m_MateralList[iIndex]->streamID1,qstrSceneName.toLocal8Bit().data()) < 0)
        {

        }
        else
        {
            CMaterialItem* pMatItem = (CMaterialItem*)ui->m_pLWMaterial->itemWidget(ui->m_pLWMaterial->item(iIndex));
            pMatItem->m_LabelName.setText(qstrSceneName);
            m_MateralList[iIndex]->name = qstrSceneName;
            if(m_MateralList[iIndex]->type == Dianbo || m_MateralList[iIndex]->type == Shexiang || m_MateralList[iIndex]->type == Hudong || m_MateralList[iIndex]->type == Live)
            {
                CPreviewItem* pPreviewItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(GetMateralSourceLinkPreviewNo(iIndex)));
                pPreviewItem->m_LbSourceName.setText(qstrSceneName);
            }
        }
    }
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        SceneMaterialItem* pScnMatItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(ui->m_pLWSceneMateralManage->item(GetSceneMaterialCurrentRow()));
        pScnMatItem->m_LabelName.setText(qstrSceneName);
    }


}
//场景素材管理素材设置
void ButelLive::OnManagerSet()
{
//    int index = ui->m_pLWSceneMateralManage->currentRow();
    int index = m_SelLevel;
    if(index < 0) return;
//    int iCurScnMat = ui->m_pLWSceneMateralManage->count() - index - 1;
    int iCurScnMat = index;
    if(iCurScnMat < 0) return;

    int iLinkMatIndex = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[iCurScnMat].id;
    bool bIsAgent = m_SceneList[GetSceneCurrentRow()]->m_LinkMateralVec[iCurScnMat].bIsAgent;
    if(bIsAgent)
    {
        if(SLiveConfigStream(m_AgentInstanceVec.at(iLinkMatIndex),m_AgentSourceList.at(iLinkMatIndex)->streamID1) < 0)
        {
            return;
        }
        else
        {

        }
    }
    else
    {        
        if(SLiveConfigStream(m_VideoInstanceVec.at(iLinkMatIndex),m_MateralList.at(iLinkMatIndex)->streamID1) < 0)
        {
            return;
        }
        else
        {
            if(m_MateralList[iLinkMatIndex]->type == Dianbo)
            {
                int iPreviewIndex = GetMateralSourceLinkPreviewNo(iLinkMatIndex);
                if(iPreviewIndex < 0) return;
                CPreviewItem* pPreviewItem = (CPreviewItem*)ui->m_pLWPreview->itemWidget(ui->m_pLWPreview->item(iPreviewIndex));
                pPreviewItem->VideoDataFresh();
            }
        }
    }

}


void ButelLive::on_m_pBtnScnMtrlMoveUp_clicked()
{
    if(m_SelLevel < 0) return;
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iIndex = GetSceneMaterialCurrentRow();
        if(iIndex <= 0) return;
        disconnect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        QListWidgetItem * item = ui->m_pLWSceneMateralManage->item(iIndex);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(item);
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(iIndex - 1,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSMItem);
        delete item;
        SetSelectSourceIndex(iIndex - 1);
        connect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        m_SceneMaterialItemIsSelectedList.swap(iIndex,iIndex - 1);
    }
    OnMoveStreamUp(true);
}

void ButelLive::on_m_pBtnScnMtrlMoveDown_clicked()
{
    if(m_SelLevel < 0) return;
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iIndex = GetSceneMaterialCurrentRow();
        if(iIndex >=  ui->m_pLWSceneMateralManage->count()-1) return;
        disconnect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        QListWidgetItem * item = ui->m_pLWSceneMateralManage->item(iIndex+1);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(item);
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(iIndex,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSMItem);
        delete item;
        SetSelectSourceIndex(iIndex + 1);
        m_SceneMaterialItemIsSelectedList.swap(iIndex,iIndex + 1);
        connect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
    }
    OnMoveStreamDowm(true);
}

void ButelLive::on_m_pBtnScnMtrlMoveTop_clicked()
{
    if(m_SelLevel < 0) return;
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iIndex = GetSceneMaterialCurrentRow();
        if(iIndex <=  0) return;
        disconnect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        QListWidgetItem * item = ui->m_pLWSceneMateralManage->item(iIndex);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(item);
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->insertItem(0,m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSMItem);
        delete item;
        SetSelectSourceIndex(0);
        m_SceneMaterialItemIsSelectedList.swap(iIndex,0);
        connect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
    }
    OnMoveStreamTop(true);
}

void ButelLive::on_m_pBtnScnMtrlMoveBottom_clicked()
{
    if(m_SelLevel < 0) return;
    if(2 == ui->m_pStckWdgt->currentIndex())
    {
        int iIndex = GetSceneMaterialCurrentRow();
        if(iIndex >=  ui->m_pLWSceneMateralManage->count()-1) return;
        disconnect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
        QListWidgetItem * item = ui->m_pLWSceneMateralManage->item(iIndex);
        SceneMaterialItem* pSMItem = (SceneMaterialItem*)ui->m_pLWSceneMateralManage->itemWidget(item);
        m_pLWSceneMaterialItem = new QListWidgetItem;
        m_pLWSceneMaterialItem->setSizeHint(QSize(0,50));
        ui->m_pLWSceneMateralManage->addItem(m_pLWSceneMaterialItem);
        ui->m_pLWSceneMateralManage->setItemWidget(m_pLWSceneMaterialItem,pSMItem);
        delete item;
        SetSelectSourceIndex(ui->m_pLWSceneMateralManage->count() - 1);
        m_SceneMaterialItemIsSelectedList.swap(iIndex,ui->m_pLWSceneMateralManage->count()-1);
        connect(ui->m_pLWSceneMateralManage,SIGNAL(currentRowChanged(int)),this,SLOT(on_m_pLWSceneMateralManage_currentRowChanged(int)));
    }
    OnMoveStreamBottom(true);   
}

void ButelLive::on_m_pLWSceneMateralManage_currentRowChanged(int currentRow)
{
    if(currentRow < 0) return;
    int iCurScene = GetSceneCurrentRow();
    if(iCurScene < 0) return;
    int iMatIndex;

    if(m_SelLevel >= 0 && m_SelLevel < ui->m_pLWSceneMateralManage->count())
    {
        bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;
        iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
        if(m_bIsEditing)
        {
            if(bIsAgent)
            {
                SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,false);
            }
            else
            {
                SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,false);
            }
        }

    }

    m_SelLevel = ui->m_pLWSceneMateralManage->count() - currentRow - 1;
    iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[m_SelLevel].bIsAgent;

    if(m_bIsEditing)
    {
        if(bIsAgent)
        {
            SLiveSelectStream(Instance,m_AgentSourceList[iMatIndex]->streamID1,true);
        }
        else
        {
            SLiveSelectStream(Instance,m_MateralList[iMatIndex]->streamID1,true);
        }
    }




}

void ButelLive::OnActFitScreen()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = 0;
    Area.top = 0;
    Area.width = m_pixWidth;
    Area.height = m_pixHeight;

    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;

    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = 0;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = 0;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].width = m_pixWidth;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].height = m_pixHeight;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = 0;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = 0;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].width = m_pixWidth;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].height = m_pixHeight;
        }
    }
}
void ButelLive::OnActResetSize()
{

}
void ButelLive::OnActSetPosSize()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    VideoArea Area;
    Area.left = m_SceneList[iCurScene]->m_LinkMateralVec[index].left;
    Area.top = m_SceneList[iCurScene]->m_LinkMateralVec[index].top;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    PositionSizeSetUI* pPosSizeSetUi = new PositionSizeSetUI(this,&Area);
    pPosSizeSetUi->show();
}
void ButelLive::OnActResetCut()
{

}

void ButelLive::OnActCenter()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = (m_pixWidth - m_SceneList[iCurScene]->m_LinkMateralVec[index].width)/2;
    Area.top = (m_pixHeight - m_SceneList[iCurScene]->m_LinkMateralVec[index].height)/2;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }

}
void ButelLive::OnActHCenter()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = (m_pixWidth - m_SceneList[iCurScene]->m_LinkMateralVec[index].width)/2;
    Area.top = m_SceneList[iCurScene]->m_LinkMateralVec[index].top;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
        }
    }

}
void ButelLive::OnActVCenter()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = m_SceneList[iCurScene]->m_LinkMateralVec[index].left;
    Area.top = (m_pixHeight - m_SceneList[iCurScene]->m_LinkMateralVec[index].height)/2;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }

}
void ButelLive::OnActMoveLeftEdge()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = 0;
    Area.top = m_SceneList[iCurScene]->m_LinkMateralVec[index].top;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = 0;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = 0;
        }
    }

}
void ButelLive::OnActMoveTopEdge()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = m_SceneList[iCurScene]->m_LinkMateralVec[index].left;
    Area.top = 0;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = 0;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = 0;
        }
    }

}
void ButelLive::OnActMoveRightEdge()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = m_pixWidth - m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.top = m_SceneList[iCurScene]->m_LinkMateralVec[index].top;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].left = Area.left;
        }
    }

}
void ButelLive::OnActMoveBottomEdge()
{
    if(!m_bIsPreview || !m_bIsLiving) return;
    int iCurScene = GetSceneCurrentRow();
//    int index = ui->m_pLWSceneMateralManage->count() - GetSceneMaterialCurrentRow() - 1;
    int index = m_SelLevel;
    int iMatIndex = m_SceneList[iCurScene]->m_LinkMateralVec[index].id;
    bool bIsAgent = m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    VideoArea Area;
    Area.left = m_SceneList[iCurScene]->m_LinkMateralVec[index].left;
    Area.top = m_pixHeight - m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.width = m_SceneList[iCurScene]->m_LinkMateralVec[index].width;
    Area.height = m_SceneList[iCurScene]->m_LinkMateralVec[index].height;
    Area.CropLeft = 0;
    Area.CropTop = 0;
    Area.CropRight = 0;
    Area.CropBottom = 0;
    if(bIsAgent)
    {
        if(SLiveUpdateStreamPosition(Instance,m_AgentSourceList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }
    else
    {
        if(SLiveUpdateStreamPosition(Instance,m_MateralList[iMatIndex]->streamID1,&Area) < 0)
        {
            return;
        }
        else
        {
            m_SceneList[iCurScene]->m_LinkMateralVec[index].top = Area.top;
        }
    }

}
//全屏预览
void ButelLive::OnPreiveFullScreen()
{
    if(!m_bIsFullScreenPreview)
    {
        ui->m_pDisArea->setWindowFlags(Qt::Dialog);
        ui->m_pDisArea->showFullScreen();
        m_bIsFullScreenPreview = true;
        SLiveStartResize(Instance,false);
        ResizeRenderFrame();

        m_PreviewMenu.actions().at(2)->setCheckable(true);
    }
    else
    {
        ui->m_pDisArea->setWindowFlags(Qt::SubWindow);
        ui->m_pDisArea->showNormal();
        m_bIsFullScreenPreview = false;
        SLiveStartResize(Instance,false);
        ResizeRenderFrame();
        m_PreviewMenu.actions().at(2)->setCheckable(false);
    }
}

void ButelLive::OnLiveAudioDBValueChanged(float LeftDb,float RightDb)
{
    ui->m_pWdgtPGMDisL->SetValue(LeftDb);
    ui->m_pWdgtPGMDisR->SetValue(RightDb);
}

void ButelLive::on_m_pSldLeftCtr_sliderReleased()
{
    SetAudioPara();
}

void ButelLive::on_m_pSldRightCtr_sliderReleased()
{
    SetAudioPara();
}

void ButelLive::on_m_pSldPGMCtr_sliderReleased()
{
    SetAudioPara();
}

void ButelLive::SetAudioPara()
{
    int iValLeft = ui->m_pSldLeftCtr->value();
    float fCalLeft = (float)iValLeft/100;
    float fValLeft = 2.0*fCalLeft;

    int iValRight = ui->m_pSldRightCtr->value();
    float fCalRight = (float)iValRight/100;
    float fValRight = 2.0*fCalRight;

    int iValMix = ui->m_pSldPGMCtr->value();
    float fCalMix = (float)iValMix/100;
    float fValMix = 2.0*fCalMix;

    SoundAndLocalMinitor soundInfo;
    soundInfo.bPlayLocal = m_bPlayLocal;
    soundInfo.bPlayLocalLive = m_bPlayLocal;
    soundInfo.fLeft = fValLeft;
    soundInfo.fRight = fValRight;
    soundInfo.fMix = fValMix;
    soundInfo.fQuotietyVolume = 3.0;

    if(SLiveSetSoundAndLocalMinitorParam(&soundInfo) < 0)
    {

    }
}

void ButelLive::on_m_pBtnReset_clicked()
{
    ui->m_pSldLeftCtr->setValue(50);
    ui->m_pSldRightCtr->setValue(50);
    ui->m_pSldPGMCtr->setValue(50);

    SoundAndLocalMinitor soundInfo;
    soundInfo.bPlayLocal = m_bPlayLocal;
    soundInfo.bPlayLocalLive = m_bPlayLocal;
    soundInfo.fLeft = 1.0;
    soundInfo.fRight = 1.0;
    soundInfo.fMix = 1.0;
    soundInfo.fQuotietyVolume = 3.0;

    if(SLiveSetSoundAndLocalMinitorParam(&soundInfo) < 0)
    {

    }
}


void ButelLive::on_m_pBtnPreviewTo_clicked()
{
    UINT iIndex = ui->m_pCmbMonitor->currentIndex();
    if(iIndex > 0)
    {
      if(SLiveEnableProjector(iIndex-1) < 0)
      {

      }
      else
      {

      }
    }
    else if(0 == iIndex)
    {
        if(SLiveDisableProjector() < 0)
        {

        }
    }
}

void ButelLive::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Alt)
    {
        m_bAltKeyPressed = true;
    }
    else if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        return QDialog::keyPressEvent(event);
    }
}
void ButelLive::keyReleaseEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Alt)
    {
        m_bAltKeyPressed = false;
    }
    else if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        return QDialog::keyPressEvent(event);
    }
}
