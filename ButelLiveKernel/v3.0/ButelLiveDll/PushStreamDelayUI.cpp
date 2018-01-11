#include "PushStreamDelayUI.h"
#include <QDialog>

PushStreamDelayUI::PushStreamDelayUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::PushStreamDelayUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("ExitInformation"));
}

PushStreamDelayUI::~PushStreamDelayUI()
{
    delete ui;
}


void PushStreamDelayUI::on_m_pBtnExit_clicked()
{
//    done(ON_BUTTON_EXIT);
    SLiveCancelDelayPush();
    this->close();
    Sleep(100);
    if(m_pParent->m_bIsDelayLive)
        m_pParent->close();
}

void PushStreamDelayUI::SetInfo(const QString& info)
{
    ui->m_pLabelShowInfo->setText(info);
}
