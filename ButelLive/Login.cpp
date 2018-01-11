#include "Login.h"
#include <QDialog>
#include <QLabel>
#include "ConfigOper.h"
#include "CNetInerface.h"
#include "MyMessageBox.h"
#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include "ChannelAndProgram.h"

ChannelAndProgramUI* g_pChannelUI = NULL;

enum {ON_BUTTON_LOGIN = 1,ON_BUTTON_FORGET_PASSWORD = 2};

LoginUI::LoginUI(QWidget *parent):QDialog(parent),
    ui(new Ui::LoginUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
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
    if(!ConfigOper::instance()->ReadCfgFile())
    {
        MyMessageBox message(this, tr("error"),tr("Error reading configuration file BLive.ini!"),BUTTON_YES);
        message.exec();
    }
    ui->m_pLineEditUserName->setText(ConfigOper::instance()->m_UseName);
    ui->m_pBtnLogin->setCursor(Qt::PointingHandCursor);

//    ui->m_pCheckPasswd->setStyleSheet("QCheckBox::indicator:unchecked"
//                                      "{"
//                                      "border-image: url(:/images/check_bg.png);"
//                                      "color:rgb(0,0,0);"
//                                      "width:20px;"
//                                      "height:20px;"
//                                      "}"
//                                      "QCheckBox::indicator:checked"
//                                      "{"
//                                      "border-image: url(:/images/checked_bg.png);"
//                                      "color:rgb(0,0,0);"
//                                      "width:20px;"
//                                      "height:20px;"
//                                      "}"
//                                      "}"
//                                      "QCheckBox"
//                                      "{"
//                                      "border-image: url(:/images/xxx.png);"
//                                      "font: 75 12pt;"
//                                      "background-color: rgba(255, 255, 255,0);"
//                                      "color: #5f5f5f;"
//                                      "}"
//                                      );
}

LoginUI::~LoginUI()
{
    delete ui;
}

void LoginUI::on_m_pBtnLogin_clicked()
{

    QString userName = ui->m_pLineEditUserName->text();
    QString passWord = ui->m_pLineEditPassWord->text();
    CNetInerface* netInterface = CNetInerface::GetInstance();
    netInterface->m_pLoginUI = this;
    netInterface->Login(userName,passWord);
    qDebug() << userName << passWord;
}

void LoginUI::on_m_pBtnUserNameDel_clicked()
{
    ui->m_pLineEditUserName->clear();
}

void LoginUI::LoginStateInfo(int state)
{
    if(state == 0)
    {
        //如果登录成功，将当前账号记住，写入配置文件BLive.ini
        QString userName = ui->m_pLineEditUserName->text();
        ConfigOper::instance()->m_UseName = userName;
        ConfigOper::instance()->WriteCfgFile();
    }
    else if(state < 0)
    {
        ui->m_pLblLoginInfo->setText(tr("Logon failure!"));
    }
}

void LoginUI::AuthorizeStateInfo(int state)
{
    if(state == 0)
    {
        if(!g_pChannelUI)
            g_pChannelUI = new ChannelAndProgramUI;
        close();
        CNetInerface::GetInstance()->m_pChannelAndProgramUI = g_pChannelUI;
        g_pChannelUI->show();
    }
    else if(state < 0)
    {
        ui->m_pLblLoginInfo->setText(tr("Authorization failure!"));
    }
}

void LoginUI::OperAccept()
{
    accept();
}

void LoginUI::keyPressEvent(QKeyEvent *event)
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
