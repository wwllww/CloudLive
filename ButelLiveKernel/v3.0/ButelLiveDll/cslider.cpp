#include "cslider.h"

CSlider::CSlider(QWidget* parent)
{
//    this->setOrientation(Qt::Horizontal);
}

void CSlider::mouseReleaseEvent(QMouseEvent *ev)
{
    //注意应先调用父类的鼠标点击处理事件，这样可以不影响拖动的情况
    QSlider::mouseReleaseEvent(ev);
    //获取鼠标的位置，这里并不能直接从ev中取值（因为如果是拖动的话，鼠标开始点击的位置没有意义了）

    if(Qt::Horizontal == orientation())
    {
        double pos = ev->pos().x() / (double)width();
//        if(maximum() > 3600)
//        {
//            if(pos * (maximum() - minimum()) + minimum() > maximum() - 30)
//                setValue(maximum() - 30);
//            else
//                setValue(pos * (maximum() - minimum()) + minimum());
//        }
//        else
//        {
            setValue(pos * (maximum() - minimum()) + minimum());
//        }
    }
    else
    {
        double pos = (height() - ev->pos().y()) / (double)height();
        setValue(pos * (maximum() - minimum()) + minimum());
    }
    emit mouseClicked();

    //向父窗口发送自定义事件event type，这样就可以在父窗口中捕获这个事件进行处理
    QEvent evEvent(static_cast<QEvent::Type>(QEvent::User + 1));
    QCoreApplication::sendEvent(parentWidget(), &evEvent);
}
