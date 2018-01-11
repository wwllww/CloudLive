#include "previewitemdisarea.h"

PreviewItemDisArea::PreviewItemDisArea(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_PaintOnScreen,true);
    connect(this,SIGNAL(Preview()),parent,SLOT(OnBtnToPreview()));
}
QPaintEngine* PreviewItemDisArea::paintEngine()const
{
    return 0;
}

void PreviewItemDisArea::mouseDoubleClickEvent(QMouseEvent *event)
{
    emit Preview();
}
