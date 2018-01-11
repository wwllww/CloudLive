/********************************************************************************
** Form generated from reading UI file 'previewitem.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef PREVIEWITEM_H
#define PREVIEWITEM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <QPushButton>
#include <QTimer>
#include <QStringList>
#include "ButelLive.h"
#include "PlayControl.h"
#include "cslider.h"
#include "previewitemdisarea.h"
class CPlayControl;
class ButelLive;
class CPreviewItem: public QWidget
{
    Q_OBJECT
public:
    CPreviewItem(ButelLive *parent = 0,QString name = 0,int type = 0,int Index = 0);

    void VideoInit();
    void VideoDataFresh();
    void VideoStateUpdate();
public:
    QGridLayout gridLayout;
    QVBoxLayout verticalLayout;
    QHBoxLayout horizontalLayout;
    QHBoxLayout horizontalLayout_2;
    QLabel      m_LbSourceName;
    QSpacerItem *horizontalSpacer;
    QPushButton m_BtnAudio;
    QPushButton m_BtnSet;
    QPushButton m_BtnToPreview;
    PreviewItemDisArea*     m_PreviewArea;
    int            m_Index;              //当前预览窗的索引号
    ButelLive*  m_pParent;
    CPlayControl*  m_pPlayCtr;
    QTimer         m_timer;



    DBOperation   m_Oper;
//    bool          m_bIsPlaying;

    int           m_AvSec;          //视频总时长
    int           m_AvCurSec;       //视频当前时戳
    int           m_CurrentIndex;
    bool          m_bIsFileLoop;
    bool          m_bIsListLoop;
    bool          m_bIsLoop;
    int           m_MediaStatus;
    QStringList   m_FileList;

public slots:
    void OnBtnToPreview();
    void OnTimeOut();
    void on_m_pSlider_Move();
    void OnBtnSetClicked();
    void OnBtnAudioClicked();
    void OnBtnPauseClicked();
    void OnBtnStopClicked();
    void OnBtnForwardClicked();
    void OnBtnNextClicked();
    void OnBtnMenuClicked();
};

#endif // PREVIEWITEM_H
