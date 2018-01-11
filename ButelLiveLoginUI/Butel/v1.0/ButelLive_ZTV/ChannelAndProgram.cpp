#include "ChannelAndProgram.h"
#include <QDialog>
#include <QLabel>
#include <QScrollBar>
#include <QKeyEvent>
#include <QFuture>
#include <QtConcurrent>
#include "ConfigOper.h"
#include "CHttpInterfaceSDK.h"
#include "Worker.h"
#include "ButelLiveDll.h"
#include "CHttpInterfaceSDK.h"
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
    ui->m_pBtnEnter->setCursor(Qt::PointingHandCursor);
    ui->m_pCombChannel->view()->verticalScrollBar()->setStyleSheet("QScrollBar"
                                                                   "{"
                                                                     "width:8px;"
                                                                   "}"
                                                                   "QScrollBar::handle:vertical"
                                                                   "{"
                                                                     "width:8px;"
                                                                     "background:rgb(79,79,83);"
                                                                     "border-radius:3px;"
                                                                   "}"
                                                                   "QScrollBar::sub-page::vertical"
                                                                   "{"
                                                                     "width:8px;"
                                                                     "background:#232325;"
                                                                   "}"
                                                                   "QScrollBar::add-page::vertical"
                                                                   "{"
                                                                     "width:8px;"
                                                                     "background:#232325;"
                                                                   "}"
                                                                   "QScrollBar::sub-line::vertical"
                                                                   "{"
                                                                     "width:8px;"
                                                                     "height:8px;"
                                                                     "background:rgb(79,79,83);"
                                                                   "}");
    ui->m_pCombChannel->setStyleSheet("QComboBox::drop-down"
                                        "{"
                                        "border-image: url(:images/login_comb_down_bg.png);"
                                       "color:rgb(0,0,0);"
                                       "width:50px;"
                                       "}"
                                       "QComboBox"
                                       "{"
                                       "background-color: rgba(30, 30, 32,0);"
                                       "border-image: url(:/images/comb_bg.png);"
                                       "color: rgb(255, 255, 255);"
                                       "font: 75 14pt;"
                                       "}"
                                      );

    QPixmap pixmap("img/logo.png");
    QPixmap fitpixmap = pixmap.scaled(ui->m_pLblLogo->width(), ui->m_pLblLogo->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->m_pLblLogo->setPixmap(fitpixmap);

    QPalette pal = this->palette();
    pal.setBrush(QPalette::Window,QPixmap("img/login_bg.png"));
    this->setPalette(pal);


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
            g_pChannelUI->AddChannelItem(pTmpChannelList->pChannelName);
        }
        SetInformation("");
    }
}

void ChannelAndProgramUI::AddChannelItem(QString name)
{
    ui->m_pCombChannel->addItem(name);
}

void ChannelAndProgramUI::ClearAllChannelItem()
{
    ui->m_pCombChannel->clear();
}

void ChannelAndProgramUI::SetInformation(QString infor)
{
    ui->m_pLblReturnInfo->setText(infor);
}

//加载直播主界面
void ChannelAndProgramUI::on_m_pBtnEnter_clicked()
{
    if(ui->m_pCombChannel->currentIndex() < 0) return;
    ui->m_pBtnEnter->setEnabled(false);

    int iCurChannelIndex = ui->m_pCombChannel->currentIndex();
    int ret = CreatButelLiveInstance(false,g_pChannelList,g_channelCount,iCurChannelIndex);
    if(0 == ret)
    {
        ChannelInfo* pCurChannelInfo = g_pChannelList + iCurChannelIndex;
        BLiveAppSetChannel(pCurChannelInfo);
        this->close();
    }
    else
    {
        ui->m_pBtnEnter->setEnabled(true);
    }

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
