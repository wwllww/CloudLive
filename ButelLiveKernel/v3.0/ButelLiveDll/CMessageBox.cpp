#include "CMessageBox.h"
#include <QDialog>

CMessageBox::CMessageBox(QDialog *parent,QString title,QString info):
    QDialog(parent),
    ui(new Ui::CMessageBoxUI)
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

CMessageBox::~CMessageBox()
{
    delete ui;
}

void CMessageBox::SetInfo(const QString& info)
{
    ui->m_pLabelShowInfo->setText(info);
}

void CMessageBox::on_m_pBtnYes_clicked()
{
    done(10);
}

void CMessageBox::on_m_pBtnNO_clicked()
{
    done(20);
}

void CMessageBox::on_m_pBtnCancel_clicked()
{
    done(0);
}
