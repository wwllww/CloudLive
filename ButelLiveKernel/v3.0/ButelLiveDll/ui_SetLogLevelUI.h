/********************************************************************************
** Form generated from reading UI file 'SetLogLevelUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETLOGLEVELUI_H
#define UI_SETLOGLEVELUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_SetLogLevelUI
{
public:
    QGridLayout *gridLayout_2;
    QWidget *widget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *m_pChckBoxErrLog;
    QCheckBox *m_pChckBoxWarnLog;
    QCheckBox *m_pChckBoxMsgLog;
    QCheckBox *m_pChckBoxDbgLog;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;
    TitleBar *m_pTitleLbl;

    void setupUi(QDialog *SetLogLevelUI)
    {
        if (SetLogLevelUI->objectName().isEmpty())
            SetLogLevelUI->setObjectName(QStringLiteral("SetLogLevelUI"));
        SetLogLevelUI->resize(397, 132);
        SetLogLevelUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(SetLogLevelUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        widget = new QWidget(SetLogLevelUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        m_pChckBoxErrLog = new QCheckBox(widget);
        m_pChckBoxErrLog->setObjectName(QStringLiteral("m_pChckBoxErrLog"));
        m_pChckBoxErrLog->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_2->addWidget(m_pChckBoxErrLog);

        m_pChckBoxWarnLog = new QCheckBox(widget);
        m_pChckBoxWarnLog->setObjectName(QStringLiteral("m_pChckBoxWarnLog"));
        m_pChckBoxWarnLog->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_2->addWidget(m_pChckBoxWarnLog);

        m_pChckBoxMsgLog = new QCheckBox(widget);
        m_pChckBoxMsgLog->setObjectName(QStringLiteral("m_pChckBoxMsgLog"));
        m_pChckBoxMsgLog->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_2->addWidget(m_pChckBoxMsgLog);

        m_pChckBoxDbgLog = new QCheckBox(widget);
        m_pChckBoxDbgLog->setObjectName(QStringLiteral("m_pChckBoxDbgLog"));
        m_pChckBoxDbgLog->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_2->addWidget(m_pChckBoxDbgLog);


        gridLayout->addLayout(horizontalLayout_2, 0, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pBtnOk = new QPushButton(widget);
        m_pBtnOk->setObjectName(QStringLiteral("m_pBtnOk"));
        m_pBtnOk->setMinimumSize(QSize(75, 20));
        m_pBtnOk->setMaximumSize(QSize(75, 20));
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


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);

        m_pTitleLbl = new TitleBar(SetLogLevelUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);


        retranslateUi(SetLogLevelUI);

        QMetaObject::connectSlotsByName(SetLogLevelUI);
    } // setupUi

    void retranslateUi(QDialog *SetLogLevelUI)
    {
        SetLogLevelUI->setWindowTitle(QApplication::translate("SetLogLevelUI", "ButelLive", 0));
        m_pChckBoxErrLog->setText(QApplication::translate("SetLogLevelUI", "ErrorLog", 0));
        m_pChckBoxWarnLog->setText(QApplication::translate("SetLogLevelUI", "WarningLog", 0));
        m_pChckBoxMsgLog->setText(QApplication::translate("SetLogLevelUI", "MessageLog", 0));
        m_pChckBoxDbgLog->setText(QApplication::translate("SetLogLevelUI", "DebugLog", 0));
        m_pBtnOk->setText(QApplication::translate("SetLogLevelUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("SetLogLevelUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class SetLogLevelUI: public Ui_SetLogLevelUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETLOGLEVELUI_H
