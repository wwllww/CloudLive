#include "SettingUI.h"
#include <QDialog>
#include <QButtonGroup>
#include <QFileDialog>
#include <QDir>
#include "configoper.h"
#include "MyMessageBox.h"
SettingUI::SettingUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::SettingUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    m_pParent = (ButelLive*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("global setting"));
    setModal(true);
    ui->m_pStckWdiget->setCurrentIndex(0);

    ui->m_pCombDirectorListenDevices->addItems(m_pParent->AudioRenderList);
    ui->m_pCombAudioOutputDevices->addItems(m_pParent->AudioRenderList);
    ui->m_pCombDisplayDevice->addItems(m_pParent->DisplayDeviceList);

    InitEncoder();
    InitLiveSetting();
    InitVideoSetting();
    InitAudioSetting();
    InitAdvanced();

    m_bEncoderChanged = false;
    m_bLiveSetttingChanged = false;
    m_bVideoSettingChanged = false;
    m_bAudioSettingChanged = false;
    m_bAdvancedChanged = false;
}

SettingUI::~SettingUI()
{
    delete ui;
}

void SettingUI::InitEncoder()
{
    QButtonGroup* pBtnGrp_Encoder = new QButtonGroup(this);
    pBtnGrp_Encoder->setExclusive(true);
    pBtnGrp_Encoder->addButton(ui->m_pRdBtnCBR);
    pBtnGrp_Encoder->addButton(ui->m_pRdBtnVBR);
    ui->m_pRdBtnH264->setChecked(true);
    ui->m_pCombQuality->setCurrentIndex(8);
    if(ConfigOper::instance()->m_bUsePadding)
    {
        ui->m_pCheckCBRPadding->setChecked(true);
    }
    else
    {
        ui->m_pCheckCBRPadding->setChecked(false);
    }
    if(ConfigOper::instance()->m_bUseCBR)
    {
        ui->m_pRdBtnCBR->setChecked(true);
        ui->m_pRdBtnVBR->setChecked(false);
        ui->m_pCombQuality->setEnabled(false);
        ui->m_pCheckCBRPadding->setEnabled(true);
    }
    else
    {
        ui->m_pRdBtnCBR->setChecked(false);
        ui->m_pRdBtnVBR->setChecked(true);
        ui->m_pCombQuality->setEnabled(true);
        ui->m_pCheckCBRPadding->setEnabled(false);
    }
    ui->m_pCombEncoder->setCurrentIndex(ConfigOper::instance()->m_AudioEncoderType);
    QString strAudioBitRate = QString::number(ConfigOper::instance()->m_AudioBitRate)+"kb/s";
    ui->m_pCombRate->setCurrentText(strAudioBitRate);
    if(44100 == ConfigOper::instance()->m_AudioSampleRate)
        ui->m_pCombSamplingRate->setCurrentIndex(0);
    ui->m_pCombSoundChannel->setCurrentIndex(ConfigOper::instance()->m_AudioChannel - 1);
}

void SettingUI::InitLiveSetting()
{
    QButtonGroup* pBtnGrp_Live = new QButtonGroup(this);
    pBtnGrp_Live->setExclusive(true);
    pBtnGrp_Live->addButton(ui->m_pRdBtnMP4);
    pBtnGrp_Live->addButton(ui->m_pRdBtnFLV);
    QButtonGroup* pBtnGrp_Sec = new QButtonGroup(this);
    pBtnGrp_Sec->setExclusive(true);
    pBtnGrp_Sec->addButton(ui->m_pRdBtnMP4Sec);
    pBtnGrp_Sec->addButton(ui->m_pRdBtnFLVSec);

//    if(1280 == ConfigOper::instance()->m_Width && 720 == ConfigOper::instance()->m_Height)
//    {
//        ui->m_pCombDefinition->setCurrentIndex(2);
//        ui->m_pCombResolution->setCurrentText("1280x720");
//    }
//    else if(640 == ConfigOper::instance()->m_Width)
//    {
//        ui->m_pCombDefinition->setCurrentIndex(0);
//        ui->m_pCombResolution->setCurrentText("640x360");
//    }
//    else if(854 == ConfigOper::instance()->m_Width)
//    {
//        ui->m_pCombDefinition->setCurrentIndex(1);
//        ui->m_pCombResolution->setCurrentText("854x480");
//    }
    QString strResolution = QString("%1x%2").arg(ConfigOper::instance()->m_Width).arg(ConfigOper::instance()->m_Height);
    ui->m_pCombResolution->setCurrentText(strResolution);
    ui->m_pCombLiveRate->setCurrentText(QString::number(ConfigOper::instance()->m_VideoBitRate));
    if(ConfigOper::instance()->m_bCustom)
    {
        ui->m_pCheckCustom->setChecked(true);
        ui->m_pCombDefinition->setEnabled(false);
        ui->m_pCombResolution->setEnabled(true);
        ui->m_pCombLiveRate->setEnabled(true);
    }
    else
    {
        ui->m_pCheckCustom->setChecked(false);
        ui->m_pCombDefinition->setEnabled(true);
        ui->m_pCombResolution->setEnabled(false);
        ui->m_pCombLiveRate->setEnabled(false);
    }
    ui->m_pLdtLivePushUrl->setText(ConfigOper::instance()->m_LivePushUrl);
    ui->m_pLdtLiveBackPushUrl->setText(ConfigOper::instance()->m_LiveBackPushUrl);
    ui->m_pCheckPushUrl->setChecked(ConfigOper::instance()->m_bUsePush);
    ui->m_pCheckBackPushUrl->setChecked(ConfigOper::instance()->m_bUseBackPush);
    ui->m_pCheckRecord->setChecked(ConfigOper::instance()->m_bRecoder);
    if(0 == ConfigOper::instance()->m_FileType)
    {
        ui->m_pRdBtnMP4->setChecked(true);
        ui->m_pRdBtnFLV->setChecked(false);
        ui->m_pLdtRecordFilePath->setText(ConfigOper::instance()->m_RecoderPath);
    }
    else
    {
        ui->m_pRdBtnMP4->setChecked(false);
        ui->m_pRdBtnFLV->setChecked(true);
        ui->m_pLdtRecordFilePath->setText(ConfigOper::instance()->m_RecoderPath);
    }


//    if(1280 == ConfigOper::instance()->m_WidthSec)
//    {
//        ui->m_pCombDefinitionSec->setCurrentIndex(2);
//        ui->m_pCombResolutionSec->setCurrentText("1280x720");
//    }
//    else if(640 == ConfigOper::instance()->m_WidthSec)
//    {
//        ui->m_pCombDefinitionSec->setCurrentIndex(0);
//        ui->m_pCombResolutionSec->setCurrentText("640x360");
//    }
//    else if(854 == ConfigOper::instance()->m_WidthSec)
//    {
//        ui->m_pCombDefinitionSec->setCurrentIndex(1);
//        ui->m_pCombResolutionSec->setCurrentText("854x480");
//    }
    QString strResolutionSec = QString("%1x%2").arg(ConfigOper::instance()->m_WidthSec).arg(ConfigOper::instance()->m_HeightSec);
    ui->m_pCombResolutionSec->setCurrentText(strResolutionSec);
    ui->m_pCombLiveRateSec->setCurrentText(QString::number(ConfigOper::instance()->m_VideoBitRateSec));
    if(ConfigOper::instance()->m_bCustomSec)
    {
        ui->m_pCheckCustomSec->setChecked(true);
        ui->m_pCombDefinitionSec->setEnabled(false);
        ui->m_pCombResolutionSec->setEnabled(true);
        ui->m_pCombLiveRateSec->setEnabled(true);
    }
    else
    {
        ui->m_pCheckCustomSec->setChecked(false);
        ui->m_pCombDefinitionSec->setEnabled(true);
        ui->m_pCombResolutionSec->setEnabled(false);
        ui->m_pCombLiveRateSec->setEnabled(false);
    }
    ui->m_pLdtLivePushUrlSec->setText(ConfigOper::instance()->m_LivePushUrlSec);
    ui->m_pLdtLiveBackPushUrlSec->setText(ConfigOper::instance()->m_LiveBackPushUrlSec);
    ui->m_pCheckPushUrlSec->setChecked(ConfigOper::instance()->m_bUsePushSec);
    ui->m_pCheckBackPushUrlSec->setChecked(ConfigOper::instance()->m_bUseBackPushSec);
    ui->m_pCheckRecordSec->setChecked(ConfigOper::instance()->m_bRecoderSec);
    if(0 == ConfigOper::instance()->m_FileTypeSec)
    {
        ui->m_pRdBtnMP4Sec->setChecked(true);
        ui->m_pRdBtnFLVSec->setChecked(false);
        ui->m_pLdtRecordFilePathSec->setText(ConfigOper::instance()->m_RecoderPathSec);
    }
    else
    {
        ui->m_pRdBtnMP4Sec->setChecked(false);
        ui->m_pRdBtnFLVSec->setChecked(true);
        ui->m_pLdtRecordFilePathSec->setText(ConfigOper::instance()->m_RecoderPathSec);
    }
    ui->m_pCheckUseSecLive->setChecked(ConfigOper::instance()->m_bUseLiveSec);
    SetSecondaryLiveEnabled(ConfigOper::instance()->m_bUseLiveSec);
    ui->m_pSBoxReconnectTimeout->setValue(ConfigOper::instance()->m_AutoConnect);
    ui->m_pSBoxDelay->setValue(ConfigOper::instance()->m_DelayTime);

    if(m_pParent->m_bIsPreview || m_pParent->m_bIsLiving)
    {
        ui->m_pCombDefinition->setEnabled(false);
        ui->m_pCheckCustom->setEnabled(false);
        ui->m_pCombResolution->setEnabled(false);
        ui->m_pCombLiveRate->setEnabled(true);
        ui->m_pLdtLivePushUrl->setEnabled(false);
        ui->m_pCheckPushUrl->setEnabled(false);
        ui->m_pLdtLiveBackPushUrl->setEnabled(false);
        ui->m_pCheckBackPushUrl->setEnabled(false);
        ui->m_pCheckRecord->setEnabled(false);
        ui->m_pLdtRecordFilePath->setEnabled(false);
        ui->m_pBtnModifyLocation->setEnabled(false);
        ui->m_pBtnOpen->setEnabled(false);
        ui->m_pRdBtnMP4->setEnabled(false);
        ui->m_pRdBtnFLV->setEnabled(false);
        ui->m_pCheckUseSecLive->setEnabled(false);

        ui->m_pCombDefinitionSec->setEnabled(false);
        ui->m_pCheckCustomSec->setEnabled(false);
        ui->m_pCombResolutionSec->setEnabled(false);
        ui->m_pCombLiveRateSec->setEnabled(true);
        ui->m_pLdtLivePushUrlSec->setEnabled(false);
        ui->m_pCheckPushUrlSec->setEnabled(false);
        ui->m_pLdtLiveBackPushUrlSec->setEnabled(false);
        ui->m_pCheckBackPushUrlSec->setEnabled(false);
        ui->m_pCheckRecordSec->setEnabled(false);
        ui->m_pLdtRecordFilePathSec->setEnabled(false);
        ui->m_pBtnModifyLocationSec->setEnabled(false);
        ui->m_pBtnOpenSec->setEnabled(false);
        ui->m_pRdBtnMP4Sec->setEnabled(false);
        ui->m_pRdBtnFLVSec->setEnabled(false);

        if(ui->m_pCheckUseSecLive->isChecked())
            ui->m_pCombLiveRateSec->setEnabled(true);
        else
            ui->m_pCombLiveRateSec->setEnabled(false);
    }
}

void SettingUI::InitVideoSetting()
{
    ui->m_pCombDisplayDevice->setCurrentIndex(ConfigOper::instance()->m_AdpterID);
    ui->m_pSBoxFramRate->setValue(ConfigOper::instance()->m_FPS);
}
void SettingUI::InitAudioSetting()
{
    ui->m_pCombDirectorListenDevices->setCurrentText(ConfigOper::instance()->m_MonitorDevice);
    ui->m_pCombAudioOutputDevices->setCurrentText(ConfigOper::instance()->m_ScrProDevice);
}
void SettingUI::InitAdvanced()
{
    ui->m_pCheckUseMultithread->setChecked(ConfigOper::instance()->m_bUseMultiThread);
    ui->m_pCombProgramLevel->setCurrentIndex(ConfigOper::instance()->m_PriorityID);
    ui->m_pSBoxPicCacheTime->setMaximum(1000);
    ui->m_pSBoxPicCacheTime->setValue(ConfigOper::instance()->m_BufferTime);
    ui->m_pCombCPUPreset->setCurrentText(ConfigOper::instance()->m_X264Preset);
    ui->m_pCombCodeCfgFile->setCurrentText(ConfigOper::instance()->m_X264Profile);
    ui->m_pSBoxKeyFrameInterval->setValue(ConfigOper::instance()->m_KeyFrame);
    ui->m_pSBoxBFrameNum->setValue(ConfigOper::instance()->m_BFrameCount);
    CNetInerface* netInterface = CNetInerface::GetInstance();
    for(int i = 0; i < netInterface->m_serverList.count(); i++)
    {
        ui->m_pCombRtmpNode->addItem(netInterface->m_serverList.at(i)->name);
    }
}

void SettingUI::on_m_pBtnOk_clicked()
{
    on_m_pBtnApply_clicked();
    close();
}

void SettingUI::on_m_pBtnCancel_clicked()
{
    if(m_bEncoderChanged || m_bLiveSetttingChanged || m_bVideoSettingChanged || m_bAudioSettingChanged || m_bAdvancedChanged)
    {
        MyMessageBox message(this, tr("information"), tr("Do you want to save and apply these changes?"),BUTTON_YES|BUTTON_NO);

        int ret = message.exec();
        if(ON_BUTTON_OK == ret)
        {
            on_m_pBtnApply_clicked();
        }
        else
        {

        }
    }
    close();
}

void SettingUI::on_m_pBtnCode_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(0);
}

void SettingUI::on_m_pBtnLiveSet_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(1);
}

void SettingUI::on_m_pBtnImage_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(2);
}

void SettingUI::on_m_pBtnSound_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(3);
}

void SettingUI::on_m_pBtnSenior_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(4);
}

void SettingUI::on_m_pRdBtnCBR_clicked()
{
    ui->m_pCombQuality->setEnabled(false);
    ui->m_pCheckCBRPadding->setEnabled(true);
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pRdBtnVBR_clicked()
{
    ui->m_pCombQuality->setEnabled(true);
    ui->m_pCheckCBRPadding->setEnabled(false);
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pRdBtnH264_clicked()
{
    ui->m_pRdBtnH264->setChecked(true);
}

void SettingUI::on_m_pBtnModifyLocation_clicked()
{
    QString strDir = QFileDialog::getExistingDirectory(this,tr("select dir"));
    if(!strDir.isEmpty())
    {
        strDir = strDir + "/.flv";
        strDir = strDir.replace("//","/");
        ui->m_pLdtRecordFilePath->setText(strDir);
    }
}

void SettingUI::on_m_pBtnOpen_clicked()
{
    QString strDir = ui->m_pLdtRecordFilePath->text();
    strDir.truncate(strDir.length()-4);
    QString strName = QFileDialog::getOpenFileName(this,tr("Open"),strDir,tr("*.flv *.mp4"));
}

void SettingUI::on_m_pRdBtnMP4_clicked()
{
    ui->m_pLdtRecordFilePath->setText(ui->m_pLdtRecordFilePath->text().replace("flv","mp4"));
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pRdBtnFLV_clicked()
{
    ui->m_pLdtRecordFilePath->setText(ui->m_pLdtRecordFilePath->text().replace("mp4","flv"));
    m_bLiveSetttingChanged = true;
}

void SettingUI::SetSecondaryLiveEnabled(bool bEnabled)
{
    if(bEnabled)
    {
        ui->m_pCombDefinitionSec->setEnabled(true);
        ui->m_pCheckCustomSec->setEnabled(true);
        ui->m_pCombResolutionSec->setEnabled(true);
        ui->m_pCombLiveRateSec->setEnabled(true);
        ui->m_pLdtLivePushUrlSec->setEnabled(true);
        ui->m_pLdtLiveBackPushUrlSec->setEnabled(true);
        ui->m_pCheckPushUrlSec->setEnabled(true);
        ui->m_pCheckBackPushUrlSec->setEnabled(true);
        ui->m_pCheckRecordSec->setEnabled(true);
        ui->m_pLdtRecordFilePathSec->setEnabled(true);
        ui->m_pBtnModifyLocationSec->setEnabled(true);
        ui->m_pBtnOpenSec->setEnabled(true);
        ui->m_pRdBtnMP4Sec->setEnabled(true);
        ui->m_pRdBtnFLVSec->setEnabled(true);
    }
    else
    {
        ui->m_pCombDefinitionSec->setEnabled(false);
        ui->m_pCheckCustomSec->setEnabled(false);
        ui->m_pCombResolutionSec->setEnabled(false);
        ui->m_pCombLiveRateSec->setEnabled(false);
        ui->m_pLdtLivePushUrlSec->setEnabled(false);
        ui->m_pLdtLiveBackPushUrlSec->setEnabled(false);
        ui->m_pCheckPushUrlSec->setEnabled(false);
        ui->m_pCheckBackPushUrlSec->setEnabled(false);
        ui->m_pCheckRecordSec->setEnabled(false);
        ui->m_pLdtRecordFilePathSec->setEnabled(false);
        ui->m_pBtnModifyLocationSec->setEnabled(false);
        ui->m_pBtnOpenSec->setEnabled(false);
        ui->m_pRdBtnMP4Sec->setEnabled(false);
        ui->m_pRdBtnFLVSec->setEnabled(false);
    }
}

void SettingUI::on_m_pBtnModifyLocationSec_clicked()
{
    QString strDir = QFileDialog::getExistingDirectory(this,tr("select dir"));
    if(!strDir.isEmpty())
    {
        strDir = strDir + "/.flv";
        strDir = strDir.replace("//","/");
        ui->m_pLdtRecordFilePathSec->setText(strDir);
    }
}

void SettingUI::on_m_pBtnOpenSec_clicked()
{
    QString strDir = ui->m_pLdtRecordFilePathSec->text();
    strDir.truncate(strDir.length()-4);
    QString strName = QFileDialog::getOpenFileName(this,tr("Open"),strDir,tr("*.flv *.mp4"));
}

void SettingUI::on_m_pRdBtnMP4Sec_clicked()
{
    ui->m_pLdtRecordFilePathSec->setText(ui->m_pLdtRecordFilePathSec->text().replace("flv","mp4"));
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pRdBtnFLVSec_clicked()
{
    ui->m_pLdtRecordFilePathSec->setText(ui->m_pLdtRecordFilePathSec->text().replace("mp4","flv"));
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckUseSecLive_clicked(bool checked)
{
    SetSecondaryLiveEnabled(checked);
}

void SettingUI::on_m_pCheckCustom_clicked(bool checked)
{
    if(checked)
    {
        ui->m_pCheckCustom->setChecked(true);
        ui->m_pCombDefinition->setEnabled(false);
        ui->m_pCombResolution->setEnabled(true);
        ui->m_pCombLiveRate->setEnabled(true);
    }
    else
    {
        ui->m_pCheckCustom->setChecked(false);
        ui->m_pCombDefinition->setEnabled(true);
        ui->m_pCombResolution->setEnabled(false);
        ui->m_pCombLiveRate->setEnabled(false);
    }
}

void SettingUI::on_m_pCombRtmpNode_currentIndexChanged(int index)
{
    if(index > 0)
    {
        CNetInerface* netInterface = CNetInerface::GetInstance();
        QString strText = ui->m_pLdtLivePushUrl->text();
        int iStart = strText.indexOf("/live/");
        strText = strText.mid(iStart);
        QString pushUrl = "rtmp://" + netInterface->m_serverList.at(index-1)->ip + ":" + QString::number(netInterface->m_serverList.at(index-1)->port) + strText;
        ui->m_pLdtLivePushUrl->setText(pushUrl);
    }
    else if(0 == index)
    {
        ui->m_pLdtLivePushUrl->setText(m_pParent->m_pChannelInfo->pushstreamurl);
    }
    ConfigOper::instance()->m_LivePushUrl = ui->m_pLdtLivePushUrl->text();
}

void SettingUI::on_m_pBtnApply_clicked()
{
    EncoderParam Encoder = {0};
    LiveSettingParam LiveSetting = {0};
    VideoSettingParam VideoSetting = {0};
    AudioSettingParam AudioSetting = {0};
    AdvancedParam Advanced = {0};

    Encoder.bChange = m_bEncoderChanged;
    Encoder.bUseCBR = ConfigOper::instance()->m_bUseCBR = ui->m_pRdBtnCBR->isChecked();
    Encoder.bUseVBR = ConfigOper::instance()->m_bUseVBR = ui->m_pRdBtnVBR->isChecked();
    Encoder.bUsePadding = ConfigOper::instance()->m_bUsePadding = ui->m_pCheckCBRPadding->isChecked();
    Encoder.Quality = ConfigOper::instance()->m_Quality = ui->m_pCombQuality->currentIndex();
    Encoder.AudioEncoderType = ConfigOper::instance()->m_AudioEncoderType = ui->m_pCombEncoder->currentIndex();
    QString strAudioBitRate = ui->m_pCombRate->currentText();
    strAudioBitRate = strAudioBitRate.mid(0,strAudioBitRate.length()-4);
    Encoder.AudioBitRate = ConfigOper::instance()->m_AudioBitRate = strAudioBitRate.toInt();
    if(0 == ui->m_pCombSamplingRate->currentIndex())
        Encoder.AudioSampleRate = ConfigOper::instance()->m_AudioSampleRate = 44100;
    else if(1 == ui->m_pCombSamplingRate->currentIndex())
        Encoder.AudioSampleRate = ConfigOper::instance()->m_AudioSampleRate = 48000;
    Encoder.AudioChannel = ConfigOper::instance()->m_AudioChannel = ui->m_pCombSoundChannel->currentIndex() + 1;

    LiveSetting.bChange = m_bLiveSetttingChanged;
    ConfigOper::instance()->m_bCustom = ui->m_pCheckCustom->isChecked();
    ConfigOper::instance()->m_bCustomSec = ui->m_pCheckCustom->isChecked();
    LiveSetting.bUsePush = ConfigOper::instance()->m_bUsePush = ui->m_pCheckPushUrl->isChecked();
    LiveSetting.bUseBackPush = ConfigOper::instance()->m_bUseBackPush = ui->m_pCheckBackPushUrl->isChecked();
    LiveSetting.bRecoder = ConfigOper::instance()->m_bRecoder = ui->m_pCheckRecord->isChecked();
    QString strWidthAndHeght = ui->m_pCombResolution->currentText();
    int xIndex = strWidthAndHeght.indexOf('x');
    QString strW = strWidthAndHeght.mid(0,xIndex);
    QString strH = strWidthAndHeght.mid(xIndex+1);
    LiveSetting.Width = ConfigOper::instance()->m_Width = strW.toInt();
    LiveSetting.Height = ConfigOper::instance()->m_Height = strH.toInt();
    LiveSetting.VideoBitRate = ConfigOper::instance()->m_VideoBitRate = ui->m_pCombLiveRate->currentText().toInt();
    ConfigOper::instance()->m_LivePushUrl = ui->m_pLdtLivePushUrl->text();
    strcpy(LiveSetting.LivePushUrl,ConfigOper::instance()->m_LivePushUrl.toLocal8Bit().data());
    ConfigOper::instance()->m_LiveBackPushUrl = ui->m_pLdtLiveBackPushUrl->text();
    strcpy(LiveSetting.LiveBackPushUrl,ConfigOper::instance()->m_LiveBackPushUrl.toLocal8Bit().data());
    ConfigOper::instance()->m_RecoderPath = ui->m_pLdtRecordFilePath->text();
    strcpy(LiveSetting.RecoderPath,ConfigOper::instance()->m_RecoderPath.toLocal8Bit().data());
    if(ui->m_pRdBtnMP4->isChecked())
        LiveSetting.FileType = ConfigOper::instance()->m_FileType = 0;
    else
        LiveSetting.FileType = ConfigOper::instance()->m_FileType = 1;

    LiveSetting.bUseLiveSec = ConfigOper::instance()->m_bUseLiveSec = ui->m_pCheckUseSecLive->isChecked();
    LiveSetting.bUsePushSec = ConfigOper::instance()->m_bUsePushSec = ui->m_pCheckPushUrlSec->isChecked();
    LiveSetting.bUseBackPushSec = ConfigOper::instance()->m_bUseBackPushSec = ui->m_pCheckBackPushUrlSec->isChecked();
    LiveSetting.bRecoderSec = ConfigOper::instance()->m_bRecoderSec = ui->m_pCheckRecordSec->isChecked();
    QString strWidthAndHeghtSec = ui->m_pCombResolutionSec->currentText();
    int xIndexSec = strWidthAndHeghtSec.indexOf('x');
    QString strWSec = strWidthAndHeghtSec.mid(0,xIndexSec);
    QString strHSec = strWidthAndHeghtSec.mid(xIndexSec+1);
    LiveSetting.WidthSec = ConfigOper::instance()->m_WidthSec = strWSec.toInt();
    LiveSetting.HeightSec = ConfigOper::instance()->m_HeightSec = strHSec.toInt();
    LiveSetting.VideoBitRateSec = ConfigOper::instance()->m_VideoBitRateSec = ui->m_pCombLiveRateSec->currentText().toInt();
    ConfigOper::instance()->m_LivePushUrlSec = ui->m_pLdtLivePushUrlSec->text();
    strcpy(LiveSetting.LivePushUrlSec,ConfigOper::instance()->m_LivePushUrlSec.toLocal8Bit().data());
    ConfigOper::instance()->m_LiveBackPushUrlSec = ui->m_pLdtLiveBackPushUrlSec->text();
    strcpy(LiveSetting.LiveBackPushUrlSec,ConfigOper::instance()->m_LiveBackPushUrlSec.toLocal8Bit().data());
    ConfigOper::instance()->m_RecoderPathSec = ui->m_pLdtRecordFilePathSec->text();
    strcpy(LiveSetting.RecoderPathSec,ConfigOper::instance()->m_RecoderPathSec.toLocal8Bit().data());
    if(ui->m_pRdBtnMP4Sec->isChecked())
        LiveSetting.FileTypeSec = ConfigOper::instance()->m_FileTypeSec = 0;
    else
       LiveSetting.FileTypeSec = ConfigOper::instance()->m_FileTypeSec = 1;
    LiveSetting.AutoConnect = ConfigOper::instance()->m_AutoConnect = ui->m_pSBoxReconnectTimeout->value();
    LiveSetting.DelayTime = ConfigOper::instance()->m_DelayTime = ui->m_pSBoxDelay->value();

    VideoSetting.AdpterID = ConfigOper::instance()->m_AdpterID = ui->m_pCombDisplayDevice->currentIndex();
    VideoSetting.bChange = m_bVideoSettingChanged;
    VideoSetting.FPS = ConfigOper::instance()->m_FPS = ui->m_pSBoxFramRate->value();

    AudioSetting.bChange = m_bAudioSettingChanged;
    ConfigOper::instance()->m_MonitorDevice = ui->m_pCombDirectorListenDevices->currentText();
    strcpy(AudioSetting.MonitorDevice,ConfigOper::instance()->m_MonitorDevice.toLocal8Bit().data());
    ConfigOper::instance()->m_ScrProDevice = ui->m_pCombAudioOutputDevices->currentText();
    strcpy(AudioSetting.ScrProDevice,ConfigOper::instance()->m_ScrProDevice.toLocal8Bit().data());

    Advanced.BFrameCount = ConfigOper::instance()->m_BFrameCount = ui->m_pSBoxBFrameNum->value();
    Advanced.BufferTime = ConfigOper::instance()->m_BufferTime = ui->m_pSBoxPicCacheTime->value();
    Advanced.bUseMultiThread = ConfigOper::instance()->m_bUseMultiThread = ui->m_pCheckUseMultithread->isChecked();
    Advanced.KeyFrame = ConfigOper::instance()->m_KeyFrame = ui->m_pSBoxKeyFrameInterval->value();
    Advanced.PriorityID = ConfigOper::instance()->m_PriorityID = ui->m_pCombProgramLevel->currentIndex();
    ConfigOper::instance()->m_X264Preset = ui->m_pCombCPUPreset->currentText();
    strcpy(Advanced.X264Preset,ConfigOper::instance()->m_X264Preset.toLocal8Bit().data());
    ConfigOper::instance()->m_X264Profile = ui->m_pCombCodeCfgFile->currentText();
    strcpy(Advanced.X264Profile,ConfigOper::instance()->m_X264Profile.toLocal8Bit().data());
    Advanced.bChange = m_bAdvancedChanged;

    SLiveParam Param = { 0 };
    Param.Encoder = Encoder;
    Param.LiveSetting = LiveSetting;
    Param.VideoSetting = VideoSetting;
    Param.AudioSetting = AudioSetting;
    Param.Advanced = Advanced;

    if(SLiveSetParam(&Param) < 0)
    {

    }
    else
    {
        m_pParent->outputCX = m_pParent->m_pixWidth = strW.toInt();
        m_pParent->outputCY = m_pParent->m_pixHeight = strH.toInt();;
        m_pParent->ResizeRenderFrame();
    }

    ConfigOper::instance()->WriteCfgFile();
}

void SettingUI::on_m_pCombDefinition_currentIndexChanged(int index)
{
    if(0 == index)
        ui->m_pCombResolution->setCurrentText("640x360");
    else if(1 == index)
        ui->m_pCombResolution->setCurrentText("854x480");
    else if(2 == index)
        ui->m_pCombResolution->setCurrentText("1280x720");
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombDefinitionSec_currentIndexChanged(int index)
{
    if(0 == index)
        ui->m_pCombResolutionSec->setCurrentText("640x360");
    else if(1 == index)
        ui->m_pCombResolutionSec->setCurrentText("854x480");
    else if(2 == index)
        ui->m_pCombResolutionSec->setCurrentText("1280x720");
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombEncoder_currentIndexChanged(int index)
{
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pCombRate_currentIndexChanged(int index)
{
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pCombSamplingRate_currentIndexChanged(int index)
{
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pCombSoundChannel_currentIndexChanged(int index)
{
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pCheckCBRPadding_clicked()
{
    m_bEncoderChanged = true;
}

void SettingUI::on_m_pCheckCustom_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombResolution_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombLiveRate_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtLivePushUrl_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtLiveBackPushUrl_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckPushUrl_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckBackPushUrl_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtRecordFilePath_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckCustomSec_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombResolutionSec_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombLiveRateSec_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtLivePushUrlSec_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtLiveBackPushUrlSec_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pLdtRecordFilePathSec_textChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pSBoxReconnectTimeout_valueChanged(int arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pSBoxDelay_valueChanged(int arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombDisplayDevice_currentIndexChanged(int index)
{
    m_bVideoSettingChanged = true;
}

void SettingUI::on_m_pSBoxFramRate_valueChanged(int arg1)
{
    m_bVideoSettingChanged = true;
}

void SettingUI::on_m_pCombDirectorListenDevices_currentIndexChanged(int index)
{
    m_bAudioSettingChanged = true;
}

void SettingUI::on_m_pCombAudioOutputDevices_currentIndexChanged(int index)
{
    m_bAudioSettingChanged = true;
}

void SettingUI::on_m_pCheckUseMultithread_clicked()
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pCombProgramLevel_currentIndexChanged(int index)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pSBoxPicCacheTime_valueChanged(int arg1)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pCombCPUPreset_currentIndexChanged(int index)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pCombCodeCfgFile_currentIndexChanged(int index)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pSBoxKeyFrameInterval_valueChanged(int arg1)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pSBoxBFrameNum_valueChanged(int arg1)
{
    m_bAdvancedChanged = true;
}

void SettingUI::on_m_pCheckUseSecLive_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckRecord_clicked()
{
    m_bLiveSetttingChanged = true;
}
