#include "SetLogLevelUI.h"
#include <QDialog>
#include "configoper.h"
SetLogLevelUI::SetLogLevelUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::SetLogLevelUI)
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
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("SetLogLevel"));

    int level = ConfigOper::instance()->m_LogLevel;
    if(level & 0x08)
    {
        ui->m_pChckBoxErrLog->setChecked(true);
    }
    else
    {
        ui->m_pChckBoxErrLog->setChecked(false);
    }
    if(level & 0x04)
    {
        ui->m_pChckBoxWarnLog->setChecked(true);
    }
    else
    {
        ui->m_pChckBoxWarnLog->setChecked(false);
    }
    if(level & 0x02)
    {
        ui->m_pChckBoxMsgLog->setChecked(true);
    }
    else
    {
        ui->m_pChckBoxMsgLog->setChecked(false);
    }
    if(level & 0x01)
    {
        ui->m_pChckBoxDbgLog->setChecked(true);
    }
    else
    {
        ui->m_pChckBoxDbgLog->setChecked(false);
    }
}

SetLogLevelUI::~SetLogLevelUI()
{
    delete ui;
}

void SetLogLevelUI::on_m_pBtnOk_clicked()
{
    //done(ON_BUTTON_OK);
    int level = 0;
    if(ui->m_pChckBoxErrLog->isChecked())
        level |= 0x08;
    if(ui->m_pChckBoxWarnLog->isChecked())
        level |= 0x04;
    if(ui->m_pChckBoxMsgLog->isChecked())
        level |= 0x02;
    if(ui->m_pChckBoxDbgLog->isChecked())
        level |= 0x01;
    if(SLiveSetLogLevel(level) < 0)
    {
        return;
    }
    else
    {
        ConfigOper::instance()->m_LogLevel = level;
        ConfigOper::instance()->WriteCfgFile();
    }
    done(ON_BUTTON_OK);
}

void SetLogLevelUI::on_m_pBtnCancel_clicked()
{
    done(ON_BUTTON_CANCEL);
}
