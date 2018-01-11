

#include "stdafx.h"
#include "SDIOutputImpl.h"

SDIOutput*	pSDI = NULL;

int SDI_Init()
{
	HRESULT ret;
	ret = CoInitialize(NULL);
	if (FAILED(ret))
		return errCOMInit;

	if (pSDI == NULL)
	{
		pSDI = new SDIOutput();
		if (pSDI)
		{
			ret = pSDI->SDI_Init();
			if (ret != errNone)
			{
				delete pSDI;
				pSDI = NULL;
			}
			return ret;
		}
		else
			return errCreateSDIOutputInstance;
	}
	else
		return errSDIOutputInstanceAlwaysExist;
}

int SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt)
{
	if (pSDI)
		return pSDI->SDI_GetDeviceList(pDevicelist, nDeviceCnt);
	return errCreateSDIOutputInstance;
}

int SDI_GetDisplayModeList(DisplayMode** pModeList, int* nModeCount)
{
	if (pSDI)
		return pSDI->SDI_GetDisplayModeList(pModeList, nModeCount);
	return errUnknown;
}

int SDI_ReleaseDeviceList()
{
	if (pSDI)
		return pSDI->SDI_ReleaseDeviceList();
	return errUnknown;
}

int SDI_ReleaseDisplayModeList()
{
	if (pSDI)
		return pSDI->SDI_ReleaseDisplayModeList();
	return errUnknown;
}

int SDI_SetDeviceProperty(int deviceID, bool bInput)
{
	if (pSDI)
		return pSDI->SDI_SetDeviceProperty(deviceID, bInput);
	return errUnknown;
}

int SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode)
{
	if (pSDI)
		return pSDI->SDI_SetOutDevicePara(deviceID, mode);
	return errUnknown;
}

mapProperty& SDI_GetPropertyMap()
{
	if (pSDI)
		return pSDI->SDI_GetPropertyMap();
}

mapOutDevicePara& SDI_GetOutDeviceParaMap()
{
	if (pSDI)
		return pSDI->SDI_GetOutDeviceParaMap();
}


int SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE mode, SDIOUT_COLORFORMAT nColorFormat, int nBufferTime)
{
	if (pSDI)
		return pSDI->SDI_StartOut(nDeviceID, mode, nColorFormat, nBufferTime);
	return errCreateSDIOutputInstance;
}

int SDI_RenderDevice(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT colorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM)
{
	if (pSDI)
		return pSDI->SDI_RenderDevice(nDeviceID, pData, nWidth, nHeight, colorFormat, bAudio, pAudioFormat, nAudioLength, bPGM);
	return errCreateSDIOutputInstance;
}

int SDI_StopOut(int nDeviceID)
{
	if (pSDI)
		return pSDI->SDI_StopOut(nDeviceID);
	return errCreateSDIOutputInstance;
}

int SDI_unInit()
{
	if (pSDI)
	{
		delete pSDI;
		pSDI = NULL;
	}
	CoUninitialize();
	return errNone;
}

EXTERN_DLLEXPORT int SDI_Configuration()
{
	if (pSDI)
		return pSDI->SDI_Configuration();
}

