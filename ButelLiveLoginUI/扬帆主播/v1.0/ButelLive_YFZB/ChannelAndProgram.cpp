#include "ChannelAndProgram.h"
#include <QDialog>
#include <QLabel>
#include <QScrollBar>
#include <QKeyEvent>
#include <QFuture>
#include <QtConcurrent>
#include <QListWidgetItem>
#include "ConfigOper.h"
#include "CHttpInterfaceSDK.h"
#include "Worker.h"
#include "ButelLiveDll.h"
#include "CHttpInterfaceSDK.h"
#include "ChannelItem.h"
bool bAuthorize = false;   //判断授权是否成功
extern ChannelAndProgramUI* g_pChannelUI;
extern ChannelInfo* g_pChannelList;
extern int          g_channelCount;

enum {ON_BUTTON_ENTER = 1,ON_BUTTON_FORGET_PASSWORD = 2};

DWORD WINAPI GetChannelListThreadProc(LPVOID lpParam)
{
    BLiveAppGetChannelList(&g_pChannelList,&g_channelCount);
    emit g_pChannelUI->GetChannelListFinished();
    return 0;
}

ChannelAndProgramUI::ChannelAndProgramUI(QWidget *parent):QDialog(parent),
    ui(new Ui::ChannelAndProgramUI)
{
    ui->setupUi(this);
    g_pChannelUI = this;
    setAttribute (Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | windowFlags());
    m_pLoginUi = NULL;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setFixedSize(40,40);
    ui->m_pTitleLbl->m_pMinimizeButton->setFixedSize(40,40);
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pMinimizeButton->setStyleSheet("QPushButton{border-image:url(:images/nav_mini.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_mini_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("SelectChannel"));

    ui->m_pLWChannel->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->m_pLWChannel->setFocusPolicy(Qt::NoFocus);

    QThread * thread = new QThread();
    Worker* worker = new Worker;
    worker->moveToThread(thread);

    connect(thread, SIGNAL(started()), worker, SLOT(process()));
    connect(worker, SIGNAL(finished()), thread, SLOT(quit()));
    connect(worker, SIGNAL(finished()), this,SLOT(OnGetChannelListFinished()));
    connect(worker, SIGNAL(finished()), worker, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();


}

ChannelAndProgramUI::~ChannelAndProgramUI()
{
    delete ui;
}
void ChannelAndProgramUI::OnGetChannelListFinished()
{
    if(g_channelCount <= 0)
    {
        SetInformation(tr("The channel list is empty!"));
    }
    else
    {
        ChannelInfo* pTmpChannelList = g_pChannelList;
        for(int i = 0; i < g_channelCount; i++,pTmpChannelList++)
        {
//            g_pChannelUI->AddChannelItem(pTmpChannelList->pChannelName);
            CChannelItem *pChannelItem = new CChannelItem(this,pTmpChannelList->pChannelName,i);
            QListWidgetItem* pLWItem = new QListWidgetItem;
            pLWItem->setSizeHint(QSize(0,50));
            ui->m_pLWChannel->addItem(pLWItem);
            ui->m_pLWChannel->setItemWidget(pLWItem,pChannelItem);
            if(2 == pTmpChannelList->nDefaultAnchorFlag)
                pChannelItem->SetDefault();
        }
    }
}

void ChannelAndProgramUI::AddChannelItem(QString name)
{
   // ui->m_pCombChannel->addItem(name);

//    CChannelItem *pChannelItem = new CChannelItem(this,qstrSceneName,iSceneIndex);
//    m_pLWSceneItem = new QListWidgetItem;
//    m_pLWSceneItem->setSizeHint(QSize(0,50));
//    ui->m_pLWScene->addItem(m_pLWSceneItem);
//    ui->m_pLWScene->setItemWidget(m_pLWSceneItem,SceneItem);

}

void ChannelAndProgramUI::ClearAllChannelItem()
{
//    ui->m_pCombChannel->clear();
}

void ChannelAndProgramUI::SetInformation(QString infor)
{
//    ui->m_pLblReturnInfo->setText(infor);
}

//加载直播主界面
void ChannelAndProgramUI::on_m_pBtnEnter_clicked()
{
//    if(ui->m_pCombChannel->currentIndex() < 0) return;
//    ui->m_pBtnEnter->setEnabled(false);

//    int iCurChannelIndex = ui->m_pCombChannel->currentIndex();
//    int ret = CreatButelLiveInstance(g_pChannelList,g_channelCount,iCurChannelIndex);
//    if(0 == ret)
//    {
//        ChannelInfo* pCurChannelInfo = g_pChannelList + iCurChannelIndex;
//        BLiveAppSetChannel(pCurChannelInfo);
//        this->close();
//    }
//    else
//    {
//        ui->m_pBtnEnter->setEnabled(true);
//    }

}

void ChannelAndProgramUI::on_m_pBtnBack_clicked()
{
    if(!m_pLoginUi)
        m_pLoginUi = new LoginUI;
    m_pLoginUi->show();
    g_pChannelUI->close();
    delete g_pChannelUI;
    g_pChannelUI = NULL;
}

void ChannelAndProgramUI::on_m_pCombChannel_currentIndexChanged(int index)
{
//    CNetInerface* netInterface = CNetInerface::GetInstance();
//    netInterface->m_iCurrentChannelIndex = index;
}

void ChannelAndProgramUI::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
    {
        on_m_pBtnEnter_clicked();
    }
    else
    {
        return QDialog::keyPressEvent(event);
    }
}
