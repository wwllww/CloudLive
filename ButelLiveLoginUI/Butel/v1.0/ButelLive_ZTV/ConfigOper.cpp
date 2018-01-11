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
        m_UpLoadUrl = m_pSettingConf->value("/Login/FileRequestUrl").toString();
        m_UpLoadAppId = m_pSettingConf->value("/Login/FileRequestAppId").toString();
        m_Appkey = m_pSettingConf->value("/Login/Appkey").toString();


        m_LogLevel = m_pSettingConf->value("/Output/LogLevel").toInt();
        m_bSDIOutput = m_pSettingConf->value("/Output/bSDIOutput").toBool();

        m_bSDIDev1 = m_pSettingConf->value("/Output/bSDIDev1").toBool();
        m_bSDIDev2 = m_pSettingConf->value("/Output/bSDIDev2").toBool();
        m_bSDIDev3 = m_pSettingConf->value("/Output/bSDIDev3").toBool();
        m_bSDIDev4 = m_pSettingConf->value("/Output/bSDIDev4").toBool();

        m_bSDIDevInput1 = m_pSettingConf->value("/Output/bSDIDevInput1").toBool();
        m_bSDIDevInput2 = m_pSettingConf->value("/Output/bSDIDevInput2").toBool();
        m_bSDIDevInput3 = m_pSettingConf->value("/Output/bSDIDevInput3").toBool();
        m_bSDIDevInput4 = m_pSettingConf->value("/Output/bSDIDevInput4").toBool();

        m_AnotherName1 = m_pSettingConf->value("/Output/AnotherName1").toString();
        m_AnotherName2 = m_pSettingConf->value("/Output/AnotherName2").toString();
        m_AnotherName3 = m_pSettingConf->value("/Output/AnotherName3").toString();
        m_AnotherName4 = m_pSettingConf->value("/Output/AnotherName4").toString();

/*
        m_FPS = m_pSettingConf->value("/LiveSetting/FPS").toInt();
        m_AudioEncoderType = m_pSettingConf->value("/LiveSetting/AudioEncoderType").toInt();
        m_AudioBitRate = m_pSettingConf->value("/LiveSetting/AudioBitRate").toInt();
        m_AudioSampleRate = m_pSettingConf->value("/LiveSetting/AudioSampleRate").toInt();
        m_AudioChannel = m_pSettingConf->value("/LiveSetting/AudioChannel").toInt();

        m_Width = m_pSettingConf->value("/LiveSetting/Width").toInt();
        m_Height = m_pSettingConf->value("/LiveSetting/Height").toInt();
        m_VideoBitRate = m_pSettingConf->value("/LiveSetting/VideoBitRate").toInt();
        m_bH264Hardware = m_pSettingConf->value("/LiveSetting/bH264Hardware").toBool();
        m_bUseCBR = m_pSettingConf->value("/LiveSetting/bUseCBR").toBool();
        m_bUseVBR = m_pSettingConf->value("/LiveSetting/bUseVBR").toBool();
        m_CBRQuality = m_pSettingConf->value("/LiveSetting/CBRQuality").toInt();
        m_VBRQuality = m_pSettingConf->value("/LiveSetting/VBRQuality").toInt();
        m_X264Preset = m_pSettingConf->value("/LiveSetting/X264Preset").toString();
        m_X264Profile = m_pSettingConf->value("/LiveSetting/X264Profile").toString();
        m_KeyFrame = m_pSettingConf->value("/LiveSetting/KeyFrame").toInt();
        m_BFrameCount = m_pSettingConf->value("/LiveSetting/BFrameCount").toInt();

        m_bUsePush = m_pSettingConf->value("/LiveSetting/bUsePush").toBool();
        m_bUseBackPush = m_pSettingConf->value("/LiveSetting/bUseBackPush").toBool();
        m_LivePushUrl = m_pSettingConf->value("/LiveSetting/LivePushUrl").toString();
        m_LiveBackPushUrl = m_pSettingConf->value("/LiveSetting/LiveBackPushUrl").toString();
        m_AutoConnect = m_pSettingConf->value("/LiveSetting/AutoConnect").toInt();
        m_DelayTime = m_pSettingConf->value("/LiveSetting/DelayTime").toInt();
        m_bRecoder = m_pSettingConf->value("/LiveSetting/bRecoder").toBool();
        m_RecoderPath = m_pSettingConf->value("/LiveSetting/RecoderPath").toString();
        m_FileType = m_pSettingConf->value("/LiveSetting/FileType").toInt();


        m_bUseLiveSec = m_pSettingConf->value("/LiveSetting/bUseLiveSec").toBool();
        m_WidthSec = m_pSettingConf->value("/LiveSetting/WidthSec").toInt();
        m_HeightSec = m_pSettingConf->value("/LiveSetting/HeightSec").toInt();
        m_VideoBitRateSec = m_pSettingConf->value("/LiveSetting/VideoBitRateSec").toInt();
        m_bH264HardwareSec = m_pSettingConf->value("/LiveSetting/bH264HardwareSec").toBool();
        m_bUseCBRSec = m_pSettingConf->value("/LiveSetting/bUseCBRSec").toBool();
        m_bUseVBRSec = m_pSettingConf->value("/LiveSetting/bUseVBRSec").toBool();
        m_CBRQualitySec = m_pSettingConf->value("/LiveSetting/CBRQualitySec").toInt();
        m_CBRQualitySec = m_pSettingConf->value("/LiveSetting/CBRQualitySec").toInt();
        m_X264PresetSec = m_pSettingConf->value("/LiveSetting/X264PresetSec").toString();
        m_X264ProfileSec = m_pSettingConf->value("/LiveSetting/X264ProfileSec").toString();
        m_KeyFrameSec = m_pSettingConf->value("/LiveSetting/KeyFrameSec").toInt();
        m_BFrameCountSec = m_pSettingConf->value("/LiveSetting/BFrameCountSec").toInt();

        m_bUsePushSec = m_pSettingConf->value("/LiveSetting/bUsePushSec").toBool();
        m_bUseBackPushSec = m_pSettingConf->value("/LiveSetting/bUseBackPushSec").toBool();
        m_LivePushUrlSec = m_pSettingConf->value("/LiveSetting/LivePushUrlSec").toString();
        m_LiveBackPushUrlSec = m_pSettingConf->value("/LiveSetting/LiveBackPushUrlSec").toString();
        m_AutoConnectSec = m_pSettingConf->value("/LiveSetting/AutoConnectSec").toInt();
        m_DelayTimeSec = m_pSettingConf->value("/LiveSetting/DelayTimeSec").toInt();
        m_bRecoderSec = m_pSettingConf->value("/LiveSetting/bRecoderSec").toBool();
        m_RecoderPathSec = m_pSettingConf->value("/LiveSetting/RecoderPathSec").toString();
        m_FileTypeSec = m_pSettingConf->value("/LiveSetting/FileTypeSec").toInt();
*/

        m_AdpterID = m_pSettingConf->value("/VideoSetting/AdpterID").toInt();
        m_MonitorDevice = m_pSettingConf->value("/VideoSetting/MonitorDevice").toString();
        m_ScrProDevice = m_pSettingConf->value("/VideoSetting/ScrProDevice").toString();

        m_bUseMultiThread = m_pSettingConf->value("/Advanced/bUseMultiThread").toBool();
        m_PriorityID = m_pSettingConf->value("/Advanced/PriorityID").toBool();
        m_BufferTime = m_pSettingConf->value("/Advanced/BufferTime").toInt();
        m_InterlacedScan = m_pSettingConf->value("/Advanced/InterlacedScan").toInt();
        bRet = true;
    }
    return bRet;
}
void ConfigOper::WriteCfgFile()
{
    m_pSettingConf->setValue("/Login/Account", m_UseName);

    m_pSettingConf->setValue("/Output/LogLevel", m_LogLevel);
    m_pSettingConf->setValue("/Output/bSDIOutput", m_bSDIOutput);

    m_pSettingConf->setValue("/Output/bSDIDev1", m_bSDIDev1);
    m_pSettingConf->setValue("/Output/bSDIDev2", m_bSDIDev2);
    m_pSettingConf->setValue("/Output/bSDIDev3", m_bSDIDev3);
    m_pSettingConf->setValue("/Output/bSDIDev4", m_bSDIDev4);

    m_pSettingConf->setValue("/Output/bSDIDevInput1", m_bSDIDevInput1);
    m_pSettingConf->setValue("/Output/bSDIDevInput2", m_bSDIDevInput2);
    m_pSettingConf->setValue("/Output/bSDIDevInput3", m_bSDIDevInput3);
    m_pSettingConf->setValue("/Output/bSDIDevInput4", m_bSDIDevInput4);

    m_pSettingConf->setValue("/Output/AnotherName1", m_AnotherName1);
    m_pSettingConf->setValue("/Output/AnotherName2", m_AnotherName2);
    m_pSettingConf->setValue("/Output/AnotherName3", m_AnotherName3);
    m_pSettingConf->setValue("/Output/AnotherName4", m_AnotherName4);

//    m_pSettingConf->setValue("/Encoder/bUseCBR",m_bUseCBR);
//    m_pSettingConf->setValue("/Encoder/bUseVBR",m_bUseVBR);
//    m_pSettingConf->setValue("/Encoder/bUsePadding",m_bUsePadding);
//    m_pSettingConf->setValue("/Encoder/VideEncoderType",m_VideEncoderType);

    //[LiveSetting]
    /*
    m_pSettingConf->setValue("/LiveSetting/FPS",m_FPS);
    m_pSettingConf->setValue("/LiveSetting/AudioEncoderType",m_AudioEncoderType);
    m_pSettingConf->setValue("/LiveSetting/AudioBitRate",m_AudioBitRate);
    m_pSettingConf->setValue("/LiveSetting/AudioSampleRate",m_AudioSampleRate);
    m_pSettingConf->setValue("/LiveSetting/AudioChannel",m_AudioChannel);

    m_pSettingConf->setValue("/LiveSetting/Width",m_Width);
    m_pSettingConf->setValue("/LiveSetting/Height",m_Height);
    m_pSettingConf->setValue("/LiveSetting/VideoBitRate",m_VideoBitRate);
    m_pSettingConf->setValue("/LiveSetting/bH264Hardware",m_bH264Hardware);
    m_pSettingConf->setValue("/LiveSetting/bUseCBR",m_bUseCBR);
    m_pSettingConf->setValue("/LiveSetting/bUseVBR",m_bUseVBR);
    m_pSettingConf->setValue("/LiveSetting/CBRQuality",m_CBRQuality);
    m_pSettingConf->setValue("/LiveSetting/VBRQuality",m_VBRQuality);
    m_pSettingConf->setValue("/LiveSetting/X264Preset",m_X264Preset);
    m_pSettingConf->setValue("/LiveSetting/X264Profile",m_X264Profile);
    m_pSettingConf->setValue("/LiveSetting/KeyFrame",m_KeyFrame);
    m_pSettingConf->setValue("/LiveSetting/BFrameCount",m_BFrameCount);

    m_pSettingConf->setValue("/LiveSetting/bUsePush",m_bUsePush);
    m_pSettingConf->setValue("/LiveSetting/bUseBackPush",m_bUseBackPush);
    m_pSettingConf->setValue("/LiveSetting/LivePushUrl",m_LivePushUrl);
    m_pSettingConf->setValue("/LiveSetting/LiveBackPushUrl",m_LiveBackPushUrl);
    m_pSettingConf->setValue("/LiveSetting/AutoConnect",m_AutoConnect);
    m_pSettingConf->setValue("/LiveSetting/DelayTime",m_DelayTime);
    m_pSettingConf->setValue("/LiveSetting/bRecoder",m_bRecoder);
    m_pSettingConf->setValue("/LiveSetting/RecoderPath",m_RecoderPath);
    m_pSettingConf->setValue("/LiveSetting/FileType",m_FileType);

    m_pSettingConf->setValue("/LiveSetting/bUseLiveSec",m_bUseLiveSec);
    m_pSettingConf->setValue("/LiveSetting/WidthSec",m_WidthSec);
    m_pSettingConf->setValue("/LiveSetting/HeightSec",m_HeightSec);
    m_pSettingConf->setValue("/LiveSetting/VideoBitRateSec",m_VideoBitRateSec);
    m_pSettingConf->setValue("/LiveSetting/bH264HardwareSec",m_bH264HardwareSec);
    m_pSettingConf->setValue("/LiveSetting/bUseCBRSec",m_bUseCBRSec);
    m_pSettingConf->setValue("/LiveSetting/bUseVBRSec",m_bUseVBRSec);
    m_pSettingConf->setValue("/LiveSetting/CBRQualitySec",m_CBRQualitySec);
    m_pSettingConf->setValue("/LiveSetting/VBRQualitySec",m_VBRQualitySec);
    m_pSettingConf->setValue("/LiveSetting/X264PresetSec",m_X264PresetSec);
    m_pSettingConf->setValue("/LiveSetting/X264ProfileSec",m_X264ProfileSec);
    m_pSettingConf->setValue("/LiveSetting/KeyFrameSec",m_KeyFrameSec);
    m_pSettingConf->setValue("/LiveSetting/BFrameCountSec",m_BFrameCountSec);

    m_pSettingConf->setValue("/LiveSetting/bUsePushSec",m_bUsePushSec);
    m_pSettingConf->setValue("/LiveSetting/bUseBackPushSec",m_bUseBackPushSec);
    m_pSettingConf->setValue("/LiveSetting/LivePushUrlSec",m_LivePushUrlSec);
    m_pSettingConf->setValue("/LiveSetting/LiveBackPushUrlSec",m_LiveBackPushUrlSec);
    m_pSettingConf->setValue("/LiveSetting/AutoConnectSec",m_AutoConnectSec);
    m_pSettingConf->setValue("/LiveSetting/DelayTimeSec",m_DelayTimeSec);
    m_pSettingConf->setValue("/LiveSetting/bRecoderSec",m_bRecoderSec);
    m_pSettingConf->setValue("/LiveSetting/RecoderPathSec",m_RecoderPathSec);
    m_pSettingConf->setValue("/LiveSetting/FileTypeSec",m_FileTypeSec);
*/
    //[VideoSetting]
    m_pSettingConf->setValue("/VideoSetting/AdpterID",m_AdpterID);
    m_pSettingConf->setValue("/VideoSetting/MonitorDevice",m_MonitorDevice);
    m_pSettingConf->setValue("/VideoSetting/ScrProDevice",m_ScrProDevice);

    //[Advanced]
    m_pSettingConf->setValue("/Advanced/bUseMultiThread",m_bUseMultiThread);
    m_pSettingConf->setValue("/Advanced/PriorityID",m_PriorityID);
    m_pSettingConf->setValue("/Advanced/BufferTime",m_BufferTime);
    m_pSettingConf->setValue("/Advanced/InterlacedScan",m_InterlacedScan);

}
