#pragma once

#include <string>
#include <list>
#include "LogDeliver.h"

#ifndef LOG_SDI
#define LOG_SDI ((long long)1<<49)
#endif // !LOG_RTSPSERV


enum SDIOUT_DISPLAYMODE
{
	NTSC = 0,
	PAL,
	HD1080p2398,
	HD1080p24,
	HD1080p25,
	HD1080p2997,
	HD1080p30,
	HD1080i50,
	HD1080i5994,
	HD1080i6000,
	HD1080p50,
	HD1080p5994,
	HD1080p60,
	HD720p50,
	HD720p5994,
	HD720p60
};

enum SDIOUT_COLORFORMAT
{
	ColorFormat_RGB32 = 0,

	ColorFormat_I420,
	ColorFormat_YV12,

	ColorFormat_YVYU,
	ColorFormat_YUY2,
	ColorFormat_UYVY,
	ColorFormat_HDYC,

	ColorFormat_RGB24,
	ColorFormat_RGBA32REVERSE
};

enum SDIOUT_ERROR
{
	errNone = 0,
	errUnknown = -1,
	errDeckLinkDrivers = -2000,      //创建COM组件失败，没有Decklink驱动
	errIDeckLink,				     //枚举设备失败
	errIDeckLinkOutput,			     //查询输出接口失败
	errDisplayMode,				     //没有找到匹配的显示模式
	errEnableVideoOutput,		     //使能视频输出失败
	errEnableAudioOutput,
	errCreateVideoFrame,		     //不能创建视频帧的接口
	errSDIOutputInstanceAlwaysExist, //SDI输出实例已存在
	errInvalidParameter,		     //无效参数
	errUnsupportedColorFormat,	     //颜色格式不支持
	errCreateSDIOutputInstance,			     //初始化实例失败
	errDeviceAlwaysStart,		     //设备已经启动
	errCOMInit,						 //COM初始化失败
	errCreateOutputCallback,
	errSetOutputCallbackToDevice
};


struct DeviceInfo
{
	int   strID;      //设备ID
	char* strName;    //设备名
};

struct DisplayMode
{
	SDIOUT_DISPLAYMODE id;
	char* strName;   
};

struct SDIID
{
	int id;
	bool enable;
};

typedef std::map<int, bool> mapProperty;
typedef std::map<int, SDIOUT_DISPLAYMODE> mapOutDevicePara;

#define EXTERN_DLLEXPORT extern "C" __declspec(dllexport)

/************************************************************************/
/* 函数名称：    SDI_Init
/* 函数功能：    初始化SDI输出模块
/* 输入参数：    无
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_Init();

/************************************************************************/
/* 函数名称：    SDI_StartOut
/* 函数功能：    在指定设备上，起动SDI输出
/* 输入参数：    nDeviceID --> SDI输出设备ID
/*               nMode     --> 设备的显示模式
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE nMode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime);

/************************************************************************/
/* 函数名称：    SDI_StopOut
/* 函数功能：    在指定设备上，停止SDI输出
/* 输入参数：    nDeviceID --> SDI输出设备ID
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_StopOut(int nDeviceID);

/************************************************************************/
/* 函数名称：    SDI_RenderDevice
/* 函数功能：    在指定设备上，输出一帧视频
/* 输入参数：    nDeviceID    --> SDI输出设备ID
/*               pData        --> 输入图像的指针
/*               nWidth       --> 输入图像的宽度
/*               nHeight      --> 输入图像的高度
/*               nColorFormat --> 输入图像的色彩格式
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_RenderDevice(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT nColorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM);

/************************************************************************/
/* 函数名称：    SDI_unInit
/* 函数功能：    反初始化SDI输出模块
/* 输入参数：    无
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_unInit();

/************************************************************************/
/* 函数名称：    SDI_GetDeviceList
/* 函数功能：    获取可用设备列表
/* 输入参数：    pDevicelist --> 设备列表指针
/*               nDeviceCnt  --> 设备数量
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt);

/************************************************************************/
/* 函数名称：    SDI_GetDisplayModeList
/* 函数功能：    获取制式列表
/* 输入参数：    pModeList --> 设备支持的制式
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_GetDisplayModeList(DisplayMode** pModeList, int* nModeCount);

/************************************************************************/
/* 函数名称：    SDI_ReleaseDeviceList
/* 函数功能：    释放可用设备列表
/* 输入参数：    
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_ReleaseDeviceList();

/************************************************************************/
/* 函数名称：    SDI_ReleaseDisplayModeList
/* 函数功能：    释放制式列表
/* 输入参数：
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_ReleaseDisplayModeList();

/************************************************************************/
/* 函数名称：    SDI_SetDeviceProperty
/* 函数功能：    设置设备输入输出
/* 输入参数：    deviceID --> 设备ID
				 bInput--> 是否输入设备 1：输入， 0：输出
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_SetDeviceProperty(int deviceID, bool bInput);

/************************************************************************/
/* 函数名称：    SDI_SetOutDevicePara
/* 函数功能：    设置设备制式
/* 输入参数：    deviceID --> 设备ID
				 mode-->  显示模式
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT int SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode);

/************************************************************************/
/* 函数名称：    SDI_GetPropertyMap
/* 函数功能：    获取设备输入设置
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT mapProperty& SDI_GetPropertyMap();
/************************************************************************/
/* 函数名称：    SDI_GetOutDeviceParaMap
/* 函数功能：    获取输出设备制式设置
/* 输出参数：    无
/* 返 回 值：    错误码
/************************************************************************/
EXTERN_DLLEXPORT mapOutDevicePara& SDI_GetOutDeviceParaMap();

EXTERN_DLLEXPORT int SDI_Configuration();