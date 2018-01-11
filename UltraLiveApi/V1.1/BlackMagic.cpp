#include "BlackMagic.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

BlackMagic* BlackMagic::blackMagic = NULL;

int BlackMagic::SDI_Init(SDIOutInfo *ID, bool *bOutSDI)
{
	int res = -10000;

	hPlugin  = LoadLibrary(L"SDIOutput.dll");
	if (hPlugin)
	{
		SDIInit = (SDI_INIT)GetProcAddress(hPlugin, "SDI_Init");
		SDIStartOut = (SDI_STARTOUT)GetProcAddress(hPlugin, "SDI_StartOut");
		SDIStopOut = (SDI_STOPOUT)GetProcAddress(hPlugin, "SDI_StopOut");
		SDIRenderDevice = (SDI_RENDERDEVICE)GetProcAddress(hPlugin, "SDI_RenderDevice");
		SDIunInit = (SDI_UNINIT)GetProcAddress(hPlugin, "SDI_unInit");
		SDIGetDeviceList = (SDI_GETDEVICELIST)GetProcAddress(hPlugin, "SDI_GetDeviceList");
		SDIGetDisplayModeList = (SDI_GETDISPLAYMODELIST)GetProcAddress(hPlugin, "SDI_GetDisplayModeList");
		SDISetDeviceProperty = (SDI_SETDEVICEPROPERTY)GetProcAddress(hPlugin, "SDI_SetDeviceProperty");
		SDISetOutDevicePara = (SDI_SETOUTDEVICEPARA)GetProcAddress(hPlugin, "SDI_SetOutDevicePara");
		SDIReleaseDeviceList = (SDI_RELEASEDEVICELIST)GetProcAddress(hPlugin, "SDI_ReleaseDeviceList");
		SDIReleaseDisplayModeList = (SDI_RELEASEDISPLAYMODELIST)GetProcAddress(hPlugin, "SDI_ReleaseDisplayModeList");
		SDIConfiguration = (SDI_CONFIGURATION)GetProcAddress(hPlugin, "SDI_Configuration");
	}

	if (SDIInit)
	{
		res = SDIInit();

		if (NULL == pDevicelist)
			SDI_GetDeviceList(&pDevicelist, &nDeviceCount);
		if (NULL == pModeList && nDeviceCount > 0)
			SDI_GetDisplayModeList(&pModeList, &nDisplayModeCount);

		if (nDeviceCount)
			pOutInof = new SDIOutInfo[nDeviceCount];
	}
	SDI_Configuration();
	for (int i = 0; i < nDeviceCount; ++i)
	{
		//�������Է���
		bool input = false;

		if (bOutSDI)
			input = bOutSDI[i];

		SDI_SetDeviceProperty(pDevicelist[i].strID, input);

		if (input)
		{
			continue;
		}
		else
		{
			SDIDeviceOutlst.push_back(&pDevicelist[i]);
		}

		bool enable = false;
		SDIID id;
		id.id = pDevicelist[i].strID;
		id.enable = enable;
		id.strName = pDevicelist[i].strName;


		//���ö�Ӧ��Դ
		
		std::string SourceName;

		if (ID)
		{
			SourceName = ID[i].SourceName;
			pOutInof[i].bEnable    = ID[i].bEnable;
			pOutInof[i].Format     = ID[i].Format;
			pOutInof[i].id         = ID[i].id;
			pOutInof[i].SourceName = ID[i].SourceName;
		}

		if (!SourceName.empty())
		{
			if (strcmp(SourceName.c_str(),"PGM") == 0)
			{
				CSLiveManager::GetInstance()->PushOrUpdateSIDId(id);
			}
			else
			{

				bool bFind = false;
				for (int j = 0; j < CSLiveManager::GetInstance()->m_InstanceList.GetSize(); ++j)
				{
					CInstanceProcess *Process = CSLiveManager::GetInstance()->m_InstanceList.GetAt(j);

					if (Process && Process->bLittlePre && !Process->bNoPreView)
					{
						if (Process->MultiRender)
						{
							for (int k = 0; k < Process->m_VideoList.Num(); ++k)
							{
								VideoStruct &OneVideo = Process->m_VideoList[k];
								if (strcmp((*OneVideo.Config)["Name"].asString().c_str(), SourceName.c_str()) == 0)
								{
									Process->MultiRender->PushOrUpdateSIDId(id);
									bFind = true;
								}

								break;
							}
						}

					}

					if (bFind)
						break;

				}
			}
		}
	}

	return res;
}

/************************************************************************/
/* �������ƣ�    SDI_StartOut
/* �������ܣ�    ��ָ���豸�ϣ���SDI���
/* ���������    nDeviceID --> SDI����豸ID
/*               nMode     --> �豸����ʾģʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE nMode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime)
{
	return (bAllEnable && SDIStartOut) ? SDIStartOut(nDeviceID, nMode, nColorFormat, nBufferTime) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_StopOut
/* �������ܣ�    ��ָ���豸�ϣ�ֹͣSDI���
/* ���������    nDeviceID --> SDI����豸ID
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_StopOut(int nDeviceID)
{
	return SDIStopOut ? SDIStopOut(nDeviceID) : -1000;
}

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
int BlackMagic::SDI_RenderDevice(const SDIID& id, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT nColorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM)
{
	return (bAllEnable && id.enable && SDIRenderDevice) ? SDIRenderDevice(id.id, pData, nWidth, nHeight, nColorFormat, bAudio, pAudioFormat, nAudioLength, bPGM) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_unInit
/* �������ܣ�    ����ʼ��SDI���ģ��
/* ���������    ��
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_unInit()
{
	for (int i = 0; i < SDIDeviceOutlst.size(); ++i)
	{
		SDI_StopOut(SDIDeviceOutlst[i]->strID);
	}

	SDI_ReleaseDeviceList();
	SDI_ReleaseDisplayModeList();

	pDevicelist = NULL;
	nDeviceCount = 0;
	SDIDeviceOutlst.clear();

	pModeList = NULL;
	nDisplayModeCount = 0;

	if (pOutInof)
	{
		delete [] pOutInof;
		pOutInof = NULL;
	}

	int res = SDIunInit ? SDIunInit() : -1000;
	if (hPlugin)
	{
		FreeLibrary(hPlugin);
		hPlugin = NULL;
	}

	return 0;
}

/************************************************************************/
/* �������ƣ�    SDI_GetDeviceList
/* �������ܣ�    ��ȡ�����豸�б�
/* ���������    pDevicelist --> �豸�б�ָ��
/*               nDeviceCnt  --> �豸����
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt)
{
	return SDIGetDeviceList ? SDIGetDeviceList(pDevicelist, nDeviceCnt) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_GetDisplayModeList
/* �������ܣ�    ��ȡ��ʽ�б�
/* ���������    pModeList --> �豸֧�ֵ���ʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_GetDisplayModeList(DisplayMode** pModeList, int* nDeviceCnt)
{
	return SDIGetDisplayModeList ? SDIGetDisplayModeList(pModeList, nDeviceCnt) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_SetDeviceProperty
/* �������ܣ�    �����豸�������
/* ���������    deviceID --> �豸ID
bInput--> �Ƿ������豸 1�����룬 0�����
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_SetDeviceProperty(int deviceID, bool bInput)
{
	return SDISetDeviceProperty ? SDISetDeviceProperty(deviceID, bInput) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_SetOutDevicePara
/* �������ܣ�    �����豸��ʽ
/* ���������    deviceID --> �豸ID
mode-->  ��ʾģʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode)
{
	return SDISetOutDevicePara ? SDISetOutDevicePara(deviceID, mode) : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_SetOutDevicePara
/* �������ܣ�    �����豸��ʽ
/* ���������    deviceID --> �豸ID
mode-->  ��ʾģʽ
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_ReleaseDeviceList()
{
	return SDIReleaseDeviceList ? SDIReleaseDeviceList() : -1000;
}

/************************************************************************/
/* �������ƣ�    SDI_ReleaseDisplayModeList
/* �������ܣ�    �ͷ���ʽ�б�
/* ���������
/* ���������    ��
/* �� �� ֵ��    ������
/************************************************************************/
int BlackMagic::SDI_ReleaseDisplayModeList()
{
	return SDIReleaseDisplayModeList ? SDIReleaseDisplayModeList() : -1000;
}

int BlackMagic::SDI_Configuration()
{
	return SDIConfiguration ? SDIConfiguration() : -1000;
}

BlackMagic::BlackMagic()
{
}

BlackMagic::~BlackMagic()
{
	SDI_unInit();
}

int BlackMagic::AllStop()
{
	bAllEnable = false;

	for (int i = 0; i < SDIDeviceOutlst.size(); ++i)
	{
		int sdiDeviceId = SDIDeviceOutlst[i]->strID;
		SDI_StopOut(sdiDeviceId);
	}

	return 0;
}

int BlackMagic::ReStart()
{
	bAllEnable = true;

	if (!pOutInof)
		return -1;

	for (int i = 0; i < SDIDeviceOutlst.size(); ++i)
	{
		int sdiDeviceId = SDIDeviceOutlst[i]->strID;

		std::string format = pOutInof[i].Format;
		int mode = NTSC;
		//��ʽ
		for (int p = 0; p < nDisplayModeCount; ++p)
		{
			std::string DispModeName = pModeList[p].strName;
			if (strcmp(format.c_str(),DispModeName.c_str()) == 0)
			{
				mode = pModeList[p].mode;
				break;
			}
		}

		//Դ
		SDIOUT_COLORFORMAT color = ColorFormat_RGB32;
		std::string SourceName = pOutInof[i].SourceName;;

		CSLiveManager *Manager = CSLiveManager::GetInstance();

		if (strcmp(SourceName.c_str(),"PGM") == 0)
		{
			color = ColorFormat_RGBA32REVERSE;

			if (pOutInof[i].bEnable)
			{
				//int bufferingTime = Manager->BSParam.Advanced.BufferTime;
				SDI_StartOut(sdiDeviceId, (SDIOUT_DISPLAYMODE)mode, color, 5);
			}


		}
		else
		{
			bool bFind = false;
			for (int j = 0; j < Manager->m_InstanceList.GetSize(); ++j)
			{
				CInstanceProcess *Process = Manager->m_InstanceList.GetAt(j);

				if (Process && Process->bLittlePre && !Process->bNoPreView)
				{
					if (Process->MultiRender)
					{
						for (int k = 0; k < Process->m_VideoList.Num(); ++k)
						{
							VideoStruct &OneVideo = Process->m_VideoList[k];
							if (strcmp((*OneVideo.Config)["Name"].asString().c_str(), SourceName.c_str()) == 0)
							{
								if (pOutInof[i].bEnable)
								{
									SDIID id;
									id.id = sdiDeviceId;
									id.enable = pOutInof[i].bEnable;
									id.strName = SDIDeviceOutlst[i]->strName;
									id.displayMode = (SDIOUT_DISPLAYMODE)mode;
									id.colorFormat = (SDIOUT_COLORFORMAT)ColorFormat_HDYC;

									//������Ӳ�һ������AudioName��Ϊ��
									bool bbFind = false;
									if (strcmp(pOutInof[i].SourceName.c_str(), pOutInof[i].AudioName.c_str()) != 0 && (strcmp(pOutInof[i].AudioName.c_str(), "��") != 0 || strcmp(pOutInof[i].AudioName.c_str(), "None") != 0))
									{
										for (int m = 0; m < Manager->m_InstanceList.GetSize(); ++m)
										{
											CInstanceProcess *Process1 = Manager->m_InstanceList.GetAt(m);

											if (Process1 && Process1->bLittlePre && !Process1->bNoPreView)
											{
												if (Process1->MultiRender)
												{
													for (int n = 0; n < Process1->m_VideoList.Num(); ++n)
													{
														VideoStruct &OneVideo1 = Process1->m_VideoList[n];
														if (strcmp((*OneVideo1.Config)["Name"].asString().c_str(), pOutInof[i].AudioName.c_str()) == 0)
														{
															//�ҵ�Audio
															Process1->MultiRender->PushOrUpdateSIDId(id);
															bbFind = true;
															break;
														}
													}
												}
											}

											if (bbFind)
												 break;
										}
									}

								

// 									if (strcmp(OneVideo.VideoStream->GainClassName(), "DSource") == 0)
// 									{
// 										SDI_StartOut(sdiDeviceId, (SDIOUT_DISPLAYMODE)mode, (SDIOUT_COLORFORMAT)Process->MultiRender->Pixformat, 0);
// 										id.colorFormat = ColorFormat_I420;
// 									}
									Process->MultiRender->PushOrUpdateSIDId(id);
								}

								bFind = true;
							}
					
							break;
						}
					}

				}
				
				if (bFind)
					break;

			}
		}
	}

	return 0;
}

void BlackMagic::ApplySDISettings(SDIOutInfo *ID, bool bForceUpdata)
{
	if (!pOutInof)
		return;
	for (int i = 0; i < SDIDeviceOutlst.size(); ++i)
	{
		bool bNoChange = false;
		//״̬
		bNoChange = (pOutInof[i].bEnable == ID[i].bEnable);

		pOutInof[i].bEnable = ID[i].bEnable;

		//������ʽ
		bNoChange = bNoChange && (strcmp(pOutInof[i].Format.c_str(), ID[i].Format.c_str()) == 0);

		pOutInof[i].Format = ID[i].Format;

		int sdiDeviceId = SDIDeviceOutlst[i]->strID;

		pOutInof[i].id = sdiDeviceId;

		int format = NTSC;
		//��ʽ
		for (int p = 0; p < nDisplayModeCount; ++p)
		{
			std::string DispModeName = pModeList[p].strName;
			if (strcmp(pOutInof[i].Format.c_str(), DispModeName.c_str()) == 0)
			{
				format = pModeList[p].mode;
				break;
			}
		}

		bNoChange = bNoChange && (strcmp(pOutInof[i].SourceName.c_str(), ID[i].SourceName.c_str()) == 0);
		pOutInof[i].SourceName = ID[i].SourceName;
		pOutInof[i].AudioName = ID[i].AudioName;

		SDIID id;
		id.id = sdiDeviceId;
		id.enable = pOutInof[i].bEnable;
		id.strName = SDIDeviceOutlst[i]->strName;
		id.displayMode = (SDIOUT_DISPLAYMODE)format;

		if (!bNoChange || bForceUpdata)
			SDI_StopOut(sdiDeviceId);

		CSLiveManager *Manager = CSLiveManager::GetInstance();

		if (strcmp(pOutInof[i].SourceName.c_str(),"PGM") == 0)
		{
			id.colorFormat = ColorFormat_RGBA32REVERSE;
			CSLiveManager::GetInstance()->PushOrUpdateSIDId(id);
			if ((!bNoChange) && pOutInof[i].bEnable && AllEnable() || bForceUpdata)
			{
				//int bufferingTime = Manager->BSParam.Advanced.BufferTime;
				blackMagic->SDI_StartOut(sdiDeviceId, (SDIOUT_DISPLAYMODE)format, ColorFormat_RGBA32REVERSE, 5);

			}
		}
		else
		{
			Manager->FindAndRemoveId(sdiDeviceId);
		}

		CInstanceProcess *AudioInstance = NULL;

		for (int j = 0; j < Manager->m_InstanceList.GetSize(); ++j)
		{
			CInstanceProcess *Process = Manager->m_InstanceList.GetAt(j);

			if (Process && Process->bLittlePre && !Process->bNoPreView)
			{
				if (Process->MultiRender)
				{
					for (int k = 0; k < Process->m_VideoList.Num(); ++k)
					{
						VideoStruct &OneVideo = Process->m_VideoList[k];
						if (strcmp((*OneVideo.Config)["Name"].asString().c_str(), pOutInof[i].SourceName.c_str()) == 0)
						{
							id.colorFormat = (SDIOUT_COLORFORMAT)ColorFormat_HDYC;
							//������Ӳ�һ������AudioName��Ϊ��
							bool bbFind = false;
							if (strcmp(pOutInof[i].SourceName.c_str(), pOutInof[i].AudioName.c_str()) != 0 && (strcmp(pOutInof[i].AudioName.c_str(), "��") != 0 || strcmp(pOutInof[i].AudioName.c_str(), "None") != 0))
							{
								for (int m = 0; m < Manager->m_InstanceList.GetSize(); ++m)
								{
									CInstanceProcess *Process1 = Manager->m_InstanceList.GetAt(m);

									if (Process1 && Process1->bLittlePre && !Process1->bNoPreView)
									{
										if (Process1->MultiRender)
										{
											for (int n = 0; n < Process1->m_VideoList.Num(); ++n)
											{
												VideoStruct &OneVideo1 = Process1->m_VideoList[n];
												if (strcmp((*OneVideo1.Config)["Name"].asString().c_str(), pOutInof[i].AudioName.c_str()) == 0)
												{
													//�ҵ�Audio
													Process1->MultiRender->PushOrUpdateSIDId(id);
													AudioInstance = Process1;
													bbFind = true;
													break;
												}
											}
										}
									}

									if (bbFind)
										break;
								}
							}

							
							if ((!bNoChange) && pOutInof[i].bEnable && AllEnable() || bForceUpdata)
							{
// 								if (strcmp(OneVideo.VideoStream->GainClassName(), "DSource") == 0)
// 								{
// 									SDI_StartOut(sdiDeviceId, (SDIOUT_DISPLAYMODE)format, (SDIOUT_COLORFORMAT)Process->MultiRender->Pixformat, 0);
// 								}
								
							}
							
							Process->MultiRender->PushOrUpdateSIDId(id);
						}
						else
						{
							if (Process != AudioInstance)
								Process->MultiRender->FindAndRemoveId(sdiDeviceId);
						}
						break;
					}
				}

			}

		}

	}
}

void BlackMagic::ApplyOutOrInSettings(bool *bOutSDI)
{
	SDIDeviceOutlst.clear();
	bool bChecked = false;
	for (int i = 0; i < nDeviceCount; ++i)
	{
		bool checked = bOutSDI[i];
		checked = !checked;

		if (checked)
		{
			SDI_SetDeviceProperty(blackMagic->pDevicelist[i].strID, false);
			SDIDeviceOutlst.push_back(&blackMagic->pDevicelist[i]);
			bChecked = true;
		}
		else
		{
			CSLiveManager::GetInstance()->FindAndRemoveId(pDevicelist[i].strID);

			for (int j = 0; j < CSLiveManager::GetInstance()->m_InstanceList.GetSize(); ++j)
			{
				CInstanceProcess *Process = CSLiveManager::GetInstance()->m_InstanceList.GetAt(j);

				if (Process && Process->bLittlePre && !Process->bNoPreView)
				{
					if (Process->MultiRender)
					{
						Process->MultiRender->FindAndRemoveId(blackMagic->pDevicelist[i].strID);
					}

				}

			}

			SDI_StopOut(blackMagic->pDevicelist[i].strID);

			SDI_SetDeviceProperty(blackMagic->pDevicelist[i].strID, true);
		}
	}

	if (bChecked)
		ApplySDISettings(pOutInof, true);
}