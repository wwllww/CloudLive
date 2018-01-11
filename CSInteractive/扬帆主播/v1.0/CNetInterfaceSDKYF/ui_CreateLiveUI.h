/********************************************************************************
** Form generated from reading UI file 'CreateLiveUI.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_CREATELIVEUI_H
#define UI_CREATELIVEUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_CreateLiveUI
{
public:
    QGridLayout *gridLayout;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QLabel *label;
    QLabel *label_2;
    QLineEdit *m_pLdtProgramName;
    QLabel *m_pLbCurChannel;
    QLabel *m_pLbPicture;
    QPushButton *m_pBtnUpload;
    QPushButton *m_pBtnGoLive;
    QProgressBar *m_pProgressBar;

    void setupUi(QDialog *CreateLiveUI)
    {
        if (CreateLiveUI->objectName().isEmpty())
            CreateLiveUI->setObjectName(QStringLiteral("CreateLiveUI"));
        CreateLiveUI->resize(600, 422);
        CreateLiveUI->setStyleSheet(QLatin1String("background-color: rgba(35, 35, 37,255);\n"
"border-image: url(:/images/xxx_bg.png);"));
        gridLayout = new QGridLayout(CreateLiveUI);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setVerticalSpacing(0);
        gridLayout->setContentsMargins(5, 0, 5, 5);
        m_pTitleLbl = new TitleBar(CreateLiveUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(CreateLiveUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(44, 44, 44);"));
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));
        label->setGeometry(QRect(40, 35, 61, 40));
        label->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        label_2 = new QLabel(widget);
        label_2->setObjectName(QStringLiteral("label_2"));
        label_2->setGeometry(QRect(40, 90, 61, 40));
        label_2->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        m_pLdtProgramName = new QLineEdit(widget);
        m_pLdtProgramName->setObjectName(QStringLiteral("m_pLdtProgramName"));
        m_pLdtProgramName->setGeometry(QRect(125, 90, 290, 40));
        m_pLdtProgramName->setStyleSheet(QLatin1String("background-color: rgb(31, 31, 32);\n"
"color: rgb(255, 255, 255);\n"
"border:0px;"));
        m_pLbCurChannel = new QLabel(widget);
        m_pLbCurChannel->setObjectName(QStringLiteral("m_pLbCurChannel"));
        m_pLbCurChannel->setGeometry(QRect(125, 35, 290, 40));
        m_pLbCurChannel->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));
        m_pLbPicture = new QLabel(widget);
        m_pLbPicture->setObjectName(QStringLiteral("m_pLbPicture"));
        m_pLbPicture->setGeometry(QRect(125, 150, 180, 100));
        m_pLbPicture->setStyleSheet(QStringLiteral(""));
        m_pBtnUpload = new QPushButton(widget);
        m_pBtnUpload->setObjectName(QStringLiteral("m_pBtnUpload"));
        m_pBtnUpload->setGeometry(QRect(315, 150, 100, 28));
        m_pBtnUpload->setStyleSheet(QLatin1String("border-image: url(:/images/btn_upload_bg.png);\n"
"color: rgb(255, 255, 255);"));
        m_pBtnGoLive = new QPushButton(widget);
        m_pBtnGoLive->setObjectName(QStringLiteral("m_pBtnGoLive"));
        m_pBtnGoLive->setGeometry(QRect(125, 290, 350, 41));
        m_pBtnGoLive->setStyleSheet(QLatin1String("border-image: url(:/images/btn_golive_bg.png);\n"
"color: rgb(255, 255, 255);"));
        m_pProgressBar = new QProgressBar(widget);
        m_pProgressBar->setObjectName(QStringLiteral("m_pProgressBar"));
        m_pProgressBar->setGeometry(QRect(125, 250, 180, 6));
        m_pProgressBar->setMinimumSize(QSize(0, 6));
        m_pProgressBar->setMaximumSize(QSize(16777215, 6));
        m_pProgressBar->setStyleSheet(QLatin1String("border:none;\n"
"background:#dddfe5;\n"
"border-radius:3px;\n"
"text-align:center;\n"
"}\n"
"QProgressBar::chunk {\n"
"background-color:#1eb5dc;\n"
"border-radius:3px;"));
        m_pProgressBar->setValue(0);
        m_pProgressBar->setTextVisible(false);

        gridLayout->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(CreateLiveUI);

        QMetaObject::connectSlotsByName(CreateLiveUI);
    } // setupUi

    void retranslateUi(QDialog *CreateLiveUI)
    {
        CreateLiveUI->setWindowTitle(QApplication::translate("CreateLiveUI", "ButelLive", 0));
        label->setText(QApplication::translate("CreateLiveUI", "CurChannel", 0));
        label_2->setText(QApplication::translate("CreateLiveUI", "LiveName", 0));
        m_pLdtProgramName->setText(QString());
        m_pLbCurChannel->setText(QString());
        m_pLbPicture->setText(QString());
        m_pBtnUpload->setText(QApplication::translate("CreateLiveUI", "Upload", 0));
        m_pBtnGoLive->setText(QApplication::translate("CreateLiveUI", "GoLive", 0));
    } // retranslateUi

};

namespace Ui {
    class CreateLiveUI: public Ui_CreateLiveUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_CREATELIVEUI_H
