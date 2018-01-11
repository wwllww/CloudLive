/********************************************************************************
** Form generated from reading UI file 'Login.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGIN_H
#define UI_LOGIN_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_LoginUI
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_11;
    QSpacerItem *horizontalSpacer_23;
    QPushButton *m_pBtnLogin;
    QSpacerItem *horizontalSpacer_24;
    QHBoxLayout *horizontalLayout_7;
    QSpacerItem *horizontalSpacer_17;
    QLabel *m_pLblPassWord;
    QLineEdit *m_pLineEditPassWord;
    QSpacerItem *horizontalSpacer_18;
    QSpacerItem *verticalSpacer_6;
    QHBoxLayout *horizontalLayout_10;
    QSpacerItem *verticalSpacer;
    QSpacerItem *verticalSpacer_4;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QLabel *m_pLblLogo;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_12;
    QSpacerItem *horizontalSpacer_25;
    QLabel *m_pLblLoginInfo;
    QSpacerItem *horizontalSpacer_26;
    QHBoxLayout *horizontalLayout_15;
    QSpacerItem *horizontalSpacer_30;
    QLabel *m_pLblUserName;
    QHBoxLayout *horizontalLayout_16;
    QLineEdit *m_pLineEditUserName;
    QPushButton *m_pBtnUserNameDel;
    QSpacerItem *horizontalSpacer_31;
    TitleBar *m_pTitleLbl;
    QSpacerItem *verticalSpacer_2;

    void setupUi(QDialog *LoginUI)
    {
        if (LoginUI->objectName().isEmpty())
            LoginUI->setObjectName(QStringLiteral("LoginUI"));
        LoginUI->resize(460, 600);
        LoginUI->setMinimumSize(QSize(460, 0));
        LoginUI->setMaximumSize(QSize(480, 16777215));
        LoginUI->setStyleSheet(QStringLiteral(""));
        gridLayout = new QGridLayout(LoginUI);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setSpacing(0);
        horizontalLayout_11->setObjectName(QStringLiteral("horizontalLayout_11"));
        horizontalSpacer_23 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_23);

        m_pBtnLogin = new QPushButton(LoginUI);
        m_pBtnLogin->setObjectName(QStringLiteral("m_pBtnLogin"));
        QSizePolicy sizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pBtnLogin->sizePolicy().hasHeightForWidth());
        m_pBtnLogin->setSizePolicy(sizePolicy);
        m_pBtnLogin->setMinimumSize(QSize(300, 50));
        m_pBtnLogin->setMaximumSize(QSize(300, 50));
        m_pBtnLogin->setStyleSheet(QString::fromUtf8("border-color: rgba(22, 121, 145,255);\n"
"font: 75 14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border-image: url(:/images/blue_bg.png);\n"
"color: rgb(255, 255, 255);"));

        horizontalLayout_11->addWidget(m_pBtnLogin);

        horizontalSpacer_24 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_24);


        gridLayout->addLayout(horizontalLayout_11, 8, 0, 1, 1);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(0);
        horizontalLayout_7->setObjectName(QStringLiteral("horizontalLayout_7"));
        horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_17);

        m_pLblPassWord = new QLabel(LoginUI);
        m_pLblPassWord->setObjectName(QStringLiteral("m_pLblPassWord"));
        sizePolicy.setHeightForWidth(m_pLblPassWord->sizePolicy().hasHeightForWidth());
        m_pLblPassWord->setSizePolicy(sizePolicy);
        m_pLblPassWord->setMinimumSize(QSize(50, 50));
        m_pLblPassWord->setMaximumSize(QSize(50, 50));
        m_pLblPassWord->setStyleSheet(QLatin1String("border-image: url(:/images/login_password.png);\n"
"background-color: rgba(0, 0, 0,50);"));

        horizontalLayout_7->addWidget(m_pLblPassWord);

        m_pLineEditPassWord = new QLineEdit(LoginUI);
        m_pLineEditPassWord->setObjectName(QStringLiteral("m_pLineEditPassWord"));
        sizePolicy.setHeightForWidth(m_pLineEditPassWord->sizePolicy().hasHeightForWidth());
        m_pLineEditPassWord->setSizePolicy(sizePolicy);
        m_pLineEditPassWord->setMinimumSize(QSize(250, 50));
        m_pLineEditPassWord->setMaximumSize(QSize(250, 50));
        m_pLineEditPassWord->setFocusPolicy(Qt::WheelFocus);
        m_pLineEditPassWord->setStyleSheet(QString::fromUtf8("background-color: rgba(0, 0, 0,75);\n"
"font: 75 14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border-image: url(:/images/xxx.png);\n"
"color: rgb(255, 255, 255);\n"
"border:1px;"));
        m_pLineEditPassWord->setEchoMode(QLineEdit::Password);

        horizontalLayout_7->addWidget(m_pLineEditPassWord);

        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_7->addItem(horizontalSpacer_18);


        gridLayout->addLayout(horizontalLayout_7, 6, 0, 1, 1);

        verticalSpacer_6 = new QSpacerItem(17, 54, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_6, 10, 0, 1, 1);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QStringLiteral("horizontalLayout_10"));

        gridLayout->addLayout(horizontalLayout_10, 9, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 54, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 3, 0, 1, 1);

        verticalSpacer_4 = new QSpacerItem(20, 37, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_4, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pLblLogo = new QLabel(LoginUI);
        m_pLblLogo->setObjectName(QStringLiteral("m_pLblLogo"));
        sizePolicy.setHeightForWidth(m_pLblLogo->sizePolicy().hasHeightForWidth());
        m_pLblLogo->setSizePolicy(sizePolicy);
        m_pLblLogo->setMinimumSize(QSize(130, 130));
        m_pLblLogo->setMaximumSize(QSize(130, 130));
        m_pLblLogo->setStyleSheet(QStringLiteral(""));

        horizontalLayout->addWidget(m_pLblLogo);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 1);

        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QStringLiteral("horizontalLayout_12"));
        horizontalSpacer_25 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_25);

        m_pLblLoginInfo = new QLabel(LoginUI);
        m_pLblLoginInfo->setObjectName(QStringLiteral("m_pLblLoginInfo"));
        sizePolicy.setHeightForWidth(m_pLblLoginInfo->sizePolicy().hasHeightForWidth());
        m_pLblLoginInfo->setSizePolicy(sizePolicy);
        m_pLblLoginInfo->setMinimumSize(QSize(300, 30));
        m_pLblLoginInfo->setMaximumSize(QSize(300, 30));
        m_pLblLoginInfo->setStyleSheet(QString::fromUtf8("background-color: rgba(85, 255, 255,0);\n"
"color: rgb(211, 91, 102);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border-image: url(:/images/xxx.png);"));
        m_pLblLoginInfo->setAlignment(Qt::AlignCenter);

        horizontalLayout_12->addWidget(m_pLblLoginInfo);

        horizontalSpacer_26 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_26);


        gridLayout->addLayout(horizontalLayout_12, 4, 0, 1, 1);

        horizontalLayout_15 = new QHBoxLayout();
        horizontalLayout_15->setSpacing(0);
        horizontalLayout_15->setObjectName(QStringLiteral("horizontalLayout_15"));
        horizontalSpacer_30 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_30);

        m_pLblUserName = new QLabel(LoginUI);
        m_pLblUserName->setObjectName(QStringLiteral("m_pLblUserName"));
        sizePolicy.setHeightForWidth(m_pLblUserName->sizePolicy().hasHeightForWidth());
        m_pLblUserName->setSizePolicy(sizePolicy);
        m_pLblUserName->setMinimumSize(QSize(50, 50));
        m_pLblUserName->setMaximumSize(QSize(50, 50));
        m_pLblUserName->setStyleSheet(QLatin1String("border-image: url(:/images/login_user.png);\n"
"background-color: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0, stop:0 rgba(255, 178, 102, 255), stop:0.55 rgba(235, 148, 61, 255), stop:0.98 rgba(0, 0, 0, 255), stop:1 rgba(0, 0, 0, 0));\n"
"background-color: rgba(0, 0, 0, 75);\n"
""));

        horizontalLayout_15->addWidget(m_pLblUserName);

        horizontalLayout_16 = new QHBoxLayout();
        horizontalLayout_16->setSpacing(0);
        horizontalLayout_16->setObjectName(QStringLiteral("horizontalLayout_16"));
        m_pLineEditUserName = new QLineEdit(LoginUI);
        m_pLineEditUserName->setObjectName(QStringLiteral("m_pLineEditUserName"));
        sizePolicy.setHeightForWidth(m_pLineEditUserName->sizePolicy().hasHeightForWidth());
        m_pLineEditUserName->setSizePolicy(sizePolicy);
        m_pLineEditUserName->setMinimumSize(QSize(200, 50));
        m_pLineEditUserName->setMaximumSize(QSize(200, 50));
        m_pLineEditUserName->setStyleSheet(QString::fromUtf8("background-color: rgba(0, 0, 0,75);\n"
"font: 75 14pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border-image: url(:/images/lineedit.png);\n"
"color: rgb(255, 255, 255);\n"
"border:1px;"));

        horizontalLayout_16->addWidget(m_pLineEditUserName);

        m_pBtnUserNameDel = new QPushButton(LoginUI);
        m_pBtnUserNameDel->setObjectName(QStringLiteral("m_pBtnUserNameDel"));
        m_pBtnUserNameDel->setMinimumSize(QSize(50, 50));
        m_pBtnUserNameDel->setMaximumSize(QSize(50, 50));
        m_pBtnUserNameDel->setStyleSheet(QLatin1String("border-color: rgb(0, 0, 0);\n"
"background-color: rgba(0, 0, 0,75);\n"
"border-image: url(:/images/nav_close.png);"));

        horizontalLayout_16->addWidget(m_pBtnUserNameDel);


        horizontalLayout_15->addLayout(horizontalLayout_16);

        horizontalSpacer_31 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_15->addItem(horizontalSpacer_31);


        gridLayout->addLayout(horizontalLayout_15, 5, 0, 1, 1);

        m_pTitleLbl = new TitleBar(LoginUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy1);
        m_pTitleLbl->setMinimumSize(QSize(0, 40));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 40));
        m_pTitleLbl->setStyleSheet(QLatin1String("background-color: rgba(0, 0, 0,0);\n"
"border-image: url(:/images/lineedit.png);\n"
""));

        gridLayout->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer_2, 7, 0, 1, 1);

        QWidget::setTabOrder(m_pLineEditUserName, m_pLineEditPassWord);
        QWidget::setTabOrder(m_pLineEditPassWord, m_pBtnLogin);
        QWidget::setTabOrder(m_pBtnLogin, m_pBtnUserNameDel);

        retranslateUi(LoginUI);

        QMetaObject::connectSlotsByName(LoginUI);
    } // setupUi

    void retranslateUi(QDialog *LoginUI)
    {
        LoginUI->setWindowTitle(QApplication::translate("LoginUI", "ButelLive", 0));
        m_pBtnLogin->setText(QApplication::translate("LoginUI", "login", 0));
        m_pLblPassWord->setText(QString());
        m_pLineEditPassWord->setText(QString());
        m_pLblLogo->setText(QString());
        m_pLblLoginInfo->setText(QString());
        m_pLblUserName->setText(QString());
        m_pLineEditUserName->setText(QString());
        m_pBtnUserNameDel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class LoginUI: public Ui_LoginUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGIN_H
