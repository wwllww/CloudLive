/********************************************************************************
** Form generated from reading UI file 'PositionSizeSet.ui'
**
** Created by: Qt User Interface Compiler version 5.7.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_POSITIONSIZESET_H
#define UI_POSITIONSIZESET_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QCheckBox>
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

class Ui_PositionSizeSetUI
{
public:
    QGridLayout *gridLayout_2;
    TitleBar *m_pTitleLbl;
    QWidget *widget;
    QGridLayout *gridLayout_3;
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *m_pWidthEdt;
    QLabel *label_6;
    QLineEdit *m_pLeftTangentEdt;
    QLabel *label_2;
    QLineEdit *m_pHeightEdt;
    QLabel *label_8;
    QLineEdit *m_pTopTangentEdt;
    QLabel *label_3;
    QLineEdit *m_pXMoveEdt;
    QLabel *label_5;
    QLineEdit *m_pRightTangentEdt;
    QLabel *label_4;
    QLineEdit *m_pYMoveEdt;
    QLabel *label_7;
    QLineEdit *m_pBottomTangentEdt;
    QHBoxLayout *horizontalLayout_2;
    QCheckBox *m_pChkBoxKeepRatio;
    QSpacerItem *horizontalSpacer_3;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer;
    QPushButton *m_pBtnSetGlbCoord;
    QPushButton *m_pBtnOk;
    QPushButton *m_pBtnCancel;
    QSpacerItem *horizontalSpacer_2;

    void setupUi(QDialog *PositionSizeSetUI)
    {
        if (PositionSizeSetUI->objectName().isEmpty())
            PositionSizeSetUI->setObjectName(QStringLiteral("PositionSizeSetUI"));
        PositionSizeSetUI->resize(482, 249);
        PositionSizeSetUI->setStyleSheet(QStringLiteral("background-color: rgb(35, 35, 37);"));
        gridLayout_2 = new QGridLayout(PositionSizeSetUI);
        gridLayout_2->setObjectName(QStringLiteral("gridLayout_2"));
        gridLayout_2->setVerticalSpacing(0);
        gridLayout_2->setContentsMargins(5, 0, 5, 5);
        m_pTitleLbl = new TitleBar(PositionSizeSetUI);
        m_pTitleLbl->setObjectName(QStringLiteral("m_pTitleLbl"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(m_pTitleLbl->sizePolicy().hasHeightForWidth());
        m_pTitleLbl->setSizePolicy(sizePolicy);
        m_pTitleLbl->setMinimumSize(QSize(0, 30));
        m_pTitleLbl->setMaximumSize(QSize(16777215, 30));

        gridLayout_2->addWidget(m_pTitleLbl, 0, 0, 1, 1);

        widget = new QWidget(PositionSizeSetUI);
        widget->setObjectName(QStringLiteral("widget"));
        widget->setStyleSheet(QStringLiteral("background-color: rgb(153, 153, 153);"));
        gridLayout_3 = new QGridLayout(widget);
        gridLayout_3->setObjectName(QStringLiteral("gridLayout_3"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QStringLiteral("gridLayout"));
        label = new QLabel(widget);
        label->setObjectName(QStringLiteral("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        m_pWidthEdt = new QLineEdit(widget);
        m_pWidthEdt->setObjectName(QStringLiteral("m_pWidthEdt"));

        gridLayout->addWidget(m_pWidthEdt, 0, 1, 1, 1);

        label_6 = new QLabel(widget);
        label_6->setObjectName(QStringLiteral("label_6"));

        gridLayout->addWidget(label_6, 0, 2, 1, 1);

        m_pLeftTangentEdt = new QLineEdit(widget);
        m_pLeftTangentEdt->setObjectName(QStringLiteral("m_pLeftTangentEdt"));

        gridLayout->addWidget(m_pLeftTangentEdt, 0, 3, 1, 1);

        label_2 = new QLabel(widget);
        label_2->setObjectName(QStringLiteral("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        m_pHeightEdt = new QLineEdit(widget);
        m_pHeightEdt->setObjectName(QStringLiteral("m_pHeightEdt"));

        gridLayout->addWidget(m_pHeightEdt, 1, 1, 1, 1);

        label_8 = new QLabel(widget);
        label_8->setObjectName(QStringLiteral("label_8"));

        gridLayout->addWidget(label_8, 1, 2, 1, 1);

        m_pTopTangentEdt = new QLineEdit(widget);
        m_pTopTangentEdt->setObjectName(QStringLiteral("m_pTopTangentEdt"));

        gridLayout->addWidget(m_pTopTangentEdt, 1, 3, 1, 1);

        label_3 = new QLabel(widget);
        label_3->setObjectName(QStringLiteral("label_3"));

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        m_pXMoveEdt = new QLineEdit(widget);
        m_pXMoveEdt->setObjectName(QStringLiteral("m_pXMoveEdt"));

        gridLayout->addWidget(m_pXMoveEdt, 2, 1, 1, 1);

        label_5 = new QLabel(widget);
        label_5->setObjectName(QStringLiteral("label_5"));

        gridLayout->addWidget(label_5, 2, 2, 1, 1);

        m_pRightTangentEdt = new QLineEdit(widget);
        m_pRightTangentEdt->setObjectName(QStringLiteral("m_pRightTangentEdt"));

        gridLayout->addWidget(m_pRightTangentEdt, 2, 3, 1, 1);

        label_4 = new QLabel(widget);
        label_4->setObjectName(QStringLiteral("label_4"));

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        m_pYMoveEdt = new QLineEdit(widget);
        m_pYMoveEdt->setObjectName(QStringLiteral("m_pYMoveEdt"));

        gridLayout->addWidget(m_pYMoveEdt, 3, 1, 1, 1);

        label_7 = new QLabel(widget);
        label_7->setObjectName(QStringLiteral("label_7"));

        gridLayout->addWidget(label_7, 3, 2, 1, 1);

        m_pBottomTangentEdt = new QLineEdit(widget);
        m_pBottomTangentEdt->setObjectName(QStringLiteral("m_pBottomTangentEdt"));

        gridLayout->addWidget(m_pBottomTangentEdt, 3, 3, 1, 1);


        gridLayout_3->addLayout(gridLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QStringLiteral("horizontalLayout_2"));
        m_pChkBoxKeepRatio = new QCheckBox(widget);
        m_pChkBoxKeepRatio->setObjectName(QStringLiteral("m_pChkBoxKeepRatio"));
        m_pChkBoxKeepRatio->setLayoutDirection(Qt::RightToLeft);

        horizontalLayout_2->addWidget(m_pChkBoxKeepRatio);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);


        gridLayout_3->addLayout(horizontalLayout_2, 1, 0, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QStringLiteral("horizontalLayout"));
        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        m_pBtnSetGlbCoord = new QPushButton(widget);
        m_pBtnSetGlbCoord->setObjectName(QStringLiteral("m_pBtnSetGlbCoord"));
        m_pBtnSetGlbCoord->setMinimumSize(QSize(75, 20));
        m_pBtnSetGlbCoord->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnSetGlbCoord);

        m_pBtnOk = new QPushButton(widget);
        m_pBtnOk->setObjectName(QStringLiteral("m_pBtnOk"));
        m_pBtnOk->setMinimumSize(QSize(75, 20));
        m_pBtnOk->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnOk);

        m_pBtnCancel = new QPushButton(widget);
        m_pBtnCancel->setObjectName(QStringLiteral("m_pBtnCancel"));
        m_pBtnCancel->setMinimumSize(QSize(75, 20));
        m_pBtnCancel->setStyleSheet(QLatin1String("color: rgb(255, 255, 255);\n"
"border-image: url(:/images/btn_back.png);"));

        horizontalLayout->addWidget(m_pBtnCancel);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        gridLayout_3->addLayout(horizontalLayout, 2, 0, 1, 1);


        gridLayout_2->addWidget(widget, 1, 0, 1, 1);


        retranslateUi(PositionSizeSetUI);

        QMetaObject::connectSlotsByName(PositionSizeSetUI);
    } // setupUi

    void retranslateUi(QDialog *PositionSizeSetUI)
    {
        PositionSizeSetUI->setWindowTitle(QApplication::translate("PositionSizeSetUI", "ButelLive", 0));
        label->setText(QApplication::translate("PositionSizeSetUI", "Width", 0));
        label_6->setText(QApplication::translate("PositionSizeSetUI", "LeftTangent", 0));
        m_pLeftTangentEdt->setText(QApplication::translate("PositionSizeSetUI", "0", 0));
        label_2->setText(QApplication::translate("PositionSizeSetUI", "Height", 0));
        label_8->setText(QApplication::translate("PositionSizeSetUI", "TopTangent", 0));
        m_pTopTangentEdt->setText(QApplication::translate("PositionSizeSetUI", "0", 0));
        label_3->setText(QApplication::translate("PositionSizeSetUI", "Xmove", 0));
        label_5->setText(QApplication::translate("PositionSizeSetUI", "RightTangent", 0));
        m_pRightTangentEdt->setText(QApplication::translate("PositionSizeSetUI", "0", 0));
        label_4->setText(QApplication::translate("PositionSizeSetUI", "Ymove", 0));
        label_7->setText(QApplication::translate("PositionSizeSetUI", "BottomTangent", 0));
        m_pBottomTangentEdt->setText(QApplication::translate("PositionSizeSetUI", "0", 0));
        m_pChkBoxKeepRatio->setText(QApplication::translate("PositionSizeSetUI", "Keep aspect ratio", 0));
        m_pBtnSetGlbCoord->setText(QApplication::translate("PositionSizeSetUI", "SetGlobalCoord", 0));
        m_pBtnOk->setText(QApplication::translate("PositionSizeSetUI", "OK", 0));
        m_pBtnCancel->setText(QApplication::translate("PositionSizeSetUI", "Cancel", 0));
    } // retranslateUi

};

namespace Ui {
    class PositionSizeSetUI: public Ui_PositionSizeSetUI {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_POSITIONSIZESET_H
