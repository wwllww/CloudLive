/****************************************************************************
*   日期:  2014-12-5
*   目的:  读取配置文件的信息，以map的形式存入
*   要求:  配置文件的格式，以#作为行注释，配置的形式是key = value，中间可有空
格，也可没有空格
*****************************************************************************/

#ifndef _GET_CONFIG_H_
#define _GET_CONFIG_H_
#pragma once
#ifdef DEBUG
#include "comLib/IniFile.h"
#else
#include "IniFile.h"
#endif


#ifdef DEBUG
#include "comLib/Mutex.h"
#else
#include "Mutex.h"
#endif


#include <set>
#include <list>
#include <QSettings>

using namespace std;

class ConfigOper
{
public:
    static ConfigOper* instance();
    static void destroy_instance();
    static bool is_instance_exit();
    ~ConfigOper();

private:
    ConfigOper();
    static ConfigOper* m_pInstance;
    static BUTELCONNECT::Mutex m_ins_mutex;

    //BUTELCONNECT::IniFile m_cfgFile;
    QSettings*   m_pSettingConf;

public:
    bool ReadCfgFile(bool bFirst = TRUE);
    void WriteCfgFile();

public:
    //登录参数
    QString m_UseName;
    QString m_StrUrl;
    QString m_UpLoadUrl;
    QString m_UpLoadAppId;
    QString m_Appkey;

    //直播设定
/*
    int       m_FPS;
    int       m_AudioSampleRate;
    int       m_AudioBitRate;
    int       m_AudioChannel;
    int       m_AudioEncoderType;

    int       m_Width;
    int       m_Height;
    int       m_VideoBitRate;
    bool      m_bH264Hardware;
    bool      m_bUseCBR;
    bool      m_bUseVBR;
    int       m_CBRQuality;
    int       m_VBRQuality;

    QString   m_X264Preset;
    QString   m_X264Profile;
    int       m_KeyFrame;
    int       m_BFrameCount;

    bool      m_bUsePush;
    bool      m_bUseBackPush;
    QString   m_LivePushUrl;
    QString   m_LiveBackPushUrl;
    bool      m_bRecoder;
    QString   m_RecoderPath;
    int       m_FileType;
    int       m_AutoConnect;
    int       m_DelayTime;

    bool      m_bUseLiveSec;
    int       m_WidthSec;
    int       m_HeightSec;
    int       m_VideoBitRateSec;
    bool      m_bH264HardwareSec;
    bool      m_bUseCBRSec;
    bool      m_bUseVBRSec;
    int       m_CBRQualitySec;
    int       m_VBRQualitySec;

    QString   m_X264PresetSec;
    QString   m_X264ProfileSec;
    int       m_KeyFrameSec;
    int       m_BFrameCountSec;

    bool      m_bUsePushSec;
    bool      m_bUseBackPushSec;
    QString    m_LivePushUrlSec;
    QString    m_LiveBackPushUrlSec;
    bool      m_bRecoderSec;
    QString   m_RecoderPathSec;
    int       m_FileTypeSec;
    int       m_AutoConnectSec;
    int       m_DelayTimeSec;

    */

    //设备
    int      m_AdpterID;
    QString   m_MonitorDevice;
    QString   m_ScrProDevice;

    //高级
    bool      m_bUseMultiThread;
    int       m_PriorityID;
    int       m_BufferTime;
    int       m_InterlacedScan;


    int       m_LogLevel;
    bool      m_bSDIOutput;

    bool      m_bSDIDev1;
    bool      m_bSDIDev2;
    bool      m_bSDIDev3;
    bool      m_bSDIDev4;

    bool      m_bSDIDevInput1;
    bool      m_bSDIDevInput2;
    bool      m_bSDIDevInput3;
    bool      m_bSDIDevInput4;

    QString   m_AnotherName1;
    QString   m_AnotherName2;
    QString   m_AnotherName3;
    QString   m_AnotherName4;

};

#endif

