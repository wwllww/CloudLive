/********************************************************************************
** Form generated from reading UI file 'AboutUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTUI_H
#define UI_ABOUTUI_H

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
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_AboutUI
{
public:
    QGridLayout *gridLayout_2;
    QWidget *widget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_2;
    QLabel *m_pLbVersion;
    TitleBar *m_pTitleLbl;

    void setupUi(QDialog *AboutUI)
    {
        if (AboutUI->objectName().isEmpty())
            AboutUI->setObjectName(QStringLiteral("AboutUI"));
        AboutUI->resize(368, 164);
        AboutUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout_2 = new QGridLayout(AboutUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        widget = new QWidget(AboutUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
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

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        m_pLbVersion = new QLabel(widget);
        m_pLbVersion->setObjectName(QStringLiteral("m_pLbVersion"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pLbVersion->sizePolicy().hasHeightForWidth());
        m_pLbVersion->setSizePolicy(sizePolicy);
        m_pLbVersion->setStyleSheet(QString::fromUtf8("color: rgb(255, 255, 255);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";"));

        horizontalLayout_2->addWidget(m_pLbVersion);


        gridLayout->addLayout(horizontalLayout_2, 0, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);

        m_pTitleLbl = new TitleBar(AboutUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy1);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);


        retranslateUi(AboutUI);

        QMetaObject::connectSlotsByName(AboutUI);
    } // setupUi

    void retranslateUi(QDialog *AboutUI)
    {
        AboutUI->setWindowTitle(QApplication::translate("AboutUI", "ButelLive", 0));
        m_pBtnOk->setText(QApplication::translate("AboutUI", "OK", 0));
        m_pLbVersion->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class AboutUI: public Ui_AboutUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTUI_H
