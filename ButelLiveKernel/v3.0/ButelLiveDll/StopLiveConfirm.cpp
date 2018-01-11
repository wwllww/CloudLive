#include "StopLiveConfirm.h"
#include <QDialog>
#include "CHttpInterfaceSDK.h"
StopLiveConfirmUI::StopLiveConfirmUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::StopLiveConfirmUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("ExitInformation"));
}

StopLiveConfirmUI::~StopLiveConfirmUI()
{
    delete ui;
}

void StopLiveConfirmUI::on_m_pBtnOk_clicked()
{
 //   done(ON_BUTTON_OK);
//    CNetInerface* netInterface = CNetInerface::GetInstance();
    if(ui->m_pRdBtnTemp->isChecked())
    {
        if(SLiveStopLive(m_pParent->Instance_2) < 0)
        {
            return;
        }

//        netInterface->PauseLive(netInterface->m_StrToken,m_pParent->m_pChannelInfo->id.toInt());
        int ret = BLiveAppAfterStopPush(OperPause);
    }
    //结束直播
    else if(ui->m_pRdBtnClose->isChecked())
    {
        if(SLiveStopLive(m_pParent->Instance_2) < 0)
        {
            return;
        }
        m_pParent->m_bChannelClosed = true;
        m_pParent->SetStartLiveEnable(false);
//        netInterface->CloseLive(netInterface->m_StrToken,m_pParent->m_pChannelInfo->id.toInt());
        int ret = BLiveAppAfterStopPush(OperClose);
    }
    done(ON_BUTTON_OK);
}

void StopLiveConfirmUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
//   close();
}

