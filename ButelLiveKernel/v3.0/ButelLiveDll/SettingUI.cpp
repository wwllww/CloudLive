#include "SettingUI.h"
#include <QDialog>
#include <QButtonGroup>
#include <QFileDialog>
#include <QDir>
#include <QProcess>
#include <QDesktopServices>
#include <QDebug>
#include "configoper.h"
#include "CMessageBox.h"
#include "BlackMagicItem.h"
#include "SeniorCodeSetUI.h"
#include "CHttpInterfaceSDK.h"

//取最大公约数
int greatCommonDivisor(int num1,int num2)
{
    if(num1*num2==0)
        return (num1==0?num2:num1);
    if(num1>num2)
        return greatCommonDivisor(num1%num2,num2);
    else if(num1<num2)
        return greatCommonDivisor(num2%num1,num1);
}
SettingUI::SettingUI(QDialog *parent):
    QDialog(parent),
    ui(new Ui::SettingUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | windowFlags());
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

    InitLiveSetting();
    InitDeviceSetting();
    InitAdvanced();
    InitBlackMagic();
    m_bEncoderChanged = false;
    m_bLiveSetttingChanged = false;
    m_bVideoSettingChanged = false;
    m_bAudioSettingChanged = false;
    m_bAdvancedChanged = false;
    m_bBlackMagicChanged = false;

    ui->m_pCombResolution->installEventFilter(this);
    ui->m_pCombResolutionSec->installEventFilter(this);


    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                  "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    QString combStyle = "QComboBox::drop-down"
                        "{"
                        "border-image: url(:images/arrow_down.png);"
                        "width:10px;"
                        "height:10px;"
                        "subcontrol-origin: padding;"
                        "subcontrol-position: center right;"
                        "margin: 14 8px;"
                        "}"
                        "QComboBox"
                        "{"
                        "height:20px;"
                        "border: 1px solid #dddddd;"
                        "background-color: rgb(221,221,221);"
                        "color: #2a2a2a;"
                        "border-radius: 4px;"
                        "}"
                        "QComboBox:!enabled"
                        "{"
                        "height:20px;"
                        "border: 1px solid #dddddd;"
                        "background-color: rgb(221,221,221);"
                        "color: rgb(160,160,160);"
                        "border-radius: 4px;"
                        "}";
    ui->m_pCombQuality->setStyleSheet(combStyle);
    ui->m_pCombEncoder->setStyleSheet(combStyle);
    ui->m_pCombRate->setStyleSheet(combStyle);
    ui->m_pCombSamplingRate->setStyleSheet(combStyle);
    ui->m_pCombSoundChannel->setStyleSheet(combStyle);

    ui->m_pCombQualitySec->setStyleSheet(combStyle);
    ui->m_pCombEncoderSec->setStyleSheet(combStyle);
    ui->m_pCombRateSec->setStyleSheet(combStyle);
    ui->m_pCombSamplingRateSec->setStyleSheet(combStyle);
    ui->m_pCombSoundChannelSec->setStyleSheet(combStyle);
    ui->m_pCombRtmpNodeSec->setStyleSheet(combStyle);

    ui->m_pCombResolution->setStyleSheet(combStyle);
    ui->m_pCombLiveRate->setStyleSheet(combStyle);

    ui->m_pCombResolutionSec->setStyleSheet(combStyle);
    ui->m_pCombLiveRateSec->setStyleSheet(combStyle);
    ui->m_pCombDisplayDevice->setStyleSheet(combStyle);
    ui->m_pCombDirectorListenDevices->setStyleSheet(combStyle);
    ui->m_pCombAudioOutputDevices->setStyleSheet(combStyle);
    ui->m_pCombProgramLevel->setStyleSheet(combStyle);

    ui->m_pCombRtmpNode->setStyleSheet(combStyle);
    ui->m_pCombInterlacedScan->setStyleSheet(combStyle);

    ui->m_pCombQuality->setMaxVisibleItems(20);
    ui->m_pCombRtmpNode->setMaxVisibleItems(20);

//    QObject::connect(this, &SettingUI::ncActive,[=](bool b){
//        if(b)
//          this->setStyleSheet("QWidget#SettingUI{background-color: rgb(35, 35, 37);}");
//        else
//          this->setStyleSheet("QWidget#SettingUI{background-color: rgb(255, 255, 255);}");
//    });
}

SettingUI::~SettingUI()
{
    delete ui;
}

void SettingUI::InitLiveSetting()
{
    //主直播
    QButtonGroup* pBtnGrp_H264 = new QButtonGroup(this);
    pBtnGrp_H264->setExclusive(true);
    pBtnGrp_H264->addButton(ui->m_pRdBtnH264Software);
    pBtnGrp_H264->addButton(ui->m_pRdBtnH264Hardware);
    QButtonGroup* pBtnGrp_Mode = new QButtonGroup(this);
    pBtnGrp_Mode->setExclusive(true);
    pBtnGrp_Mode->addButton(ui->m_pRdBtnCBR);
    pBtnGrp_Mode->addButton(ui->m_pRdBtnVBR);

    QString strResolution = QString("%1x%2").arg(m_pParent->m_LiveSettingParam.liveSettingParam.Width).arg(m_pParent->m_LiveSettingParam.liveSettingParam.Height);
    ui->m_pCombResolution->setCurrentText(strResolution);
    ui->m_pLbResolutionScal->setText(calcScalText(ui->m_pCombResolution->currentText()));

    ui->m_pSBoxFramRate->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.FPS);
    ui->m_pCombLiveRate->setCurrentText(QString::number(m_pParent->m_LiveSettingParam.liveSettingParam.VideoBitRate));
    if(m_pParent->m_LiveSettingParam.liveSettingParam.bUseHardEncoder)
    {
        ui->m_pRdBtnH264Software->setChecked(false);
        ui->m_pRdBtnH264Hardware->setChecked(true);
        ui->m_pRdBtnCBR->setEnabled(false);
        ui->m_pRdBtnVBR->setEnabled(false);
        ui->m_pCombQuality->setEnabled(false);
        ui->m_pBtnSeniorCodeSet->setEnabled(false);
    }
    else
    {
        ui->m_pRdBtnH264Software->setChecked(true);
        ui->m_pRdBtnH264Hardware->setChecked(false);
        ui->m_pRdBtnCBR->setEnabled(true);
        ui->m_pRdBtnVBR->setEnabled(true);
        ui->m_pCombQuality->setEnabled(true);
        ui->m_pBtnSeniorCodeSet->setEnabled(true);
    }
    ui->m_pCombQuality->clear();
    if(m_pParent->m_LiveSettingParam.liveSettingParam.bUseCBR)
    {
        ui->m_pRdBtnCBR->setChecked(true);
        ui->m_pRdBtnVBR->setChecked(false);
        ui->m_pCombQuality->addItem(tr("high performance"));
        ui->m_pCombQuality->addItem(tr("high quality"));
        ui->m_pCombQuality->setCurrentIndex(m_pParent->m_LiveSettingParam.CBRQuality);
    }
    else
    {
        ui->m_pRdBtnCBR->setChecked(false);
        ui->m_pRdBtnVBR->setChecked(true);
        for(int i = 0; i <= 10; i++)
        {
            ui->m_pCombQuality->addItem(QString("%1").arg(i));
        }
        ui->m_pCombQuality->setCurrentIndex(m_pParent->m_LiveSettingParam.VBRQuality);
    }


    if(44100 == m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate)
        ui->m_pCombSamplingRate->setCurrentIndex(0);
    else if(48000 == m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate)
        ui->m_pCombSamplingRate->setCurrentIndex(1);
    if(m_pParent->m_bIsPreview || m_pParent->m_bIsLiving)
    {
        ui->m_pCombSamplingRate->setEnabled(false);
    }
    QString strAudioBitRate = QString::number(m_pParent->m_LiveSettingParam.liveSettingParam.AudioBitRate)+"kb/s";
    ui->m_pCombRate->setCurrentText(strAudioBitRate);
    ui->m_pCombSoundChannel->setCurrentIndex(m_pParent->m_LiveSettingParam.liveSettingParam.AudioChannel - 1);
    ui->m_pCombEncoder->setCurrentIndex(m_pParent->m_LiveSettingParam.liveSettingParam.AudioEncoderType);

    ui->m_pCheckPushUrl->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bUsePush);
    ui->m_pCheckBackPushUrl->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bUseBackPush);
    ui->m_pLdtLivePushUrl->setText(m_pParent->m_pChannelInfo->pPushUrl);
    if(strlen(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrl) <= 0)
    {
        ui->m_pLdtLiveBackPushUrl->setText(QString(m_pParent->m_pChannelInfo->pPushUrl)+"_back");
    }
    else
    {
        ui->m_pLdtLiveBackPushUrl->setText(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrl);
    }
    ui->m_pSBoxReconnectTimeout->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.AutoConnect);
    ui->m_pSBoxDelay->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.DelayTime);

    ui->m_pCheckRecord->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bRecoder);
    if(0 == m_pParent->m_LiveSettingParam.liveSettingParam.FileType)
    {
        ui->m_pRdBtnMP4->setChecked(true);
        ui->m_pRdBtnFLV->setChecked(false);
    }
    else
    {
        ui->m_pRdBtnMP4->setChecked(false);
        ui->m_pRdBtnFLV->setChecked(true);
    }
    ui->m_pCombQualitySec->clear();
    if(m_pParent->m_LiveSettingParam.liveSettingParam.bUseCBRSec)
    {
        ui->m_pRdBtnCBRSec->setChecked(true);
        ui->m_pRdBtnVBRSec->setChecked(false);
        ui->m_pCombQualitySec->addItem(tr("high performance"));
        ui->m_pCombQualitySec->addItem(tr("high quality"));
        ui->m_pCombQualitySec->setCurrentIndex(m_pParent->m_LiveSettingParam.CBRQualitySec);
    }
    else
    {
        ui->m_pRdBtnCBRSec->setChecked(false);
        ui->m_pRdBtnVBRSec->setChecked(true);
        for(int i = 0; i <= 10; i++)
        {
            ui->m_pCombQualitySec->addItem(QString("%1").arg(i));
        }
        ui->m_pCombQualitySec->setCurrentIndex(m_pParent->m_LiveSettingParam.VBRQualitySec);
    }
    ui->m_pLdtRecordFilePath->setText(m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPath);

    //次直播
    ui->m_pCheckUseSecLive->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bUseLiveSec);

    QButtonGroup* pBtnGrp_H264_Sec = new QButtonGroup(this);
    pBtnGrp_H264_Sec->setExclusive(true);
    pBtnGrp_H264_Sec->addButton(ui->m_pRdBtnH264SoftwareSec);
    pBtnGrp_H264_Sec->addButton(ui->m_pRdBtnH264HardwareSec);
    QButtonGroup* pBtnGrp_Mode_Sec = new QButtonGroup(this);
    pBtnGrp_Mode_Sec->setExclusive(true);
    pBtnGrp_Mode_Sec->addButton(ui->m_pRdBtnCBRSec);
    pBtnGrp_Mode_Sec->addButton(ui->m_pRdBtnVBRSec);

    QString strResolutionSec = QString("%1x%2").arg(m_pParent->m_LiveSettingParam.liveSettingParam.WidthSec).arg(m_pParent->m_LiveSettingParam.liveSettingParam.HeightSec);
    ui->m_pCombResolutionSec->setCurrentText(strResolutionSec);
    ui->m_pLbResolutionScalSec->setText(calcScalText(ui->m_pCombResolutionSec->currentText()));

    ui->m_pSBoxFramRateSec->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.FPS);
    ui->m_pCombLiveRateSec->setCurrentText(QString::number(m_pParent->m_LiveSettingParam.liveSettingParam.VideoBitRateSec));
    if(m_pParent->m_LiveSettingParam.liveSettingParam.bUseHardEncoderSec)
    {
        ui->m_pRdBtnH264SoftwareSec->setChecked(false);
        ui->m_pRdBtnH264HardwareSec->setChecked(true);
        ui->m_pRdBtnCBRSec->setEnabled(false);
        ui->m_pRdBtnVBRSec->setEnabled(false);
        ui->m_pCombQualitySec->setEnabled(false);
        ui->m_pBtnSeniorCodeSetSec->setEnabled(false);
    }
    else
    {
        ui->m_pRdBtnH264SoftwareSec->setChecked(true);
        ui->m_pRdBtnH264HardwareSec->setChecked(false);
        ui->m_pRdBtnCBRSec->setEnabled(true);
        ui->m_pRdBtnVBRSec->setEnabled(true);
        ui->m_pCombQualitySec->setEnabled(true);
        ui->m_pBtnSeniorCodeSetSec->setEnabled(true);
    }

    if(44100 == m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate)
        ui->m_pCombSamplingRateSec->setCurrentIndex(0);
    else if(48000 == m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate)
        ui->m_pCombSamplingRateSec->setCurrentIndex(1);
    if(m_pParent->m_bIsPreview || m_pParent->m_bIsLiving)
    {
        ui->m_pCombSamplingRateSec->setEnabled(false);
    }
    QString strAudioBitRateSec = QString::number(m_pParent->m_LiveSettingParam.liveSettingParam.AudioBitRate)+"kb/s";
    ui->m_pCombRateSec->setCurrentText(strAudioBitRateSec);
    ui->m_pCombSoundChannelSec->setCurrentIndex(m_pParent->m_LiveSettingParam.liveSettingParam.AudioChannel - 1);
    ui->m_pCombEncoderSec->setCurrentIndex(m_pParent->m_LiveSettingParam.liveSettingParam.AudioEncoderType);

    ui->m_pCheckPushUrlSec->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bUsePushSec);
    ui->m_pCheckBackPushUrlSec->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bUseBackPushSec);
    ui->m_pLdtLivePushUrlSec->setText(m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrlSec);
    ui->m_pLdtLiveBackPushUrlSec->setText(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrlSec);

    ui->m_pSBoxReconnectTimeoutSec->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.AutoConnectSec);
    ui->m_pSBoxDelaySec->setValue(m_pParent->m_LiveSettingParam.liveSettingParam.DelayTimeSec);

    ui->m_pCheckRecordSec->setChecked(m_pParent->m_LiveSettingParam.liveSettingParam.bRecoderSec);
    if(0 == m_pParent->m_LiveSettingParam.liveSettingParam.FileTypeSec)
    {
        ui->m_pRdBtnMP4Sec->setChecked(true);
        ui->m_pRdBtnFLVSec->setChecked(false);
    }
    else
    {
        ui->m_pRdBtnMP4Sec->setChecked(false);
        ui->m_pRdBtnFLVSec->setChecked(true);
    }
    ui->m_pLdtRecordFilePathSec->setText(m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPathSec);

    NodeInfo* pTmpNode = m_pParent->m_pServerList;
    for(int i = 0; i < m_pParent->m_ServerCount; i++,pTmpNode++)
    {
        ui->m_pCombRtmpNode->addItem(pTmpNode->pdescribe);
        ui->m_pCombRtmpNodeSec->addItem(pTmpNode->pdescribe);
    }


    SetSecondaryLiveEnabled(m_pParent->m_LiveSettingParam.liveSettingParam.bUseLiveSec);

    if(m_pParent->m_bIsPreview || m_pParent->m_bIsLiving)
    {
        ui->m_pCombSamplingRate->setEnabled(false);

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

    if(m_pParent->m_bIsRecording)
    {
        ui->m_pCheckRecord->setEnabled(false);
        ui->m_pLdtRecordFilePath->setEnabled(false);
        ui->m_pRdBtnMP4->setEnabled(false);
        ui->m_pRdBtnFLV->setEnabled(false);

        ui->m_pCheckRecordSec->setEnabled(false);
        ui->m_pLdtRecordFilePathSec->setEnabled(false);
        ui->m_pRdBtnMP4Sec->setEnabled(false);
        ui->m_pRdBtnFLVSec->setEnabled(false);
    }

    if(m_pParent->m_bIsLiving)
    {
        ui->m_pSBoxReconnectTimeout->setEnabled(false);
        ui->m_pSBoxDelay->setEnabled(false);
        ui->m_pSBoxReconnectTimeoutSec->setEnabled(false);
        ui->m_pSBoxDelaySec->setEnabled(false);
    }
}

void SettingUI::InitDeviceSetting()
{
    ui->m_pCombDisplayDevice->setCurrentIndex(ConfigOper::instance()->m_AdpterID);
    ui->m_pCombDirectorListenDevices->setCurrentText(ConfigOper::instance()->m_MonitorDevice);
    ui->m_pCombAudioOutputDevices->setCurrentText(ConfigOper::instance()->m_ScrProDevice);

}
void SettingUI::InitAdvanced()
{

    ui->m_pCheckUseMultithread->setChecked(ConfigOper::instance()->m_bUseMultiThread);
    ui->m_pCombProgramLevel->setCurrentIndex(ConfigOper::instance()->m_PriorityID);
    ui->m_pSBoxPicCacheTime->setMaximum(1000);
    ui->m_pSBoxPicCacheTime->setValue(ConfigOper::instance()->m_BufferTime);
    ui->m_pCombInterlacedScan->setCurrentIndex(ConfigOper::instance()->m_InterlacedScan);
}
void SettingUI::InitBlackMagic()
{
    m_pParent->InitDeviceList();
    for(int i = 0; i < m_pParent->m_SDIDeviceVec.count(); i++)
    {
        CBlackMagicItem* pBlkMgcItem = new CBlackMagicItem(this,m_pParent->m_SDIDeviceVec[i].name,i);
        QListWidgetItem* pLWItem = new QListWidgetItem(ui->m_pLWBlackMagic);
        pLWItem->setSizeHint(QSize(0,50));
        if(0 == i)
        {
            if(ConfigOper::instance()->m_bSDIDevInput1)
                pBlkMgcItem->m_RdBtnInput.setChecked(true);
            else
                pBlkMgcItem->m_RdBtnOutput.setChecked(true);
            pBlkMgcItem->m_LdtAnotherName.setText(ConfigOper::instance()->m_AnotherName1);
        }
        else if(1 == i)
        {
            if(ConfigOper::instance()->m_bSDIDevInput2)
                pBlkMgcItem->m_RdBtnInput.setChecked(true);
            else
                pBlkMgcItem->m_RdBtnOutput.setChecked(true);
            pBlkMgcItem->m_LdtAnotherName.setText(ConfigOper::instance()->m_AnotherName2);
        }
        else if(2 == i)
        {
            if(ConfigOper::instance()->m_bSDIDevInput3)
                pBlkMgcItem->m_RdBtnInput.setChecked(true);
            else
                pBlkMgcItem->m_RdBtnOutput.setChecked(true);
            pBlkMgcItem->m_LdtAnotherName.setText(ConfigOper::instance()->m_AnotherName3);
        }
        else if(3 == i)
        {
            if(ConfigOper::instance()->m_bSDIDevInput4)
                pBlkMgcItem->m_RdBtnInput.setChecked(true);
            else
                pBlkMgcItem->m_RdBtnOutput.setChecked(true);
            pBlkMgcItem->m_LdtAnotherName.setText(ConfigOper::instance()->m_AnotherName4);
        }
        ui->m_pLWBlackMagic->addItem(pLWItem);
        ui->m_pLWBlackMagic->setItemWidget(pLWItem,pBlkMgcItem);
    }
    ui->m_pLWBlackMagic->setFocusPolicy(Qt::NoFocus);
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
        CMessageBox message(this, tr("information"), tr("The configuration has been changed. Do you want to save and apply these changes?"));
        int ret = message.exec();
        if(10 == ret)
        {
            on_m_pBtnApply_clicked();
            close();
        }
        else if(20 == ret)
        {
            close();
        }
    } 
}

void SettingUI::on_m_pBtnCode_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(0);
    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                     "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pBtnLiveSet_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(0);
    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pBtnImage_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(1);
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                     "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pBtnSound_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(3);

    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                     "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pBtnSenior_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(2);
    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                     "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                        "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pRdBtnCBR_clicked()
{
//    ui->m_pCombQuality->setEnabled(false);
//    ui->m_pCheckCBRPadding->setEnabled(true);
    ui->m_pCombQuality->clear();
    ui->m_pCombQuality->addItem(tr("high performance"));
    ui->m_pCombQuality->addItem(tr("high quality"));
    ui->m_pCombQuality->setCurrentIndex(m_pParent->m_LiveSettingParam.CBRQuality);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pRdBtnVBR_clicked()
{
//    ui->m_pCombQuality->setEnabled(true);
//    ui->m_pCheckCBRPadding->setEnabled(false);
    ui->m_pCombQuality->clear();
    for(int i = 0; i <= 10; i++)
    {
        ui->m_pCombQuality->addItem(QString("%1").arg(i));
    }
    ui->m_pCombQuality->setCurrentIndex(m_pParent->m_LiveSettingParam.VBRQuality);
    m_bLiveSetttingChanged = true;
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
//    QString strName = QFileDialog::getOpenFileName(this,tr("Open"),strDir,tr("*.flv *.mp4"));
    QDir dir;
    QString path = dir.currentPath();
    path = path + "/" + strDir;
    QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));

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
        ui->m_pRdBtnH264SoftwareSec->setEnabled(true);
        ui->m_pRdBtnH264HardwareSec->setEnabled(true);
        if(!ui->m_pRdBtnH264HardwareSec->isChecked())
        {
            ui->m_pRdBtnCBRSec->setEnabled(true);
            ui->m_pRdBtnVBRSec->setEnabled(true);
            ui->m_pCombQualitySec->setEnabled(true);
            ui->m_pBtnSeniorCodeSetSec->setEnabled(true);
        }

        ui->m_pCombRtmpNodeSec->setEnabled(true);
        ui->m_pSBoxReconnectTimeoutSec->setEnabled(true);
        ui->m_pSBoxDelaySec->setEnabled(true);
    }
    else
    {
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
        ui->m_pRdBtnH264SoftwareSec->setEnabled(false);
        ui->m_pRdBtnH264HardwareSec->setEnabled(false);
        ui->m_pRdBtnCBRSec->setEnabled(false);
        ui->m_pRdBtnVBRSec->setEnabled(false);
        ui->m_pCombQualitySec->setEnabled(false);
        ui->m_pBtnSeniorCodeSetSec->setEnabled(false);
        ui->m_pCombRtmpNodeSec->setEnabled(false);
        ui->m_pSBoxReconnectTimeoutSec->setEnabled(false);
        ui->m_pSBoxDelaySec->setEnabled(false);
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
//    QString strName = QFileDialog::getOpenFileName(this,tr("Open"),strDir,tr("*.flv *.mp4"));

    QDir dir;
    QString path = dir.currentPath();
    path = path + "/" + strDir;
    QDesktopServices::openUrl(QUrl(path, QUrl::TolerantMode));
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

void SettingUI::on_m_pCombRtmpNode_currentIndexChanged(int index)
{
    if(index > 0)
    {
        QString strText = ui->m_pLdtLivePushUrl->text();
        int iStart = strText.indexOf("/live/");
        strText = strText.mid(iStart);
        NodeInfo* pCurNode = m_pParent->m_pServerList + index - 1;
        QString strIP = QString(pCurNode->ip);
        QString strPort = QString::number(pCurNode->port);
        QString pushUrl = "rtmp://" + strIP + ":" + strPort + strText;
        ui->m_pLdtLivePushUrl->setText(pushUrl);

    }
    else if(0 == index)
    {
        ui->m_pLdtLivePushUrl->setText(m_pParent->m_pChannelInfo->pPushUrl);
    }
//    ConfigOper::instance()->m_LivePushUrl = ui->m_pLdtLivePushUrl->text();
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrl,ui->m_pLdtLivePushUrl->text().toLocal8Bit().data());
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pBtnApply_clicked()
{
//    EncoderParam Encoder = {0};
    LiveSettingParam LiveSetting = {0};
    DeviceParam  DeviceSetting ={0};
//    VideoSettingParam VideoSetting = {0};
//    AudioSettingParam AudioSetting = {0};
    AdvancedParam Advanced = {0};

    //主直播
    LiveSetting.bChange = m_bLiveSetttingChanged;
    //视频
    QString strWidthAndHeght = ui->m_pCombResolution->currentText();
    int xIndex = strWidthAndHeght.indexOf('x');
    QString strW = strWidthAndHeght.mid(0,xIndex);
    QString strH = strWidthAndHeght.mid(xIndex+1);
    LiveSetting.Width = m_pParent->m_LiveSettingParam.liveSettingParam.Width = strW.toInt();
    LiveSetting.Height = m_pParent->m_LiveSettingParam.liveSettingParam.Height = strH.toInt();
    LiveSetting.FPS = m_pParent->m_LiveSettingParam.liveSettingParam.FPS = ui->m_pSBoxFramRate->value();
    LiveSetting.VideoBitRate = m_pParent->m_LiveSettingParam.liveSettingParam.VideoBitRate = ui->m_pCombLiveRate->currentText().toInt();
    LiveSetting.bUseHardEncoder = m_pParent->m_LiveSettingParam.liveSettingParam.bUseHardEncoder= ui->m_pRdBtnH264Hardware->isChecked();
    LiveSetting.bUseCBR = m_pParent->m_LiveSettingParam.liveSettingParam.bUseCBR = ui->m_pRdBtnCBR->isChecked();
    if(LiveSetting.bUseCBR)
    {
        LiveSetting.Quality = m_pParent->m_LiveSettingParam.CBRQuality = ui->m_pCombQuality->currentIndex();
    }
    else
    {
        LiveSetting.Quality = m_pParent->m_LiveSettingParam.VBRQuality = ui->m_pCombQuality->currentIndex();
    }
    m_pParent->m_LiveSettingParam.liveSettingParam.Quality = LiveSetting.Quality;
    //音频
    LiveSetting.AudioEncoderType = m_pParent->m_LiveSettingParam.liveSettingParam.AudioEncoderType = ui->m_pCombEncoder->currentIndex();
    QString strAudioBitRate = ui->m_pCombRate->currentText();
    strAudioBitRate = strAudioBitRate.mid(0,strAudioBitRate.length()-4);
    LiveSetting.AudioBitRate = m_pParent->m_LiveSettingParam.liveSettingParam.AudioBitRate = strAudioBitRate.toInt();
    if(0 == ui->m_pCombSamplingRate->currentIndex())
        LiveSetting.AudioSampleRate = m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate = 44100;
    else if(1 == ui->m_pCombSamplingRate->currentIndex())
        LiveSetting.AudioSampleRate = m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate = 48000;
    LiveSetting.AudioChannel = m_pParent->m_LiveSettingParam.liveSettingParam.AudioChannel = ui->m_pCombSoundChannel->currentIndex() + 1;

    //高级编码设置
    LiveSetting.BFrameCount = m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount;
    LiveSetting.KeyFrame = m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrame;
    strcpy(LiveSetting.X264Preset,m_pParent->m_LiveSettingParam.liveSettingParam.X264Preset);
    strcpy(LiveSetting.X264Profile,m_pParent->m_LiveSettingParam.liveSettingParam.X264Profile);

    //推流
    LiveSetting.bUsePush = m_pParent->m_LiveSettingParam.liveSettingParam.bUsePush = ui->m_pCheckPushUrl->isChecked();
    LiveSetting.bUseBackPush = m_pParent->m_LiveSettingParam.liveSettingParam.bUseBackPush = ui->m_pCheckBackPushUrl->isChecked();
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrl,ui->m_pLdtLivePushUrl->text().toLocal8Bit().data());
    strcpy(LiveSetting.LivePushUrl,m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrl);
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrl,ui->m_pLdtLiveBackPushUrl->text().toLocal8Bit().data());
    strcpy(LiveSetting.LiveBackPushUrl,m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrl);
    LiveSetting.AutoConnect =m_pParent->m_LiveSettingParam.liveSettingParam.AutoConnect = ui->m_pSBoxReconnectTimeout->value();
    LiveSetting.DelayTime = m_pParent->m_LiveSettingParam.liveSettingParam.DelayTime = ui->m_pSBoxDelay->value();

    //录制
    LiveSetting.bRecoder = m_pParent->m_LiveSettingParam.liveSettingParam.bRecoder = ui->m_pCheckRecord->isChecked();
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPath,ui->m_pLdtRecordFilePath->text().toLocal8Bit().data());
    strcpy(LiveSetting.RecoderPath,m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPath);
    if(ui->m_pRdBtnMP4->isChecked())
        LiveSetting.FileType = m_pParent->m_LiveSettingParam.liveSettingParam.FileType = 0;
    else
        LiveSetting.FileType = m_pParent->m_LiveSettingParam.liveSettingParam.FileType = 1;

    //次直播
    LiveSetting.bUseLiveSec = m_pParent->m_LiveSettingParam.liveSettingParam.bUseLiveSec = ui->m_pCheckUseSecLive->isChecked();
    //视频
    QString strWidthAndHeghtSec = ui->m_pCombResolutionSec->currentText();
    int xIndexSec = strWidthAndHeghtSec.indexOf('x');
    QString strWSec = strWidthAndHeghtSec.mid(0,xIndexSec);
    QString strHSec = strWidthAndHeghtSec.mid(xIndexSec+1);
    LiveSetting.WidthSec = m_pParent->m_LiveSettingParam.liveSettingParam.WidthSec = strWSec.toInt();
    LiveSetting.HeightSec = m_pParent->m_LiveSettingParam.liveSettingParam.HeightSec = strHSec.toInt();
    LiveSetting.FPSSec = m_pParent->m_LiveSettingParam.liveSettingParam.FPS = ui->m_pSBoxFramRate->value();
    LiveSetting.VideoBitRateSec = m_pParent->m_LiveSettingParam.liveSettingParam.VideoBitRateSec = ui->m_pCombLiveRateSec->currentText().toInt();
    LiveSetting.bUseHardEncoderSec = m_pParent->m_LiveSettingParam.liveSettingParam.bUseHardEncoderSec = ui->m_pRdBtnH264HardwareSec->isChecked();
    LiveSetting.bUseCBRSec = m_pParent->m_LiveSettingParam.liveSettingParam.bUseCBRSec = ui->m_pRdBtnCBRSec->isChecked();

    if(LiveSetting.bUseCBRSec)
    {
        LiveSetting.QualitySec = m_pParent->m_LiveSettingParam.CBRQualitySec = ui->m_pCombQualitySec->currentIndex();
    }
    else
    {
        LiveSetting.QualitySec = m_pParent->m_LiveSettingParam.VBRQualitySec = ui->m_pCombQualitySec->currentIndex();
    }
    m_pParent->m_LiveSettingParam.liveSettingParam.QualitySec = LiveSetting.QualitySec;
    //音频
    LiveSetting.AudioEncoderTypeSec = m_pParent->m_LiveSettingParam.liveSettingParam.AudioEncoderType;
    LiveSetting.AudioBitRateSec = m_pParent->m_LiveSettingParam.liveSettingParam.AudioBitRate;
    if(0 == ui->m_pCombSamplingRate->currentIndex())
        LiveSetting.AudioSampleRateSec = m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate = 44100;
    else if(1 == ui->m_pCombSamplingRate->currentIndex())
        LiveSetting.AudioSampleRateSec = m_pParent->m_LiveSettingParam.liveSettingParam.AudioSampleRate = 48000;
    LiveSetting.AudioChannelSec = m_pParent->m_LiveSettingParam.liveSettingParam.AudioChannel;

    //高级编码设置
    LiveSetting.BFrameCountSec = m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec;
    LiveSetting.KeyFrameSec = m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrameSec;
    strcpy(LiveSetting.X264PresetSec,m_pParent->m_LiveSettingParam.liveSettingParam.X264PresetSec);
    strcpy(LiveSetting.X264ProfileSec,m_pParent->m_LiveSettingParam.liveSettingParam.X264ProfileSec);

    //推流
    LiveSetting.bUsePushSec = m_pParent->m_LiveSettingParam.liveSettingParam.bUsePushSec = ui->m_pCheckPushUrlSec->isChecked();
    LiveSetting.bUseBackPushSec = m_pParent->m_LiveSettingParam.liveSettingParam.bUseBackPushSec = ui->m_pCheckBackPushUrlSec->isChecked();
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrlSec,ui->m_pLdtLivePushUrlSec->text().toLocal8Bit().data());
    strcpy(LiveSetting.LivePushUrlSec,m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrlSec);
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrlSec, ui->m_pLdtLiveBackPushUrlSec->text().toLocal8Bit().data());
    strcpy(LiveSetting.LiveBackPushUrlSec,m_pParent->m_LiveSettingParam.liveSettingParam.LiveBackPushUrlSec);
    LiveSetting.AutoConnectSec = m_pParent->m_LiveSettingParam.liveSettingParam.AutoConnectSec = ui->m_pSBoxReconnectTimeoutSec->value();
    LiveSetting.DelayTimeSec = m_pParent->m_LiveSettingParam.liveSettingParam.DelayTimeSec = ui->m_pSBoxDelaySec->value();

    //录制
    LiveSetting.bRecoderSec = m_pParent->m_LiveSettingParam.liveSettingParam.bRecoderSec = ui->m_pCheckRecordSec->isChecked();
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPathSec,ui->m_pLdtRecordFilePathSec->text().toLocal8Bit().data());
    strcpy(LiveSetting.RecoderPathSec,m_pParent->m_LiveSettingParam.liveSettingParam.RecoderPathSec);
    if(ui->m_pRdBtnMP4Sec->isChecked())
        LiveSetting.FileTypeSec = m_pParent->m_LiveSettingParam.liveSettingParam.FileTypeSec = 0;
    else
        LiveSetting.FileTypeSec = m_pParent->m_LiveSettingParam.liveSettingParam.FileTypeSec = 1;

    //音视频设备
    DeviceSetting.bChange = m_bVideoSettingChanged;
    DeviceSetting.AdpterID = ConfigOper::instance()->m_AdpterID = ui->m_pCombDisplayDevice->currentIndex();
    ConfigOper::instance()->m_MonitorDevice = ui->m_pCombDirectorListenDevices->currentText();
    strcpy(DeviceSetting.MonitorDevice,ConfigOper::instance()->m_MonitorDevice.toLocal8Bit().data());
    ConfigOper::instance()->m_ScrProDevice = ui->m_pCombAudioOutputDevices->currentText();
    strcpy(DeviceSetting.ScrProDevice,ConfigOper::instance()->m_ScrProDevice.toLocal8Bit().data());

    //高级
    Advanced.bChange = m_bAdvancedChanged;
    Advanced.bUseMultiThread = ConfigOper::instance()->m_bUseMultiThread = ui->m_pCheckUseMultithread->isChecked();
    Advanced.PriorityID = ConfigOper::instance()->m_PriorityID = ui->m_pCombProgramLevel->currentIndex();
    Advanced.BufferTime = ConfigOper::instance()->m_BufferTime = ui->m_pSBoxPicCacheTime->value();
    Advanced.DeinterlaceType = ConfigOper::instance()->m_InterlacedScan = ui->m_pCombInterlacedScan->currentIndex();


    for(int i = 0; i < ui->m_pLWBlackMagic->count(); i++)
    {
        CBlackMagicItem* pBlkItem = (CBlackMagicItem*)ui->m_pLWBlackMagic->itemWidget(ui->m_pLWBlackMagic->item(i));
        if(pBlkItem->m_RdBtnInput.isChecked())
            m_pParent->m_bArrBlackMagic[i] = true;
        else
            m_pParent->m_bArrBlackMagic[i] = false;

        if(0 == i)
        {
            ConfigOper::instance()->m_AnotherName1 = pBlkItem->m_LdtAnotherName.text();
        }
        else if(1 == i)
        {
            ConfigOper::instance()->m_AnotherName2 = pBlkItem->m_LdtAnotherName.text();
        }
        else if(2 == i)
        {
            ConfigOper::instance()->m_AnotherName3 = pBlkItem->m_LdtAnotherName.text();
        }
        else if(3 == i)
        {
            ConfigOper::instance()->m_AnotherName4 = pBlkItem->m_LdtAnotherName.text();
        }
    }
    ConfigOper::instance()->m_bSDIDevInput1 = m_pParent->m_bArrBlackMagic[0];
    ConfigOper::instance()->m_bSDIDevInput2 = m_pParent->m_bArrBlackMagic[1];
    ConfigOper::instance()->m_bSDIDevInput3 = m_pParent->m_bArrBlackMagic[2];
    ConfigOper::instance()->m_bSDIDevInput4 = m_pParent->m_bArrBlackMagic[3];

    BlackMagicParam BlackMagic = {0};
    BlackMagic.bChange = m_bBlackMagicChanged;
    BlackMagic.bOutSDI = m_pParent->m_bArrBlackMagic;

    SLiveParam Param = { 0 };
    qDebug() << LiveSetting.FileType <<LiveSetting.FileTypeSec;
    Param.LiveSetting = LiveSetting;
    Param.DeviceSetting = DeviceSetting;
    Param.Advanced = Advanced;
    Param.BlackMagic = BlackMagic;
    if(SLiveSetParam(&Param) < 0)
    {

    }
    else
    {
        int iWidth = strW.toInt();
        int iHeight = strH.toInt();
        int iWidthSec = strWSec.toInt();
        int iHeightSec = strHSec.toInt();
        if(LiveSetting.bUseLiveSec)
        {
            if(iWidthSec > iWidth)
            {
                m_pParent->outputCX = m_pParent->m_pixWidth = iWidthSec;
                m_pParent->outputCY = m_pParent->m_pixHeight = iHeightSec;
            }
            else if(iWidthSec == iWidth)
            {
                if(iHeightSec > iHeight)
                {
                    m_pParent->outputCX = m_pParent->m_pixWidth = iWidthSec;
                    m_pParent->outputCY = m_pParent->m_pixHeight = iHeightSec;
                }
                else
                {
                    m_pParent->outputCX = m_pParent->m_pixWidth = iWidth;
                    m_pParent->outputCY = m_pParent->m_pixHeight = iHeight;
                }
            }
            else if(iWidthSec < iWidth)
            {
                m_pParent->outputCX = m_pParent->m_pixWidth = iWidth;
                m_pParent->outputCY = m_pParent->m_pixHeight = iHeight;
            }
        }
        else
        {
            m_pParent->outputCX = m_pParent->m_pixWidth = iWidth;
            m_pParent->outputCY = m_pParent->m_pixHeight = iHeight;

        }
        m_pParent->ResizeRenderFrame();
    }
    ConfigOper::instance()->WriteCfgFile();

    if(!m_pParent->m_bIsPreview && !m_pParent->m_bIsLiving)
        m_pParent->UpdateRecordState();

}

void SettingUI::on_m_pCombEncoder_currentIndexChanged(int index)
{
    ui->m_pCombEncoderSec->setCurrentIndex(index);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombRate_currentIndexChanged(int index)
{
    ui->m_pCombRateSec->setCurrentIndex(index);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombSamplingRate_currentIndexChanged(int index)
{
    ui->m_pCombSamplingRateSec->setCurrentIndex(index);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombSoundChannel_currentIndexChanged(int index)
{
    ui->m_pCombSoundChannelSec->setCurrentIndex(index);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombResolution_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
    ui->m_pLbResolutionScal->setText(calcScalText(ui->m_pCombResolution->currentText()));
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

void SettingUI::on_m_pCombResolutionSec_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
    ui->m_pLbResolutionScalSec->setText(calcScalText(ui->m_pCombResolutionSec->currentText()));
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
    ui->m_pSBoxFramRateSec->setValue(arg1);
    m_bVideoSettingChanged = true;
}

void SettingUI::on_m_pCombDirectorListenDevices_currentIndexChanged(int index)
{
    m_bVideoSettingChanged = true;
}

void SettingUI::on_m_pCombAudioOutputDevices_currentIndexChanged(int index)
{
    m_bVideoSettingChanged = true;
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

void SettingUI::on_m_pCheckUseSecLive_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckRecord_clicked()
{
    m_bLiveSetttingChanged = true;

}

bool SettingUI::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->m_pCombResolution || watched == ui->m_pCombResolutionSec)
    {
        if(event->type() == QEvent::InputMethod)
        {
            QInputMethodEvent* iEvent = dynamic_cast<QInputMethodEvent*> (event);
            iEvent->setCommitString("");
            return true;
         }
         else if(event->type() == QEvent::KeyPress)
         {
             QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
             if((keyEvent->key() >= Qt::Key_0  && keyEvent->key() <= Qt::Key_9) || keyEvent->key() == Qt::Key_X || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)
             {
                 return QDialog::eventFilter(watched, event);
             }
         }
         else if(event->type() == QEvent::KeyRelease)
         {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if((keyEvent->key() >= Qt::Key_0  &&  keyEvent->key() <= Qt::Key_9) || keyEvent->key() == Qt::Key_X || keyEvent->key() == Qt::Key_Backspace || keyEvent->key() == Qt::Key_Delete || keyEvent->key() == Qt::Key_Left || keyEvent->key() == Qt::Key_Right)
            {
                if(watched == ui->m_pCombResolution)
                    ui->m_pLbResolutionScal->setText(calcScalText(ui->m_pCombResolution->currentText()));
                else if(watched == ui->m_pCombResolutionSec)
                    ui->m_pLbResolutionScalSec->setText(calcScalText(ui->m_pCombResolutionSec->currentText()));
                return QDialog::eventFilter(watched,event);
            }
         }
         else
         {
             return false;
         }
    }
    else
    {
         return QDialog::eventFilter(watched, event);
    }
}

//根据传入的分辨率文本返回比例文本，例如：1280x720 -> 16 : 9
QString SettingUI::calcScalText(const QString& strScal)
{
    if(1 != strScal.count('x'))
    {
        return tr("Format error");
    }
    else
    {
        int xIndex = strScal.indexOf('x');
        QString strW = strScal.mid(0,xIndex);
        QString strH = strScal.mid(xIndex+1);
        int iWidth = strW.toInt();
        int iHeight = strH.toInt();
        int iMaxDivisor = greatCommonDivisor(iWidth,iHeight);
        QString strText = tr("%1 : %2").arg(iWidth/iMaxDivisor).arg(iHeight/iMaxDivisor);
        return strText;
    }
}

void SettingUI::on_m_pCombResolution_currentTextChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
    ui->m_pLbResolutionScal->setText(calcScalText(ui->m_pCombResolution->currentText()));
}

void SettingUI::on_m_pCombResolutionSec_currentTextChanged(const QString &arg1)
{
    m_bLiveSetttingChanged = true;
    ui->m_pLbResolutionScalSec->setText(calcScalText(ui->m_pCombResolutionSec->currentText()));
}

void SettingUI::on_m_pBtnBlackMagic_clicked()
{
    ui->m_pStckWdiget->setCurrentIndex(3);
    ui->m_pBtnBlackMagic->setStyleSheet("QPushButton{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnLiveSet->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                     "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
    ui->m_pBtnImage->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                   "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");

    ui->m_pBtnSenior->setStyleSheet("QPushButton{border-image:url(:images/btn_setting.png);color:rgb(0,0,0);}"
                                    "QPushButton:hover{border-image:url(:images/btn_setting_select.png);color:rgb(255,255,255);}");
}

void SettingUI::on_m_pCheckRecordSec_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckPushUrlSec_clicked()
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCheckBackPushUrlSec_clicked()
{
    m_bLiveSetttingChanged = true;
}
void SettingUI::on_m_pRdBtnH264Software_clicked()
{
    m_bLiveSetttingChanged = true;
    ui->m_pRdBtnCBR->setEnabled(true);
    ui->m_pRdBtnVBR->setEnabled(true);
    ui->m_pCombQuality->setEnabled(true);
    ui->m_pBtnSeniorCodeSet->setEnabled(true);
}

void SettingUI::on_m_pRdBtnH264Hardware_clicked()
{
    m_bLiveSetttingChanged = true;
    ui->m_pRdBtnCBR->setEnabled(false);
    ui->m_pRdBtnVBR->setEnabled(false);
    ui->m_pCombQuality->setEnabled(false);
    ui->m_pBtnSeniorCodeSet->setEnabled(false);
}

void SettingUI::on_m_pCombQuality_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}


void SettingUI::on_m_pRdBtnH264SoftwareSec_clicked()
{
    m_bLiveSetttingChanged = true;
    ui->m_pRdBtnCBRSec->setEnabled(true);
    ui->m_pRdBtnVBRSec->setEnabled(true);
    ui->m_pCombQualitySec->setEnabled(true);
    ui->m_pBtnSeniorCodeSet->setEnabled(true);
}

void SettingUI::on_m_pRdBtnH264HardwareSec_clicked()
{
    m_bLiveSetttingChanged = true;
    ui->m_pRdBtnCBRSec->setEnabled(false);
    ui->m_pRdBtnVBRSec->setEnabled(false);
    ui->m_pCombQualitySec->setEnabled(false);
    ui->m_pBtnSeniorCodeSet->setEnabled(false);
}

void SettingUI::on_m_pRdBtnCBRSec_clicked()
{
    ui->m_pCombQualitySec->clear();
    ui->m_pCombQualitySec->addItem(tr("high performance"));
    ui->m_pCombQualitySec->addItem(tr("high quality"));
    ui->m_pCombQualitySec->setCurrentIndex(m_pParent->m_LiveSettingParam.CBRQualitySec);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pRdBtnVBRSec_clicked()
{
    ui->m_pCombQualitySec->clear();
    for(int i = 0; i <= 10; i++)
    {
        ui->m_pCombQualitySec->addItem(QString("%1").arg(i));
    }
    ui->m_pCombQualitySec->setCurrentIndex(m_pParent->m_LiveSettingParam.VBRQualitySec);
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombQualitySec_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pCombRtmpNodeSec_currentIndexChanged(int index)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pSBoxReconnectTimeoutSec_valueChanged(int arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pSBoxDelaySec_valueChanged(int arg1)
{
    m_bLiveSetttingChanged = true;
}

void SettingUI::on_m_pBtnSeniorCodeSet_clicked()
{
    SeniorCodeSetUI *pSeniorCodeSetUI = new SeniorCodeSetUI(this,true);
    pSeniorCodeSetUI->show();
}


void SettingUI::on_m_pBtnSeniorCodeSetSec_clicked()
{
    SeniorCodeSetUI *pSeniorCodeSetUI = new SeniorCodeSetUI(this,false);
    pSeniorCodeSetUI->show();
}

void SettingUI::on_m_pCombQualitySec_currentIndexChanged(const QString &arg1)
{
    if(tr("high performance") == arg1)
    {
        m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec = 0;
    }
    else if(tr("high quality") == arg1)
    {
        m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec = 2;
    }
}

void SettingUI::on_m_pCombQuality_currentIndexChanged(const QString &arg1)
{
    if(tr("high performance") == arg1)
    {
        m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount = 0;
    }
    else if(tr("high quality") == arg1)
    {
        m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount = 2;
    }
}

void SettingUI::on_m_pSBoxFramRateSec_valueChanged(int arg1)
{
    m_bLiveSetttingChanged = true;
    ui->m_pSBoxFramRateSec->setValue(arg1);
}
void SettingUI::OnBlackMagicChanged()
{
    m_bBlackMagicChanged = true;
}

void SettingUI::on_m_pCombInterlacedScan_currentIndexChanged(int index)
{
    m_bAdvancedChanged = true;
}

//bool SettingUI::nativeEvent(const QByteArray &eventType, void *message, long *result)
//{
//    MSG *msg = static_cast< MSG * >( message );
//    // TODO: filter out or modify msg struct here
//    // ...
//    if(msg->message == WM_NCACTIVATE)
//    {
//        bool active = (bool)(msg->wParam);
//        if (msg->wParam)
//        {
//            qDebug() << "active:" << true;

//        }
//        else
//        {
//            qDebug() << "active:" << false;

//        }

//        emit ncActive(active);
//    }

//    return false;
//}


void SettingUI::on_m_pBtnSeniorDefaultSet_clicked()
{
    ui->m_pCombProgramLevel->setCurrentIndex(0);
    ui->m_pSBoxPicCacheTime->setValue(200);
    ui->m_pCombInterlacedScan->setCurrentIndex(1);
    on_m_pBtnApply_clicked();
}

void SettingUI::on_m_pBtnLiveSetDefaultSet_clicked()
{
    ui->m_pCombResolution->setCurrentText("1280x720");
    ui->m_pSBoxFramRate->setValue(25);
    ui->m_pCombLiveRate->setCurrentText("1200");
    ui->m_pRdBtnH264Software->setChecked(true);
    ui->m_pRdBtnH264Software->click();
    ui->m_pRdBtnH264Hardware->setChecked(false);
    ui->m_pRdBtnCBR->setChecked(true);
    ui->m_pRdBtnVBR->setChecked(false);
    ui->m_pCombQuality->clear();
    ui->m_pCombQuality->addItem(tr("high performance"));
    ui->m_pCombQuality->addItem(tr("high quality"));
    ui->m_pCombQuality->setCurrentIndex(0);

    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.X264Preset,"veryfast");
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.X264Profile,"high");
    m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrame = 1;
    m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount= 0;

    ui->m_pCombSamplingRate->setCurrentText("44.1kHz");
    ui->m_pCombRate->setCurrentText("96kb/s");
    ui->m_pCombSoundChannel->setCurrentIndex(1);
    ui->m_pCombEncoder->setCurrentIndex(1);

    ui->m_pLdtLivePushUrl->setText(m_pParent->m_LiveSettingParam.liveSettingParam.LivePushUrl);
    ui->m_pLdtLiveBackPushUrl->setText(QString(m_pParent->m_pChannelInfo->pPushUrl)+"_back");
    ui->m_pCheckPushUrl->setChecked(true);
    ui->m_pCheckBackPushUrl->setChecked(false);
    ui->m_pCombRtmpNode->setCurrentIndex(0);
    ui->m_pSBoxReconnectTimeout->setValue(10);
    ui->m_pSBoxDelay->setValue(0);

    ui->m_pCheckRecord->setChecked(true);
    ui->m_pLdtRecordFilePath->setText("Videos/.flv");
    ui->m_pRdBtnMP4->setChecked(false);
    ui->m_pRdBtnFLV->setChecked(true);
    on_m_pBtnApply_clicked();
}

void SettingUI::on_m_pBtnLiveSetSecDefaultSet_clicked()
{
    ui->m_pCheckUseSecLive->setChecked(false);
    SetSecondaryLiveEnabled(false);
    ui->m_pCombResolutionSec->setCurrentText("1280x720");
//    ui->m_pSBoxFramRateSec->setValue(25);
    ui->m_pCombLiveRateSec->setCurrentText("1200");
    ui->m_pRdBtnH264SoftwareSec->setChecked(true);
    ui->m_pRdBtnH264HardwareSec->setChecked(false);
    ui->m_pRdBtnCBRSec->setChecked(true);
    ui->m_pRdBtnVBRSec->setChecked(false);
    ui->m_pCombQualitySec->clear();
    ui->m_pCombQualitySec->addItem(tr("high performance"));
    ui->m_pCombQualitySec->addItem(tr("high quality"));
    ui->m_pCombQualitySec->setCurrentIndex(0);

    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.X264PresetSec,"veryfast");
    strcpy(m_pParent->m_LiveSettingParam.liveSettingParam.X264ProfileSec,"high");
    m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrameSec = 1;
    m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec= 0;

//    ui->m_pCombSamplingRateSec->setCurrentText("44100");
//    ui->m_pCombRateSec->setCurrentText("96kb/s");
//    ui->m_pCombSoundChannelSec->setCurrentIndex(1);
//    ui->m_pCombEncoderSec->setCurrentIndex(1);

    ui->m_pLdtLivePushUrlSec->setText("");
    ui->m_pLdtLiveBackPushUrlSec->setText("");
    ui->m_pCheckPushUrlSec->setChecked(true);
    ui->m_pCheckBackPushUrlSec->setChecked(false);
    ui->m_pCombRtmpNodeSec->setCurrentIndex(0);
    ui->m_pSBoxReconnectTimeoutSec->setValue(10);
    ui->m_pSBoxDelaySec->setValue(0);

    ui->m_pCheckRecordSec->setChecked(true);
    ui->m_pLdtRecordFilePathSec->setText("Videos/.flv");
    ui->m_pRdBtnMP4Sec->setChecked(false);
    ui->m_pRdBtnFLVSec->setChecked(true);
    on_m_pBtnApply_clicked();
}

void SettingUI::on_m_pBtnDeviceDefaultSet_clicked()
{
    ui->m_pCombDisplayDevice->setCurrentIndex(0);
    ui->m_pCombDirectorListenDevices->setCurrentIndex(1);
    ui->m_pCombAudioOutputDevices->setCurrentIndex(1);
    on_m_pBtnApply_clicked();
}
