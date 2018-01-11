#include <QDialog>
#include <QLabel>
#include "SeniorCodeSetUI.h"
#include "SettingUI.h"
#include "configoper.h"
//enum {ON_BUTTON_OK = 1,ON_BUTTON_CANCEL};

SeniorCodeSetUI::SeniorCodeSetUI(QDialog *parent,bool bFirstLive) :
    QDialog(parent),
    ui(new Ui::SeniorCodeSetUI)
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    setModal(true);
    m_bFirstLive = bFirstLive;
    m_pParent = (SettingUI*)parent;
    ui->m_pTitleLbl->m_IsChildWnd = true;
    ui->m_pTitleLbl->m_pMinimizeButton->hide();
    ui->m_pTitleLbl->m_pMaximizeButton->hide();
    ui->m_pTitleLbl->m_pMenuButton->hide();
    ui->m_pTitleLbl->m_pCloseButton->setStyleSheet("QPushButton{border-image:url(:images/nav_close.png);}"
                                                   "QPushButton:hover{border-image:url(:images/nav_close_hover.png);}");
    ui->m_pTitleLbl->m_pTitleLabel->setStyleSheet("QLabel{color:#ffffff;font:11pt;}");
    ui->m_pTitleLbl->m_pIconLabel->hide();
    ui->m_pTitleLbl->m_pTitleLabel->setText(tr("SeniorCodeSetUI"));

    if(bFirstLive)
    {

        ui->m_pCombCPUPreset->setCurrentText(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264Preset);
        ui->m_pCombCodeCfgFile->setCurrentText(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264Profile);
        ui->m_pSBoxKeyFrameInterval->setValue(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrame);
        ui->m_pSBoxBFrameNum->setValue(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount);
    }
    else
    {
        ui->m_pCombCPUPreset->setCurrentText(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264PresetSec);
        ui->m_pCombCodeCfgFile->setCurrentText(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264ProfileSec);
        ui->m_pSBoxKeyFrameInterval->setValue(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrameSec);
        ui->m_pSBoxBFrameNum->setValue(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec);
    }
}

SeniorCodeSetUI::~SeniorCodeSetUI()
{
    delete ui;
}

void SeniorCodeSetUI::on_m_pBtnOk_clicked()
{
    if(m_bFirstLive)
    {
        m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCount = ui->m_pSBoxBFrameNum->value();
        m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrame = ui->m_pSBoxKeyFrameInterval->value();
        strcpy(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264Preset,ui->m_pCombCPUPreset->currentText().toLocal8Bit().data());
        strcpy(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264Profile,ui->m_pCombCodeCfgFile->currentText().toLocal8Bit().data()) ;
    }
    else
    {
        m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.BFrameCountSec = ui->m_pSBoxBFrameNum->value();
        m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.KeyFrameSec = ui->m_pSBoxKeyFrameInterval->value();
        strcpy(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264PresetSec,ui->m_pCombCPUPreset->currentText().toLocal8Bit().data());
        strcpy(m_pParent->m_pParent->m_LiveSettingParam.liveSettingParam.X264ProfileSec,ui->m_pCombCodeCfgFile->currentText().toLocal8Bit().data()) ;
    }
    close();
}

void SeniorCodeSetUI::on_m_pBtnCancel_clicked()
{
    close();
}


void SeniorCodeSetUI::on_m_pCombCPUPreset_currentIndexChanged(int index)
{
    m_pParent->m_bLiveSetttingChanged = true;
}

void SeniorCodeSetUI::on_m_pCombCodeCfgFile_activated(const QString &arg1)
{
    m_pParent->m_bLiveSetttingChanged = true;
}

void SeniorCodeSetUI::on_m_pSBoxKeyFrameInterval_valueChanged(int arg1)
{
    m_pParent->m_bLiveSetttingChanged = true;
}

void SeniorCodeSetUI::on_m_pSBoxBFrameNum_valueChanged(int arg1)
{
    m_pParent->m_bLiveSetttingChanged = true;
}
