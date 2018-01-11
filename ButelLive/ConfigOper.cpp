#include "ConfigOper.h"
#include <tchar.h>
#pragma warning(disable:4996)
ConfigOper* ConfigOper::m_pInstance = NULL;
BUTELCONNECT::Mutex ConfigOper::m_ins_mutex;
ConfigOper::ConfigOper()
{
    m_pSettingConf = new QSettings("BLive.ini", QSettings::IniFormat);
}
ConfigOper::~ConfigOper()
{

}
ConfigOper* ConfigOper::instance()
{
    if (NULL == m_pInstance)
    {
        BUTELCONNECT::SingleMutex s(m_ins_mutex);
        if (NULL == m_pInstance)
        {
            m_pInstance = new ConfigOper;
        }
    }

    return m_pInstance;
}

void ConfigOper::destroy_instance()
{
    if (NULL != m_pInstance)
    {
        BUTELCONNECT::SingleMutex s(m_ins_mutex);
        if (NULL != m_pInstance)
        {
            delete m_pInstance;
            m_pInstance = NULL;
        }
    }
}
bool ConfigOper::is_instance_exit()
{
    BUTELCONNECT::SingleMutex s(m_ins_mutex);
    return m_pInstance!=NULL;
}

bool ConfigOper::ReadCfgFile(bool bFirst)
{
    bool bRet = false;
    if(m_pSettingConf)
    {
        m_UseName = m_pSettingConf->value("/Login/Account").toString();
        m_StrUrl = m_pSettingConf->value("/Login/UrlPre").toString();
//        m_UpLoadUrl = m_pSettingConf->value("/Login/FileRequestUrl").toString();
//        m_UpLoadAppId = m_pSettingConf->value("/Login/FileRequestAppId").toString();

//        m_InputAudio = m_pSettingConf->value("/input/Audio").toString();
//        m_InputVideo = m_pSettingConf->value("/input/Video").toString();
//        m_OutputAudio = m_pSettingConf->value("/Output/Audio").toString();
//        m_OutputVideo = m_pSettingConf->value("/Output/Video").toString();
//        m_bUseCamera  = m_pSettingConf->value("/input/UseCamera").toBool();
//        m_bUseMic  =    m_pSettingConf->value("/input/UseMic").toBool();
        m_EncodeFormat = m_pSettingConf->value("/encode/EncodeFormat").toInt();

        m_bUseCBR = m_pSettingConf->value("/Encoder/bUseCBR").toBool();
        m_bUseVBR = m_pSettingConf->value("/Encoder/bUseVBR").toBool();
        m_bUsePadding = m_pSettingConf->value("/Encoder/bUsePadding").toBool();
        m_VideEncoderType = m_pSettingConf->value("/Encoder/VideEncoderType").toInt();
        m_Quality = m_pSettingConf->value("/Encoder/Quality").toInt();
        m_AudioEncoderType = m_pSettingConf->value("/Encoder/AudioEncoderType").toInt();
        m_AudioBitRate = m_pSettingConf->value("/Encoder/AudioBitRate").toInt();
        m_AudioSampleRate = m_pSettingConf->value("/Encoder/AudioSampleRate").toInt();
        m_AudioChannel = m_pSettingConf->value("/Encoder/AudioChannel").toInt();

        m_bCustom = m_pSettingConf->value("/LiveSetting/bCustom").toBool();
        m_bUsePush = m_pSettingConf->value("/LiveSetting/bUsePush").toBool();
        m_bUseBackPush = m_pSettingConf->value("/LiveSetting/bUseBackPush").toBool();
        m_bRecoder = m_pSettingConf->value("/LiveSetting/bRecoder").toBool();
        m_Width = m_pSettingConf->value("/LiveSetting/Width").toInt();
        m_Height = m_pSettingConf->value("/LiveSetting/Height").toInt();
        m_VideoBitRate = m_pSettingConf->value("/LiveSetting/VideoBitRate").toInt();
        m_LivePushUrl = m_pSettingConf->value("/LiveSetting/LivePushUrl").toString();
        m_LiveBackPushUrl = m_pSettingConf->value("/LiveSetting/LiveBackPushUrl").toString();
        m_RecoderPath = m_pSettingConf->value("/LiveSetting/RecoderPath").toString();
        m_FileType = m_pSettingConf->value("/LiveSetting/FileType").toInt();
        m_bCustomSec = m_pSettingConf->value("/LiveSetting/bCustomSec").toBool();
        m_bUseLiveSec = m_pSettingConf->value("/LiveSetting/bUseLiveSec").toBool();
        m_bUsePushSec = m_pSettingConf->value("/LiveSetting/bUsePushSec").toBool();
        m_bUseBackPushSec = m_pSettingConf->value("/LiveSetting/bUseBackPushSec").toBool();
        m_bRecoderSec = m_pSettingConf->value("/LiveSetting/bRecoderSec").toBool();
        m_WidthSec = m_pSettingConf->value("/LiveSetting/WidthSec").toInt();
        m_HeightSec = m_pSettingConf->value("/LiveSetting/HeightSec").toInt();
        m_VideoBitRateSec = m_pSettingConf->value("/LiveSetting/VideoBitRateSec").toInt();
        m_LivePushUrlSec = m_pSettingConf->value("/LiveSetting/LivePushUrlSec").toString();
        m_LiveBackPushUrlSec = m_pSettingConf->value("/LiveSetting/LiveBackPushUrlSec").toString();
        m_RecoderPathSec = m_pSettingConf->value("/LiveSetting/RecoderPathSec").toString();
        m_FileTypeSec = m_pSettingConf->value("/LiveSetting/FileTypeSec").toInt();
        m_AutoConnect = m_pSettingConf->value("/LiveSetting/AutoConnect").toInt();
        m_DelayTime = m_pSettingConf->value("/LiveSetting/DelayTime").toInt();

        m_AdpterID = m_pSettingConf->value("/VideoSetting/AdpterID").toInt();
        m_FPS = m_pSettingConf->value("/VideoSetting/FPS").toInt();

        m_MonitorDevice = m_pSettingConf->value("/AudioSetting/MonitorDevice").toString();
        m_ScrProDevice = m_pSettingConf->value("/AudioSetting/ScrProDevice").toString();

        m_bUseMultiThread = m_pSettingConf->value("/Advanced/bUseMultiThread").toBool();
        m_PriorityID = m_pSettingConf->value("/Advanced/PriorityID").toBool();
        m_BufferTime = m_pSettingConf->value("/Advanced/BufferTime").toInt();
        m_X264Preset = m_pSettingConf->value("/Advanced/X264Preset").toString();
        m_X264Profile = m_pSettingConf->value("/Advanced/X264Profile").toString();
        m_KeyFrame = m_pSettingConf->value("/Advanced/KeyFrame").toInt();
        m_BFrameCount = m_pSettingConf->value("/Advanced/BFrameCount").toInt();

        bRet = true;
    }
    return bRet;
}
void ConfigOper::WriteCfgFile()
{
    m_pSettingConf->setValue("/Login/Account", m_UseName);

//    m_pSettingConf->setValue("/input/Audio", m_InputAudio);
//    m_pSettingConf->setValue("/input/Video", m_InputVideo);
//    m_pSettingConf->setValue("/Output/Audio", m_OutputAudio);
//    m_pSettingConf->setValue("/Output/Video", m_OutputVideo);
//    m_pSettingConf->setValue("/input/UseCamera", m_bUseCamera);
//    m_pSettingConf->setValue("/input/UseMic", m_bUseMic);
    m_pSettingConf->setValue("/encode/EncodeFormat", m_EncodeFormat);

    m_pSettingConf->setValue("/Encoder/bUseCBR",m_bUseCBR);
    m_pSettingConf->setValue("/Encoder/bUseVBR",m_bUseVBR);
    m_pSettingConf->setValue("/Encoder/bUsePadding",m_bUsePadding);
    m_pSettingConf->setValue("/Encoder/VideEncoderType",m_VideEncoderType);
    m_pSettingConf->setValue("/Encoder/Quality",m_Quality);
    m_pSettingConf->setValue("/Encoder/AudioEncoderType",m_AudioEncoderType);
    m_pSettingConf->setValue("/Encoder/AudioBitRate",m_AudioBitRate);
    m_pSettingConf->setValue("/Encoder/AudioSampleRate",m_AudioSampleRate);
    m_pSettingConf->setValue("/Encoder/AudioChannel",m_AudioChannel);

    m_pSettingConf->setValue("/LiveSetting/bUsePush",m_bUsePush);
    m_pSettingConf->setValue("/LiveSetting/bUseBackPush",m_bUseBackPush);
    m_pSettingConf->setValue("/LiveSetting/bRecoder",m_bRecoder);
    m_pSettingConf->setValue("/LiveSetting/Width",m_Width);
    m_pSettingConf->setValue("/LiveSetting/Height",m_Height);
    m_pSettingConf->setValue("/LiveSetting/VideoBitRate",m_VideoBitRate);
    m_pSettingConf->setValue("/LiveSetting/LivePushUrl",m_LivePushUrl);
    m_pSettingConf->setValue("/LiveSetting/LiveBackPushUrl",m_LiveBackPushUrl);
    m_pSettingConf->setValue("/LiveSetting/RecoderPath",m_RecoderPath);
    m_pSettingConf->setValue("/LiveSetting/FileType",m_FileType);
    m_pSettingConf->setValue("/LiveSetting/bCustom",m_bCustom);
    m_pSettingConf->setValue("/LiveSetting/bUseLiveSec",m_bUseLiveSec);
    m_pSettingConf->setValue("/LiveSetting/bUsePushSec",m_bUsePushSec);
    m_pSettingConf->setValue("/LiveSetting/bUseBackPushSec",m_bUseBackPushSec);
    m_pSettingConf->setValue("/LiveSetting/bRecoderSec",m_bRecoderSec);
    m_pSettingConf->setValue("/LiveSetting/WidthSec",m_WidthSec);
    m_pSettingConf->setValue("/LiveSetting/HeightSec",m_HeightSec);
    m_pSettingConf->setValue("/LiveSetting/VideoBitRateSec",m_VideoBitRateSec);
    m_pSettingConf->setValue("/LiveSetting/LivePushUrlSec",m_LivePushUrlSec);
    m_pSettingConf->setValue("/LiveSetting/LiveBackPushUrlSec",m_LiveBackPushUrlSec);
    m_pSettingConf->setValue("/LiveSetting/RecoderPathSec",m_RecoderPathSec);
    m_pSettingConf->setValue("/LiveSetting/FileTypeSec",m_FileTypeSec);
    m_pSettingConf->setValue("/LiveSetting/bCustomSec",m_bCustomSec);
    m_pSettingConf->setValue("/LiveSetting/AutoConnect",m_AutoConnect);
    m_pSettingConf->setValue("/LiveSetting/DelayTime",m_DelayTime);

    m_pSettingConf->setValue("/VideoSetting/AdpterID",m_AdpterID);
    m_pSettingConf->setValue("/VideoSetting/FPS",m_FPS);

    m_pSettingConf->setValue("/AudioSetting/MonitorDevice",m_MonitorDevice);
    m_pSettingConf->setValue("/AudioSetting/ScrProDevice",m_ScrProDevice);

    m_pSettingConf->setValue("/Advanced/bUseMultiThread",m_bUseMultiThread);
    m_pSettingConf->setValue("/Advanced/PriorityID",m_PriorityID);
    m_pSettingConf->setValue("/Advanced/BufferTime",m_BufferTime);
    m_pSettingConf->setValue("/Advanced/X264Preset",m_X264Preset);
    m_pSettingConf->setValue("/Advanced/X264Profile",m_X264Profile);
    m_pSettingConf->setValue("/Advanced/KeyFrame",m_KeyFrame);
    m_pSettingConf->setValue("/Advanced/BFrameCount",m_BFrameCount);

}
