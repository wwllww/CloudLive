#include "ChannelAndProgram.h"
#include <QDialog>
#include <QLabel>
#include <QScrollBar>
#include "ConfigOper.h"
#include "CNetInerface.h"
bool bAuthorize = false;   //判断授权是否成功
extern ChannelAndProgramUI* g_pChannelUI;
enum {ON_BUTTON_ENTER = 1,ON_BUTTON_FORGET_PASSWORD = 2};
ChannelAndProgramUI::ChannelAndProgramUI(QWidget *parent):QDialog(parent),
    ui(new Ui::ChannelAndProgramUI)
{
    ui->setupUi(this);
    setAttribute (Qt::WA_DeleteOnClose);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
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

    CNetInerface* netInterface = CNetInerface::GetInstance();
    netInterface->GetChannelList(netInterface->m_StrToken,netInterface->m_LoginInfo.nubernumber,1,10);
    netInterface->m_pChannelAndProgramUI = this;
}

ChannelAndProgramUI::~ChannelAndProgramUI()
{
    delete ui;
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
    CNetInerface* netInterface = CNetInerface::GetInstance();
    tagChannelInfo* channelInfo = netInterface->m_channelList.at(ui->m_pCombChannel->currentIndex());
    this->hide();
    ButelLive* ButelLiveUi = new ButelLive(channelInfo);
    ButelLiveUi->showMaximized();
    ButelLiveUi->SLiveApiInit();
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
    CNetInerface* netInterface = CNetInerface::GetInstance();
    netInterface->m_iCurrentChannelIndex = index;
}

void ChannelAndProgramUI::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Escape)
    {
        event->ignore();
    }
    else
    {
        return QDialog::keyPressEvent(event);
    }
}
