/********************************************************************************
** Form generated from reading UI file 'ButelLive.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BUTELLIVE_H
#define UI_BUTELLIVE_H

#include <LiveDislplayArea.h>
#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <cslider.h>
#include <pgmdisplay.h>
#include <previewlabel.h>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_ButelLive
{
public:
    QGridLayout *gridLayout_5;
    TitleBar *m_pTitleBarLabel;
    QLabel *label_2;
    QHBoxLayout *horizontalLayout_16;
    QVBoxLayout *verticalLayout_3;
    QHBoxLayout *horizontalLayout_14;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_4;
    QLabel *m_pLblLocalPreview;
    QPushButton *m_pBtnLocalPreview;
    QLabel *m_pLblEditMode;
    QPushButton *m_pBtnEditMode;
    QLabel *m_pLblStartLive;
    QPushButton *m_pBtnStartLive;
    QLabel *m_pLbLiveTime;
    QSpacerItem *horizontalSpacer;
    QLabel *m_pLbRecord;
    QPushButton *m_pBtnRecord;
    QLabel *m_pLbRecordTime;
    QSpacerItem *horizontalSpacer_7;
    QHBoxLayout *horizontalLayout_12;
    QPushButton *m_pBtnSDISwitch;
    QPushButton *m_pBtnSDISet;
    QLabel *label_33;
    QLabel *label_10;
    QHBoxLayout *horizontalLayout_6;
    QLabel *label_6;
    PreviewLabel *m_pDisArea;
    QLabel *label_22;
    LiveDislplayArea *m_pDisArea_2;
    QLabel *label_9;
    QHBoxLayout *horizontalLayout_15;
    QLabel *label_34;
    QLabel *label_36;
    QLabel *label_35;
    QLabel *label_3;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_5;
    QComboBox *m_pCmbToLive;
    QLineEdit *m_pLdtToLiveTime;
    QPushButton *m_pBtnToLive;
    QSpacerItem *horizontalSpacer_8;
    QComboBox *m_pCmbMonitor;
    QSpacerItem *horizontalSpacer_9;
    QPushButton *m_pBtnPreviewTo;
    QSpacerItem *horizontalSpacer_3;
    QLabel *label_11;
    QListWidget *m_pLWPreview;
    QLabel *label_4;
    QLabel *label_5;
    QVBoxLayout *verticalLayout_10;
    QWidget *m_pWdgtMix;
    QGridLayout *gridLayout_3;
    QVBoxLayout *verticalLayout_4;
    QLabel *label_17;
    QHBoxLayout *horizontalLayout_8;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_12;
    QLabel *label_13;
    QLabel *label_14;
    QLabel *label_15;
    QLabel *label_16;
    QSpacerItem *horizontalSpacer_2;
    QVBoxLayout *verticalLayout_7;
    CSlider *m_pSldLeftCtr;
    QLabel *label_18;
    QVBoxLayout *verticalLayout_8;
    CSlider *m_pSldRightCtr;
    QLabel *label_19;
    QVBoxLayout *verticalLayout_9;
    CSlider *m_pSldPGMCtr;
    QLabel *label_20;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_7;
    PGMDisplay *m_pWdgtPGMDisL;
    PGMDisplay *m_pWdgtPGMDisR;
    QLabel *label_21;
    QVBoxLayout *verticalLayout_11;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_37;
    QPushButton *m_pBtnListen;
    QSpacerItem *verticalSpacer_2;
    QLabel *label_38;
    QPushButton *m_pBtnListenPGM;
    QSpacerItem *verticalSpacer;
    QPushButton *m_pBtnReset;
    QLabel *label_23;
    QFrame *line;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *m_pBtnScene;
    QPushButton *m_pBtnMaterial;
    QPushButton *m_pBtnChat;
    QStackedWidget *m_pStckWdgt;
    QWidget *page;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_5;
    QPushButton *m_pBtnAddScene;
    QPushButton *m_pBtnDelScene;
    QPushButton *m_pBtnMoveUp;
    QPushButton *m_pBtnMoveDown;
    QListWidget *m_pLWScene;
    QWidget *page_2;
    QGridLayout *gridLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QPushButton *m_pBtnAddMaterial;
    QPushButton *m_pBtnDelMaterial;
    QHBoxLayout *horizontalLayout_4;
    QLabel *label_7;
    QLabel *label_8;
    QListWidget *m_pLWMaterial;
    QFrame *line_2;
    QFrame *line_3;
    QWidget *page_3;
    QGridLayout *gridLayout_4;
    QVBoxLayout *verticalLayout_13;
    QHBoxLayout *horizontalLayout_10;
    QPushButton *m_pBtnReturnScene;
    QLabel *label;
    QHBoxLayout *horizontalLayout_11;
    QPushButton *m_pBtnScnMtrlAdd;
    QPushButton *m_pBtnScnMtrlDel;
    QPushButton *m_pBtnScnMtrlMoveUp;
    QPushButton *m_pBtnScnMtrlMoveDown;
    QPushButton *m_pBtnScnMtrlMoveTop;
    QPushButton *m_pBtnScnMtrlMoveBottom;
    QListWidget *m_pLWSceneMateralManage;
    QWidget *page_4;
    QGridLayout *gridLayout_7;
    QScrollArea *m_pChatScrollArea;
    QWidget *scrollAreaWidgetContents;
    QWidget *m_pWndStatus;
    QGridLayout *gridLayout_6;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_6;
    QLabel *label_31;
    QLabel *m_pLbFPS;
    QLabel *label_32;
    QLabel *label_24;
    QLabel *m_pLbStatusRGB0;
    QLabel *m_pLbStatus0BitRate;
    QLabel *m_pLbStatus0Lost;
    QLabel *m_pLbStatus0Percent;
    QLabel *label_25;
    QLabel *label_26;
    QLabel *m_pLbStatusRGB1;
    QLabel *m_pLbStatus1BitRate;
    QLabel *m_pLbStatus1Lost;
    QLabel *m_pLbStatus1Percent;
    QLabel *label_27;
    QLabel *label_28;
    QLabel *m_pLbStatusRGB2;
    QLabel *m_pLbStatus2BitRate;
    QLabel *m_pLbStatus2Lost;
    QLabel *m_pLbStatus2Percent;
    QLabel *label_29;
    QLabel *label_30;
    QLabel *m_pLbStatusRGB3;
    QLabel *m_pLbStatus3BitRate;
    QLabel *m_pLbStatus3Lost;
    QLabel *m_pLbStatus3Percent;

    void setupUi(QDialog *ButelLive)
    {
        if (ButelLive->objectName().isEmpty())
            ButelLive->setObjectName(QStringLiteral("ButelLive"));
        ButelLive->resize(1064, 607);
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(ButelLive->sizePolicy().hasHeightForWidth());
        ButelLive->setSizePolicy(sizePolicy);
        ButelLive->setMinimumSize(QSize(835, 450));
        ButelLive->setFocusPolicy(Qt::ClickFocus);
        ButelLive->setStyleSheet(QLatin1String("background-color:rgb(35, 35, 37);\n"
""));
        gridLayout_5 = new QGridLayout(ButelLive);
        gridLayout_5->setSpacing(0);
        gridLayout_5->setObjectName(QStringLiteral("gridLayout_5"));
        gridLayout_5->setContentsMargins(5, 5, 5, 5);
        m_pTitleBarLabel = new TitleBar(ButelLive);
        m_pTitleBarLabel->setObjectName(QStringLiteral("m_pTitleBarLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_pTitleBarLabel->sizePolicy().hasHeightForWidth());
        m_pTitleBarLabel->setSizePolicy(sizePolicy1);
        m_pTitleBarLabel->setMinimumSize(QSize(0, 30));
        m_pTitleBarLabel->setMaximumSize(QSize(16777215, 30));

        gridLayout_5->addWidget(m_pTitleBarLabel, 0, 0, 1, 1);

        label_2 = new QLabel(ButelLive);
        label_2->setObjectName(QStringLiteral("label_2"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy2);
        label_2->setMaximumSize(QSize(16777215, 2));
        label_2->setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 255);"));

        gridLayout_5->addWidget(label_2, 1, 0, 1, 1);

        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setSpacing(0);
        horizontalLayout_16->setObjectName(QStringLiteral("horizontalLayout_16"));
        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setObjectName(QStringLiteral("verticalLayout_3"));
        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setObjectName(QStringLiteral("horizontalLayout_14"));
        horizontalLayout_14->setContentsMargins(-1, -1, 10, -1);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(5);
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalLayout->setSizeConstraint(QLayout::SetFixedSize);
        horizontalLayout->setContentsMargins(-1, 5, -1, 5);
        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Maximum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);

        m_pLblLocalPreview = new QLabel(ButelLive);
        m_pLblLocalPreview->setObjectName(QStringLiteral("m_pLblLocalPreview"));
        sizePolicy.setHeightForWidth(m_pLblLocalPreview->sizePolicy().hasHeightForWidth());
        m_pLblLocalPreview->setSizePolicy(sizePolicy);
        m_pLblLocalPreview->setMinimumSize(QSize(22, 22));
        m_pLblLocalPreview->setMaximumSize(QSize(22, 22));

        horizontalLayout->addWidget(m_pLblLocalPreview);

        m_pBtnLocalPreview = new QPushButton(ButelLive);
        m_pBtnLocalPreview->setObjectName(QStringLiteral("m_pBtnLocalPreview"));
        sizePolicy.setHeightForWidth(m_pBtnLocalPreview->sizePolicy().hasHeightForWidth());
        m_pBtnLocalPreview->setSizePolicy(sizePolicy);
        m_pBtnLocalPreview->setMaximumSize(QSize(75, 25));
        m_pBtnLocalPreview->setFocusPolicy(Qt::NoFocus);
        m_pBtnLocalPreview->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 0);"));

        horizontalLayout->addWidget(m_pBtnLocalPreview);

        m_pLblEditMode = new QLabel(ButelLive);
        m_pLblEditMode->setObjectName(QStringLiteral("m_pLblEditMode"));
        sizePolicy.setHeightForWidth(m_pLblEditMode->sizePolicy().hasHeightForWidth());
        m_pLblEditMode->setSizePolicy(sizePolicy);
        m_pLblEditMode->setMinimumSize(QSize(22, 22));
        m_pLblEditMode->setMaximumSize(QSize(22, 22));

        horizontalLayout->addWidget(m_pLblEditMode);

        m_pBtnEditMode = new QPushButton(ButelLive);
        m_pBtnEditMode->setObjectName(QStringLiteral("m_pBtnEditMode"));
        sizePolicy.setHeightForWidth(m_pBtnEditMode->sizePolicy().hasHeightForWidth());
        m_pBtnEditMode->setSizePolicy(sizePolicy);
        m_pBtnEditMode->setMaximumSize(QSize(75, 25));
        m_pBtnEditMode->setFocusPolicy(Qt::NoFocus);
        m_pBtnEditMode->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 0);"));

        horizontalLayout->addWidget(m_pBtnEditMode);

        m_pLblStartLive = new QLabel(ButelLive);
        m_pLblStartLive->setObjectName(QStringLiteral("m_pLblStartLive"));
        sizePolicy.setHeightForWidth(m_pLblStartLive->sizePolicy().hasHeightForWidth());
        m_pLblStartLive->setSizePolicy(sizePolicy);
        m_pLblStartLive->setMinimumSize(QSize(22, 22));
        m_pLblStartLive->setMaximumSize(QSize(22, 22));

        horizontalLayout->addWidget(m_pLblStartLive);

        m_pBtnStartLive = new QPushButton(ButelLive);
        m_pBtnStartLive->setObjectName(QStringLiteral("m_pBtnStartLive"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Maximum);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(m_pBtnStartLive->sizePolicy().hasHeightForWidth());
        m_pBtnStartLive->setSizePolicy(sizePolicy3);
        m_pBtnStartLive->setMaximumSize(QSize(75, 25));
        m_pBtnStartLive->setFocusPolicy(Qt::NoFocus);
        m_pBtnStartLive->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 0);"));

        horizontalLayout->addWidget(m_pBtnStartLive);

        m_pLbLiveTime = new QLabel(ButelLive);
        m_pLbLiveTime->setObjectName(QStringLiteral("m_pLbLiveTime"));
        m_pLbLiveTime->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout->addWidget(m_pLbLiveTime);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pLbRecord = new QLabel(ButelLive);
        m_pLbRecord->setObjectName(QStringLiteral("m_pLbRecord"));
        m_pLbRecord->setMinimumSize(QSize(24, 24));
        m_pLbRecord->setMaximumSize(QSize(24, 24));
        m_pLbRecord->setStyleSheet(QStringLiteral("border-image: url(:/images/video_rec_unselect.png);"));

        horizontalLayout->addWidget(m_pLbRecord);

        m_pBtnRecord = new QPushButton(ButelLive);
        m_pBtnRecord->setObjectName(QStringLiteral("m_pBtnRecord"));
        m_pBtnRecord->setEnabled(false);
        m_pBtnRecord->setMinimumSize(QSize(75, 25));
        m_pBtnRecord->setMaximumSize(QSize(75, 25));
        m_pBtnRecord->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnRecord->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 0);"));

        horizontalLayout->addWidget(m_pBtnRecord);

        m_pLbRecordTime = new QLabel(ButelLive);
        m_pLbRecordTime->setObjectName(QStringLiteral("m_pLbRecordTime"));
        m_pLbRecordTime->setStyleSheet(QStringLiteral("color: rgb(192, 42, 66);"));

        horizontalLayout->addWidget(m_pLbRecordTime);

        horizontalSpacer_7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_7);


        horizontalLayout_14->addLayout(horizontalLayout);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setSpacing(0);
        horizontalLayout_12->setObjectName(QStringLiteral("horizontalLayout_12"));
        horizontalLayout_12->setContentsMargins(-1, -1, 10, -1);
        m_pBtnSDISwitch = new QPushButton(ButelLive);
        m_pBtnSDISwitch->setObjectName(QStringLiteral("m_pBtnSDISwitch"));
        m_pBtnSDISwitch->setMinimumSize(QSize(27, 30));
        m_pBtnSDISwitch->setMaximumSize(QSize(27, 30));
        m_pBtnSDISwitch->setFocusPolicy(Qt::NoFocus);
        m_pBtnSDISwitch->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/SDI_on.png);"));

        horizontalLayout_12->addWidget(m_pBtnSDISwitch);

        m_pBtnSDISet = new QPushButton(ButelLive);
        m_pBtnSDISet->setObjectName(QStringLiteral("m_pBtnSDISet"));
        m_pBtnSDISet->setMinimumSize(QSize(27, 30));
        m_pBtnSDISet->setMaximumSize(QSize(27, 30));
        m_pBtnSDISet->setFocusPolicy(Qt::NoFocus);
        m_pBtnSDISet->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/SDI_install.png);"));

        horizontalLayout_12->addWidget(m_pBtnSDISet);


        horizontalLayout_14->addLayout(horizontalLayout_12);

        label_33 = new QLabel(ButelLive);
        label_33->setObjectName(QStringLiteral("label_33"));
        label_33->setMinimumSize(QSize(0, 0));
        label_33->setMaximumSize(QSize(100, 18));
        label_33->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
""));

        horizontalLayout_14->addWidget(label_33);


        verticalLayout_3->addLayout(horizontalLayout_14);

        label_10 = new QLabel(ButelLive);
        label_10->setObjectName(QStringLiteral("label_10"));
        sizePolicy2.setHeightForWidth(label_10->sizePolicy().hasHeightForWidth());
        label_10->setSizePolicy(sizePolicy2);
        label_10->setMaximumSize(QSize(16777215, 2));
        label_10->setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 255);"));

        verticalLayout_3->addWidget(label_10);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setSpacing(0);
        horizontalLayout_6->setObjectName(QStringLiteral("horizontalLayout_6"));
        horizontalLayout_6->setContentsMargins(-1, 0, -1, -1);
        label_6 = new QLabel(ButelLive);
        label_6->setObjectName(QStringLiteral("label_6"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(label_6->sizePolicy().hasHeightForWidth());
        label_6->setSizePolicy(sizePolicy4);
        label_6->setMaximumSize(QSize(20, 16777215));
        label_6->setStyleSheet(QStringLiteral("background-color: rgb(23, 23, 24);"));

        horizontalLayout_6->addWidget(label_6);

        m_pDisArea = new PreviewLabel(ButelLive);
        m_pDisArea->setObjectName(QStringLiteral("m_pDisArea"));
        sizePolicy4.setHeightForWidth(m_pDisArea->sizePolicy().hasHeightForWidth());
        m_pDisArea->setSizePolicy(sizePolicy4);
        m_pDisArea->setMaximumSize(QSize(16777215, 16777215));
        m_pDisArea->setStyleSheet(QStringLiteral(""));

        horizontalLayout_6->addWidget(m_pDisArea);

        label_22 = new QLabel(ButelLive);
        label_22->setObjectName(QStringLiteral("label_22"));
        label_22->setMaximumSize(QSize(1, 16777215));
        label_22->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));

        horizontalLayout_6->addWidget(label_22);

        m_pDisArea_2 = new LiveDislplayArea(ButelLive);
        m_pDisArea_2->setObjectName(QStringLiteral("m_pDisArea_2"));

        horizontalLayout_6->addWidget(m_pDisArea_2);

        label_9 = new QLabel(ButelLive);
        label_9->setObjectName(QStringLiteral("label_9"));
        sizePolicy4.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy4);
        label_9->setMaximumSize(QSize(20, 16777215));
        label_9->setStyleSheet(QStringLiteral("background-color: rgb(23, 23, 24);"));

        horizontalLayout_6->addWidget(label_9);


        verticalLayout_3->addLayout(horizontalLayout_6);

        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setSpacing(0);
        horizontalLayout_15->setObjectName(QStringLiteral("horizontalLayout_15"));
        label_34 = new QLabel(ButelLive);
        label_34->setObjectName(QStringLiteral("label_34"));
        label_34->setStyleSheet(QString::fromUtf8("color: rgb(0, 255, 255);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"background-color: rgb(23, 23, 24);"));
        label_34->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_34);

        label_36 = new QLabel(ButelLive);
        label_36->setObjectName(QStringLiteral("label_36"));
        label_36->setMinimumSize(QSize(1, 0));
        label_36->setMaximumSize(QSize(1, 16777215));
        label_36->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));

        horizontalLayout_15->addWidget(label_36);

        label_35 = new QLabel(ButelLive);
        label_35->setObjectName(QStringLiteral("label_35"));
        label_35->setStyleSheet(QString::fromUtf8("color: rgb(255, 0, 0);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"background-color: rgb(23, 23, 24);"));
        label_35->setAlignment(Qt::AlignCenter);

        horizontalLayout_15->addWidget(label_35);


        verticalLayout_3->addLayout(horizontalLayout_15);

        label_3 = new QLabel(ButelLive);
        label_3->setObjectName(QStringLiteral("label_3"));
        sizePolicy2.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy2);
        label_3->setMaximumSize(QSize(16777215, 2));
        label_3->setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 255);"));

        verticalLayout_3->addWidget(label_3);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setSpacing(20);
        horizontalLayout_9->setObjectName(QStringLiteral("horizontalLayout_9"));
        horizontalSpacer_5 = new QSpacerItem(100, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_5);

        m_pCmbToLive = new QComboBox(ButelLive);
        m_pCmbToLive->setObjectName(QStringLiteral("m_pCmbToLive"));
        m_pCmbToLive->setMinimumSize(QSize(100, 0));
        m_pCmbToLive->setFocusPolicy(Qt::ClickFocus);
        m_pCmbToLive->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"background-color: rgb(78, 78, 79);"));

        horizontalLayout_9->addWidget(m_pCmbToLive);

        m_pLdtToLiveTime = new QLineEdit(ButelLive);
        m_pLdtToLiveTime->setObjectName(QStringLiteral("m_pLdtToLiveTime"));
        m_pLdtToLiveTime->setMinimumSize(QSize(50, 20));
        m_pLdtToLiveTime->setMaximumSize(QSize(50, 16777215));
        m_pLdtToLiveTime->setStyleSheet(QLatin1String("background-color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);\n"
"color: rgb(255, 255, 255);\n"
"border:1px;"));

        horizontalLayout_9->addWidget(m_pLdtToLiveTime);

        m_pBtnToLive = new QPushButton(ButelLive);
        m_pBtnToLive->setObjectName(QStringLiteral("m_pBtnToLive"));
        m_pBtnToLive->setEnabled(true);
        sizePolicy.setHeightForWidth(m_pBtnToLive->sizePolicy().hasHeightForWidth());
        m_pBtnToLive->setSizePolicy(sizePolicy);
        m_pBtnToLive->setMinimumSize(QSize(70, 20));
        m_pBtnToLive->setMaximumSize(QSize(70, 20));
        m_pBtnToLive->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnToLive->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);\n"
""));

        horizontalLayout_9->addWidget(m_pBtnToLive);

        horizontalSpacer_8 = new QSpacerItem(200, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_8);

        m_pCmbMonitor = new QComboBox(ButelLive);
        m_pCmbMonitor->setObjectName(QStringLiteral("m_pCmbMonitor"));
        m_pCmbMonitor->setMinimumSize(QSize(100, 0));
        m_pCmbMonitor->setFocusPolicy(Qt::ClickFocus);
        m_pCmbMonitor->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"background-color: rgb(78, 78, 79);"));

        horizontalLayout_9->addWidget(m_pCmbMonitor);

        horizontalSpacer_9 = new QSpacerItem(40, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_9);

        m_pBtnPreviewTo = new QPushButton(ButelLive);
        m_pBtnPreviewTo->setObjectName(QStringLiteral("m_pBtnPreviewTo"));
        m_pBtnPreviewTo->setMinimumSize(QSize(70, 20));
        m_pBtnPreviewTo->setMaximumSize(QSize(70, 20));
        m_pBtnPreviewTo->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnPreviewTo->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_9->addWidget(m_pBtnPreviewTo);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_3);


        verticalLayout_3->addLayout(horizontalLayout_9);

        label_11 = new QLabel(ButelLive);
        label_11->setObjectName(QStringLiteral("label_11"));
        sizePolicy2.setHeightForWidth(label_11->sizePolicy().hasHeightForWidth());
        label_11->setSizePolicy(sizePolicy2);
        label_11->setMinimumSize(QSize(0, 25));
        label_11->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        label_11->setTextFormat(Qt::AutoText);
        label_11->setAlignment(Qt::AlignCenter);

        verticalLayout_3->addWidget(label_11);

        m_pLWPreview = new QListWidget(ButelLive);
        m_pLWPreview->setObjectName(QStringLiteral("m_pLWPreview"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Maximum);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(m_pLWPreview->sizePolicy().hasHeightForWidth());
        m_pLWPreview->setSizePolicy(sizePolicy5);
        m_pLWPreview->setMinimumSize(QSize(0, 150));
        m_pLWPreview->setMaximumSize(QSize(16777215, 150));
        m_pLWPreview->setFocusPolicy(Qt::StrongFocus);
        m_pLWPreview->setLayoutDirection(Qt::LeftToRight);
        m_pLWPreview->setStyleSheet(QString::fromUtf8("font: 9pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        m_pLWPreview->setFrameShape(QFrame::NoFrame);
        m_pLWPreview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_pLWPreview->setSelectionMode(QAbstractItemView::NoSelection);
        m_pLWPreview->setFlow(QListView::LeftToRight);

        verticalLayout_3->addWidget(m_pLWPreview);

        label_4 = new QLabel(ButelLive);
        label_4->setObjectName(QStringLiteral("label_4"));
        sizePolicy2.setHeightForWidth(label_4->sizePolicy().hasHeightForWidth());
        label_4->setSizePolicy(sizePolicy2);
        label_4->setMaximumSize(QSize(16777215, 2));
        label_4->setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 255);"));

        verticalLayout_3->addWidget(label_4);


        horizontalLayout_16->addLayout(verticalLayout_3);

        label_5 = new QLabel(ButelLive);
        label_5->setObjectName(QStringLiteral("label_5"));
        sizePolicy.setHeightForWidth(label_5->sizePolicy().hasHeightForWidth());
        label_5->setSizePolicy(sizePolicy);
        label_5->setMaximumSize(QSize(2, 900));
        label_5->setStyleSheet(QStringLiteral("background-color: rgba(0, 0, 0, 255);"));

        horizontalLayout_16->addWidget(label_5);

        verticalLayout_10 = new QVBoxLayout();
        verticalLayout_10->setObjectName(QStringLiteral("verticalLayout_10"));
        m_pWdgtMix = new QWidget(ButelLive);
        m_pWdgtMix->setObjectName(QStringLiteral("m_pWdgtMix"));
        sizePolicy.setHeightForWidth(m_pWdgtMix->sizePolicy().hasHeightForWidth());
        m_pWdgtMix->setSizePolicy(sizePolicy);
        m_pWdgtMix->setMinimumSize(QSize(360, 250));
        m_pWdgtMix->setMaximumSize(QSize(400, 16777215));
        gridLayout_3 = new QGridLayout(m_pWdgtMix);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        verticalLayout_4 = new QVBoxLayout();
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QStringLiteral("verticalLayout_4"));
        label_17 = new QLabel(m_pWdgtMix);
        label_17->setObjectName(QStringLiteral("label_17"));
        label_17->setMinimumSize(QSize(60, 30));
        label_17->setMaximumSize(QSize(1000, 30));
        label_17->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_17->setAlignment(Qt::AlignCenter);

        verticalLayout_4->addWidget(label_17);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setSpacing(25);
        horizontalLayout_8->setObjectName(QStringLiteral("horizontalLayout_8"));
        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(4);
        verticalLayout_5->setObjectName(QStringLiteral("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(-1, 2, -1, -1);
        label_12 = new QLabel(m_pWdgtMix);
        label_12->setObjectName(QStringLiteral("label_12"));
        sizePolicy.setHeightForWidth(label_12->sizePolicy().hasHeightForWidth());
        label_12->setSizePolicy(sizePolicy);
        label_12->setMinimumSize(QSize(30, 30));
        label_12->setMaximumSize(QSize(30, 30));
        label_12->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_12->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout_5->addWidget(label_12);

        label_13 = new QLabel(m_pWdgtMix);
        label_13->setObjectName(QStringLiteral("label_13"));
        sizePolicy.setHeightForWidth(label_13->sizePolicy().hasHeightForWidth());
        label_13->setSizePolicy(sizePolicy);
        label_13->setMinimumSize(QSize(30, 30));
        label_13->setMaximumSize(QSize(30, 30));
        label_13->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_13->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout_5->addWidget(label_13);

        label_14 = new QLabel(m_pWdgtMix);
        label_14->setObjectName(QStringLiteral("label_14"));
        sizePolicy.setHeightForWidth(label_14->sizePolicy().hasHeightForWidth());
        label_14->setSizePolicy(sizePolicy);
        label_14->setMinimumSize(QSize(30, 30));
        label_14->setMaximumSize(QSize(30, 30));
        label_14->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_14->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout_5->addWidget(label_14);

        label_15 = new QLabel(m_pWdgtMix);
        label_15->setObjectName(QStringLiteral("label_15"));
        sizePolicy.setHeightForWidth(label_15->sizePolicy().hasHeightForWidth());
        label_15->setSizePolicy(sizePolicy);
        label_15->setMinimumSize(QSize(30, 30));
        label_15->setMaximumSize(QSize(30, 30));
        label_15->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_15->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout_5->addWidget(label_15);

        label_16 = new QLabel(m_pWdgtMix);
        label_16->setObjectName(QStringLiteral("label_16"));
        sizePolicy.setHeightForWidth(label_16->sizePolicy().hasHeightForWidth());
        label_16->setSizePolicy(sizePolicy);
        label_16->setMinimumSize(QSize(30, 30));
        label_16->setMaximumSize(QSize(30, 30));
        label_16->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_16->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);

        verticalLayout_5->addWidget(label_16);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Maximum, QSizePolicy::Minimum);

        verticalLayout_5->addItem(horizontalSpacer_2);


        horizontalLayout_8->addLayout(verticalLayout_5);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(10);
        verticalLayout_7->setObjectName(QStringLiteral("verticalLayout_7"));
        m_pSldLeftCtr = new CSlider(m_pWdgtMix);
        m_pSldLeftCtr->setObjectName(QStringLiteral("m_pSldLeftCtr"));
        sizePolicy5.setHeightForWidth(m_pSldLeftCtr->sizePolicy().hasHeightForWidth());
        m_pSldLeftCtr->setSizePolicy(sizePolicy5);
        m_pSldLeftCtr->setMinimumSize(QSize(0, 150));
        m_pSldLeftCtr->setLayoutDirection(Qt::LeftToRight);
        m_pSldLeftCtr->setOrientation(Qt::Vertical);

        verticalLayout_7->addWidget(m_pSldLeftCtr);

        label_18 = new QLabel(m_pWdgtMix);
        label_18->setObjectName(QStringLiteral("label_18"));
        sizePolicy2.setHeightForWidth(label_18->sizePolicy().hasHeightForWidth());
        label_18->setSizePolicy(sizePolicy2);
        label_18->setMinimumSize(QSize(30, 20));
        label_18->setMaximumSize(QSize(30, 20));
        label_18->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_18->setAlignment(Qt::AlignCenter);

        verticalLayout_7->addWidget(label_18);


        horizontalLayout_8->addLayout(verticalLayout_7);

        verticalLayout_8 = new QVBoxLayout();
        verticalLayout_8->setSpacing(10);
        verticalLayout_8->setObjectName(QStringLiteral("verticalLayout_8"));
        m_pSldRightCtr = new CSlider(m_pWdgtMix);
        m_pSldRightCtr->setObjectName(QStringLiteral("m_pSldRightCtr"));
        sizePolicy5.setHeightForWidth(m_pSldRightCtr->sizePolicy().hasHeightForWidth());
        m_pSldRightCtr->setSizePolicy(sizePolicy5);
        m_pSldRightCtr->setMinimumSize(QSize(0, 150));
        m_pSldRightCtr->setOrientation(Qt::Vertical);

        verticalLayout_8->addWidget(m_pSldRightCtr);

        label_19 = new QLabel(m_pWdgtMix);
        label_19->setObjectName(QStringLiteral("label_19"));
        sizePolicy2.setHeightForWidth(label_19->sizePolicy().hasHeightForWidth());
        label_19->setSizePolicy(sizePolicy2);
        label_19->setMinimumSize(QSize(30, 20));
        label_19->setMaximumSize(QSize(30, 20));
        label_19->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_19->setAlignment(Qt::AlignCenter);

        verticalLayout_8->addWidget(label_19);


        horizontalLayout_8->addLayout(verticalLayout_8);

        verticalLayout_9 = new QVBoxLayout();
        verticalLayout_9->setSpacing(10);
        verticalLayout_9->setObjectName(QStringLiteral("verticalLayout_9"));
        m_pSldPGMCtr = new CSlider(m_pWdgtMix);
        m_pSldPGMCtr->setObjectName(QStringLiteral("m_pSldPGMCtr"));
        sizePolicy5.setHeightForWidth(m_pSldPGMCtr->sizePolicy().hasHeightForWidth());
        m_pSldPGMCtr->setSizePolicy(sizePolicy5);
        m_pSldPGMCtr->setMinimumSize(QSize(0, 150));
        m_pSldPGMCtr->setFocusPolicy(Qt::StrongFocus);
        m_pSldPGMCtr->setOrientation(Qt::Vertical);

        verticalLayout_9->addWidget(m_pSldPGMCtr);

        label_20 = new QLabel(m_pWdgtMix);
        label_20->setObjectName(QStringLiteral("label_20"));
        sizePolicy2.setHeightForWidth(label_20->sizePolicy().hasHeightForWidth());
        label_20->setSizePolicy(sizePolicy2);
        label_20->setMinimumSize(QSize(30, 20));
        label_20->setMaximumSize(QSize(30, 20));
        label_20->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_20->setAlignment(Qt::AlignCenter);

        verticalLayout_9->addWidget(label_20);


        horizontalLayout_8->addLayout(verticalLayout_9);

        verticalLayout_2 = new QVBoxLayout();
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QStringLiteral("verticalLayout_2"));
        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(0);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalLayout_7->setSizeConstraint(QLayout::SetMinAndMaxSize);
        m_pWdgtPGMDisL = new PGMDisplay(m_pWdgtMix);
        m_pWdgtPGMDisL->setObjectName(QStringLiteral("m_pWdgtPGMDisL"));
        sizePolicy.setHeightForWidth(m_pWdgtPGMDisL->sizePolicy().hasHeightForWidth());
        m_pWdgtPGMDisL->setSizePolicy(sizePolicy);
        m_pWdgtPGMDisL->setMinimumSize(QSize(10, 155));
        m_pWdgtPGMDisL->setLayoutDirection(Qt::LeftToRight);

        horizontalLayout_7->addWidget(m_pWdgtPGMDisL);

        m_pWdgtPGMDisR = new PGMDisplay(m_pWdgtMix);
        m_pWdgtPGMDisR->setObjectName(QStringLiteral("m_pWdgtPGMDisR"));
        sizePolicy.setHeightForWidth(m_pWdgtPGMDisR->sizePolicy().hasHeightForWidth());
        m_pWdgtPGMDisR->setSizePolicy(sizePolicy);
        m_pWdgtPGMDisR->setMinimumSize(QSize(10, 155));

        horizontalLayout_7->addWidget(m_pWdgtPGMDisR);


        verticalLayout_2->addLayout(horizontalLayout_7);

        label_21 = new QLabel(m_pWdgtMix);
        label_21->setObjectName(QStringLiteral("label_21"));
        sizePolicy2.setHeightForWidth(label_21->sizePolicy().hasHeightForWidth());
        label_21->setSizePolicy(sizePolicy2);
        label_21->setMinimumSize(QSize(60, 30));
        label_21->setMaximumSize(QSize(30, 30));
        label_21->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_21->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(label_21);


        horizontalLayout_8->addLayout(verticalLayout_2);

        verticalLayout_11 = new QVBoxLayout();
        verticalLayout_11->setSpacing(0);
        verticalLayout_11->setObjectName(QStringLiteral("verticalLayout_11"));
        verticalLayout_11->setContentsMargins(-1, -1, -1, 3);
        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setObjectName(QStringLiteral("verticalLayout_6"));
        label_37 = new QLabel(m_pWdgtMix);
        label_37->setObjectName(QStringLiteral("label_37"));
        label_37->setMaximumSize(QSize(30, 16777215));
        label_37->setStyleSheet(QStringLiteral("color: rgb(0, 255, 255);"));
        label_37->setAlignment(Qt::AlignCenter);

        verticalLayout_6->addWidget(label_37);

        m_pBtnListen = new QPushButton(m_pWdgtMix);
        m_pBtnListen->setObjectName(QStringLiteral("m_pBtnListen"));
        sizePolicy.setHeightForWidth(m_pBtnListen->sizePolicy().hasHeightForWidth());
        m_pBtnListen->setSizePolicy(sizePolicy);
        m_pBtnListen->setMinimumSize(QSize(30, 30));
        m_pBtnListen->setMaximumSize(QSize(30, 30));
        m_pBtnListen->setFocusPolicy(Qt::NoFocus);
        m_pBtnListen->setLayoutDirection(Qt::LeftToRight);

        verticalLayout_6->addWidget(m_pBtnListen);

        verticalSpacer_2 = new QSpacerItem(30, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        verticalLayout_6->addItem(verticalSpacer_2);

        label_38 = new QLabel(m_pWdgtMix);
        label_38->setObjectName(QStringLiteral("label_38"));
        label_38->setMaximumSize(QSize(30, 16777215));
        label_38->setStyleSheet(QStringLiteral("color: rgb(255, 0, 0);"));
        label_38->setAlignment(Qt::AlignCenter);

        verticalLayout_6->addWidget(label_38);

        m_pBtnListenPGM = new QPushButton(m_pWdgtMix);
        m_pBtnListenPGM->setObjectName(QStringLiteral("m_pBtnListenPGM"));
        sizePolicy.setHeightForWidth(m_pBtnListenPGM->sizePolicy().hasHeightForWidth());
        m_pBtnListenPGM->setSizePolicy(sizePolicy);
        m_pBtnListenPGM->setMinimumSize(QSize(30, 30));
        m_pBtnListenPGM->setMaximumSize(QSize(30, 30));
        m_pBtnListenPGM->setFocusPolicy(Qt::NoFocus);
        m_pBtnListenPGM->setLayoutDirection(Qt::LeftToRight);

        verticalLayout_6->addWidget(m_pBtnListenPGM);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer);


        verticalLayout_11->addLayout(verticalLayout_6);

        m_pBtnReset = new QPushButton(m_pWdgtMix);
        m_pBtnReset->setObjectName(QStringLiteral("m_pBtnReset"));
        m_pBtnReset->setMinimumSize(QSize(30, 30));
        m_pBtnReset->setMaximumSize(QSize(30, 30));
        m_pBtnReset->setFocusPolicy(Qt::NoFocus);
        m_pBtnReset->setStyleSheet(QStringLiteral(""));

        verticalLayout_11->addWidget(m_pBtnReset);

        label_23 = new QLabel(m_pWdgtMix);
        label_23->setObjectName(QStringLiteral("label_23"));
        label_23->setMinimumSize(QSize(0, 30));
        label_23->setMaximumSize(QSize(16777215, 30));
        label_23->setStyleSheet(QStringLiteral("color: rgb(38, 161, 192);"));
        label_23->setAlignment(Qt::AlignCenter);

        verticalLayout_11->addWidget(label_23);


        horizontalLayout_8->addLayout(verticalLayout_11);


        verticalLayout_4->addLayout(horizontalLayout_8);


        gridLayout_3->addLayout(verticalLayout_4, 0, 0, 1, 1);


        verticalLayout_10->addWidget(m_pWdgtMix);

        line = new QFrame(ButelLive);
        line->setObjectName(QStringLiteral("line"));
        line->setMinimumSize(QSize(0, 2));
        line->setMaximumSize(QSize(16777215, 2));
        line->setStyleSheet(QStringLiteral("background-color: rgb(0, 0, 0);"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_10->addWidget(line);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        horizontalLayout_2->setSizeConstraint(QLayout::SetMaximumSize);
        m_pBtnScene = new QPushButton(ButelLive);
        m_pBtnScene->setObjectName(QStringLiteral("m_pBtnScene"));
        sizePolicy.setHeightForWidth(m_pBtnScene->sizePolicy().hasHeightForWidth());
        m_pBtnScene->setSizePolicy(sizePolicy);
        m_pBtnScene->setMinimumSize(QSize(0, 42));
        m_pBtnScene->setMaximumSize(QSize(190, 42));
        m_pBtnScene->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnScene->setFocusPolicy(Qt::NoFocus);
        m_pBtnScene->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"border-image: url(:/images/btn_scene_materal_press.png);"));

        horizontalLayout_2->addWidget(m_pBtnScene);

        m_pBtnMaterial = new QPushButton(ButelLive);
        m_pBtnMaterial->setObjectName(QStringLiteral("m_pBtnMaterial"));
        sizePolicy.setHeightForWidth(m_pBtnMaterial->sizePolicy().hasHeightForWidth());
        m_pBtnMaterial->setSizePolicy(sizePolicy);
        m_pBtnMaterial->setMinimumSize(QSize(0, 42));
        m_pBtnMaterial->setMaximumSize(QSize(190, 42));
        m_pBtnMaterial->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnMaterial->setFocusPolicy(Qt::NoFocus);
        m_pBtnMaterial->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"border-image: url(:/images/btn_materal.png);"));

        horizontalLayout_2->addWidget(m_pBtnMaterial);

        m_pBtnChat = new QPushButton(ButelLive);
        m_pBtnChat->setObjectName(QStringLiteral("m_pBtnChat"));
        sizePolicy3.setHeightForWidth(m_pBtnChat->sizePolicy().hasHeightForWidth());
        m_pBtnChat->setSizePolicy(sizePolicy3);
        m_pBtnChat->setMinimumSize(QSize(0, 42));
        m_pBtnChat->setMaximumSize(QSize(190, 42));
        m_pBtnChat->setCursor(QCursor(Qt::PointingHandCursor));
        m_pBtnChat->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"border-image: url(:/images/btn_materal.png);"));

        horizontalLayout_2->addWidget(m_pBtnChat);


        verticalLayout->addLayout(horizontalLayout_2);

        m_pStckWdgt = new QStackedWidget(ButelLive);
        m_pStckWdgt->setObjectName(QStringLiteral("m_pStckWdgt"));
        QSizePolicy sizePolicy6(QSizePolicy::Maximum, QSizePolicy::Preferred);
        sizePolicy6.setHorizontalStretch(0);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(m_pStckWdgt->sizePolicy().hasHeightForWidth());
        m_pStckWdgt->setSizePolicy(sizePolicy6);
        m_pStckWdgt->setMaximumSize(QSize(400, 16777215));
        m_pStckWdgt->setFrameShape(QFrame::NoFrame);
        page = new QWidget();
        page->setObjectName(QStringLiteral("page"));
        gridLayout = new QGridLayout(page);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setSpacing(6);
        horizontalLayout_5->setObjectName(QStringLiteral("horizontalLayout_5"));
        m_pBtnAddScene = new QPushButton(page);
        m_pBtnAddScene->setObjectName(QStringLiteral("m_pBtnAddScene"));
        QSizePolicy sizePolicy7(QSizePolicy::Maximum, QSizePolicy::Fixed);
        sizePolicy7.setHorizontalStretch(0);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(m_pBtnAddScene->sizePolicy().hasHeightForWidth());
        m_pBtnAddScene->setSizePolicy(sizePolicy7);
        m_pBtnAddScene->setMinimumSize(QSize(75, 42));
        m_pBtnAddScene->setMaximumSize(QSize(75, 42));
        m_pBtnAddScene->setFocusPolicy(Qt::NoFocus);
        m_pBtnAddScene->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_5->addWidget(m_pBtnAddScene);

        m_pBtnDelScene = new QPushButton(page);
        m_pBtnDelScene->setObjectName(QStringLiteral("m_pBtnDelScene"));
        sizePolicy7.setHeightForWidth(m_pBtnDelScene->sizePolicy().hasHeightForWidth());
        m_pBtnDelScene->setSizePolicy(sizePolicy7);
        m_pBtnDelScene->setMinimumSize(QSize(75, 42));
        m_pBtnDelScene->setMaximumSize(QSize(75, 42));
        m_pBtnDelScene->setFocusPolicy(Qt::NoFocus);
        m_pBtnDelScene->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_5->addWidget(m_pBtnDelScene);

        m_pBtnMoveUp = new QPushButton(page);
        m_pBtnMoveUp->setObjectName(QStringLiteral("m_pBtnMoveUp"));
        sizePolicy7.setHeightForWidth(m_pBtnMoveUp->sizePolicy().hasHeightForWidth());
        m_pBtnMoveUp->setSizePolicy(sizePolicy7);
        m_pBtnMoveUp->setMinimumSize(QSize(75, 42));
        m_pBtnMoveUp->setMaximumSize(QSize(75, 42));
        m_pBtnMoveUp->setFocusPolicy(Qt::NoFocus);
        m_pBtnMoveUp->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_5->addWidget(m_pBtnMoveUp);

        m_pBtnMoveDown = new QPushButton(page);
        m_pBtnMoveDown->setObjectName(QStringLiteral("m_pBtnMoveDown"));
        sizePolicy7.setHeightForWidth(m_pBtnMoveDown->sizePolicy().hasHeightForWidth());
        m_pBtnMoveDown->setSizePolicy(sizePolicy7);
        m_pBtnMoveDown->setMinimumSize(QSize(75, 42));
        m_pBtnMoveDown->setMaximumSize(QSize(75, 42));
        m_pBtnMoveDown->setFocusPolicy(Qt::NoFocus);
        m_pBtnMoveDown->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_5->addWidget(m_pBtnMoveDown);


        gridLayout->addLayout(horizontalLayout_5, 0, 0, 1, 1);

        m_pLWScene = new QListWidget(page);
        m_pLWScene->setObjectName(QStringLiteral("m_pLWScene"));
        QSizePolicy sizePolicy8(QSizePolicy::Maximum, QSizePolicy::Expanding);
        sizePolicy8.setHorizontalStretch(0);
        sizePolicy8.setVerticalStretch(0);
        sizePolicy8.setHeightForWidth(m_pLWScene->sizePolicy().hasHeightForWidth());
        m_pLWScene->setSizePolicy(sizePolicy8);
        m_pLWScene->setMinimumSize(QSize(360, 0));
        m_pLWScene->setMaximumSize(QSize(360, 16777215));
        m_pLWScene->setFocusPolicy(Qt::NoFocus);
        m_pLWScene->setStyleSheet(QStringLiteral("background-color: rgb(35, 35, 37);"));
        m_pLWScene->setFrameShape(QFrame::NoFrame);
        m_pLWScene->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        gridLayout->addWidget(m_pLWScene, 1, 0, 1, 1);

        m_pStckWdgt->addWidget(page);
        page_2 = new QWidget();
        page_2->setObjectName(QStringLiteral("page_2"));
        gridLayout_2 = new QGridLayout(page_2);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(4);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        m_pBtnAddMaterial = new QPushButton(page_2);
        m_pBtnAddMaterial->setObjectName(QStringLiteral("m_pBtnAddMaterial"));
        m_pBtnAddMaterial->setMinimumSize(QSize(177, 42));
        m_pBtnAddMaterial->setMaximumSize(QSize(177, 42));
        m_pBtnAddMaterial->setFocusPolicy(Qt::NoFocus);
        m_pBtnAddMaterial->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"border-image: url(:/images/btn_materal_manager.png);\n"
"background-color: rgba(57, 57, 59, 255);"));

        horizontalLayout_3->addWidget(m_pBtnAddMaterial);

        m_pBtnDelMaterial = new QPushButton(page_2);
        m_pBtnDelMaterial->setObjectName(QStringLiteral("m_pBtnDelMaterial"));
        m_pBtnDelMaterial->setMinimumSize(QSize(177, 42));
        m_pBtnDelMaterial->setMaximumSize(QSize(177, 42));
        m_pBtnDelMaterial->setFocusPolicy(Qt::NoFocus);
        m_pBtnDelMaterial->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_materal_manager.png);"));

        horizontalLayout_3->addWidget(m_pBtnDelMaterial);


        gridLayout_2->addLayout(horizontalLayout_3, 0, 0, 1, 1);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setSpacing(0);
        horizontalLayout_4->setObjectName(QStringLiteral("horizontalLayout_4"));
        label_7 = new QLabel(page_2);
        label_7->setObjectName(QStringLiteral("label_7"));
        sizePolicy2.setHeightForWidth(label_7->sizePolicy().hasHeightForWidth());
        label_7->setSizePolicy(sizePolicy2);
        label_7->setMinimumSize(QSize(0, 25));
        label_7->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 255);"));

        horizontalLayout_4->addWidget(label_7);

        label_8 = new QLabel(page_2);
        label_8->setObjectName(QStringLiteral("label_8"));
        sizePolicy2.setHeightForWidth(label_8->sizePolicy().hasHeightForWidth());
        label_8->setSizePolicy(sizePolicy2);
        label_8->setMinimumSize(QSize(0, 25));
        label_8->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 255);"));

        horizontalLayout_4->addWidget(label_8);


        gridLayout_2->addLayout(horizontalLayout_4, 2, 0, 1, 1);

        m_pLWMaterial = new QListWidget(page_2);
        m_pLWMaterial->setObjectName(QStringLiteral("m_pLWMaterial"));
        QSizePolicy sizePolicy9(QSizePolicy::Minimum, QSizePolicy::Expanding);
        sizePolicy9.setHorizontalStretch(0);
        sizePolicy9.setVerticalStretch(0);
        sizePolicy9.setHeightForWidth(m_pLWMaterial->sizePolicy().hasHeightForWidth());
        m_pLWMaterial->setSizePolicy(sizePolicy9);
        m_pLWMaterial->setMinimumSize(QSize(360, 0));
        m_pLWMaterial->setMaximumSize(QSize(360, 16777215));
        m_pLWMaterial->setSizeIncrement(QSize(0, 0));
        m_pLWMaterial->setFocusPolicy(Qt::NoFocus);
        m_pLWMaterial->setFrameShape(QFrame::NoFrame);

        gridLayout_2->addWidget(m_pLWMaterial, 4, 0, 1, 1);

        line_2 = new QFrame(page_2);
        line_2->setObjectName(QStringLiteral("line_2"));
        line_2->setMinimumSize(QSize(0, 1));
        line_2->setMaximumSize(QSize(16777215, 1));
        line_2->setStyleSheet(QLatin1String("background-color: rgb(38, 38, 39);\n"
"border: 0px;"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        gridLayout_2->addWidget(line_2, 1, 0, 1, 1);

        line_3 = new QFrame(page_2);
        line_3->setObjectName(QStringLiteral("line_3"));
        line_3->setMinimumSize(QSize(0, 1));
        line_3->setMaximumSize(QSize(16777215, 1));
        line_3->setStyleSheet(QLatin1String("background-color: rgb(38, 38, 39);\n"
"border: 0px;"));
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);

        gridLayout_2->addWidget(line_3, 3, 0, 1, 1);

        m_pStckWdgt->addWidget(page_2);
        page_3 = new QWidget();
        page_3->setObjectName(QStringLiteral("page_3"));
        gridLayout_4 = new QGridLayout(page_3);
        gridLayout_4->setObjectName(QStringLiteral("gridLayout_4"));
        verticalLayout_13 = new QVBoxLayout();
        verticalLayout_13->setSpacing(6);
        verticalLayout_13->setObjectName(QStringLiteral("verticalLayout_13"));
        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setSpacing(0);
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));
        horizontalLayout_10->setSizeConstraint(QLayout::SetDefaultConstraint);
        m_pBtnReturnScene = new QPushButton(page_3);
        m_pBtnReturnScene->setObjectName(QStringLiteral("m_pBtnReturnScene"));
        sizePolicy.setHeightForWidth(m_pBtnReturnScene->sizePolicy().hasHeightForWidth());
        m_pBtnReturnScene->setSizePolicy(sizePolicy);
        m_pBtnReturnScene->setMinimumSize(QSize(35, 34));
        m_pBtnReturnScene->setStyleSheet(QLatin1String("border-image: url(:/images/back.png);\n"
"color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(23, 23, 24, 255);"));

        horizontalLayout_10->addWidget(m_pBtnReturnScene);

        label = new QLabel(page_3);
        label->setObjectName(QStringLiteral("label"));
        sizePolicy2.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
        label->setSizePolicy(sizePolicy2);
        label->setMinimumSize(QSize(30, 34));
        label->setStyleSheet(QString::fromUtf8("font: 10pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(23, 23, 24, 255);"));
        label->setAlignment(Qt::AlignCenter);

        horizontalLayout_10->addWidget(label);


        verticalLayout_13->addLayout(horizontalLayout_10);

        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(6);
        horizontalLayout_11->setObjectName(QStringLiteral("horizontalLayout_11"));
        m_pBtnScnMtrlAdd = new QPushButton(page_3);
        m_pBtnScnMtrlAdd->setObjectName(QStringLiteral("m_pBtnScnMtrlAdd"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlAdd->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlAdd->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlAdd->setMinimumSize(QSize(50, 40));
        m_pBtnScnMtrlAdd->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlAdd->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlAdd->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlAdd);

        m_pBtnScnMtrlDel = new QPushButton(page_3);
        m_pBtnScnMtrlDel->setObjectName(QStringLiteral("m_pBtnScnMtrlDel"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlDel->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlDel->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlDel->setMinimumSize(QSize(50, 40));
        m_pBtnScnMtrlDel->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlDel->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlDel->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlDel);

        m_pBtnScnMtrlMoveUp = new QPushButton(page_3);
        m_pBtnScnMtrlMoveUp->setObjectName(QStringLiteral("m_pBtnScnMtrlMoveUp"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlMoveUp->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlMoveUp->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlMoveUp->setMinimumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveUp->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveUp->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlMoveUp->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlMoveUp);

        m_pBtnScnMtrlMoveDown = new QPushButton(page_3);
        m_pBtnScnMtrlMoveDown->setObjectName(QStringLiteral("m_pBtnScnMtrlMoveDown"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlMoveDown->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlMoveDown->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlMoveDown->setMinimumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveDown->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveDown->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlMoveDown->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlMoveDown);

        m_pBtnScnMtrlMoveTop = new QPushButton(page_3);
        m_pBtnScnMtrlMoveTop->setObjectName(QStringLiteral("m_pBtnScnMtrlMoveTop"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlMoveTop->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlMoveTop->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlMoveTop->setMinimumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveTop->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveTop->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlMoveTop->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlMoveTop);

        m_pBtnScnMtrlMoveBottom = new QPushButton(page_3);
        m_pBtnScnMtrlMoveBottom->setObjectName(QStringLiteral("m_pBtnScnMtrlMoveBottom"));
        sizePolicy.setHeightForWidth(m_pBtnScnMtrlMoveBottom->sizePolicy().hasHeightForWidth());
        m_pBtnScnMtrlMoveBottom->setSizePolicy(sizePolicy);
        m_pBtnScnMtrlMoveBottom->setMinimumSize(QSize(0, 40));
        m_pBtnScnMtrlMoveBottom->setMaximumSize(QSize(50, 40));
        m_pBtnScnMtrlMoveBottom->setFocusPolicy(Qt::NoFocus);
        m_pBtnScnMtrlMoveBottom->setStyleSheet(QLatin1String("color: rgba(255, 255, 255, 255);\n"
"background-color: rgba(57, 57, 59, 255);\n"
"border-image: url(:/images/btn_scene_manager.png);"));

        horizontalLayout_11->addWidget(m_pBtnScnMtrlMoveBottom);


        verticalLayout_13->addLayout(horizontalLayout_11);

        m_pLWSceneMateralManage = new QListWidget(page_3);
        m_pLWSceneMateralManage->setObjectName(QStringLiteral("m_pLWSceneMateralManage"));
        m_pLWSceneMateralManage->setFocusPolicy(Qt::NoFocus);
        m_pLWSceneMateralManage->setFrameShape(QFrame::NoFrame);

        verticalLayout_13->addWidget(m_pLWSceneMateralManage);


        gridLayout_4->addLayout(verticalLayout_13, 0, 0, 1, 1);

        m_pStckWdgt->addWidget(page_3);
        page_4 = new QWidget();
        page_4->setObjectName(QStringLiteral("page_4"));
        gridLayout_7 = new QGridLayout(page_4);
        gridLayout_7->setObjectName(QStringLiteral("gridLayout_7"));
        m_pChatScrollArea = new QScrollArea(page_4);
        m_pChatScrollArea->setObjectName(QStringLiteral("m_pChatScrollArea"));
        m_pChatScrollArea->setStyleSheet(QStringLiteral("border:0px;"));
        m_pChatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        m_pChatScrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QStringLiteral("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 360, 207));
        m_pChatScrollArea->setWidget(scrollAreaWidgetContents);

        gridLayout_7->addWidget(m_pChatScrollArea, 0, 0, 1, 1);

        m_pStckWdgt->addWidget(page_4);

        verticalLayout->addWidget(m_pStckWdgt);


        verticalLayout_10->addLayout(verticalLayout);


        horizontalLayout_16->addLayout(verticalLayout_10);


        gridLayout_5->addLayout(horizontalLayout_16, 2, 0, 1, 1);

        m_pWndStatus = new QWidget(ButelLive);
        m_pWndStatus->setObjectName(QStringLiteral("m_pWndStatus"));
        m_pWndStatus->setMinimumSize(QSize(0, 20));
        m_pWndStatus->setMaximumSize(QSize(16777215, 20));
        m_pWndStatus->setStyleSheet(QStringLiteral("background-color: rgb(51, 51, 51);"));
        gridLayout_6 = new QGridLayout(m_pWndStatus);
        gridLayout_6->setSpacing(0);
        gridLayout_6->setObjectName(QStringLiteral("gridLayout_6"));
        gridLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QStringLiteral("horizontalLayout_13"));
        horizontalSpacer_6 = new QSpacerItem(200, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_6);

        label_31 = new QLabel(m_pWndStatus);
        label_31->setObjectName(QStringLiteral("label_31"));
        label_31->setMinimumSize(QSize(1, 0));
        label_31->setMaximumSize(QSize(1, 16777215));
        label_31->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

        horizontalLayout_13->addWidget(label_31);

        m_pLbFPS = new QLabel(m_pWndStatus);
        m_pLbFPS->setObjectName(QStringLiteral("m_pLbFPS"));
        m_pLbFPS->setMinimumSize(QSize(60, 0));
        m_pLbFPS->setMaximumSize(QSize(60, 16777215));
        m_pLbFPS->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbFPS);

        label_32 = new QLabel(m_pWndStatus);
        label_32->setObjectName(QStringLiteral("label_32"));
        label_32->setMinimumSize(QSize(1, 0));
        label_32->setMaximumSize(QSize(1, 16777215));
        label_32->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

        horizontalLayout_13->addWidget(label_32);

        label_24 = new QLabel(m_pWndStatus);
        label_24->setObjectName(QStringLiteral("label_24"));
        label_24->setMinimumSize(QSize(40, 0));
        label_24->setMaximumSize(QSize(40, 16777215));
        label_24->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(label_24);

        m_pLbStatusRGB0 = new QLabel(m_pWndStatus);
        m_pLbStatusRGB0->setObjectName(QStringLiteral("m_pLbStatusRGB0"));
        m_pLbStatusRGB0->setMinimumSize(QSize(20, 20));
        m_pLbStatusRGB0->setMaximumSize(QSize(20, 20));
        m_pLbStatusRGB0->setStyleSheet(QStringLiteral("background-color: rgb(100, 100, 100);"));

        horizontalLayout_13->addWidget(m_pLbStatusRGB0);

        m_pLbStatus0BitRate = new QLabel(m_pWndStatus);
        m_pLbStatus0BitRate->setObjectName(QStringLiteral("m_pLbStatus0BitRate"));
        m_pLbStatus0BitRate->setMinimumSize(QSize(60, 0));
        m_pLbStatus0BitRate->setMaximumSize(QSize(60, 16777215));
        m_pLbStatus0BitRate->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(m_pLbStatus0BitRate);

        m_pLbStatus0Lost = new QLabel(m_pWndStatus);
        m_pLbStatus0Lost->setObjectName(QStringLiteral("m_pLbStatus0Lost"));
        m_pLbStatus0Lost->setMaximumSize(QSize(50, 16777215));
        m_pLbStatus0Lost->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus0Lost);

        m_pLbStatus0Percent = new QLabel(m_pWndStatus);
        m_pLbStatus0Percent->setObjectName(QStringLiteral("m_pLbStatus0Percent"));
        m_pLbStatus0Percent->setMinimumSize(QSize(70, 0));
        m_pLbStatus0Percent->setMaximumSize(QSize(70, 16777215));
        m_pLbStatus0Percent->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus0Percent);

        label_25 = new QLabel(m_pWndStatus);
        label_25->setObjectName(QStringLiteral("label_25"));
        label_25->setMinimumSize(QSize(1, 0));
        label_25->setMaximumSize(QSize(1, 16777215));
        label_25->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

        horizontalLayout_13->addWidget(label_25);

        label_26 = new QLabel(m_pWndStatus);
        label_26->setObjectName(QStringLiteral("label_26"));
        label_26->setMinimumSize(QSize(40, 0));
        label_26->setMaximumSize(QSize(40, 16777215));
        label_26->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(label_26);

        m_pLbStatusRGB1 = new QLabel(m_pWndStatus);
        m_pLbStatusRGB1->setObjectName(QStringLiteral("m_pLbStatusRGB1"));
        m_pLbStatusRGB1->setMinimumSize(QSize(20, 20));
        m_pLbStatusRGB1->setMaximumSize(QSize(20, 20));
        m_pLbStatusRGB1->setStyleSheet(QStringLiteral("background-color: rgb(100, 100, 100);"));

        horizontalLayout_13->addWidget(m_pLbStatusRGB1);

        m_pLbStatus1BitRate = new QLabel(m_pWndStatus);
        m_pLbStatus1BitRate->setObjectName(QStringLiteral("m_pLbStatus1BitRate"));
        m_pLbStatus1BitRate->setMinimumSize(QSize(60, 0));
        m_pLbStatus1BitRate->setMaximumSize(QSize(60, 16777215));
        m_pLbStatus1BitRate->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(m_pLbStatus1BitRate);

        m_pLbStatus1Lost = new QLabel(m_pWndStatus);
        m_pLbStatus1Lost->setObjectName(QStringLiteral("m_pLbStatus1Lost"));
        m_pLbStatus1Lost->setMaximumSize(QSize(50, 16777215));
        m_pLbStatus1Lost->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus1Lost);

        m_pLbStatus1Percent = new QLabel(m_pWndStatus);
        m_pLbStatus1Percent->setObjectName(QStringLiteral("m_pLbStatus1Percent"));
        m_pLbStatus1Percent->setMinimumSize(QSize(70, 0));
        m_pLbStatus1Percent->setMaximumSize(QSize(70, 16777215));
        m_pLbStatus1Percent->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus1Percent);

        label_27 = new QLabel(m_pWndStatus);
        label_27->setObjectName(QStringLiteral("label_27"));
        label_27->setMinimumSize(QSize(1, 0));
        label_27->setMaximumSize(QSize(1, 16777215));
        label_27->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

        horizontalLayout_13->addWidget(label_27);

        label_28 = new QLabel(m_pWndStatus);
        label_28->setObjectName(QStringLiteral("label_28"));
        label_28->setMinimumSize(QSize(40, 0));
        label_28->setMaximumSize(QSize(40, 16777215));
        label_28->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(label_28);

        m_pLbStatusRGB2 = new QLabel(m_pWndStatus);
        m_pLbStatusRGB2->setObjectName(QStringLiteral("m_pLbStatusRGB2"));
        m_pLbStatusRGB2->setMinimumSize(QSize(20, 20));
        m_pLbStatusRGB2->setMaximumSize(QSize(20, 20));
        m_pLbStatusRGB2->setStyleSheet(QStringLiteral("background-color: rgb(100, 100, 100);"));

        horizontalLayout_13->addWidget(m_pLbStatusRGB2);

        m_pLbStatus2BitRate = new QLabel(m_pWndStatus);
        m_pLbStatus2BitRate->setObjectName(QStringLiteral("m_pLbStatus2BitRate"));
        m_pLbStatus2BitRate->setMinimumSize(QSize(60, 0));
        m_pLbStatus2BitRate->setMaximumSize(QSize(60, 16777215));
        m_pLbStatus2BitRate->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(m_pLbStatus2BitRate);

        m_pLbStatus2Lost = new QLabel(m_pWndStatus);
        m_pLbStatus2Lost->setObjectName(QStringLiteral("m_pLbStatus2Lost"));
        m_pLbStatus2Lost->setMaximumSize(QSize(50, 16777215));
        m_pLbStatus2Lost->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus2Lost);

        m_pLbStatus2Percent = new QLabel(m_pWndStatus);
        m_pLbStatus2Percent->setObjectName(QStringLiteral("m_pLbStatus2Percent"));
        m_pLbStatus2Percent->setMinimumSize(QSize(70, 0));
        m_pLbStatus2Percent->setMaximumSize(QSize(70, 16777215));
        m_pLbStatus2Percent->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus2Percent);

        label_29 = new QLabel(m_pWndStatus);
        label_29->setObjectName(QStringLiteral("label_29"));
        label_29->setMinimumSize(QSize(1, 0));
        label_29->setMaximumSize(QSize(1, 16777215));
        label_29->setStyleSheet(QStringLiteral("background-color: rgb(255, 255, 255);"));

        horizontalLayout_13->addWidget(label_29);

        label_30 = new QLabel(m_pWndStatus);
        label_30->setObjectName(QStringLiteral("label_30"));
        label_30->setMinimumSize(QSize(40, 0));
        label_30->setMaximumSize(QSize(40, 16777215));
        label_30->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(label_30);

        m_pLbStatusRGB3 = new QLabel(m_pWndStatus);
        m_pLbStatusRGB3->setObjectName(QStringLiteral("m_pLbStatusRGB3"));
        m_pLbStatusRGB3->setMinimumSize(QSize(20, 20));
        m_pLbStatusRGB3->setMaximumSize(QSize(20, 20));
        m_pLbStatusRGB3->setStyleSheet(QStringLiteral("background-color: rgb(100, 100, 100);"));

        horizontalLayout_13->addWidget(m_pLbStatusRGB3);

        m_pLbStatus3BitRate = new QLabel(m_pWndStatus);
        m_pLbStatus3BitRate->setObjectName(QStringLiteral("m_pLbStatus3BitRate"));
        m_pLbStatus3BitRate->setMinimumSize(QSize(60, 0));
        m_pLbStatus3BitRate->setMaximumSize(QSize(60, 16777215));
        m_pLbStatus3BitRate->setStyleSheet(QStringLiteral("color: rgb(0, 255, 0);"));

        horizontalLayout_13->addWidget(m_pLbStatus3BitRate);

        m_pLbStatus3Lost = new QLabel(m_pWndStatus);
        m_pLbStatus3Lost->setObjectName(QStringLiteral("m_pLbStatus3Lost"));
        m_pLbStatus3Lost->setMaximumSize(QSize(50, 16777215));
        m_pLbStatus3Lost->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus3Lost);

        m_pLbStatus3Percent = new QLabel(m_pWndStatus);
        m_pLbStatus3Percent->setObjectName(QStringLiteral("m_pLbStatus3Percent"));
        m_pLbStatus3Percent->setMinimumSize(QSize(100, 0));
        m_pLbStatus3Percent->setMaximumSize(QSize(100, 16777215));
        m_pLbStatus3Percent->setStyleSheet(QStringLiteral("color: rgb(0, 170, 255);"));

        horizontalLayout_13->addWidget(m_pLbStatus3Percent);


        gridLayout_6->addLayout(horizontalLayout_13, 0, 0, 1, 1);


        gridLayout_5->addWidget(m_pWndStatus, 3, 0, 1, 1);


        retranslateUi(ButelLive);

        m_pStckWdgt->setCurrentIndex(3);


        QMetaObject::connectSlotsByName(ButelLive);
    } // setupUi

    void retranslateUi(QDialog *ButelLive)
    {
        ButelLive->setWindowTitle(QApplication::translate("ButelLive", "ButelLive", 0));
        label_2->setText(QString());
        m_pLblLocalPreview->setText(QString());
        m_pBtnLocalPreview->setText(QApplication::translate("ButelLive", "LocalPreview", 0));
        m_pLblEditMode->setText(QString());
        m_pBtnEditMode->setText(QApplication::translate("ButelLive", "EditMode", 0));
        m_pLblStartLive->setText(QString());
        m_pBtnStartLive->setText(QApplication::translate("ButelLive", "StartLive", 0));
        m_pLbLiveTime->setText(QApplication::translate("ButelLive", "00:00:00", 0));
        m_pLbRecord->setText(QString());
        m_pBtnRecord->setText(QApplication::translate("ButelLive", "StartRecord", 0));
        m_pLbRecordTime->setText(QApplication::translate("ButelLive", "00:00:00", 0));
        m_pBtnSDISwitch->setText(QString());
        m_pBtnSDISet->setText(QString());
        label_33->setText(QApplication::translate("ButelLive", "SDIOutput", 0));
        label_10->setText(QString());
        label_6->setText(QString());
        m_pDisArea->setText(QString());
        label_22->setText(QString());
        label_9->setText(QString());
        label_34->setText(QApplication::translate("ButelLive", "PVW", 0));
        label_36->setText(QString());
        label_35->setText(QApplication::translate("ButelLive", "PGM", 0));
        label_3->setText(QString());
        m_pCmbToLive->clear();
        m_pCmbToLive->insertItems(0, QStringList()
         << QApplication::translate("ButelLive", "cut", 0)
         << QApplication::translate("ButelLive", "fade-in fade-out ", 0)
         << QApplication::translate("ButelLive", "Top-down", 0)
         << QApplication::translate("ButelLive", "spread", 0)
         << QApplication::translate("ButelLive", "round spread", 0)
        );
        m_pLdtToLiveTime->setText(QApplication::translate("ButelLive", "1000", 0));
        m_pBtnToLive->setText(QApplication::translate("ButelLive", "TurnToLive", 0));
        m_pCmbMonitor->clear();
        m_pCmbMonitor->insertItems(0, QStringList()
         << QApplication::translate("ButelLive", "Disabled", 0)
        );
        m_pBtnPreviewTo->setText(QApplication::translate("ButelLive", "PreviewTo", 0));
        label_11->setText(QApplication::translate("ButelLive", "Audio Video Preview", 0));
        label_4->setText(QString());
        label_5->setText(QString());
        label_17->setText(QApplication::translate("ButelLive", "Mixer", 0));
        label_12->setText(QApplication::translate("ButelLive", " 40 -", 0));
        label_13->setText(QApplication::translate("ButelLive", " 20 -", 0));
        label_14->setText(QApplication::translate("ButelLive", "  0 -", 0));
        label_15->setText(QApplication::translate("ButelLive", "-20 -", 0));
        label_16->setText(QApplication::translate("ButelLive", "-40 -", 0));
        label_18->setText(QApplication::translate("ButelLive", "L", 0));
        label_19->setText(QApplication::translate("ButelLive", "R", 0));
        label_20->setText(QApplication::translate("ButelLive", "MIX", 0));
        label_21->setText(QApplication::translate("ButelLive", "L     R", 0));
        label_37->setText(QApplication::translate("ButelLive", "PVW", 0));
        m_pBtnListen->setText(QString());
        label_38->setText(QApplication::translate("ButelLive", "PGM", 0));
        m_pBtnListenPGM->setText(QString());
        m_pBtnReset->setText(QString());
        label_23->setText(QApplication::translate("ButelLive", "Reset", 0));
        m_pBtnScene->setText(QApplication::translate("ButelLive", "scene", 0));
        m_pBtnMaterial->setText(QApplication::translate("ButelLive", "materal", 0));
        m_pBtnChat->setText(QApplication::translate("ButelLive", "chat", 0));
        m_pBtnAddScene->setText(QApplication::translate("ButelLive", "add", 0));
        m_pBtnDelScene->setText(QApplication::translate("ButelLive", "delete", 0));
        m_pBtnMoveUp->setText(QApplication::translate("ButelLive", "move up", 0));
        m_pBtnMoveDown->setText(QApplication::translate("ButelLive", "move down", 0));
        m_pBtnAddMaterial->setText(QApplication::translate("ButelLive", "add", 0));
        m_pBtnDelMaterial->setText(QApplication::translate("ButelLive", "delete", 0));
        label_7->setText(QApplication::translate("ButelLive", "name", 0));
        label_8->setText(QApplication::translate("ButelLive", "type", 0));
        m_pBtnReturnScene->setText(QString());
        label->setText(QApplication::translate("ButelLive", "Scene material management", 0));
        m_pBtnScnMtrlAdd->setText(QApplication::translate("ButelLive", "add", 0));
        m_pBtnScnMtrlDel->setText(QApplication::translate("ButelLive", "delete", 0));
        m_pBtnScnMtrlMoveUp->setText(QApplication::translate("ButelLive", "move up", 0));
        m_pBtnScnMtrlMoveDown->setText(QApplication::translate("ButelLive", "move down", 0));
        m_pBtnScnMtrlMoveTop->setText(QApplication::translate("ButelLive", "move top", 0));
        m_pBtnScnMtrlMoveBottom->setText(QApplication::translate("ButelLive", "move bottom", 0));
        label_31->setText(QString());
        m_pLbFPS->setText(QApplication::translate("ButelLive", "FPS:", 0));
        label_32->setText(QString());
        label_24->setText(QApplication::translate("ButelLive", "URL1:", 0));
        m_pLbStatusRGB0->setText(QString());
        m_pLbStatus0BitRate->setText(QApplication::translate("ButelLive", "0kb/s", 0));
        m_pLbStatus0Lost->setText(QApplication::translate("ButelLive", "Lost:0", 0));
        m_pLbStatus0Percent->setText(QApplication::translate("ButelLive", "(0.00%)", 0));
        label_25->setText(QString());
        label_26->setText(QApplication::translate("ButelLive", "URL2:", 0));
        m_pLbStatusRGB1->setText(QString());
        m_pLbStatus1BitRate->setText(QApplication::translate("ButelLive", "0kb/s", 0));
        m_pLbStatus1Lost->setText(QApplication::translate("ButelLive", "Lost:0", 0));
        m_pLbStatus1Percent->setText(QApplication::translate("ButelLive", "(0.00%)", 0));
        label_27->setText(QString());
        label_28->setText(QApplication::translate("ButelLive", "URL3:", 0));
        m_pLbStatusRGB2->setText(QString());
        m_pLbStatus2BitRate->setText(QApplication::translate("ButelLive", "0kb/s", 0));
        m_pLbStatus2Lost->setText(QApplication::translate("ButelLive", "Lost:0", 0));
        m_pLbStatus2Percent->setText(QApplication::translate("ButelLive", "(0.00%)", 0));
        label_29->setText(QString());
        label_30->setText(QApplication::translate("ButelLive", "URL4:", 0));
        m_pLbStatusRGB3->setText(QString());
        m_pLbStatus3BitRate->setText(QApplication::translate("ButelLive", "0kb/s", 0));
        m_pLbStatus3Lost->setText(QApplication::translate("ButelLive", "Lost:0", 0));
        m_pLbStatus3Percent->setText(QApplication::translate("ButelLive", "(0.00%)", 0));
    } // retranslateUi

};

namespace Ui {
    class ButelLive: public Ui_ButelLive {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BUTELLIVE_H
