/********************************************************************************
** Form generated from reading UI file 'MessageInfoUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MESSAGEINFOUI_H
#define UI_MESSAGEINFOUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_MessageInfoUI
{
public:
    QGridLayout *gridLayout_2;
    QWidget *widget;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QLabel *m_pLabelShowInfo;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnExit;
    QSpacerItem *horizontalSpacer_2;
    TitleBar *m_pTitleLbl;

    void setupUi(QDialog *MessageInfoUI)
    {
        if (MessageInfoUI->objectName().isEmpty())
            MessageInfoUI->setObjectName(QStringLiteral("MessageInfoUI"));
        MessageInfoUI->resize(300, 144);
        MessageInfoUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(MessageInfoUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        widget = new QWidget(MessageInfoUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        m_pLabelShowInfo = new QLabel(widget);
        m_pLabelShowInfo->setObjectName(QStringLiteral("m_pLabelShowInfo"));
        m_pLabelShowInfo->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));
        m_pLabelShowInfo->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(m_pLabelShowInfo);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pBtnExit = new QPushButton(widget);
        m_pBtnExit->setObjectName(QStringLiteral("m_pBtnExit"));
        m_pBtnExit->setMinimumSize(QSize(75, 20));
        m_pBtnExit->setMaximumSize(QSize(75, 20));
        m_pBtnExit->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnExit);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);

        m_pTitleLbl = new TitleBar(MessageInfoUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);


        retranslateUi(MessageInfoUI);

        QMetaObject::connectSlotsByName(MessageInfoUI);
    } // setupUi

    void retranslateUi(QDialog *MessageInfoUI)
    {
        MessageInfoUI->setWindowTitle(QApplication::translate("MessageInfoUI", "ButelLive", 0));
        m_pLabelShowInfo->setText(QString());
        m_pBtnExit->setText(QApplication::translate("MessageInfoUI", "OK", 0));
    } // retranslateUi

};

namespace Ui {
    class MessageInfoUI: public Ui_MessageInfoUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MESSAGEINFOUI_H
