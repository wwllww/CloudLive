/********************************************************************************
** Form generated from reading UI file 'StopLiveConfirm.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_STOPLIVECONFIRM_H
#define UI_STOPLIVECONFIRM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_StopLiveConfirmUI
{
public:
    QGridLayout *gridLayout_2;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QGridLayout *gridLayout;
    QRadioButton *m_pRdBtnTemp;
    QRadioButton *m_pRdBtnClose;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *StopLiveConfirmUI)
    {
        if (StopLiveConfirmUI->objectName().isEmpty())
            StopLiveConfirmUI->setObjectName(QStringLiteral("StopLiveConfirmUI"));
        StopLiveConfirmUI->resize(687, 140);
        StopLiveConfirmUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(StopLiveConfirmUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        m_pTitleLbl = new TitleBar(StopLiveConfirmUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(StopLiveConfirmUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        m_pRdBtnTemp = new QRadioButton(widget);
        m_pRdBtnTemp->setObjectName(QStringLiteral("m_pRdBtnTemp"));
        m_pRdBtnTemp->setChecked(true);

        gridLayout->addWidget(m_pRdBtnTemp, 0, 0, 1, 1);

        m_pRdBtnClose = new QRadioButton(widget);
        m_pRdBtnClose->setObjectName(QStringLiteral("m_pRdBtnClose"));

        gridLayout->addWidget(m_pRdBtnClose, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pBtnOk = new QPushButton(widget);
        m_pBtnOk->setObjectName(QStringLiteral("m_pBtnOk"));
        m_pBtnOk->setMinimumSize(QSize(75, 20));
        m_pBtnOk->setMaximumSize(QSize(50, 20));
        m_pBtnOk->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnOk);

        m_pBtnCancel = new QPushButton(widget);
        m_pBtnCancel->setObjectName(QStringLiteral("m_pBtnCancel"));
        m_pBtnCancel->setMinimumSize(QSize(75, 20));
        m_pBtnCancel->setMaximumSize(QSize(75, 20));
        m_pBtnCancel->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnCancel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(StopLiveConfirmUI);

        QMetaObject::connectSlotsByName(StopLiveConfirmUI);
    } // setupUi

    void retranslateUi(QDialog *StopLiveConfirmUI)
    {
        StopLiveConfirmUI->setWindowTitle(QApplication::translate("StopLiveConfirmUI", "ButelLive", 0));
        m_pRdBtnTemp->setText(QApplication::translate("StopLiveConfirmUI", "Exit temporarily:After exiting, you can enter the live broadcast again", 0));
        m_pRdBtnClose->setText(QApplication::translate("StopLiveConfirmUI", "Close living:After exiting, can generate a video back to view, and you will can't to re-enter the living", 0));
        m_pBtnOk->setText(QApplication::translate("StopLiveConfirmUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("StopLiveConfirmUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class StopLiveConfirmUI: public Ui_StopLiveConfirmUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_STOPLIVECONFIRM_H
