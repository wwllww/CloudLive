#include "SceneMateralItem.h"

#include <QHBoxLayout>
#include <QEvent>
#include <QMouseEvent>
SceneMaterialItem::SceneMaterialItem(ButelLive *parent,QString name,int type,int Index):
    QWidget(parent)
{
    m_pParent= parent;
    m_LabelName.setText(name);
    if(type == Dianbo)
        m_LabelType.setText(tr("VOD source"));
    else if(type == Shexiang)
        m_LabelType.setText(tr("Video capture source"));
    else if(type == Hudong)
        m_LabelType.setText(tr("Interactive link source"));
    else if(type == Tupian)
        m_LabelType.setText(tr("Picture source"));
    else if(type == Datetime)
        m_LabelType.setText(tr("Datetime source"));
    else if(type == AudioCapture)
        m_LabelType.setText(tr("Audio capture source"));
    else if(type == Text)
        m_LabelType.setText(tr("Text source"));
    else if(type == AgentSource)
        m_LabelType.setText(tr("Agent source"));
    else if(type == Live)
        m_LabelType.setText(tr("Live source"));
    else if(type == MonitorCapture)
        m_LabelType.setText(tr("MonitorCapture source"));
    else if(type == WindowCapture)
        m_LabelType.setText(tr("WindowCapture source"));
    else if(type == ProcTopWindow)
        m_LabelType.setText(tr("ProcTopWindow source"));
    m_Index = Index;
    m_CheckBox.setChecked(true);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(50);
    m_LabelName.setFixedWidth(140);
    m_LabelType.setFixedWidth(110);
    m_BtnOperator.setFixedSize(30,30);
    m_BtnOperator.setText("");
    m_BtnOperator.setStyleSheet("QPushButton{border-image:url(:images/scene_edit.png);}"
                         "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                         "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_CheckBox);
    hBoxLayout->addWidget(&m_LabelName);
    hBoxLayout->addWidget(&m_LabelType);
    hBoxLayout->addWidget(&m_BtnOperator);
    setLayout(hBoxLayout);
    QObject::connect(&m_CheckBox,SIGNAL(stateChanged(int)),this,SLOT(OnCheckBoxClicked(int)));
    QObject::connect(&m_BtnOperator,SIGNAL(clicked()),this,SLOT(OnEditClicked()));
    QObject::connect(this,SIGNAL(EditClicked()),parent,SLOT(OnSceneMaterialEditClicked()));

    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(35,35,37));
    setPalette(palette);
    m_CheckBox.setStyleSheet("QCheckBox{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(35,35,37,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(35,35,37,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");



}


void SceneMaterialItem::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
        SetItemInfo(m_Index);
}


bool SceneMaterialItem::event(QEvent *event)
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

void SceneMaterialItem::OnCheckBoxClicked(int state)
{
    int iCount = m_pParent->m_SceneList[m_pParent->GetSceneCurrentRow()]->m_LinkMateralVec.count();
    int index = iCount - m_Index -1;
    int iCurScene = m_pParent->GetSceneCurrentRow();
//    SetItemInfo();
    m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bRender = state;
    bool bIsAgent = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bIsAgent;
    bool bRender = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index].bRender;
    LinkSourceInfo sourceInfo = m_pParent->m_SceneList[iCurScene]->m_LinkMateralVec[index];
    int iSourceId = sourceInfo.id;

    if(bIsAgent && (m_pParent->m_bIsPreview || m_pParent->m_bIsLiving))
    {
        if(SLiveSetRenderStream(m_pParent->Instance,m_pParent->m_AgentSourceList[sourceInfo.id]->streamID1,bRender) < 0)
        {

        }
    }
    else if(!bIsAgent && (m_pParent->m_bIsPreview || m_pParent->m_bIsLiving))
    {
        if(SLiveSetRenderStream(m_pParent->Instance,m_pParent->m_MateralList[iSourceId]->streamID1,bRender) < 0)
        {

        }
    }
}

void SceneMaterialItem::OnEditClicked()
{
    SetItemInfo(m_Index);
    emit EditClicked();
}


//设置项目信息：包括当前选中项目索引、所有项目的颜色
void SceneMaterialItem::SetItemInfo(int index)
{
    if(m_pParent->m_SceneMaterialItemIsSelectedList.count() < 0)
        return;
    for(int iItemCount = 0; iItemCount < m_pParent->m_SceneMaterialItemIsSelectedList.count(); iItemCount++)
    {
        m_pParent->m_SceneMaterialItemIsSelectedList.replace(iItemCount,false);
        m_pParent->SetSceneMaterialItemColor(iItemCount,false);
    }
    m_pParent->m_SceneMaterialItemIsSelectedList.replace(index,true);
    m_pParent->SetSceneMaterialCurrentRow(index);
    setAutoFillBackground(true);
    QPalette palette;
    palette.setColor(QPalette::Base, QColor(47,160,190));
    setPalette(palette);
    m_CheckBox.setStyleSheet("QCheckBox{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_LabelType.setStyleSheet("QLabel{color:rgba(255,255,255,255);background-color:rgba(47,160,190,255);font: 10pt;}");
    m_BtnOperator.setStyleSheet("QPushButton{background-color:rgba(47,160,190,255);}"
                                "QPushButton{border-image:url(:images/scene_edit.png);}"
                                "QPushButton:hover{border-image:url(:images/scene_edit_hover.png);}"
                                "QPushButton:pressed{border-image:url(:images/scene_edit_selected.png);}");
}
