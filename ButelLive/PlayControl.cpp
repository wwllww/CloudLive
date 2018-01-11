#include "PlayControl.h"
#include "SLiveApi.h"
#include <QSpacerItem>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QGraphicsOpacityEffect>
CPlayControl::CPlayControl(QWidget *parent):
    QWidget(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint);
    setWindowOpacity(0.7);
    m_pParent = (CPreviewItem*)parent;
    resize(QSize(190,35));
    m_BtnPause.setFixedSize(15,15);
    m_BtnStop.setFixedSize(15,15);
    m_BtnForward.setFixedSize(15,15);
    m_BtnNext.setFixedSize(15,15);
    m_BtnMenu.setFixedSize(15,15);

    verticalLayout.addWidget(&m_ProgressCtr);
    m_BtnPause.setText("P");
    m_BtnStop.setText("S");
    m_BtnForward.setText("F");
    m_BtnNext.setText("N");
    m_BtnMenu.setText("M");
    m_TimeLbl.setText("00:00:00/00:00:00");
    horizontalLayout.addWidget(&m_BtnPause);
    horizontalLayout.addWidget(&m_BtnStop);
    horizontalLayout.addWidget(&m_BtnForward);
    horizontalLayout.addWidget(&m_BtnNext);
    horizontalLayout.addWidget(&m_BtnMenu);
    horizontalLayout.addWidget(&m_TimeLbl);
    horizontalLayout.setContentsMargins(0,0,0,0);
    verticalLayout.setSpacing(0);
    horizontalLayout.setSpacing(0);
    verticalLayout.addLayout(&horizontalLayout);
    verticalLayout.setContentsMargins(0,0,0,0);
    setLayout(&verticalLayout);

    setStyleSheet("QWidget{color:#ffffff;font:8pt;}");
//    m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Pause.png);}");
}

CPlayControl::~CPlayControl()
{

}

