#include "LiveDislplayArea.h"
#include "LogDeliver.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

LiveDislplayArea::LiveDislplayArea(QWidget *parent) :
    QWidget(parent)
{
    m_Parent = (ButelLive*)parent;
    setAttribute(Qt::WA_PaintOnScreen,true);
}


QPaintEngine* LiveDislplayArea::paintEngine()const
{
    return 0;
}

