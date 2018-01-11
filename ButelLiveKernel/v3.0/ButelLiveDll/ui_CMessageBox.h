/********************************************************************************
** Form generated from reading UI file 'CMessageBox.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CMESSAGEBOX_H
#define UI_CMESSAGEBOX_H

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

class Ui_CMessageBoxUI
{
public:
    QGridLayout *gridLayout_2;
    QWidget *widget;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QLabel *m_pLabelShowInfo;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnYes;
    QPushButton *m_pBtnNO;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;
    TitleBar *m_pTitleLbl;

    void setupUi(QDialog *CMessageBoxUI)
    {
        if (CMessageBoxUI->objectName().isEmpty())
            CMessageBoxUI->setObjectName(QStringLiteral("CMessageBoxUI"));
        CMessageBoxUI->resize(300, 144);
        CMessageBoxUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(CMessageBoxUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        widget = new QWidget(CMessageBoxUI);
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

        m_pBtnYes = new QPushButton(widget);
        m_pBtnYes->setObjectName(QStringLiteral("m_pBtnYes"));
        m_pBtnYes->setMinimumSize(QSize(75, 20));
        m_pBtnYes->setMaximumSize(QSize(75, 20));
        m_pBtnYes->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnYes);

        m_pBtnNO = new QPushButton(widget);
        m_pBtnNO->setObjectName(QStringLiteral("m_pBtnNO"));
        m_pBtnNO->setMinimumSize(QSize(75, 20));
        m_pBtnNO->setMaximumSize(QSize(75, 20));
        m_pBtnNO->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnNO);

        m_pBtnCancel = new QPushButton(widget);
        m_pBtnCancel->setObjectName(QStringLiteral("m_pBtnCancel"));
        m_pBtnCancel->setMinimumSize(QSize(75, 20));
        m_pBtnCancel->setMaximumSize(QSize(75, 20));
        m_pBtnCancel->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnCancel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);

        m_pTitleLbl = new TitleBar(CMessageBoxUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);


        retranslateUi(CMessageBoxUI);

        QMetaObject::connectSlotsByName(CMessageBoxUI);
    } // setupUi

    void retranslateUi(QDialog *CMessageBoxUI)
    {
        CMessageBoxUI->setWindowTitle(QApplication::translate("CMessageBoxUI", "ButelLive", 0));
        m_pLabelShowInfo->setText(QString());
        m_pBtnYes->setText(QApplication::translate("CMessageBoxUI", "YES", 0));
        m_pBtnNO->setText(QApplication::translate("CMessageBoxUI", "NO", 0));
        m_pBtnCancel->setText(QApplication::translate("CMessageBoxUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class CMessageBoxUI: public Ui_CMessageBoxUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CMESSAGEBOX_H
