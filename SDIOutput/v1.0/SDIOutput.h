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
	errDeckLinkDrivers = -2000,      //����COM���ʧ�ܣ�û��Decklink����
	errIDeckLink,				     //ö���豸ʧ��
	errIDeckLinkOutput,			     //��ѯ����ӿ�ʧ��
	errDisplayMode,				     //û���ҵ�ƥ�����ʾģʽ
	errEnableVideoOutput,		     //ʹ����Ƶ���ʧ��
	errEnableAudioOutput,
	errCreateVideoFrame,		     //���ܴ�����Ƶ֡�Ľӿ�
	errSDIOutputInstanceAlwaysExist, //SDI���ʵ���Ѵ���
	errInvalidParameter,		     //��Ч����
	errUnsupportedColorFormat,	     //��ɫ��ʽ��֧��
	errCreateSDIOutputInstance,			     //��ʼ��ʵ��ʧ��
	errDeviceAlwaysStart,		     //�豸�Ѿ�����
	errCOMInit,						 //COM��ʼ��ʧ��
	errCreateOutputCallback,
	errSetOutputCallbackToDevice
};


struct DeviceInfo
{
	int   strID;      //�豸ID
	char* strName;    //�豸��
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
/* �������ƣ�    SDI_Init
/* �������ܣ�    ��ʼ��SDI���ģ��
/* ���������    ��
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_Init();

/************************************************************************/
/* �������ƣ�    SDI_StartOut
/* �������ܣ�    ��ָ���豸�ϣ���SDI���
/* ���������    nDeviceID --> SDI����豸ID
/*               nMode     --> �豸����ʾģʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE nMode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime);

/************************************************************************/
/* �������ƣ�    SDI_StopOut
/* �������ܣ�    ��ָ���豸�ϣ�ֹͣSDI���
/* ���������    nDeviceID --> SDI����豸ID
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_StopOut(int nDeviceID);

/************************************************************************/
/* �������ƣ�    SDI_RenderDevice
/* �������ܣ�    ��ָ���豸�ϣ����һ֡��Ƶ
/* ���������    nDeviceID    --> SDI����豸ID
/*               pData        --> ����ͼ���ָ��
/*               nWidth       --> ����ͼ��Ŀ��
/*               nHeight      --> ����ͼ��ĸ߶�
/*               nColorFormat --> ����ͼ���ɫ�ʸ�ʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_RenderDevice(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT nColorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM);

/************************************************************************/
/* �������ƣ�    SDI_unInit
/* �������ܣ�    ����ʼ��SDI���ģ��
/* ���������    ��
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_unInit();

/************************************************************************/
/* �������ƣ�    SDI_GetDeviceList
/* �������ܣ�    ��ȡ�����豸�б�
/* ���������    pDevicelist --> �豸�б�ָ��
/*               nDeviceCnt  --> �豸����
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt);

/************************************************************************/
/* �������ƣ�    SDI_GetDisplayModeList
/* �������ܣ�    ��ȡ��ʽ�б�
/* ���������    pModeList --> �豸֧�ֵ���ʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_GetDisplayModeList(DisplayMode** pModeList, int* nModeCount);

/************************************************************************/
/* �������ƣ�    SDI_ReleaseDeviceList
/* �������ܣ�    �ͷſ����豸�б�
/* ���������    
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_ReleaseDeviceList();

/************************************************************************/
/* �������ƣ�    SDI_ReleaseDisplayModeList
/* �������ܣ�    �ͷ���ʽ�б�
/* ���������
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_ReleaseDisplayModeList();

/************************************************************************/
/* �������ƣ�    SDI_SetDeviceProperty
/* �������ܣ�    �����豸�������
/* ���������    deviceID --> �豸ID
				 bInput--> �Ƿ������豸 1�����룬 0�����
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_SetDeviceProperty(int deviceID, bool bInput);

/************************************************************************/
/* �������ƣ�    SDI_SetOutDevicePara
/* �������ܣ�    �����豸��ʽ
/* ���������    deviceID --> �豸ID
				 mode-->  ��ʾģʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT int SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode);

/************************************************************************/
/* �������ƣ�    SDI_GetPropertyMap
/* �������ܣ�    ��ȡ�豸��������
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT mapProperty& SDI_GetPropertyMap();
/************************************************************************/
/* �������ƣ�    SDI_GetOutDeviceParaMap
/* �������ܣ�    ��ȡ����豸��ʽ����
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
EXTERN_DLLEXPORT mapOutDevicePara& SDI_GetOutDeviceParaMap();

EXTERN_DLLEXPORT int SDI_Configuration();