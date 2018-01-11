#ifndef LIVEDISPLAYAREA_H
#define LIVEDISPLAYAREA_H
#include "ButelLive.h"
#include <QWidget>
//投影窗口类
class LiveDislplayArea : public QWidget
{
    Q_OBJECT
public:
    explicit LiveDislplayArea(QWidget *parent = 0);

protected:
    QPaintEngine* paintEngine()const;
public:
    ButelLive*      m_Parent;

};

#endif // LIVEDISPLAYAREA_H
