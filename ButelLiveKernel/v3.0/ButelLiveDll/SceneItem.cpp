#include "SceneItem.h"

#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
CSceneItem::CSceneItem(ButelLive *parent,QString name,int Index):
    QWidget(parent)
{
    m_pParent= parent;
    m_LabelName.setText(name);
    m_Index = Index;
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(50);
    m_BtnOperator.setFixedSize(30,30);
    m_BtnOperator.setText("");
//    m_BtnOperator.setStyleSheet("QPushButton{border-image:url(:images/scene_edit.png);}"
//                         "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                         "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");

    m_BtnRightOperator.setFixedSize(30,30);
    m_BtnRightOperator.setText("");
//    m_BtnRightOperator.setStyleSheet("QPushButton{border-image:url(:images/scene_arrow.png);}"
//                        "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
//                        "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_LabelName);
    hBoxLayout->addSpacing(30);
    hBoxLayout->addWidget(&m_BtnOperator);
    hBoxLayout->addSpacing(30);
    hBoxLayout->addWidget(&m_BtnRightOperator);
    setLayout(hBoxLayout);
    QObject::connect(&m_BtnOperator,SIGNAL(clicked()),this,SLOT(OnBtnSceneEditClicked()));
    QObject::connect(this,SIGNAL(SceneEditClicked()),parent,SLOT(OnSceneEditClicked()));
    QObject::connect(&m_BtnRightOperator,SIGNAL(clicked()),this,SLOT(OnBtnSceneArrowClicked()));
    QObject::connect(this,SIGNAL(SceneArrowClicked()),parent,SLOT(OnSceneArrowClicked()));

    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(35,35,37));
    setPalette(palette);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                     "QPushButton{border-image:url(:images/scene_arrow.png);}"
                                     "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
                                     "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");

}


void CSceneItem::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
        SetItemInfo();
}

//void CSceneItem::enterEvent(QEvent *event)
//{
//    setAutoFillBackground(true);
//    QPalette palette;
//    if(m_pParent->m_ItemIsSelectedList.at(m_Index))
//    {
//        palette.setColor(QPalette::Base, QColor(47,160,190));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//        m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
//                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
//                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
//    }
//    else
//    {

//        palette.setColor(QPalette::Base, QColor(35,35,37));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//        m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
//                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
//                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
//    }
//}

//void CSceneItem::leaveEvent(QEvent *event)
//{
//    setAutoFillBackground(true);
//    QPalette palette;
//    if(!m_pParent->m_ItemIsSelectedList.at(m_Index))
//    {
//        palette.setColor(QPalette::Base, QColor(35,35,37));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//        m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
//                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
//                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
//    }
//    else
//    {
//        palette.setColor(QPalette::Base, QColor(47,160,190));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//        m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                         "QPushButton{border-image:url(:images/scene_arrow.png);}"
//                                         "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
//                                         "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
//    }
//}

bool CSceneItem::event(QEvent *event)
{
    switch(event->type())
    {
        case QEvent::Enter:
            enterEvent(event);
            if (!event->isAccepted())
                return false;
            break;
        case QEvent::Leave:
            leaveEvent(event);
            if (!event->isAccepted())
                return false;
            break;
        case QEvent::MouseButtonPress:
            mousePressEvent((QMouseEvent*)event);
            if (!event->isAccepted())
                return false;
        default:
            break;
    }
    return true;
}

void CSceneItem::OnBtnSceneEditClicked()
{
    SetItemInfo();
    emit SceneEditClicked();
}

void CSceneItem::OnBtnSceneArrowClicked()
{
    SetItemInfo();
    emit SceneArrowClicked();
}

//设置项目信息：包括当前选中项目索引、所有项目的颜色
void CSceneItem::SetItemInfo()
{
    for(int iItemCount = 0; iItemCount < m_pParent->m_ItemIsSelectedList.count(); iItemCount++)
    {
        m_pParent->m_ItemIsSelectedList.replace(iItemCount,false);
        m_pParent->SetItemColor(iItemCount,false);
    }
    m_pParent->m_ItemIsSelectedList.replace(m_Index,true);
    m_pParent->SetSceneCurrentRow(m_Index);
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(47,160,190));
    setPalette(palette);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    m_BtnRightOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                     "QPushButton{border-image:url(:images/scene_arrow.png);}"
                                     "QPushButton:hover{border-image:url(:images/scene_arrow_hover.png);}"
                                     "QPushButton:pressed{border-image:url(:images/scene_arrow_selected.png);}");
}
void CSceneItem::MouseScrollEnterItem()
{
    enterEvent(NULL);
}
