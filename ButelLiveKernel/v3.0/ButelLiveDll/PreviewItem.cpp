#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QDebug>

#include "PreviewItem.h"
#include "ButelLive.h"
#include "PlayControl.h"
#include "VideoListUI.h"
#include "LogDeliver.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

QJsonObject getJsonObjectFromString(const QString jsonString);


void ConvertGBKToUtf8(std::string& utf8, const char *strGBK);

CPreviewItem::CPreviewItem(ButelLive *parent,QString name,int type,int Index)
{
    setFixedSize(190, 150);
    m_pParent = parent;
    m_Index = Index;
    m_LbSourceName.setText(name);
//    if(type == Dianbo)
//        m_LbSourceType.setText(QStringLiteral("点播源"));
//    else if(type == Shexiang)
//        m_LbSourceType.setText(QStringLiteral("视频捕捉源"));
//    else if(type == Hudong)
//        m_LbSourceType.setText(QStringLiteral("互动连接源"));
//    else if(type == Tupian)
//        m_LbSourceType.setText(QStringLiteral("图片源"));
//    m_TimeLbl.setText("000:00/000:00");
    horizontalLayout.addWidget(&m_LbSourceName);
    horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout.addItem(horizontalSpacer);
//    m_BtnAudio.setText(tr(""));
//    m_BtnSet.setText(tr(""));
//    m_BtnToPreview.setText(tr(""));
    m_BtnAudio.setFixedSize(20,20);
    m_BtnSet.setFixedSize(20,20);
    m_BtnToPreview.setFixedSize(20,20);

    m_BtnAudio.setStyleSheet("QPushButton{border-image:url(:images/preview_listen_close.png);}");
    m_BtnSet.setStyleSheet("QPushButton{border-image:url(:images/preview_install.png);}");
    m_BtnToPreview.setStyleSheet("QPushButton{border-image:url(:images/preview_add.png);}");

    horizontalLayout.addWidget(&m_BtnToPreview);
    horizontalLayout.addWidget(&m_BtnAudio);
    horizontalLayout.addWidget(&m_BtnSet);
    verticalLayout.addLayout(&horizontalLayout);
    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    m_PreviewArea = new PreviewItemDisArea(this);
    sizePolicy.setHeightForWidth(m_PreviewArea->sizePolicy().hasHeightForWidth());
    m_PreviewArea->setSizePolicy(sizePolicy);
    verticalLayout.addWidget(m_PreviewArea);
    verticalLayout.setSpacing(0);
    gridLayout.addLayout(&verticalLayout, 0, 0, 1, 1);
    setLayout(&gridLayout);

    if(type == Dianbo)
    {
        m_pPlayCtr = new CPlayControl(this);
        //m_pPlayCtr->setGeometry(20,105,160,35);
        m_pPlayCtr->move(15,122);
//        m_pPlayCtr->setFixedSize(160,35);
//        m_pPlayCtr->show();
        connect(&m_timer,SIGNAL(timeout()),this,SLOT(OnTimeOut()));        
//        connect(&m_pPlayCtr->m_ProgressCtr,SIGNAL(sliderReleased()),this,SLOT(on_m_pSlider_Move()));
        connect(&m_pPlayCtr->m_ProgressCtr,SIGNAL(mouseClicked()),this,SLOT(on_m_pSlider_Move()));
        connect(&m_pPlayCtr->m_BtnPause,SIGNAL(clicked(bool)),this,SLOT(OnBtnPauseClicked()));
        connect(&m_pPlayCtr->m_BtnStop,SIGNAL(clicked(bool)),this,SLOT(OnBtnStopClicked()));
        connect(&m_pPlayCtr->m_BtnForward,SIGNAL(clicked(bool)),this,SLOT(OnBtnForwardClicked()));
        connect(&m_pPlayCtr->m_BtnNext,SIGNAL(clicked(bool)),this,SLOT(OnBtnNextClicked()));
        connect(&m_pPlayCtr->m_BtnMenu,SIGNAL(clicked(bool)),this,SLOT(OnBtnMenuClicked()));
    }
    connect(&m_BtnSet,SIGNAL(clicked(bool)),this,SLOT(OnBtnSetClicked()));
    connect(&m_BtnToPreview,SIGNAL(clicked(bool)),this,SLOT(OnBtnToPreview()));
    connect(&m_BtnAudio,SIGNAL(clicked(bool)),this,SLOT(OnBtnAudioClicked()));
    setStyleSheet("QWidget{color:#ffffff;font:10pt;}");
}
void CPreviewItem::VideoInit()
{

    char* info;
    int iMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    Log::writeMessage(LOG_RTSPSERV, 1, "iMatIndex = %d,m_Index = %d", iMatIndex,m_Index);
    if(iMatIndex == -1) return;
    m_timer.start(1000);
    if(SLiveGetStreamInfo(m_pParent->m_VideoInstanceVec.at(iMatIndex), m_pParent->m_MateralList.at(iMatIndex)->streamID1,&info) < 0)
    {
//        qDebug() << "PoleLive::on_m_pBtnVideo_clicked() SLiveGetStreamInfo failed!";
//        Log::writeMessage(LOG_RTSPSERV, 1, "%s SLiveGetStreamInfo failed!", __FUNCTION__);
        m_AvSec = 0;
//        return;
    }
    else
    {
        VideoDataFresh();
        SLiveFreeMemory(info);
    }
//    QJsonObject info_json_obj = getJsonObjectFromString(QString(info));
//    QJsonValue duration_json_val = info_json_obj.value("DemandFileDuration");
//    m_AvSec = duration_json_val.toInt();
//    m_pPlayCtr->m_ProgressCtr.setRange(0,m_AvSec);
//    qDebug() << "m_AvSec = " << m_AvSec;
//    m_timer.start(1000);

    m_pPlayCtr->m_ProgressCtr.setValue(0);

//    m_bIsPlaying = true;
    m_Oper = Pause;
//    m_timer.start(1000);
}
void CPreviewItem::OnBtnToPreview()
{
    m_pParent->SetPreviewCurrentRow(m_Index);

    int iCurScene = m_pParent->GetSceneCurrentRow();
    if(iCurScene < 0) return;

    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    m_pParent->AddMateralToScene(iLinkMatIndex,iCurScene);
}

void CPreviewItem::OnTimeOut()
{
//    if(m_AvSec == 0) return;
    int gSec = m_AvSec/1000%60%60;
    int gHour = m_AvSec/1000/60/60;
    int gMin = (m_AvSec/1000 - 60*60*gHour)/60;

//    if((gSec < 0 || gSec > 59) || (gMin < 0 || gMin > 59) ) return;
    QString strgTime = "%1:%2:%3";
    strgTime = strgTime.arg(gHour,2,10,QChar('0')).arg(gMin,2,10,QChar('0')).arg(gSec,2,10,QChar('0'));
    UINT pos;
    int iMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iMatIndex == -1) return;
    SLiveGetStreamPlayPos(m_pParent->m_VideoInstanceVec.at(iMatIndex),m_pParent->m_MateralList.at(iMatIndex)->streamID1, &pos );
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s pos = %d",__FUNCTION__, pos);
    int iVal = pos;
    m_pPlayCtr->m_ProgressCtr.setValue(iVal);
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s iVal = %d",__FUNCTION__, iVal);
    int sec = pos/1000%60%60;
    int hour = pos/1000/60/60;
    int min = (pos/1000 - 60*60*hour)/60;
    QString strTime = "%1:%2:%3";
    strTime = strTime.arg(hour,2,10,QChar('0')).arg(min,2,10,QChar('0')).arg(sec,2,10,QChar('0'));
    m_pPlayCtr->m_TimeLbl.setText(strTime+'/'+strgTime);
//    Log::writeMessage(LOG_RTSPSERV, 1, "%s strTime = %s",__FUNCTION__, strTime.toStdString().c_str());

    if(pos <= 5000 && (m_Oper == Play || m_MediaStatus == 2))
    {
        VideoDataFresh();
    }
    else
    {
        VideoStateUpdate();
    }
}

void CPreviewItem::on_m_pSlider_Move()
{
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    int iVal =  m_pPlayCtr->m_ProgressCtr.value();
    int pos = iVal;
    if (SLiveSetStreamPlayPos(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,pos) < 0)
    {
        return;
    }
}

void CPreviewItem::VideoStateUpdate()
{
//    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    char* strJsonInfo;
    if(SLiveGetStreamInfo(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,&strJsonInfo) < 0)
    {
        m_AvSec = 0;
        return;
    }
    else
    {
        std::string utf;
        ConvertGBKToUtf8(utf,strJsonInfo);
        SLiveFreeMemory(strJsonInfo);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();

        QJsonValue MediaStatus_value = json_root_obj.take("MediaStatus");
        m_MediaStatus = MediaStatus_value.toInt();
        if(m_MediaStatus == 2)
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Play.png);}");
        else
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Pause.png);}");

        if(m_MediaStatus == 2)
            m_Oper = Play;
        else
            m_Oper = Pause;
    }
}
void CPreviewItem::VideoDataFresh()
{
//    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    char* strJsonInfo;
    if(SLiveGetStreamInfo(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,&strJsonInfo) < 0)
    {
        m_AvSec = 0;
        return;
    }
    else
    {
        qDebug() << strJsonInfo;
        std::string utf;
        ConvertGBKToUtf8(utf,strJsonInfo);
        SLiveFreeMemory(strJsonInfo);
        QString val(utf.c_str());
        QJsonDocument json_root_doc = QJsonDocument::fromJson(val.toStdString().data());
        if(json_root_doc.isNull())
            return;
        QJsonObject json_root_obj = json_root_doc.object();

        QJsonValue DemandFileDuration_value = json_root_obj.take("DemandFileDuration");
        m_AvSec = DemandFileDuration_value.toInt();
        m_pPlayCtr->m_ProgressCtr.setRange(0,m_AvSec);

        Log::writeMessage(LOG_RTSPSERV, 1, "%s m_AvSec = %d",__FUNCTION__, m_AvSec);

        QJsonValue CurrentIndex_value = json_root_obj.take("CurrentIndex");
        m_CurrentIndex = CurrentIndex_value.toInt();

        QJsonValue IsFileLoop_value = json_root_obj.take("IsFileLoop");
        m_bIsFileLoop = IsFileLoop_value.toBool();

        QJsonValue IsListLoop_value = json_root_obj.take("IsListLoop");
        m_bIsListLoop = IsListLoop_value.toBool();

        QJsonValue IsLoop_value = json_root_obj.take("IsLoop");
        m_bIsLoop = IsLoop_value.toBool();

        QJsonValue MediaStatus_value = json_root_obj.take("MediaStatus");
        m_MediaStatus = MediaStatus_value.toInt();


        if(m_MediaStatus == 2)
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Play.png);}");
        else
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Pause.png);}");

        if(m_MediaStatus == 2)
            m_Oper = Play;
        else
            m_Oper = Pause;

        QJsonValue playlist_value = json_root_obj.value(QString("playlist"));
        m_FileList.clear();
        if(playlist_value.isArray())
        {
            QJsonArray playlist_arr = playlist_value.toArray();
            for(int i = 0; i < playlist_arr.count(); i++)
            {
                QJsonValue playlist_arr_item =  playlist_arr.at(i);
                QString fileName = playlist_arr_item.toString();
                m_FileList.append(fileName);
            }
        }
        int iPlayCount =  m_FileList.count();
        if(0 == m_CurrentIndex && 1 == iPlayCount)
        {
            m_pPlayCtr->m_BtnForward.setStyleSheet("QPushButton{border-image:url(:images/player_last_dis.png);}");
            m_pPlayCtr->m_BtnNext.setStyleSheet("QPushButton{border-image:url(:images/player_next_dis.png);}");
        }
        else if(0 == m_CurrentIndex && iPlayCount > 1)
        {
            m_pPlayCtr->m_BtnForward.setStyleSheet("QPushButton{border-image:url(:images/player_last_dis.png);}");
            m_pPlayCtr->m_BtnNext.setStyleSheet("QPushButton{border-image:url(:images/player_next.png);}"
                                                 "QPushButton:hover{border-image:url(:images/player_next_hover.png);}");
        }
        else if(m_CurrentIndex > 0 && m_CurrentIndex < iPlayCount - 1)
        {
            m_pPlayCtr->m_BtnForward.setStyleSheet("QPushButton{border-image:url(:images/player_last.png);}"
                                                   "QPushButton:hover{border-image:url(:images/player_last_hover.png);}");
            m_pPlayCtr->m_BtnNext.setStyleSheet("QPushButton{border-image:url(:images/player_next.png);}"
                                                 "QPushButton:hover{border-image:url(:images/player_next_hover.png);}");
        }
        else if((m_CurrentIndex == iPlayCount - 1) && iPlayCount > 1)
        {
            m_pPlayCtr->m_BtnForward.setStyleSheet("QPushButton{border-image:url(:images/player_last.png);}"
                                                   "QPushButton:hover{border-image:url(:images/player_last_hover.png);}");
            m_pPlayCtr->m_BtnNext.setStyleSheet("QPushButton{border-image:url(:images/player_next_dis.png);}");
        }
    }
}
void CPreviewItem::OnBtnSetClicked()
{
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
//    if(SLiveConfigStream(m_pParent->m_VideoInstanceVec.at(m_Index), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1) < 0)
    qDebug() << m_pParent->m_VideoInstanceVec.at(iLinkMatIndex) << "*********" << m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1;
    if(SLiveConfigStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1) < 0)
    {
        return;
    }
    else
    {
        if(m_pParent->m_MateralList[iLinkMatIndex]->type == Dianbo)
            VideoDataFresh();
        else if(m_pParent->m_MateralList[iLinkMatIndex]->type == Shexiang && (!m_pParent->m_bIsPreview && !m_pParent->m_bIsLiving))
        {
            m_pParent->UpdateRecordState();
        }
    }
}
void CPreviewItem::OnBtnAudioClicked()
{
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
//    if(SLiveConfigStream(m_pParent->m_VideoInstanceVec.at(m_Index), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1) < 0)
    bool bRet = false;
    if(SLiveSetPlayPreAudio(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,&bRet) < 0)
    {
        return;
    }
    else
    {
        if(bRet)
            m_BtnAudio.setStyleSheet("QPushButton{border-image:url(:images/preview_listen.png);}");
        else
            m_BtnAudio.setStyleSheet("QPushButton{border-image:url(:images/preview_listen_close.png);}");
    }
}
void CPreviewItem::OnBtnPauseClicked()
{
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    if(m_Oper == Pause)
        m_Oper = Play;
    else if(m_Oper == Play)
        m_Oper = Pause;
    qDebug() << m_pParent->m_VideoInstanceVec.at(iLinkMatIndex) << "*********" << m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1;
    if(SLiveOperaterStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,m_Oper) < 0)
    {
        if(m_Oper == Pause)
            m_Oper = Play;
        else if(m_Oper == Play)
            m_Oper = Pause;
        return;
    }
    else
    {
        if(m_Oper == Pause)
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Pause.png);}");
        else if(m_Oper == Play)
            m_pPlayCtr->m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Play.png);}");
    }
}
void CPreviewItem::OnBtnStopClicked()
{
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;

    m_Oper = ReStart;

    if(SLiveOperaterStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,m_Oper) < 0)
    {
        return;
    }
    else
    {
        VideoDataFresh();
        if(m_MediaStatus == 2)
            m_Oper = Play;
        else
            m_Oper = Pause;
    }
}
void CPreviewItem::OnBtnForwardClicked()
{
    if(m_CurrentIndex <= 0) return;
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    if(SLiveOperaterStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,PlayPrev) < 0)
    {
        return;
    }
    else
    {
        VideoDataFresh();
    }
}
void CPreviewItem::OnBtnNextClicked()
{
    if(m_CurrentIndex >= m_FileList.count()-1) return;
    m_pParent->SetPreviewCurrentRow(m_Index);
    int iLinkMatIndex = m_pParent->GetPreviewSourceLinkMateralNo(m_Index);
    if(iLinkMatIndex == -1) return;
    if(SLiveOperaterStream(m_pParent->m_VideoInstanceVec.at(iLinkMatIndex), m_pParent->m_MateralList.at(iLinkMatIndex)->streamID1,PlayNext) < 0)
    {
        return;
    }
    else
    {
        VideoDataFresh();
    }
}

void CPreviewItem::OnBtnMenuClicked()
{
    int ii  =  m_Index;
    m_pParent->SetPreviewCurrentRow(m_Index);
    VideoDataFresh();
    VideoListUI* pVideoListUi = new VideoListUI(m_FileList,m_pParent,m_CurrentIndex);
    pVideoListUi->show();
}
