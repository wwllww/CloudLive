#include "ChannelItem.h"
#include "CHttpInterfaceSDK.h"
#include "ButelLiveDll.h"
#include <QHBoxLayout>
#include <QDebug>

extern ChannelInfo*   g_pChannelList;
extern int           g_channelCount;

CChannelItem::CChannelItem(QWidget *parent,QString name,int index):
    QWidget(parent)
{
    m_index = index;
    m_LabelDefault.setFixedSize(20,20);
    m_LabelName.setText(name);
    m_LabelName.setStyleSheet("QLabel{color:rgba(255,255,255,255);font: 10pt;}");
    setFixedHeight(50);
    m_RdBtnSel.setFixedSize(30,30);
    m_RdBtnSel.setText("");

    QHBoxLayout *hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(&m_LabelDefault);
    hBoxLayout->addSpacing(30);
    hBoxLayout->addWidget(&m_LabelName);
    hBoxLayout->addSpacing(30);
    hBoxLayout->addWidget(&m_RdBtnSel);
    setLayout(hBoxLayout);
    QObject::connect(&m_RdBtnSel,SIGNAL(clicked(bool)),this,SLOT(OnRdBtnSelClicked()));
    QObject::connect(this,SIGNAL(EnterButelLive()),parent,SLOT(close()));
}

void CChannelItem::OnRdBtnSelClicked()
{
    if(m_index < 0) return;
    ChannelInfo* pCurChannelInfo = g_pChannelList + m_index;
    BLiveAppSetChannel(pCurChannelInfo);
    qDebug() << pCurChannelInfo->nChannelId;
    qDebug() << pCurChannelInfo->pChannelName;

    int ret = CreatButelLiveInstance(true,g_pChannelList,g_channelCount,m_index);
    if(0 == ret)
    {
        ChannelInfo* pCurChannelInfo = g_pChannelList + m_index;
        BLiveAppSetChannel(pCurChannelInfo);
        qDebug() << pCurChannelInfo->nChannelId;
        qDebug() << pCurChannelInfo->pChannelName;

        emit EnterButelLive();
    }
}

void CChannelItem::SetDefault()
{
    QPixmap pixmap("img/DefaultChannel.png");
    QPixmap fitpixmap = pixmap.scaled(20, 20, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_LabelDefault.setPixmap(fitpixmap);
}
