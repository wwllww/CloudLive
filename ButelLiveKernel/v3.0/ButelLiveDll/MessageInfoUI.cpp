#include "MessageInfoUI.h"
#include <QDialog>

MessageInfoUI::MessageInfoUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::MessageInfoUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
//    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pBtnExit->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("Information"));
}

MessageInfoUI::~MessageInfoUI()
{
    delete ui;
}


void MessageInfoUI::on_m_pBtnExit_clicked()
{
    this->close();
}

void MessageInfoUI::SetInfo(const QString& info)
{
    ui->m_pLabelShowInfo->setText(info);
}

void MessageInfoUI::SetOKBtnShow(bool bShow)
{
    if(bShow)
        ui->m_pBtnExit->show();
    else
        ui->m_pBtnExit->hide();
}
