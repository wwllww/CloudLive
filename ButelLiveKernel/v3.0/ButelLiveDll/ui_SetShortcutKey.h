/********************************************************************************
** Form generated from reading UI file 'SetShortcutKey.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETSHORTCUTKEY_H
#define UI_SETSHORTCUTKEY_H

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
#include <QtWidgets/QWidget>
#include <titlebar.h>

QT_BEGIN_NAMESPACE

class Ui_SetShortcutKeyUI
{
public:
    QGridLayout *gridLayout_2;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout_2;
    QLabel *label;
    QLineEdit *m_pLineEditInputName;
    QPushButton *pushButton;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;

    void setupUi(QDialog *SetShortcutKeyUI)
    {
        if (SetShortcutKeyUI->objectName().isEmpty())
            SetShortcutKeyUI->setObjectName(QStringLiteral("SetShortcutKeyUI"));
        SetShortcutKeyUI->resize(414, 113);
        SetShortcutKeyUI->setStyleSheet(QStringLiteral("background-color: rgb(35, 35, 37);"));
        gridLayout_2 = new QGridLayout(SetShortcutKeyUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        m_pTitleLbl = new TitleBar(SetShortcutKeyUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(SetShortcutKeyUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout = new QGridLayout(widget);
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));
        label->setStyleSheet(QStringLiteral("color: rgb(255, 255, 255);"));

        horizontalLayout_2->addWidget(label);

        m_pLineEditInputName = new QLineEdit(widget);
        m_pLineEditInputName->setObjectName(QStringLiteral("m_pLineEditInputName"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Maximum);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(m_pLineEditInputName->sizePolicy().hasHeightForWidth());
        m_pLineEditInputName->setSizePolicy(sizePolicy1);
        m_pLineEditInputName->setMinimumSize(QSize(0, 30));
        m_pLineEditInputName->setMaximumSize(QSize(16777215, 30));
        m_pLineEditInputName->setStyleSheet(QString::fromUtf8("background-color: rgb(255, 255, 255);\n"
"color: rgb(0, 0, 0);\n"
"font: 75 11pt \"\345\276\256\350\275\257\351\233\205\351\273\221\";\n"
"border:1px;"));
        m_pLineEditInputName->setReadOnly(false);

        horizontalLayout_2->addWidget(m_pLineEditInputName);

        pushButton = new QPushButton(widget);
        pushButton->setObjectName(QStringLiteral("pushButton"));
        pushButton->setMinimumSize(QSize(75, 20));
        pushButton->setMaximumSize(QSize(75, 20));
        pushButton->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout_2->addWidget(pushButton);


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


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(SetShortcutKeyUI);

        QMetaObject::connectSlotsByName(SetShortcutKeyUI);
    } // setupUi

    void retranslateUi(QDialog *SetShortcutKeyUI)
    {
        SetShortcutKeyUI->setWindowTitle(QApplication::translate("SetShortcutKeyUI", "ButelLive", 0));
        label->setText(QApplication::translate("SetShortcutKeyUI", "Shortcut Key\357\274\232", 0));
        m_pLineEditInputName->setText(QString());
        m_pLineEditInputName->setPlaceholderText(QApplication::translate("SetShortcutKeyUI", "Start with the Ctr key", 0));
        pushButton->setText(QApplication::translate("SetShortcutKeyUI", "DelShortcut", 0));
        m_pBtnOk->setText(QApplication::translate("SetShortcutKeyUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("SetShortcutKeyUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class SetShortcutKeyUI: public Ui_SetShortcutKeyUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETSHORTCUTKEY_H
