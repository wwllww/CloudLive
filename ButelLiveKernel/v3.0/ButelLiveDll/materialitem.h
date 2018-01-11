#ifndef MATERIALITEM_H
#define MATERIALITEM_H

#include "ButelLive.h"
#include <QLabel>
#include <QPushButton>
class ButelLive;

class CMaterialItem: public QWidget
{
    Q_OBJECT
public:
    CMaterialItem(ButelLive *parent = 0,QString name = 0,QString type = 0,int Index = 0);
    void SetItemInfo();                  //设置所有项颜色和索引编号
public:
    ButelLive*     m_pParent;
    QLabel         m_LabelName;
    QLabel         m_LabelType;
    QPushButton    m_BtnOperator;
    int            m_Index;              //当前项的索引号
signals:
    void MaterialEditClicked();
public slots:
    void OnBtnMaterialEditClicked();
protected:
    void mousePressEvent(QMouseEvent *event);
//    void enterEvent(QEvent *event);
//    void leaveEvent(QEvent *event);
    bool event(QEvent *event);
};

#endif // MATERIALITEM_H
