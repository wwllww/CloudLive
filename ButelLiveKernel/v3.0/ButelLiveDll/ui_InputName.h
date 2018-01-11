/********************************************************************************
** Form generated from reading UI file 'InputName.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_INPUTNAME_H
#define UI_INPUTNAME_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_InputNameUI
{
public:
    QGridLayout *gridLayout_2;
    QWidget *widget;
    QGridLayout *gridLayout;
    QLineEdit *m_pLineEditInputName;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    TitleBar *m_pTitleLbl;

    void setupUi(QDialog *InputNameUI)
    {
        if (InputNameUI->objectName().isEmpty())
            InputNameUI->setObjectName(QStringLiteral("InputNameUI"));
        InputNameUI->resize(414, 128);
        InputNameUI->setStyleSheet(QStringLiteral("background-color: rgb(35, 35, 37);"));
        gridLayout_2 = new QGridLayout(InputNameUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        widget = new QWidget(InputNameUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        m_pLineEditInputName = new QLineEdit(widget);
        m_pLineEditInputName->setObjectName(QStringLiteral("m_pLineEditInputName"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pLineEditInputName->sizePolicy().hasHeightForWidth());
        m_pLineEditInputName->setSizePolicy(sizePolicy);
        m_pLineEditInputName->setMinimumSize(QSize(0, 30));
        m_pLineEditInputName->setMaximumSize(QSize(16777215, 30));
        m_pLineEditInputName->setStyleSheet(QString::fromUtf8("background-color: rgb(121, 121, 121);\n"
"color: rgb(255, 255, 255);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:1px;"));

        gridLayout->addWidget(m_pLineEditInputName, 0, 0, 1, 1);

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


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);

        m_pTitleLbl = new TitleBar(InputNameUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy1);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);


        retranslateUi(InputNameUI);

        QMetaObject::connectSlotsByName(InputNameUI);
    } // setupUi

    void retranslateUi(QDialog *InputNameUI)
    {
        InputNameUI->setWindowTitle(QApplication::translate("InputNameUI", "ButelLive", 0));
        m_pBtnOk->setText(QApplication::translate("InputNameUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("InputNameUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class InputNameUI: public Ui_InputNameUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_INPUTNAME_H
