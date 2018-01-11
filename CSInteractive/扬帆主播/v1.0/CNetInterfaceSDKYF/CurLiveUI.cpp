#include "CurLiveUI.h"
#include <QDialog>
#include "CreateLiveUI.h"
#include "CNetInterfaceSDK.h"
CurLiveUI::CurLiveUI(QDialog *parent,QString title,QString info):
    QDialog(parent),
    ui(new Ui::CurLiveUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    ui->m_pLabelShowInfo->setText(info);
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(title);
}

CurLiveUI::~CurLiveUI()
{
    delete ui;
}

void CurLiveUI::on_m_pBtnContinue_clicked()
{
    done(10);
}

void CurLiveUI::on_m_pBtnCreate_clicked()
{
    done(20);
}
