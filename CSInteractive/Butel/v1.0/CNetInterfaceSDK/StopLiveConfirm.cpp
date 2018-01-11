#include <QDialog>
#include "StopLiveConfirm.h"
#include "CNetInterfaceSDK.h"
StopLiveConfirmUI::StopLiveConfirmUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::StopLiveConfirmUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);

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
    //暂停直播
    if(ui->m_pRdBtnTemp->isChecked())
    {
        CNetInterfaceSDK::GetInstance()->m_StopOperType = OperPause;
    }
    //结束直播
    else if(ui->m_pRdBtnClose->isChecked())
    {
        CNetInterfaceSDK::GetInstance()->m_StopOperType = OperClose;
    }
    done(10);
}

void StopLiveConfirmUI::on_m_pBtnCancel_clicked()
{
    done(20);
}

