/********************************************************************************
** Form generated from reading UI file 'ChannelAndProgram.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CHANNELANDPROGRAM_H
#define UI_CHANNELANDPROGRAM_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QListWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_ChannelAndProgramUI
{
public:
    QGridLayout *gridLayout;
    TitleBar *m_pTitleLbl;
    QListWidget *m_pLWChannel;

    void setupUi(QDialog *ChannelAndProgramUI)
    {
        if (ChannelAndProgramUI->objectName().isEmpty())
            ChannelAndProgramUI->setObjectName(QStringLiteral("ChannelAndProgramUI"));
        ChannelAndProgramUI->resize(460, 339);
        ChannelAndProgramUI->setMinimumSize(QSize(460, 0));
        ChannelAndProgramUI->setMaximumSize(QSize(480, 16777215));
        ChannelAndProgramUI->setStyleSheet(QLatin1String("QWidget#ChannelAndProgramUI{\n"
"	background-color: rgb(35,35,37);\n"
"}"));
        gridLayout = new QGridLayout(ChannelAndProgramUI);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(-1, 0, -1, -1);
        m_pTitleLbl = new TitleBar(ChannelAndProgramUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 40));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 40));
        m_pTitleLbl->setStyleSheet(QLatin1String("background-color: rgba(0, 0, 0,0);\n"
"border-image: url(:/images/lineedit.png);\n"
""));

        gridLayout->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        m_pLWChannel = new QListWidget(ChannelAndProgramUI);
        m_pLWChannel->setObjectName(QStringLiteral("m_pLWChannel"));
        m_pLWChannel->setStyleSheet(QLatin1String("background-color: rgb(44, 44, 44);\n"
"border:0px;"));
        m_pLWChannel->setSelectionMode(QAbstractItemView::NoSelection);

        gridLayout->addWidget(m_pLWChannel, 1, 0, 1, 1);


        retranslateUi(ChannelAndProgramUI);

        QMetaObject::connectSlotsByName(ChannelAndProgramUI);
    } // setupUi

    void retranslateUi(QDialog *ChannelAndProgramUI)
    {
        ChannelAndProgramUI->setWindowTitle(QApplication::translate("ChannelAndProgramUI", "ButelLive", 0));
    } // retranslateUi

};

namespace Ui {
    class ChannelAndProgramUI: public Ui_ChannelAndProgramUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CHANNELANDPROGRAM_H
