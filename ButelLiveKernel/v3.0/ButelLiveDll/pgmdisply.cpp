#include "pgmdisplay.h"
#include "ButelLive.h"

#include <QVBoxLayout>
#include <QHBoxLayout>

PGMDisplay::PGMDisplay(QWidget *parent, QString str) :
    QWidget(parent)
{
    QVBoxLayout *vBoxLayoutL = new QVBoxLayout(this);
    for(int row = 0; row < 19; row++)
    {
        m_PGMItemList[row].setStyleSheet("QLabel{background-color:rgba(11, 11, 15, 255);}");
        m_PGMItemList[row].setFixedSize(10,2);
        vBoxLayoutL->addWidget(&m_PGMItemList[row]);
    }
    m_PGMItemList[0].setFixedHeight(4);
    m_PGMItemList[0].setStyleSheet("QLabel{background-color:rgba(201, 68, 89, 255);}");
    setLayout(vBoxLayoutL);
}

PGMDisplay::~PGMDisplay()
{

}

void PGMDisplay::SetValue(float fVal)
{
    for(int row = 1; row < 19; row++)
    {
        m_PGMItemList[row].setStyleSheet("QLabel{background-color:rgba(11, 11, 15, 255);}");
    }

    float fCal = fVal/(-96);
    int iCount = 18*fCal;
    for(int i = 18; i > iCount; i--)
    {
        m_PGMItemList[i].setStyleSheet("QLabel{background-color:rgba(38, 161, 192, 255);}");
    }
}
