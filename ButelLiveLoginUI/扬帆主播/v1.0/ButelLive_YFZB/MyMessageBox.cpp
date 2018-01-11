#include "MyMessageBox.h"
#include <QDialog>

MyMessageBox::MyMessageBox(QDialog *parent,QString title,QString info,int buttons):
    QDialog(parent),
    ui(new Ui::MyMessageBoxUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
//    m_pParent = (ButelLive*)parent;
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
    if(buttons == BUTTON_YES)
    {
        ui->m_pBtnCancel->hide();
    }
    else if(buttons == BUTTON_NO)
    {
        ui->m_pBtnOk->hide();
    }
}

MyMessageBox::~MyMessageBox()
{
    delete ui;
}

void MyMessageBox::on_m_pBtnOk_clicked()
{
    done(ON_BUTTON_OK);
}

void MyMessageBox::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}


void MyMessageBox::SetInfo(const QString& info)
{
    ui->m_pLabelShowInfo->setText(info);
}
