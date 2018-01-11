#ifndef CSLIDER_H
#define CSLIDER_H

#include <QMouseEvent>
#include <QSlider>
#include <QCoreApplication>
class CSlider : public QSlider
{
    Q_OBJECT
public:
    CSlider(QWidget* parent = 0);

signals:
    void mouseClicked();
protected:
    void mouseReleaseEvent(QMouseEvent *ev);
};

#endif // CSLIDER_H
