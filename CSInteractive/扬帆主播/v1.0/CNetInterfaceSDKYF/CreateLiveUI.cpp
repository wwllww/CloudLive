#include "CreateLiveUI.h"
#include <QDialog>
#include <QFile>
#include <QPainter>
#include <QPixmap>
#include <QKeyEvent>
#include <QFileDialog>
#include "CNetInterfaceSDK.h"
#include "LogDeliver.h"
#include "RedCDNUploadSDKPub.h"
#include "MyMessageBox.h"

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif

CreateLiveUI* g_This;

void OnFileUpload(void* pHandle, int nRet, bool bUploadFinished, int pUploadRate, char* pCreator, char* pUploadSpeed, char* pFileDownloadUrl, char* pHost, char* pRange, char* pCID, bool bAbortUpload, long long ctx)
{
    if (!nRet)
    {
        QString downloadUrl(pFileDownloadUrl);
        emit g_This->ProgressChanged(pUploadRate,downloadUrl);
    }
    else
    {
        emit g_This->UploadFailed();
        //g_This->ReuploadFile(pCreator, pHost, pRange, pCID);
        qDebug() << "FileUpload faild";
    }
}

void On_UploadLogin(void* pHandle, int nRet, char* pToken, long long ctx)
{
    Log::writeMessage(LOG_RTSPSERV,1,"%s pToken = %s",__FUNCTION__, pToken);
    if (!nRet)
    {
        qDebug() << "On_UploadLogin pToken = " << pToken;
        g_This->SetUploadGetAddr(pToken);
    }
    else
    {
        qDebug() << "UploadLogin faild";
    }
}

void On_UploadGetAddr(void* pHandle, int nRet, char* pFileUploadUrl, char* pFileProcUrl, long long ctx)
{
    if (!nRet)
    {
        g_This->StartFileUpload(pFileUploadUrl, pFileProcUrl);
    }
    else
    {
        qDebug() << "UploadGetAddr faild";
    }
}

CreateLiveUI::CreateLiveUI(QDialog *parent,QString title,QString channelName,QString programName):
    QDialog(parent),
    ui(new Ui::CreateLiveUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);

    g_This = this;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(title);
    ui->m_pLbCurChannel->setText(channelName);
    if(!programName.isEmpty())
        ui->m_pLdtProgramName->setText(programName);

    ui->m_pProgressBar->hide();

    QPixmap pixmap("img/image_default.png");
    QPixmap fitpixmap = pixmap.scaled(ui->m_pLbPicture->width(), ui->m_pLbPicture->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->m_pLbPicture->setPixmap(fitpixmap);

    connect((QObject*)this,SIGNAL(UploadOK()),this,SLOT(OnUploadOK()));
    connect((QObject*)this,SIGNAL(ProgressChanged(int,QString)),this,SLOT(OnProgressChanged(int,QString)));
    connect((QObject*)this,SIGNAL(UploadFailed()),this,SLOT(OnUploadFailed()));
}

CreateLiveUI::~CreateLiveUI()
{
    delete ui;
}


void CreateLiveUI::on_m_pBtnUpload_clicked()
{
    QString strName =  QFileDialog::getOpenFileName(this,tr("Select file"),"",tr("*.png *.xpm *.jpg *.bmp"));
    if(strName.isEmpty())
        return;
    else
        m_FileName = strName;
    Log::writeMessage(LOG_RTSPSERV,1,"%s m_FileName = %s",__FUNCTION__, m_FileName.toLocal8Bit().data());
//    m_bJuzhaoUpload = false;
    if(!m_FileName.isEmpty()) {
       //上传剧照
        ui->m_pProgressBar->setValue(0);
//        ui->m_pLbProgress->setText(QString("0%"));
//        ui->m_pLbInfo->setText(QStringLiteral("剧照上传中，请稍后"));
        DrawImage();
        InitFileUpload();
//        ui->m_pLbJuzhao->setStyleSheet("QLabel{border-color: rgba(0, 0, 0, 125);}");
//        ui->m_pLbInfo->show();
//        ui->m_pLbProgress->show();
        ui->m_pProgressBar->show();
//        ui->m_pLbDefault->hide();
        ui->m_pBtnUpload->setEnabled(false);
        //m_bJuzhaoUpload = false;
        ui->m_pBtnGoLive->setEnabled(false);
    }
    else
        return;
}

void CreateLiveUI::on_m_pBtnGoLive_clicked()
{
    QString strLiveName = ui->m_pLdtProgramName->text();
    if(strLiveName.isEmpty())
    {
        MyMessageBox *pMsgBox = new MyMessageBox(this,tr("Error"),tr("Live name cannot be empty"),BUTTON_YES);
        pMsgBox->show();
        return;
    }
    CNetInterfaceSDK::GetInstance()->m_ProgramInfo.name = ui->m_pLdtProgramName->text();
    done(10);
}


void CreateLiveUI::InitFileUpload()
{
    Handle = CreatUpload();
    if (Handle)
    {
        Log::writeMessage(LOG_RTSPSERV,1,"%s Url = %s, AppId = %d",__FUNCTION__,CNetInterfaceSDK::GetInstance()->m_strUploadUrl.toStdString().c_str(),CNetInterfaceSDK::GetInstance()->m_strAppId.toStdString().c_str());
        UploadLogin(Handle, (char*)CNetInterfaceSDK::GetInstance()->m_strUploadUrl.toStdString().c_str(), (char*)CNetInterfaceSDK::GetInstance()->m_strAppId.toStdString().c_str(), (long long)this, &On_UploadLogin);
    }
    else
    {
        Log::writeMessage(LOG_RTSPSERV,1,"%s GreatUpload faild",__FUNCTION__);
        qDebug() << "GreatUpload faild";
    }
}

void CreateLiveUI::SetUploadGetAddr(char* token)
{
    Token = token;
    UploadGetAddr(Handle, (char*)CNetInterfaceSDK::GetInstance()->m_strUploadUrl.toStdString().c_str(), Token, (long long)this, &On_UploadGetAddr);
}

void CreateLiveUI::StartFileUpload(char* pFileUploadUrl, char* pFileProcUrl)
{
    QFile file(m_FileName);
    if (!file.open(QIODevice::ReadOnly)) return;
    qint64 size = file.size();
    file.close();
    long long chunk = size > 1024 * 1024 * 5 ? 1024 * 1024 : 100*1024;
    SetUploadFileLength(Handle, chunk);
    FileUpload(Handle, pFileUploadUrl, Token, (char*)"FileUpload", (char*)m_FileName.toLocal8Bit().data(), 3, (char*)"NULL", (char*)"NULL", (long long)this, &OnFileUpload);
}

void CreateLiveUI::ReuploadFile(char* pCreator, char* pHost, char* pRange, char* pCID)
{
    FileUploadAgain(Handle, pHost, pRange, pCID, Token, pCreator, (char*)m_FileName.toLocal8Bit().data(), 3,(char*)"NULL", (char*)"NULL", (long long)this, &OnFileUpload);
}

void CreateLiveUI::DrawImage()
{
    Log::writeMessage(LOG_RTSPSERV,1,"%s invoke begin!",__FUNCTION__);
    QPainter painter;
    QPixmap bitmap;
//    UINT w,h;
//    char* imageFormat;
//    if(SLiveGetImgInfo(m_FileName.toLocal8Bit().data(),&w,&h,&imageFormat) < 0)
//    {
//        Log::writeMessage(LOG_RTSPSERV,1,"%s SLiveGetImgInfo failed!",__FUNCTION__);
//        return;
//    }
//    if(!strcmp(imageFormat,"unkown"))
//    {
//        SLiveFreeMemory(imageFormat);
//        Log::writeMessage(LOG_RTSPSERV,1,"%s imageFormat = unkown",__FUNCTION__);
//        return;
//    }
    bool bSuc = bitmap.load(m_FileName/*,imageFormat*/);
//    SLiveFreeMemory(imageFormat);
    if(!bSuc)
    {
        Log::writeMessage(LOG_RTSPSERV,1,"%s invoke end! bSuc = false",__FUNCTION__);
        return;
    }
    bool  bLoad = painter.begin(&bitmap);
    if(bLoad)
    {
        ui->m_pLbPicture->setScaledContents(true);
//        painter.drawPixmap(0,0,ui->m_pLbPicture->width(), ui->m_pLbPicture->height(), bitmap);
        ui->m_pLbPicture->setPixmap(bitmap);
        painter.end();
    }

    Log::writeMessage(LOG_RTSPSERV,1,"%s invoke end! bLoad = %s",__FUNCTION__,bLoad ? "ture" : "false");

}


void CreateLiveUI::OnProgressChanged(int iVal,QString juzhaoDownLoadUrl)
{

//    ui->m_pLbProgress->setText(QString::number(iVal)+'%');
    ui->m_pProgressBar->setValue(iVal);
    if(iVal >= 100)
    {
//        m_bJuzhaoUpload = true;
        ui->m_pProgressBar->hide();
        ui->m_pBtnGoLive->setEnabled(true);
//        ui->m_pLbInfo->setText(QStringLiteral("剧照上传成功"));
        ui->m_pBtnUpload->setEnabled(true);
        CNetInterfaceSDK::GetInstance()->m_ProgramInfo.stills = m_JuzhaoDownLoadUrl = juzhaoDownLoadUrl;
//        Log::writeMessage(LOG_RTSPSERV,1,"%s m_JuzhaoDownLoadUrl = %s",__FUNCTION__,m_JuzhaoDownLoadUrl.toStdString().c_str());
    }
}

void CreateLiveUI::OnUploadFailed()
{
//    ui->m_pLbInfo->setText(QStringLiteral("剧照上传失败，请重试"));
    ui->m_pProgressBar->hide();
//    ui->m_pLbPicture->setStyleSheet("QLabel{border-image:url(:images/image_error.png);}");

//    QPainter painter;
//    QPixmap bitmap;
//    bool bSuc = bitmap.load("img/image_error.png"/*,imageFormat*/);
////    SLiveFreeMemory(imageFormat);
//    if(!bSuc)
//    {
//        Log::writeMessage(LOG_RTSPSERV,1,"%s invoke end! bSuc = false",__FUNCTION__);
//        return;
//    }
//    bool  bLoad = painter.begin(&bitmap);
//    if(bLoad)
//    {
//        ui->m_pLbPicture->setScaledContents(true);
////        painter.drawPixmap(0,0,ui->m_pLbPicture->width(), ui->m_pLbPicture->height(), bitmap);
//        ui->m_pLbPicture->setPixmap(bitmap);
//        painter.end();
//    }

    QPixmap pixmap("img/image_error.png");
    QPixmap fitpixmap = pixmap.scaled(ui->m_pLbPicture->width(), ui->m_pLbPicture->height(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ui->m_pLbPicture->setPixmap(fitpixmap);

    ui->m_pBtnGoLive->setEnabled(true);
    ui->m_pBtnUpload->setEnabled(true);
}

void CreateLiveUI::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if(keyEvent->key() == Qt::Key_Escape)
        {
            event->ignore();
        }
    }
}
