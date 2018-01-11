#ifndef PGMDISPLAY_H
#define PGMDISPLAY_H

#include <QWidget>
#include <QLabel>
#include <QList>

#include "ButelLive.h"

class PGMDisplay : public QWidget
{
  Q_OBJECT

public:
    explicit PGMDisplay(QWidget *parent = 0,QString=0);
    ~PGMDisplay();

    void SetValue(float fVal);

public:
    QLabel m_PGMItemList[19];        //声音动态显示
};

#endif // PGMDISPLAY_H
