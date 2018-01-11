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
    m_pParent = (CPreviewItem*)parent;
    setFixedSize(160,20);
    m_ProgressCtr.setFixedSize(160,10);
    m_BtnPause.setFixedSize(10,10);
    m_BtnStop.setFixedSize(10,10);
    m_BtnForward.setFixedSize(10,10);
    m_BtnNext.setFixedSize(10,10);
    m_BtnMenu.setFixedSize(10,10);
    m_TimeLbl.setFixedSize(110,10);

    m_ProgressCtr.setOrientation(Qt::Horizontal);

    setStyleSheet("QWidget{background-color:rgb(35,35,37);color:rgb(255,255,255);font:8pt;}");

    m_ProgressCtr.setStyleSheet("QSlider::add-page:horizontal"
                                    "{"
                                       "background-color:rgb(174,174,174);"
                                       "height:2px;"
                                    "}"
                                    "QSlider::sub-page:horizontal"
                                    "{"
                                        "background-color:rgb(230,0,0);"
                                        "height:2px;"
                                    "}"
                                    "QSlider::groove:horizontal"
                                    "{"
                                        "background:transparent;"
                                        "height:2px;"
                                    "}"
                                    "QSlider::handle:horizontal"
                                    "{"
                                       "width:8px;"
                                       "height:7px;"
                                       "border-image: url(:/images/Slider.png);"
                                       "margin: -4 0px;"
                                    "}"
                                    );
    m_BtnPause.setStyleSheet("QPushButton{border-image:url(:images/Pause.png);}");
    m_BtnStop.setStyleSheet("QPushButton{border-image:url(:images/Stop.png);}");
    m_BtnForward.setStyleSheet("QPushButton{border-image:url(:images/player_last.png);}");
    m_BtnNext.setStyleSheet("QPushButton{border-image:url(:images/player_next.png);}");
    m_BtnMenu.setStyleSheet("QPushButton{border-image:url(:images/player_menu.png);}"
                            "QPushButton:hover{border-image:url(:images/player_menu_hover.png);}");

    verticalLayout.addWidget(&m_ProgressCtr);
//    m_BtnPause.setText("");
//    m_BtnStop.setText("");
//    m_BtnForward.setText("");
//    m_BtnNext.setText("");
//    m_BtnMenu.setText("");
    m_TimeLbl.setText("00:00:00/00:00:00");
    m_TimeLbl.setAlignment(Qt::AlignCenter);
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
    setContentsMargins(0,0,0,0);
}

CPlayControl::~CPlayControl()
{

}

