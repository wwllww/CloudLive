#include "LiveDisplayArea.h"
#include "PoleLive.h"

LiveDisplayArea::LiveDisplayArea(QWidget *parent):
    QWidget(parent)
{
    m_Parent = (PoleLive*)parent;
    setAttribute(Qt::WA_PaintOnScreen,true);
}

QPaintEngine* LiveDisplayArea::paintEngine()const
{
    return 0;
}

