
#include "D3DRender.h"

#define DXGI_ERROR_ACCESS_LOST      _HRESULT_TYPEDEF_(0x887A0026L)
#define DXGI_ERROR_WAIT_TIMEOUT     _HRESULT_TYPEDEF_(0x887A0027L)

bool D3D10OutputDuplicator::Init(UINT output)
{
	copyTex = NULL;
    HRESULT hRes;

    bool bSuccess = false;

    IDXGIDevice *device;
	if (SUCCEEDED(hRes = System->GetDeviceInline()->QueryInterface(__uuidof(IDXGIDevice), (void**)&device)))
    {
        IDXGIAdapter *adapter;
        if(SUCCEEDED(hRes = device->GetAdapter(&adapter)))
        {
            IDXGIOutput *outputInterface;
            if(SUCCEEDED(hRes = adapter->EnumOutputs(output, &outputInterface)))
            {
                IDXGIOutput1 *output1;

                if(SUCCEEDED(hRes = outputInterface->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1)))
                {
					if (SUCCEEDED(hRes = output1->DuplicateOutput(System->GetDeviceInline(), &duplicator)))
                        bSuccess = true;
                    /*else
                        AppWarning(TEXT("D3D10OutputDuplicator::Init: output1->DuplicateOutput failed, result = %u"), (UINT)hRes);*/

                    output1->Release();
                }
                /*else
                    AppWarning(TEXT("D3D10OutputDuplicator::Init: outputInterface->QueryInterface failed, result = %u"), (UINT)hRes);*/

                outputInterface->Release();
            }
            /*else
                AppWarning(TEXT("D3D10OutputDuplicator::Init: adapter->EnumOutputs failed, result = %u"), (UINT)hRes);*/

            adapter->Release();
        }
        /*else
            AppWarning(TEXT("D3D10OutputDuplicator::Init: device->GetAdapter failed, result = %u"), (UINT)hRes);*/

        device->Release();
    }
    /*else
        AppWarning(TEXT("D3D10OutputDuplicator::Init: GetD3D()->QueryInterface failed, result = %u"), (UINT)hRes);*/

    return bSuccess;
}

D3D10OutputDuplicator::D3D10OutputDuplicator(D3D10System *System)
{
	this->System = System;
}

D3D10OutputDuplicator::~D3D10OutputDuplicator()
{
    SafeRelease(duplicator);
	if (copyTex)
		delete copyTex;
}

DuplicatorInfo D3D10OutputDuplicator::AcquireNextFrame(UINT timeout)
{
    if(!duplicator)
    {
        AppWarning(TEXT("D3D10OutputDuplicator::AcquireNextFrame: Well, apparently there's no duplicator."));
        return DuplicatorInfo_Error;
    }

    //------------------------------------------

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource *tempResource = NULL;

    HRESULT hRes = duplicator->AcquireNextFrame(timeout, &frameInfo, &tempResource);
    if(hRes == DXGI_ERROR_ACCESS_LOST)
        return DuplicatorInfo_Lost;
    else if(hRes == DXGI_ERROR_WAIT_TIMEOUT)
        return DuplicatorInfo_Timeout;
    else if(FAILED(hRes))
        return DuplicatorInfo_Error;

    //------------------------------------------

    ID3D11Texture2D *texVal;
    if(FAILED(hRes = tempResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&texVal)))
    {
        SafeRelease(tempResource);
        AppWarning(TEXT("D3D10OutputDuplicator::AcquireNextFrame: could not query interface, result = 0x%08lX"), hRes);
        return DuplicatorInfo_Error;
    }

    tempResource->Release();

    //------------------------------------------

    D3D11_TEXTURE2D_DESC texDesc;
    texVal->GetDesc(&texDesc);
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(copyTex);
	if (!d3d10Tex || d3d10Tex->width != texDesc.Width || d3d10Tex->height != texDesc.Height)
    {
		copyTex = System->CreateTexture(texDesc.Width, texDesc.Height, ConvertGIBackBufferFormat(texDesc.Format), NULL, FALSE, TRUE);
    }
    if(copyTex)
    {
        D3D10Texture *d3dCopyTex = (D3D10Texture*)copyTex;
		System->GetContextInline()->CopyResource(d3dCopyTex->texture, texVal);
    }

    SafeRelease(texVal);
    duplicator->ReleaseFrame();

    return DuplicatorInfo_Acquired;
}

Texture* D3D10OutputDuplicator::GetCopyTexture()
{
    return copyTex;
}

Texture* D3D10OutputDuplicator::GetCursorTex(POINT* pos)
{
    if(pos)
        mcpy(pos, &cursorPos, sizeof(POINT));

    if(bCursorVis)
        return cursorTex;

    return NULL;
}
