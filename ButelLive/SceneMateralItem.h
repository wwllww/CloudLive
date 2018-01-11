#ifndef SCENEMATERIALITEM_H
#define SCENEMATERIALITEM_H

#include "ButelLive.h"
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
class ButelLive;

class SceneMaterialItem: public QWidget
{
    Q_OBJECT
public:
    SceneMaterialItem(ButelLive *parent = 0,QString name = 0,int type = 0,int Index = 0);
    void SetItemInfo(int index);                  //设置所有项颜色和索引编号
public:
    ButelLive*     m_pParent;
    QCheckBox      m_CheckBox;
    QLabel         m_LabelName;
    QLabel         m_LabelType;
    QPushButton    m_BtnOperator;
    int            m_Index;              //当前项的索引号
signals:
    void EditClicked();
public slots:
    void OnEditClicked();
    void OnCheckBoxClicked(int state);
protected:
    void mousePressEvent(QMouseEvent *event);
//    void enterEvent(QEvent *event);
//    void leaveEvent(QEvent *event);
    bool event(QEvent *event);
};

#endif // SCENEMATERIALITEM_H
