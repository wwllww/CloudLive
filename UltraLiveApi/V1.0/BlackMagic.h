#pragma once
#include <string>
#include <list>
#include "BaseAfx.h"

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

struct SDIID
{
	int id;
	char* strName;    //�豸��
	bool enable;
	SDIOUT_COLORFORMAT colorFormat;
	SDIOUT_DISPLAYMODE displayMode;
};

struct DeviceInfo
{
	int   strID;      //�豸ID
	char* strName;    //�豸��
};

struct SDIOutInfo
{
	int  id;
	bool bEnable;
	std::string SourceName;
	std::string Format;
};

struct DisplayMode
{
	int mode;
	char* strName;
};

typedef int(*SDI_INIT)();
typedef int(*SDI_STARTOUT)(int nDeviceID, SDIOUT_DISPLAYMODE nMode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime);
typedef int(*SDI_STOPOUT)(int nDeviceID);
typedef int(*SDI_RENDERDEVICE)(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT nColorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM);
typedef int(*SDI_UNINIT)();
typedef int(*SDI_GETDEVICELIST)(DeviceInfo** pDevicelist, int* nDeviceCnt);
typedef int(*SDI_GETDISPLAYMODELIST)(DisplayMode** pModeList, int* nDeviceCnt);
typedef int(*SDI_SETDEVICEPROPERTY)(int deviceID, bool bInput);
typedef int(*SDI_SETOUTDEVICEPARA)(int deviceID, SDIOUT_DISPLAYMODE mode);
typedef int(*SDI_RELEASEDEVICELIST)();
typedef int(*SDI_RELEASEDISPLAYMODELIST)();
typedef int(*SDI_CONFIGURATION)();

class BlackMagic
{
public:
	static BlackMagic* Instance()
	{
		if (blackMagic)
		{
			return blackMagic;
		}

		blackMagic = new BlackMagic();
		return  blackMagic;
	}

	static void Destroy()
	{
		delete blackMagic;
		blackMagic = NULL;
	}

	int SDI_Init(SDIOutInfo *ID, bool *bOutSDI);

	/************************************************************************/
	/* �������ƣ�    SDI_StartOut
	/* �������ܣ�    ��ָ���豸�ϣ���SDI���
	/* ���������    nDeviceID --> SDI����豸ID
	/*               nMode     --> �豸����ʾģʽ
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE nMode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime);
	/************************************************************************/
	/* �������ƣ�    SDI_StopOut
	/* �������ܣ�    ��ָ���豸�ϣ�ֹͣSDI���
	/* ���������    nDeviceID --> SDI����豸ID
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_StopOut(int nDeviceID);

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
	int SDI_RenderDevice(const SDIID& id, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT nColorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM);

	/************************************************************************/
	/* �������ƣ�    SDI_unInit
	/* �������ܣ�    ����ʼ��SDI���ģ��
	/* ���������    ��
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_unInit();

	/************************************************************************/
	/* �������ƣ�    SDI_GetDeviceList
	/* �������ܣ�    ��ȡ�����豸�б�
	/* ���������    pDevicelist --> �豸�б�ָ��
	/*               nDeviceCnt  --> �豸����
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt);

	/************************************************************************/
	/* �������ƣ�    SDI_GetDisplayModeList
	/* �������ܣ�    ��ȡ��ʽ�б�
	/* ���������    pModeList --> �豸֧�ֵ���ʽ
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_GetDisplayModeList(DisplayMode** pModeList, int* nDeviceCnt);

	/************************************************************************/
	/* �������ƣ�    SDI_SetDeviceProperty
	/* �������ܣ�    �����豸�������
	/* ���������    deviceID --> �豸ID
	bInput--> �Ƿ������豸 1�����룬 0�����
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_SetDeviceProperty(int deviceID, bool bInput);

	/************************************************************************/
	/* �������ƣ�    SDI_SetOutDevicePara
	/* �������ܣ�    �����豸��ʽ
	/* ���������    deviceID --> �豸ID
	mode-->  ��ʾģʽ
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode);

	/************************************************************************/
	/* �������ƣ�    SDI_ReleaseDeviceList
	/* �������ܣ�    �ͷſ����豸�б�
	/* ���������
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_ReleaseDeviceList();

	/************************************************************************/
	/* �������ƣ�    SDI_ReleaseDisplayModeList
	/* �������ܣ�    �ͷ���ʽ�б�
	/* ���������
	/* ���������    ��
	/* �� �� ֵ��    ������
	/************************************************************************/
	int SDI_ReleaseDisplayModeList();

	void ApplySDISettings(SDIOutInfo *ID,bool bForceUpdata = false);
	void ApplyOutOrInSettings(bool *bOutSDI);
	
	int SDI_Configuration();

	int AllStop();
	int ReStart();
	bool AllEnable()
	{
		return bAllEnable;
	}

private:
	bool bAllEnable = true;

public:
	DeviceInfo* pDevicelist = NULL;
	int nDeviceCount = 0;

	std::vector<DeviceInfo*> SDIDeviceOutlst;

	DisplayMode* pModeList = NULL;
	int nDisplayModeCount = 0;
	SDIOutInfo *pOutInof = NULL;

private:
	BlackMagic();
	~BlackMagic();
	static BlackMagic* blackMagic;

private:

	HMODULE hPlugin = NULL;
	SDI_INIT SDIInit = NULL;
	SDI_STARTOUT SDIStartOut = NULL;
	SDI_STOPOUT SDIStopOut = NULL;
	SDI_RENDERDEVICE SDIRenderDevice = NULL;
	SDI_UNINIT SDIunInit = NULL;
	SDI_GETDEVICELIST SDIGetDeviceList = NULL;
	SDI_GETDISPLAYMODELIST SDIGetDisplayModeList = NULL;
	SDI_SETDEVICEPROPERTY SDISetDeviceProperty = NULL;
	SDI_SETOUTDEVICEPARA SDISetOutDevicePara = NULL;
	SDI_RELEASEDEVICELIST SDIReleaseDeviceList = NULL;
	SDI_RELEASEDISPLAYMODELIST SDIReleaseDisplayModeList = NULL;
	SDI_CONFIGURATION SDIConfiguration = NULL;
};


