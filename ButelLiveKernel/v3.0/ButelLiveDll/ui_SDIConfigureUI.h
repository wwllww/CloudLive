/********************************************************************************
** Form generated from reading UI file 'SDIConfigureUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SDICONFIGUREUI_H
#define UI_SDICONFIGUREUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_SDIConfigureUI
{
public:
    QGridLayout *gridLayout_2;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QGridLayout *gridLayout;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QLabel *label_2;
    QLabel *label_3;
    QLabel *label_4;
    QListWidget *m_pLWSDIDevice;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *SDIConfigureUI)
    {
        if (SDIConfigureUI->objectName().isEmpty())
            SDIConfigureUI->setObjectName(QStringLiteral("SDIConfigureUI"));
        SDIConfigureUI->resize(668, 394);
        SDIConfigureUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(SDIConfigureUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 2, 5, 5);
        m_pTitleLbl = new TitleBar(SDIConfigureUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(SDIConfigureUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        verticalLayout = new QVBoxLayout();
        verticalLayout->setObjectName(QStringLiteral("verticalLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));
        label->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(label);

        label_2 = new QLabel(widget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_2->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(label_2);

        label_3 = new QLabel(widget);
        label_3->setObjectName(QStringLiteral("label_3"));
        label_3->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_3->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(label_3);

        label_4 = new QLabel(widget);
        label_4->setObjectName(QStringLiteral("label_4"));
        label_4->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_4->setAlignment(Qt::AlignCenter);

        horizontalLayout_2->addWidget(label_4);


        verticalLayout->addLayout(horizontalLayout_2);

        m_pLWSDIDevice = new QListWidget(widget);
        m_pLWSDIDevice->setObjectName(QStringLiteral("m_pLWSDIDevice"));
        m_pLWSDIDevice->setSelectionMode(QAbstractItemView::NoSelection);

        verticalLayout->addWidget(m_pLWSDIDevice);

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


        verticalLayout->addLayout(horizontalLayout);


        gridLayout->addLayout(verticalLayout, 0, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(SDIConfigureUI);

        QMetaObject::connectSlotsByName(SDIConfigureUI);
    } // setupUi

    void retranslateUi(QDialog *SDIConfigureUI)
    {
        SDIConfigureUI->setWindowTitle(QApplication::translate("SDIConfigureUI", "ButelLive", 0));
        label->setText(QApplication::translate("SDIConfigureUI", "DeviceName", 0));
        label_2->setText(QApplication::translate("SDIConfigureUI", "OutputSource", 0));
        label_3->setText(QApplication::translate("SDIConfigureUI", "DeviceStandard", 0));
        label_4->setText(QApplication::translate("SDIConfigureUI", "State", 0));
        m_pBtnOk->setText(QApplication::translate("SDIConfigureUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("SDIConfigureUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class SDIConfigureUI: public Ui_SDIConfigureUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SDICONFIGUREUI_H
