#ifndef PREVIEWITEMDISAREA_H
#define PREVIEWITEMDISAREA_H

#include <QWidget>

class PreviewItemDisArea : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewItemDisArea(QWidget *parent = 0);

protected:
    QPaintEngine* paintEngine()const;
    void mouseDoubleClickEvent(QMouseEvent *event);

signals:
    void Preview();

public slots:
};

#endif // PREVIEWITEMDISAREA_H
