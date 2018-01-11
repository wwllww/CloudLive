#ifndef PLAYCONTROL_H
#define PLAYCONTROL_H
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#include "cslider.h"
#include "PreviewItem.h"

class CPreviewItem;

class CPlayControl: public QWidget
{
    Q_OBJECT
public:
    CPlayControl(QWidget *parent);
    virtual ~CPlayControl();
    QGridLayout gridLayout;
    QVBoxLayout verticalLayout;
    QHBoxLayout horizontalLayout;

    CSlider         m_ProgressCtr;
    QPushButton     m_BtnPause;
    QPushButton     m_BtnStop;
    QPushButton     m_BtnForward;
    QPushButton     m_BtnNext;
    QPushButton     m_BtnMenu;
    QLabel          m_TimeLbl;
    CPreviewItem*   m_pParent;

};

#endif // PLAYCONTROL_H
