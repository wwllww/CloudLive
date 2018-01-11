#include "Login.h"
#include <QDialog>
#include <QLabel>
#include "ConfigOper.h"
//#include "CNetInerface.h"
#include "MyMessageBox.h"
#include <QByteArray>
#include <QDebug>
#include <QSettings>
#include <QKeyEvent>
#include "ChannelAndProgram.h"
#include "CHttpInterfaceSDK.h"
#include "LogDeliver.h"

ChannelAndProgramUI* g_pChannelUI = NULL;

ChannelInfo*   g_pChannelList = NULL;
int            g_channelCount = 0;


#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif


enum {ON_BUTTON_LOGIN = 1,ON_BUTTON_FORGET_PASSWORD = 2};

LoginUI::LoginUI(QWidget *parent):QDialog(parent),
    ui(new Ui::LoginUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | windowFlags());

    char* ch = (char*)"-dGMfyds";
    Log::open(true,ch,true);

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

    QPixmap logoPixmap("img/logo.png");
    QPixmap logoFitPixmap = logoPixmap.scaled(ui->m_pLblLogo->width(), ui->m_pLblLogo->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->m_pLblLogo->setPixmap(logoFitPixmap);

    QPalette pal = this->palette();
    pal.setBrush(QPalette::Window,QPixmap("img/login_bg.png"));
    this->setPalette(pal);


    if(!ConfigOper::instance()->ReadCfgFile())
    {
        MyMessageBox message(this, tr("error"),tr("Error reading configuration file BLive.ini!"),BUTTON_YES);
        message.exec();
    }
    ui->m_pLineEditUserName->setText(ConfigOper::instance()->m_UseName);
    ui->m_pBtnLogin->setCursor(Qt::PointingHandCursor);

    int ret = BLiveAppInit(ConfigOper::instance()->m_StrUrl.toLocal8Bit().data(),NULL,NULL);
}

LoginUI::~LoginUI()
{
    delete ui;
}

void LoginUI::on_m_pBtnLogin_clicked()
{
    ui->m_pLblLoginInfo->clear();
    ui->m_pBtnLogin->setEnabled(false);

    QString userName = ui->m_pLineEditUserName->text();
    QString passWord = ui->m_pLineEditPassWord->text();
    int ret = BLiveAppLogin(userName.toLocal8Bit().data(),passWord.toLocal8Bit().data(),0);
    if(0 == ret )
    {
        ret = BLiveAppAuthorize();
        if(0 == ret)
        {
            LoginStateInfo(ret,"");
            ChannelAndProgramUI *g_pChannelUI = new ChannelAndProgramUI;
            g_pChannelUI->show();
            g_pChannelUI->setFocus();
            this->close();
        }
        else
        {
            ui->m_pLblLoginInfo->setText(tr("Authorization failure!"));
        }
    }
    else if(-2 == ret)
    {
        ui->m_pBtnLogin->setEnabled(true);
        ui->m_pLblLoginInfo->setText(tr("Invalid username or password"));
    }
    else
    {
        ui->m_pBtnLogin->setEnabled(true);
        ui->m_pLblLoginInfo->setText(tr("Could not connect to the server"));
    }
}

void LoginUI::on_m_pBtnUserNameDel_clicked()
{
    ui->m_pLineEditUserName->clear();
}

void LoginUI::LoginStateInfo(int state,const QString& info)
{
    if(state == 0)
    {
        //如果登录成功，将当前账号记住，写入配置文件BLive.ini
        QString userName = ui->m_pLineEditUserName->text();
        ConfigOper::instance()->m_UseName = userName;
        ConfigOper::instance()->WriteCfgFile();

    }
    else
    {
       ui->m_pLblLoginInfo->setText(info);
    }
}

void LoginUI::AuthorizeStateInfo(int state)
{
    if(state == 0)
    {
//        if(!g_pChannelUI)
//            g_pChannelUI = new ChannelAndProgramUI;
//        close();
//        CNetInerface::GetInstance()->m_pChannelAndProgramUI = g_pChannelUI;
//        g_pChannelUI->show();
//        g_pChannelUI->setFocus();
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
