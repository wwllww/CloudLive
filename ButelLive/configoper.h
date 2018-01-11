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

    //配置设备参数
//    QString m_InputAudio;
//    QString m_InputVideo;
//    QString m_OutputAudio;
//    QString m_OutputVideo;
//    bool    m_bUseCamera;
//    bool    m_bUseMic;
    int     m_EncodeFormat; //分辨率

    //编码
    bool      m_bUseCBR;
    bool      m_bUseVBR;
    bool      m_bUsePadding;
    int       m_VideEncoderType;
    int       m_Quality;
    int       m_AudioEncoderType;
    int       m_AudioBitRate;
    int       m_AudioSampleRate;
    int       m_AudioChannel;

    //直播设定
    bool      m_bCustom;
    bool      m_bUsePush;
    bool      m_bUseBackPush;
    bool      m_bRecoder;
    int       m_Width;
    int       m_Height;
    int       m_VideoBitRate;
    QString   m_LivePushUrl;
    QString   m_LiveBackPushUrl;
    QString   m_RecoderPath;
    int       m_FileType;
    bool      m_bCustomSec;
    bool      m_bUseLiveSec;
    bool      m_bUsePushSec;
    bool      m_bUseBackPushSec;
    bool      m_bRecoderSec;
    int       m_WidthSec;
    int       m_HeightSec;
    int       m_VideoBitRateSec;
    QString   m_LivePushUrlSec;
    QString   m_LiveBackPushUrlSec;
    QString   m_RecoderPathSec;
    int       m_FileTypeSec;
    int       m_AutoConnect;
    int       m_DelayTime;

    //影像
    int      m_AdpterID;
    int      m_FPS;

    //音效
    QString   m_MonitorDevice;
    QString   m_ScrProDevice;

    //高级
    bool      m_bUseMultiThread;
    int       m_PriorityID;
    int       m_BufferTime;
    QString   m_X264Preset;
    QString   m_X264Profile;
    int       m_KeyFrame;
    int       m_BFrameCount;

};

#endif

