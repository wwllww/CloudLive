#include "MaterialItem.h"

#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
CMaterialItem::CMaterialItem(ButelLive *parent,QString name,QString type,int Index):
    QWidget(parent)
{
    m_pParent= parent;
    m_LabelName.setText(name);
    m_LabelType.setText(type);
    m_Index = Index;
//    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
//    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(50);
    m_LabelName.setFixedWidth(140);
    m_LabelType.setFixedWidth(110);
    m_BtnOperator.setFixedSize(30,30);
    m_BtnOperator.setText("");
//    m_BtnOperator.setStyleSheet("QPushButton{border-image:url(:images/scene_edit.png);}"
//                         "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                         "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_LabelName);
    hBoxLayout->addWidget(&m_LabelType);
    hBoxLayout->addWidget(&m_BtnOperator);
    setLayout(hBoxLayout);
    QObject::connect(&m_BtnOperator,SIGNAL(clicked()),this,SLOT(OnBtnMaterialEditClicked()));
    QObject::connect(this,SIGNAL(MaterialEditClicked()),parent,SLOT(OnMaterialEditClicked()));

    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(35,35,37));
    setPalette(palette);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
}


void CMaterialItem::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
        SetItemInfo();
}

//void CMaterialItem::enterEvent(QEvent *event)
//{
//    setAutoFillBackground(true);
//    QPalette palette;
//    if(m_pParent->m_MaterialItemIsSelectedList.at(m_Index))
//    {
//        palette.setColor(QPalette::Base, QColor(47,160,190));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//    }
//    else
//    {

//        palette.setColor(QPalette::Base, QColor(35,35,37));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//    }
//}

//void CMaterialItem::leaveEvent(QEvent *event)
//{
//    setAutoFillBackground(true);
//    QPalette palette;
//    if(!m_pParent->m_MaterialItemIsSelectedList.at(m_Index))
//    {
//        palette.setColor(QPalette::Base, QColor(35,35,37));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//    }
//    else
//    {
//        palette.setColor(QPalette::Base, QColor(47,160,190));
//        setPalette(palette);
//        m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
//        m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
//                                    "QPushButton{border-image:url(:images/scene_edit.png);}"
//                                    "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
//                                    "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
//    }
//}

bool CMaterialItem::event(QEvent *event)
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

void CMaterialItem::OnBtnMaterialEditClicked()
{
    SetItemInfo();
    emit MaterialEditClicked();
}


//设置项目信息：包括当前选中项目索引、所有项目的颜色
void CMaterialItem::SetItemInfo()
{
    for(int iItemCount = 0; iItemCount < m_pParent->m_MaterialItemIsSelectedList.count(); iItemCount++)
    {
        m_pParent->m_MaterialItemIsSelectedList.replace(iItemCount,false);
        m_pParent->SetMaterialItemColor(iItemCount,false);
    }
    m_pParent->m_MaterialItemIsSelectedList.replace(m_Index,true);
    m_pParent->SetMaterialCurrentRow(m_Index);
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(47,160,190));
    setPalette(palette);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
}
