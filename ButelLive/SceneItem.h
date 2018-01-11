#ifndef CSCENEITEM_H
#define CSCENEITEM_H
#include "ButelLive.h"
#include <QLabel>
#include <QPushButton>
class ButelLive;

class CSceneItem: public QWidget
{
    Q_OBJECT
public:
    CSceneItem(ButelLive *parent = 0,QString name = 0,int Index = 0);
    void SetItemInfo();                  //设置所有项颜色和索引编号
    void MouseScrollEnterItem();
public:
    ButelLive*          m_pParent;
    QLabel               m_LabelName;
    QPushButton     m_BtnOperator;
    QPushButton     m_BtnRightOperator;
    int                     m_Index;              //当前项的索引号
signals:
    void SceneEditClicked();
    void SceneArrowClicked();
public slots:
    void OnBtnSceneEditClicked();
    void OnBtnSceneArrowClicked();
protected:
    void mousePressEvent(QMouseEvent *event);
//    void enterEvent(QEvent *event);
//    void leaveEvent(QEvent *event);
    bool event(QEvent *event);
};

#endif // CSCENEITEM_H
