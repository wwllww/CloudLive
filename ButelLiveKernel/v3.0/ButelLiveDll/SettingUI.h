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
    void InitLiveSetting();
    void InitDeviceSetting();
    void InitAdvanced();
    void InitBlackMagic();
    QString calcScalText(const QString& strScal);

    ButelLive*     m_pParent;

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

    void on_m_pBtnModifyLocation_clicked();

    void on_m_pBtnOpen_clicked();

    void on_m_pRdBtnMP4_clicked();

    void on_m_pRdBtnFLV_clicked();

    void on_m_pBtnModifyLocationSec_clicked();

    void on_m_pBtnOpenSec_clicked();

    void on_m_pRdBtnMP4Sec_clicked();

    void on_m_pRdBtnFLVSec_clicked();

    void on_m_pCheckUseSecLive_clicked(bool checked);

    void on_m_pCombRtmpNode_currentIndexChanged(int index);

    void on_m_pBtnApply_clicked();

    void on_m_pCombEncoder_currentIndexChanged(int index);

    void on_m_pCombRate_currentIndexChanged(int index);

    void on_m_pCombSamplingRate_currentIndexChanged(int index);

    void on_m_pCombSoundChannel_currentIndexChanged(int index);

    void on_m_pCombResolution_currentIndexChanged(int index);

    void on_m_pCombLiveRate_currentIndexChanged(int index);

    void on_m_pLdtLivePushUrl_textChanged(const QString &arg1);

    void on_m_pLdtLiveBackPushUrl_textChanged(const QString &arg1);

    void on_m_pCheckPushUrl_clicked();

    void on_m_pCheckBackPushUrl_clicked();

    void on_m_pLdtRecordFilePath_textChanged(const QString &arg1);

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

    void on_m_pCheckUseSecLive_clicked();

    void on_m_pCheckRecord_clicked();
    void on_m_pCombResolution_currentTextChanged(const QString &arg1);

    void on_m_pCombResolutionSec_currentTextChanged(const QString &arg1);


    void on_m_pBtnBlackMagic_clicked();

    void on_m_pCheckRecordSec_clicked();

    void on_m_pCheckPushUrlSec_clicked();

    void on_m_pCheckBackPushUrlSec_clicked();

    void on_m_pRdBtnH264Software_clicked();

    void on_m_pRdBtnH264Hardware_clicked();

    void on_m_pCombQuality_currentIndexChanged(int index);

    void on_m_pRdBtnH264SoftwareSec_clicked();

    void on_m_pRdBtnH264HardwareSec_clicked();

    void on_m_pRdBtnCBRSec_clicked();

    void on_m_pRdBtnVBRSec_clicked();

    void on_m_pCombQualitySec_currentIndexChanged(int index);

    void on_m_pCombRtmpNodeSec_currentIndexChanged(int index);

    void on_m_pSBoxReconnectTimeoutSec_valueChanged(int arg1);

    void on_m_pSBoxDelaySec_valueChanged(int arg1);

    void on_m_pBtnSeniorCodeSet_clicked();

    void on_m_pBtnSeniorCodeSetSec_clicked();

    void on_m_pCombQualitySec_currentIndexChanged(const QString &arg1);

    void on_m_pCombQuality_currentIndexChanged(const QString &arg1);

    void on_m_pSBoxFramRateSec_valueChanged(int arg1);
    void on_m_pCombInterlacedScan_currentIndexChanged(int index);

    void on_m_pBtnSeniorDefaultSet_clicked();

    void on_m_pBtnLiveSetDefaultSet_clicked();

    void on_m_pBtnLiveSetSecDefaultSet_clicked();

    void on_m_pBtnDeviceDefaultSet_clicked();

public slots:
    void OnBlackMagicChanged();
protected:
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::SettingUI *ui;


public:
    bool           m_bEncoderChanged;
    bool           m_bLiveSetttingChanged;
    bool           m_bVideoSettingChanged;
    bool           m_bAudioSettingChanged;
    bool           m_bAdvancedChanged;
    bool           m_bBlackMagicChanged;
};

#endif // SETTINGUI_H
