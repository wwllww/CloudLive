/********************************************************************************
** Form generated from reading UI file 'SeniorCodeSetUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SENIORCODESETUI_H
#define UI_SENIORCODESETUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_SeniorCodeSetUI
{
public:
    QGridLayout *gridLayout_2;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QGridLayout *gridLayout_3;
    QGridLayout *gridLayout;
    QLabel *label_40;
    QComboBox *m_pCombCPUPreset;
    QLabel *label_41;
    QComboBox *m_pCombCodeCfgFile;
    QLabel *label_42;
    QSpinBox *m_pSBoxKeyFrameInterval;
    QLabel *label_43;
    QSpinBox *m_pSBoxBFrameNum;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *SeniorCodeSetUI)
    {
        if (SeniorCodeSetUI->objectName().isEmpty())
            SeniorCodeSetUI->setObjectName(QStringLiteral("SeniorCodeSetUI"));
        SeniorCodeSetUI->resize(452, 177);
        SeniorCodeSetUI->setStyleSheet(QStringLiteral("background-color: rgb(35, 35, 37);"));
        gridLayout_2 = new QGridLayout(SeniorCodeSetUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        m_pTitleLbl = new TitleBar(SeniorCodeSetUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(SeniorCodeSetUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(201, 201, 201);"));
        gridLayout_3 = new QGridLayout(widget);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label_40 = new QLabel(widget);
        label_40->setObjectName(QStringLiteral("label_40"));
        label_40->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        gridLayout->addWidget(label_40, 0, 0, 1, 1);

        m_pCombCPUPreset = new QComboBox(widget);
        m_pCombCPUPreset->setObjectName(QStringLiteral("m_pCombCPUPreset"));
        m_pCombCPUPreset->setStyleSheet(QLatin1String("QComboBox::drop-down{\n"
"	border-image: url(:images/arrow_down.png);\n"
"    width:10px;\n"
"    height:10px;\n"
"    subcontrol-origin: padding;\n"
"    subcontrol-position: center right;\n"
"    margin: 14 8px;\n"
"}\n"
"QComboBox{\n"
"    height:20px;\n"
"    border: 1px solid #dddddd;\n"
"    background-color: rgb(221,221,221);\n"
"    color: #2a2a2a;\n"
"	border-radius: 4px;\n"
"}"));

        gridLayout->addWidget(m_pCombCPUPreset, 0, 1, 1, 1);

        label_41 = new QLabel(widget);
        label_41->setObjectName(QStringLiteral("label_41"));
        label_41->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        gridLayout->addWidget(label_41, 1, 0, 1, 1);

        m_pCombCodeCfgFile = new QComboBox(widget);
        m_pCombCodeCfgFile->setObjectName(QStringLiteral("m_pCombCodeCfgFile"));
        m_pCombCodeCfgFile->setStyleSheet(QLatin1String("QComboBox::drop-down{\n"
"	border-image: url(:images/arrow_down.png);\n"
"    width:10px;\n"
"    height:10px;\n"
"    subcontrol-origin: padding;\n"
"    subcontrol-position: center right;\n"
"    margin: 14 8px;\n"
"}\n"
"QComboBox{\n"
"    height:20px;\n"
"    border: 1px solid #dddddd;\n"
"    background-color: rgb(221,221,221);\n"
"    color: #2a2a2a;\n"
"	border-radius: 4px;\n"
"}"));

        gridLayout->addWidget(m_pCombCodeCfgFile, 1, 1, 1, 1);

        label_42 = new QLabel(widget);
        label_42->setObjectName(QStringLiteral("label_42"));
        label_42->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        gridLayout->addWidget(label_42, 2, 0, 1, 1);

        m_pSBoxKeyFrameInterval = new QSpinBox(widget);
        m_pSBoxKeyFrameInterval->setObjectName(QStringLiteral("m_pSBoxKeyFrameInterval"));

        gridLayout->addWidget(m_pSBoxKeyFrameInterval, 2, 1, 1, 1);

        label_43 = new QLabel(widget);
        label_43->setObjectName(QStringLiteral("label_43"));
        label_43->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        gridLayout->addWidget(label_43, 3, 0, 1, 1);

        m_pSBoxBFrameNum = new QSpinBox(widget);
        m_pSBoxBFrameNum->setObjectName(QStringLiteral("m_pSBoxBFrameNum"));

        gridLayout->addWidget(m_pSBoxBFrameNum, 3, 1, 1, 1);


        gridLayout_3->addLayout(gridLayout, 0, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pBtnOk = new QPushButton(widget);
        m_pBtnOk->setObjectName(QStringLiteral("m_pBtnOk"));
        m_pBtnOk->setMinimumSize(QSize(75, 20));
        m_pBtnOk->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnOk);

        m_pBtnCancel = new QPushButton(widget);
        m_pBtnCancel->setObjectName(QStringLiteral("m_pBtnCancel"));
        m_pBtnCancel->setMinimumSize(QSize(75, 20));
        m_pBtnCancel->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnCancel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout_3->addLayout(horizontalLayout, 1, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(SeniorCodeSetUI);

        QMetaObject::connectSlotsByName(SeniorCodeSetUI);
    } // setupUi

    void retranslateUi(QDialog *SeniorCodeSetUI)
    {
        SeniorCodeSetUI->setWindowTitle(QApplication::translate("SeniorCodeSetUI", "ButelLive", 0));
        label_40->setText(QApplication::translate("SeniorCodeSetUI", "H264CPU Preset\357\274\232", 0));
        m_pCombCPUPreset->clear();
        m_pCombCPUPreset->insertItems(0, QStringList()
         << QApplication::translate("SeniorCodeSetUI", "ultrafast", 0)
         << QApplication::translate("SeniorCodeSetUI", "superfast", 0)
         << QApplication::translate("SeniorCodeSetUI", "veryfast", 0)
         << QApplication::translate("SeniorCodeSetUI", "faster", 0)
         << QApplication::translate("SeniorCodeSetUI", "fast", 0)
         << QApplication::translate("SeniorCodeSetUI", "medium", 0)
         << QApplication::translate("SeniorCodeSetUI", "slow", 0)
         << QApplication::translate("SeniorCodeSetUI", "slower", 0)
        );
        label_41->setText(QApplication::translate("SeniorCodeSetUI", "Code configuration file\357\274\232", 0));
        m_pCombCodeCfgFile->clear();
        m_pCombCodeCfgFile->insertItems(0, QStringList()
         << QApplication::translate("SeniorCodeSetUI", "main", 0)
         << QApplication::translate("SeniorCodeSetUI", "high", 0)
        );
        label_42->setText(QApplication::translate("SeniorCodeSetUI", "Keyframe interval (SEC, 0= automatically)\357\274\232", 0));
        label_43->setText(QApplication::translate("SeniorCodeSetUI", "The number of B frames between key frames (3= default)\357\274\232", 0));
        m_pBtnOk->setText(QApplication::translate("SeniorCodeSetUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("SeniorCodeSetUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class SeniorCodeSetUI: public Ui_SeniorCodeSetUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SENIORCODESETUI_H
