#ifndef LIVEDISPLAYAREA_H
#define LIVEDISPLAYAREA_H
#include "PoleLive.h"
#include <QPaintEngine>

class LiveDisplayArea: public QWidget
{
    Q_OBJECT
public:
    LiveDisplayArea(QWidget *parent = 0);

protected:
    QPaintEngine* paintEngine()const;
private:

    PoleLive*       m_Parent;
};

#endif // LIVEDISPLAYAREA_H
