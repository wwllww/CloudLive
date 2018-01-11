#ifndef SETTINGUI_H
#define SETTINGUI_H
#include <QDialog>
#include "ui_SettingUI.h"
#include "ButelLive.h"


namespace Ui {
class SettingUI;
}
class SettingUI: public QDialog
{
    Q_OBJECT
public:
    SettingUI(QDialog *parent = 0);
    ~SettingUI();
    void SetSecondaryLiveEnabled(bool bEnabled);
    void InitEncoder();
    void InitLiveSetting();
    void InitVideoSetting();
    void InitAudioSetting();
    void InitAdvanced();
private slots:
    void on_m_pBtnOk_clicked();

    void on_m_pBtnCancel_clicked();

    void on_m_pBtnCode_clicked();

    void on_m_pBtnLiveSet_clicked();

    void on_m_pBtnImage_clicked();

    void on_m_pBtnSound_clicked();

    void on_m_pBtnSenior_clicked();

    void on_m_pRdBtnCBR_clicked();

    void on_m_pRdBtnVBR_clicked();

    void on_m_pRdBtnH264_clicked();

    void on_m_pBtnModifyLocation_clicked();

    void on_m_pBtnOpen_clicked();

    void on_m_pRdBtnMP4_clicked();

    void on_m_pRdBtnFLV_clicked();

    void on_m_pBtnModifyLocationSec_clicked();

    void on_m_pBtnOpenSec_clicked();

    void on_m_pRdBtnMP4Sec_clicked();

    void on_m_pRdBtnFLVSec_clicked();

    void on_m_pCheckUseSecLive_clicked(bool checked);

    void on_m_pCheckCustom_clicked(bool checked);

    void on_m_pCombRtmpNode_currentIndexChanged(int index);

    void on_m_pBtnApply_clicked();

    void on_m_pCombDefinition_currentIndexChanged(int index);

    void on_m_pCombDefinitionSec_currentIndexChanged(int index);

    void on_m_pCombEncoder_currentIndexChanged(int index);

    void on_m_pCombRate_currentIndexChanged(int index);

    void on_m_pCombSamplingRate_currentIndexChanged(int index);

    void on_m_pCombSoundChannel_currentIndexChanged(int index);

    void on_m_pCheckCBRPadding_clicked();

    void on_m_pCheckCustom_clicked();

    void on_m_pCombResolution_currentIndexChanged(int index);

    void on_m_pCombLiveRate_currentIndexChanged(int index);

    void on_m_pLdtLivePushUrl_textChanged(const QString &arg1);

    void on_m_pLdtLiveBackPushUrl_textChanged(const QString &arg1);

    void on_m_pCheckPushUrl_clicked();

    void on_m_pCheckBackPushUrl_clicked();

    void on_m_pLdtRecordFilePath_textChanged(const QString &arg1);

    void on_m_pCheckCustomSec_clicked();

    void on_m_pCombResolutionSec_currentIndexChanged(int index);

    void on_m_pCombLiveRateSec_currentIndexChanged(int index);

    void on_m_pLdtLivePushUrlSec_textChanged(const QString &arg1);

    void on_m_pLdtLiveBackPushUrlSec_textChanged(const QString &arg1);

    void on_m_pLdtRecordFilePathSec_textChanged(const QString &arg1);

    void on_m_pSBoxReconnectTimeout_valueChanged(int arg1);

    void on_m_pSBoxDelay_valueChanged(int arg1);

    void on_m_pCombDisplayDevice_currentIndexChanged(int index);

    void on_m_pSBoxFramRate_valueChanged(int arg1);

    void on_m_pCombDirectorListenDevices_currentIndexChanged(int index);

    void on_m_pCombAudioOutputDevices_currentIndexChanged(int index);

    void on_m_pCheckUseMultithread_clicked();

    void on_m_pCombProgramLevel_currentIndexChanged(int index);

    void on_m_pSBoxPicCacheTime_valueChanged(int arg1);

    void on_m_pCombCPUPreset_currentIndexChanged(int index);

    void on_m_pCombCodeCfgFile_currentIndexChanged(int index);

    void on_m_pSBoxKeyFrameInterval_valueChanged(int arg1);

    void on_m_pSBoxBFrameNum_valueChanged(int arg1);

    void on_m_pCheckUseSecLive_clicked();

    void on_m_pCheckRecord_clicked();

private:
    Ui::SettingUI *ui;
    ButelLive*     m_pParent;

    bool           m_bEncoderChanged;
    bool           m_bLiveSetttingChanged;
    bool           m_bVideoSettingChanged;
    bool           m_bAudioSettingChanged;
    bool           m_bAdvancedChanged;
};

#endif // SETTINGUI_H
