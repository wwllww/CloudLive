
#include "D3DRender.h"

static SYSTEM_INFO si;
static OSVERSIONINFO osVersionInfo;

static void HandleNvidiaOptimus(IDXGIFactory1 *factory, IDXGIAdapter1 *&adapter, UINT &adapterID)
{
    if (adapterID != 1)
        return;

    DXGI_ADAPTER_DESC adapterDesc;
    if (SUCCEEDED(adapter->GetDesc(&adapterDesc)))
    {
        String name = adapterDesc.Description;
        name.KillSpaces();

        if (name.IsEmpty())
            return;

		if (sstri((const TCHAR*)adapterDesc.Description, (const TCHAR*)L"NVIDIA") != NULL)
        {
            if (name[name.Length()-1] == 'M' || name[name.Length()-1] == 'm') {
                adapter->Release();

                adapterID = 0;
				Log((const TCHAR*)L"Nvidia optimus detected, second adapter selected, ignoring useless second adapter, I guess.");
                if(FAILED(factory->EnumAdapters1(adapterID, &adapter)))
                    CrashError(TEXT("无法获取显卡 %d，请检查显卡设置！"));
            }
        }
    }
}

void Init()
{
	GetSystemInfo(&si);

	osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
	GetVersionEx(&osVersionInfo);
}

int STDCALL OSVersion()
{
	if (osVersionInfo.dwMajorVersion > 6)
		return 8;

	if (osVersionInfo.dwMajorVersion == 6)
	{
		//Windows 8
		if (osVersionInfo.dwMinorVersion >= 2)
			return 8;

		//Windows 7
		if (osVersionInfo.dwMinorVersion == 1)
			return 7;

		//Vista
		if (osVersionInfo.dwMinorVersion == 0)
			return 6;
	}

	return 0;
}

const static D3D_FEATURE_LEVEL featureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

D3D10System::D3D10System()
{
	Init();
	curVertexBuffer = NULL;
	memset(curProjMatrix, 0, sizeof(curProjMatrix));
	memset(curViewMatrix, 0, sizeof(curViewMatrix));
	memset(curViewProjMatrix, 0, sizeof(curViewProjMatrix));
	memset(curBlendFactor, 0, sizeof(curBlendFactor));
	memset(curCropping, 0, sizeof(curCropping));
}

D3D10System::~D3D10System()
{
}

void D3D10System::UnloadAllData()
{
    LoadVertexShader(NULL);
    LoadPixelShader(NULL);
    LoadVertexBuffer(NULL);
    for(UINT i=0; i<8; i++)
    {
        LoadSamplerState(NULL, i);
        LoadTexture(NULL, i);
    }

    UINT zeroVal = 0;
    LPVOID nullBuff[8];
    float bla[4] = {1.0f, 1.0f, 1.0f, 1.0f};

    zero(nullBuff, sizeof(nullBuff));

    context->VSSetConstantBuffers(0, 1, (ID3D11Buffer**)nullBuff);
    context->PSSetConstantBuffers(0, 1, (ID3D11Buffer**)nullBuff);
    context->OMSetDepthStencilState(NULL, 0);
    context->PSSetSamplers(0, 1, (ID3D11SamplerState**)nullBuff);
    context->OMSetBlendState(NULL, bla, 0xFFFFFFFF);
    context->OMSetRenderTargets(1, (ID3D11RenderTargetView**)nullBuff, NULL);
    context->IASetVertexBuffers(0, 8, (ID3D11Buffer**)nullBuff, &zeroVal, &zeroVal);
    context->PSSetShaderResources(0, 8, (ID3D11ShaderResourceView**)nullBuff);
    context->IASetInputLayout(NULL);
    context->PSSetShader(NULL, NULL, 0);
    context->VSSetShader(NULL, NULL, 0);
    context->RSSetState(NULL);
    context->RSSetScissorRects(0, NULL);
}

LPVOID D3D10System::GetDevice()
{
	return (LPVOID)d3d;
}

LPVOID D3D10System::GetContext()
{
	return (LPVOID)context;
}

Texture* D3D10System::CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bBuildMipMaps, BOOL bStatic)
{
    return D3D10Texture::CreateTexture(width, height, colorFormat, lpData, bBuildMipMaps, bStatic,this);
}

Texture* D3D10System::CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps, UINT &Width, UINT& Height)
{
    return D3D10Texture::CreateTextureFromFile(lpFile, bBuildMipMaps,Width,Height,this);
}

Texture* D3D10System::CreateGDITexture(unsigned int width, unsigned int height)
{
	return D3D10Texture::CreateGDITexture(width, height,this);
}

Texture* D3D10System::CreateTextureRead(unsigned int width, unsigned int height)
{
	return D3D10Texture::CreateTextureRead(width, height,this);
}

Texture* D3D10System::CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps)
{
    return D3D10Texture::CreateRenderTarget(width, height, colorFormat, bGenMipMaps,this);
}

SamplerState* D3D10System::CreateSamplerState(SamplerInfo &info)
{
	return D3D10SamplerState::CreateSamplerState(info,this);
}

Shader* D3D10System::CreateVertexShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName)
{
	return D3D10VertexShader::CreateVertexShaderFromBlob(blob, lpShader, lpFileName,this);
}

Shader* D3D10System::CreatePixelShaderFromBlob(ShaderBlob const &blob, CTSTR lpShader, CTSTR lpFileName)
{
	return D3D10PixelShader::CreatePixelShaderFromBlob(blob, lpShader, lpFileName,this);
}

Shader* D3D10System::CreateVertexShaderFromFile(CTSTR lpFileName)
{
	return D3D10VertexShader::CreateVertexShaderFromFile(lpFileName,this);
}

Shader* D3D10System::CreatePixelShaderFromFile(CTSTR lpFileName)
{
	return D3D10PixelShader::CreatePixelShaderFromFile(lpFileName,this);
}

void D3D10System::CreateVertexShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName)
{
	D3D10VertexShader::CreateVertexShaderBlob(blob, lpShader, lpFileName,this);
}

void D3D10System::CreatePixelShaderBlob(ShaderBlob &blob, CTSTR lpShader, CTSTR lpFileName)
{
	D3D10PixelShader::CreatePixelShaderBlob(blob, lpShader, lpFileName,this);
}

D3D10VertexBuffer* D3D10System::CreateVertexBuffer(VBData *vbData, BOOL bStatic)
{
	return D3D10VertexBuffer::CreateVertexBuffer(vbData, bStatic,this);
}

UINT D3D10System::GetNumOutputs()
{
    UINT count = 0;

    IDXGIDevice *device;
    if(SUCCEEDED(d3d->QueryInterface(__uuidof(IDXGIDevice), (void**)&device)))
    {
        IDXGIAdapter *adapter;
        if(SUCCEEDED(device->GetAdapter(&adapter)))
        {
            IDXGIOutput *outputInterface;
            while(SUCCEEDED(adapter->EnumOutputs(count, &outputInterface)))
            {
                count++;
                outputInterface->Release();
            }
            adapter->Release();
        }
        device->Release();
    }

    return count;
}

OutputDuplicator *D3D10System::CreateOutputDuplicator(UINT outputID)
{
    D3D10OutputDuplicator *duplicator = new D3D10OutputDuplicator(this);
    if(duplicator->Init(outputID))
        return duplicator;

    delete duplicator;
    return NULL;
}

void D3D10System::LoadVertexBuffer(D3D10VertexBuffer* vb)
{
   if(vb != curVertexBuffer)
    {
        D3D10VertexBuffer *d3dVB = static_cast<D3D10VertexBuffer*>(vb);
        if(curVertexShader)
        {
            List<ID3D11Buffer*> buffers;
            List<UINT> strides;
            List<UINT> offsets;

            if(d3dVB)
                d3dVB->MakeBufferList(curVertexShader, buffers, strides);
            else
            {
                UINT nBuffersToClear = curVertexShader->NumBuffersExpected();
                buffers.SetSize(nBuffersToClear);
                strides.SetSize(nBuffersToClear);
            }

            offsets.SetSize(buffers.Num());
            context->IASetVertexBuffers(0, buffers.Num(), buffers.Array(), strides.Array(), offsets.Array());
        }
        curVertexBuffer = d3dVB;
    }
}

void D3D10System::LoadTexture(Texture *texture, UINT idTexture)
{
    //if(curTextures[idTexture] != texture)
    {
        D3D10Texture *d3dTex = static_cast<D3D10Texture*>(texture);
        if(d3dTex)
            context->PSSetShaderResources(idTexture, 1, &d3dTex->resource);
        else
        {
            LPVOID lpNull = NULL;
            context->PSSetShaderResources(idTexture, 1, (ID3D11ShaderResourceView**)&lpNull);
        }
        curTextures[idTexture] = d3dTex;
    }
}

void D3D10System::LoadSamplerState(SamplerState *sampler, UINT idSampler)
{
    if(curSamplers[idSampler] != sampler)
    {
        D3D10SamplerState *d3dSampler = static_cast<D3D10SamplerState*>(sampler);
        if(d3dSampler)
            context->PSSetSamplers(idSampler, 1, &d3dSampler->state);
        else
        {
            LPVOID lpNull = NULL;
            context->PSSetSamplers(idSampler, 1, (ID3D11SamplerState**)&lpNull);
        }
        curSamplers[idSampler] = d3dSampler;
    }
}

void D3D10System::LoadVertexShader(Shader *vShader)
{
   if(curVertexShader != vShader)
    {
        if(vShader)
        {
            D3D10VertexBuffer *lastVertexBuffer = curVertexBuffer;
            if(curVertexBuffer)
                LoadVertexBuffer(NULL);

            D3D10VertexShader *shader = static_cast<D3D10VertexShader*>(vShader);
            context->VSSetShader(shader->vertexShader, NULL, 0);
            context->IASetInputLayout(shader->inputLayout);
            context->VSSetConstantBuffers(0, 1, &shader->constantBuffer);

            if(lastVertexBuffer)
                LoadVertexBuffer(lastVertexBuffer);
        }
        else
        {
            LPVOID lpNULL = NULL;
            context->VSSetShader(NULL, NULL, 0);
            context->VSSetConstantBuffers(0, 1, (ID3D11Buffer**)&lpNULL);
        }
        curVertexShader = static_cast<D3D10VertexShader*>(vShader);
    }
}

void D3D10System::LoadPixelShader(Shader *pShader)
{
   if(curPixelShader != pShader)
    {
        if(pShader)
        {
            D3D10PixelShader *shader = static_cast<D3D10PixelShader*>(pShader);
            context->PSSetShader(shader->pixelShader, NULL, 0);
			if (shader->constantBuffer)
				context->PSSetConstantBuffers(0, 1, &shader->constantBuffer);

            for(UINT i=0; i<shader->Samplers.Num(); i++)
                LoadSamplerState(shader->Samplers[i].sampler, i);
        }
        else
        {
            LPVOID lpNULL = NULL;
            context->PSSetShader(NULL, NULL, 0);
            context->PSSetConstantBuffers(0, 1, (ID3D11Buffer**)&lpNULL);

            for(UINT i=0; i<8; i++)
                curSamplers[i] = NULL;

            ID3D11SamplerState *states[8];
            zero(states, sizeof(states));
            context->PSSetSamplers(0, 8, states);
        }
        curPixelShader = static_cast<D3D10PixelShader*>(pShader);
    }
}

Shader* D3D10System::GetCurrentPixelShader()
{
    return curPixelShader;
}

Shader* D3D10System::GetCurrentVertexShader()
{
    return curVertexShader;
}

void D3D10System::initD3D(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID)
{
	//Source->Start()
	HRESULT err;

#ifdef USE_DXGI1_2
	REFIID iidVal = OSVersion() >= 8 ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);
#else
	REFIID iidVal = __uuidof(IDXGIFactory1);
#endif

	if (FAILED(err = CreateDXGIFactory1(iidVal, (void**)&factory)))
		CrashError(TEXT("Could not create DXGI factory"));

	UINT numAdapters = 0, i = 0;
	IDXGIAdapter1 *giAdapter;
	while (factory->EnumAdapters1(i++, &giAdapter) == S_OK)
	{
		numAdapters++;
		giAdapter->Release();
	}

	if (adapterID >= numAdapters)
	{
		Log(TEXT("Invalid adapter id %d, only %d adapters on system. Resetting to 0."), adapterID, numAdapters);
		adapterID = 0;
	}

	IDXGIAdapter1 *adapter;
	if (FAILED(err = factory->EnumAdapters1(adapterID, &adapter)))
		CrashError(TEXT("无法获取显卡 %d，请检查显卡设置！"), 0);

	HandleNvidiaOptimus(factory, adapter, adapterID);

	DXGI_SWAP_CHAIN_DESC swapDesc;
	zero(&swapDesc, sizeof(swapDesc));
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.Width = renderFrameWidth;
	swapDesc.BufferDesc.Height = renderFrameHeight;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = hwndRenderFrame;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = TRUE;

	bDisableCompatibilityMode = 1;

	UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL level;

	String adapterName;
	DXGI_ADAPTER_DESC desc;
	if (adapter->GetDesc(&desc) == S_OK)
		adapterName = desc.Description;
	else
		adapterName = TEXT("<unknown>");

	adapterName.KillSpaces();

	Log(TEXT("---adapterName %s---"), adapterName.Array());

	err = D3D11CreateDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, createFlags, featureLevels, sizeof(featureLevels) / sizeof(featureLevels[0]), D3D11_SDK_VERSION, &swapDesc, &swap, &d3d, &level, &context);
	if (FAILED(err))
	{
		Log(TEXT("D3D11CreateDeviceAndSwapChain: Failed on %s: 0x%08x"), adapterName.Array(), err);
		CrashError(TEXT("Could not initialize DirectX 11 on %s.  This error can happen for one of the following reasons:\r\n\r\n1.) Your GPU is not supported (DirectX 11 is required - note that many integrated laptop GPUs do not support DX11)\r\n2.) You're running Windows Vista without the \"Platform Update\"\r\n3.) Your video card drivers are out of date\r\n\r\nIf you are using a laptop with NVIDIA Optimus or AMD Switchable Graphics, make sure BLive is set to run on the high performance GPU in your driver settings."), adapterName.Array());
	}

	adapter->Release();

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	zero(&depthDesc, sizeof(depthDesc));
	depthDesc.DepthEnable = FALSE;

	err = d3d->CreateDepthStencilState(&depthDesc, &depthState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create depth state"));

	context->OMSetDepthStencilState(depthState, 0);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	zero(&rasterizerDesc, sizeof(rasterizerDesc));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthClipEnable = TRUE;

	err = d3d->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create rasterizer state"));

	context->RSSetState(rasterizerState);

	rasterizerDesc.ScissorEnable = TRUE;

	err = d3d->CreateRasterizerState(&rasterizerDesc, &scissorState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create scissor state"));


	ID3D11Texture2D *backBuffer = NULL;
	err = swap->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);
	if (FAILED(err))
		CrashError(TEXT("Unable to get back buffer from swap chain"));

	err = d3d->CreateRenderTargetView(backBuffer, NULL, &swapRenderView);
	if (FAILED(err))
		CrashError(TEXT("Unable to get render view from back buffer"));

	backBuffer->Release();


	D3D11_BLEND_DESC disabledBlendDesc;
	zero(&disabledBlendDesc, sizeof(disabledBlendDesc));
	for (int i = 0; i < 8; i++)
	{
		disabledBlendDesc.RenderTarget[i].BlendEnable = TRUE;
		disabledBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		disabledBlendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		disabledBlendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		disabledBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		disabledBlendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		disabledBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		disabledBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
	}

	err = d3d->CreateBlendState(&disabledBlendDesc, &disabledBlend);
	if (FAILED(err))
		CrashError(TEXT("Unable to create disabled blend state"));

	this->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
	bPreBlendEnabled = true;

	VBData *data = new VBData;
	data->UVList.SetSize(1);
	data->VertList.SetSize(4);
	data->UVList[0].SetSize(4);
	spriteVertexBuffer = CreateVertexBuffer(data, FALSE);

	data = new VBData;
	data->VertList.SetSize(5);
	boxVertexBuffer = CreateVertexBuffer(data, FALSE);
}

void D3D10System::initD3D(UINT adapterID)
{
	HRESULT err;

#ifdef USE_DXGI1_2
	REFIID iidVal = OSVersion() >= 8 ? __uuidof(IDXGIFactory2) : __uuidof(IDXGIFactory1);
#else
	REFIID iidVal = __uuidof(IDXGIFactory1);
#endif

	if (FAILED(err = CreateDXGIFactory1(iidVal, (void**)&factory)))
		CrashError(TEXT("Could not create DXGI factory"));

	UINT numAdapters = 0, i = 0;
	IDXGIAdapter1 *giAdapter;
	while (factory->EnumAdapters1(i++, &giAdapter) == S_OK)
	{
		numAdapters++;
		giAdapter->Release();
	}

	if (adapterID >= numAdapters)
	{
		Log(TEXT("Invalid adapter id %d, only %d adapters on system. Resetting to 0."), adapterID, numAdapters);
		adapterID = 0;
	}

	IDXGIAdapter1 *adapter;
	if (FAILED(err = factory->EnumAdapters1(adapterID, &adapter)))
		CrashError(TEXT("无法获取显卡 %d，请检查显卡设置！"), 0);

	HandleNvidiaOptimus(factory, adapter, adapterID);

	bDisableCompatibilityMode = 1;

	UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

	D3D_FEATURE_LEVEL level;

	String adapterName;
	DXGI_ADAPTER_DESC desc;
	if (adapter->GetDesc(&desc) == S_OK)
		adapterName = desc.Description;
	else
		adapterName = TEXT("<unknown>");

	adapterName.KillSpaces();

	Log(TEXT("---adapterName %s---"), adapterName.Array());

	err = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, createFlags, featureLevels, sizeof(featureLevels) / sizeof(featureLevels[0]), D3D11_SDK_VERSION, &d3d, &level, &context);
	if (FAILED(err))
	{
		Log(TEXT("D3D11CreateDeviceAndSwapChain: Failed on %s: 0x%08x"), adapterName.Array(), err);
		CrashError(TEXT("Could not initialize DirectX 11 on %s.  This error can happen for one of the following reasons:\r\n\r\n1.) Your GPU is not supported (DirectX 11 is required - note that many integrated laptop GPUs do not support DX11)\r\n2.) You're running Windows Vista without the \"Platform Update\"\r\n3.) Your video card drivers are out of date\r\n\r\nIf you are using a laptop with NVIDIA Optimus or AMD Switchable Graphics, make sure BLive is set to run on the high performance GPU in your driver settings."), adapterName.Array());
	}

	adapter->Release();

	D3D11_DEPTH_STENCIL_DESC depthDesc;
	zero(&depthDesc, sizeof(depthDesc));
	depthDesc.DepthEnable = FALSE;

	err = d3d->CreateDepthStencilState(&depthDesc, &depthState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create depth state"));

	context->OMSetDepthStencilState(depthState, 0);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	zero(&rasterizerDesc, sizeof(rasterizerDesc));
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthClipEnable = TRUE;

	err = d3d->CreateRasterizerState(&rasterizerDesc, &rasterizerState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create rasterizer state"));

	context->RSSetState(rasterizerState);

	rasterizerDesc.ScissorEnable = TRUE;

	err = d3d->CreateRasterizerState(&rasterizerDesc, &scissorState);
	if (FAILED(err))
		CrashError(TEXT("Unable to create scissor state"));

	D3D11_BLEND_DESC disabledBlendDesc;
	zero(&disabledBlendDesc, sizeof(disabledBlendDesc));
	for (int i = 0; i < 8; i++)
	{
		disabledBlendDesc.RenderTarget[i].BlendEnable = TRUE;
		disabledBlendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		disabledBlendDesc.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		disabledBlendDesc.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		disabledBlendDesc.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		disabledBlendDesc.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
		disabledBlendDesc.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		disabledBlendDesc.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
	}

	err = d3d->CreateBlendState(&disabledBlendDesc, &disabledBlend);
	if (FAILED(err))
		CrashError(TEXT("Unable to create disabled blend state"));

	this->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
	bPreBlendEnabled = true;

	VBData *data = new VBData;
	data->UVList.SetSize(1);
	data->VertList.SetSize(4);
	data->UVList[0].SetSize(4);
	spriteVertexBuffer = CreateVertexBuffer(data, FALSE);

	data = new VBData;
	data->VertList.SetSize(5);
	boxVertexBuffer = CreateVertexBuffer(data, FALSE);
}

void D3D10System::unInitD3D()
{
	UnloadAllData();

	delete spriteVertexBuffer;
	delete boxVertexBuffer;

	for (UINT i = 0; i < blends.Num(); i++)
		SafeRelease(blends[i].blendState);

	SafeRelease(scissorState);
	SafeRelease(rasterizerState);
	SafeRelease(depthState);
	SafeRelease(disabledBlend);
	SafeRelease(swapRenderView);
	SafeRelease(swap);
	SafeRelease(context);
	SafeRelease(d3d);
	SafeRelease(factory);
}

void D3D10System::SetRenderTarget(Texture *texture)
{
	if (curRenderTarget != texture)
	{
		if (texture)
		{
			ID3D11RenderTargetView *view = static_cast<D3D10Texture*>(texture)->renderTarget;
			if (!view)
			{
				AppWarning(TEXT("tried to set a texture that wasn't a render target as a render target"));
				return;
			}
			context->OMSetRenderTargets(1, &view, NULL);
		}
		else
			context->OMSetRenderTargets(1, &swapRenderView, NULL);

		curRenderTarget = static_cast<D3D10Texture*>(texture);
	}
	else
	{
		//这里当有一个RenderTarget的时候没有更新

// 		ID3D11RenderTargetView *view = NULL;
// 		context->OMSetRenderTargets(1, &view, NULL);

		if (texture)
		{
			ID3D11RenderTargetView *view = static_cast<D3D10Texture*>(texture)->renderTarget;
			if (!view)
			{
				AppWarning(TEXT("tried to set a texture that wasn't a render target as a render target"));
				return;
			}
			context->OMSetRenderTargets(1, &view, NULL);
		}
	}
}

const D3D11_PRIMITIVE_TOPOLOGY topologies[] = {D3D11_PRIMITIVE_TOPOLOGY_POINTLIST, D3D11_PRIMITIVE_TOPOLOGY_LINELIST, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP};

void D3D10System::Draw(GSDrawMode drawMode, DWORD startVert, DWORD nVerts)
{
    if(!curVertexBuffer)
    {
        AppWarning(TEXT("Tried to call draw without setting a vertex buffer"));
        return;
    }
    if(!curVertexShader)
    {
        AppWarning(TEXT("Tried to call draw without setting a vertex shader"));
        return;
    }
    if(!curPixelShader)
    {
        AppWarning(TEXT("Tried to call draw without setting a pixel shader"));
        return;
    }
    curVertexShader->SetMatrix(curVertexShader->GetViewProj(), curViewProjMatrix);

    curVertexShader->UpdateParams();
    curPixelShader->UpdateParams();

    D3D11_PRIMITIVE_TOPOLOGY newTopology = topologies[(int)drawMode];
    if(newTopology != curTopology)
    {
        context->IASetPrimitiveTopology(newTopology);
        curTopology = newTopology;
    }
    if(nVerts == 0)
        nVerts = static_cast<D3D10VertexBuffer*>(curVertexBuffer)->numVerts;

    context->Draw(nVerts, startVert);
}

const D3D11_BLEND blendConvert[] = {D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_DEST_COLOR, D3D11_BLEND_INV_DEST_COLOR, D3D11_BLEND_DEST_ALPHA, D3D11_BLEND_INV_DEST_ALPHA, D3D11_BLEND_BLEND_FACTOR, D3D11_BLEND_INV_BLEND_FACTOR};

void  D3D10System::EnableBlending(BOOL bEnable)
{
	if (bPreBlendEnabled != bEnable)
    {
		bPreBlendEnabled = bEnable;
        if(bEnable)
            context->OMSetBlendState(curBlendState, curBlendFactor, 0xFFFFFFFF);
        else
            context->OMSetBlendState(disabledBlend, curBlendFactor, 0xFFFFFFFF);
    }
}

void D3D10System::BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor)
{
    bool bUseFactor = (srcFactor >= GS_BLEND_FACTOR || destFactor >= GS_BLEND_FACTOR);

    if(bUseFactor)
        curBlendFactor[0] = curBlendFactor[1] = curBlendFactor[2] = curBlendFactor[3] = fFactor;

    for(UINT i=0; i<blends.Num(); i++)
    {
        SavedBlendState &blendInfo = blends[i];
        if(blendInfo.srcFactor == srcFactor && blendInfo.destFactor == destFactor)
        {
            if(bUseFactor || curBlendState != blendInfo.blendState)
            {
                context->OMSetBlendState(blendInfo.blendState, curBlendFactor, 0xFFFFFFFF);
                curBlendState = blendInfo.blendState;
            }
            return;
        }
    }
    D3D11_BLEND_DESC blendDesc;
    zero(&blendDesc, sizeof(blendDesc));
    for(int i=0; i<8; i++)
    {
        blendDesc.RenderTarget[i].BlendEnable               = TRUE;
        blendDesc.RenderTarget[i].RenderTargetWriteMask     = D3D11_COLOR_WRITE_ENABLE_ALL;
        blendDesc.RenderTarget[i].BlendOpAlpha              = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].BlendOp                   = D3D11_BLEND_OP_ADD;
        blendDesc.RenderTarget[i].SrcBlendAlpha             = D3D11_BLEND_ONE;
        blendDesc.RenderTarget[i].DestBlendAlpha            = D3D11_BLEND_ZERO;
        blendDesc.RenderTarget[i].SrcBlend                  = blendConvert[srcFactor];
        blendDesc.RenderTarget[i].DestBlend                 = blendConvert[destFactor];
    }

    SavedBlendState *savedBlend = blends.CreateNew();
    savedBlend->destFactor      = destFactor;
    savedBlend->srcFactor       = srcFactor;

    if(FAILED(d3d->CreateBlendState(&blendDesc, &savedBlend->blendState)))
        CrashError(TEXT("Could not set blend state"));

	if (bPreBlendEnabled)
        context->OMSetBlendState(savedBlend->blendState, curBlendFactor, 0xFFFFFFFF);

    curBlendState = savedBlend->blendState;
}

void D3D10System::ClearRenderTarget(DWORD color)
{
    Color4 floatColor;
    floatColor.MakeFromRGBA(color);

    D3D10Texture *d3dTex = static_cast<D3D10Texture*>(curRenderTarget);
    if(d3dTex)
        context->ClearRenderTargetView(d3dTex->renderTarget, floatColor.ptr);
    else
        context->ClearRenderTargetView(swapRenderView, floatColor.ptr);
}

void D3D10System::Ortho(float left, float right, float top, float bottom, float znear, float zfar)
{
    Matrix4x4Ortho(curProjMatrix, left, right, top, bottom, znear, zfar);
    ResetViewMatrix();
}

void D3D10System::SetViewport(float x, float y, float width, float height)
{
    D3D11_VIEWPORT vp;
    zero(&vp, sizeof(vp));
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = FLOAT(x);
    vp.TopLeftY = FLOAT(y);
    vp.Width    = FLOAT(width);
    vp.Height   = FLOAT(height);
    context->RSSetViewports(1, &vp);
}

void D3D10System::SetScissorRect(XRect *pRect)
{
    if(pRect)
    {
        context->RSSetState(scissorState);
        D3D11_RECT rc = {pRect->x, pRect->y, pRect->x+pRect->cx, pRect->y+pRect->cy};
        context->RSSetScissorRects(1, &rc);
    }
    else
    {
        context->RSSetState(rasterizerState);
        context->RSSetScissorRects(0, NULL);
    }
}

void D3D10System::SetCropping(float left, float top, float right, float bottom)
{
    curCropping[0] = left;
    curCropping[1] = top;
    curCropping[2] = right;
    curCropping[3] = bottom;
}

void D3D10System::DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2)
{
    DrawSpriteExRotate(texture, color, x, y, x2, y2, 0.0f, u, v, u2, v2, 0.0f);
}

void D3D10System::DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees)
{
    if(!curPixelShader)
        return; 

    if(!texture)
    {
        AppWarning(TEXT("Trying to draw a sprite with a NULL texture"));
        return;
    }

    HANDLE hColor = curPixelShader->GetParameterByName(TEXT("outputColor"));

    if(hColor)
        curPixelShader->SetColor(hColor, color);

    Vect2 totalSize = Vect2(x2-x, y2-y);
    Vect2 invMult   = Vect2(totalSize.x < 0.0f ? -1.0f : 1.0f, totalSize.y < 0.0f ? -1.0f : 1.0f);
    totalSize.Abs();

    if(y2-y < 0) {
        float tempFloat = curCropping[1];
        curCropping[1] = curCropping[3];
        curCropping[3] = tempFloat;
    }

    if(x2-x < 0) {
        float tempFloat = curCropping[0];
        curCropping[0] = curCropping[2];
        curCropping[2] = tempFloat;
    }

    bool bFlipX = (x2 - x) < 0.0f;
    bool bFlipY = (y2 - y) < 0.0f;

    x  += curCropping[0] * invMult.x;
    y  += curCropping[1] * invMult.y;
    x2 -= curCropping[2] * invMult.x;
    y2 -= curCropping[3] * invMult.y;

    bool cropXUnder = bFlipX ? ((x - x2) < 0.0f) : ((x2 - x) < 0.0f);
    bool cropYUnder = bFlipY ? ((y - y2) < 0.0f) : ((y2 - y) < 0.0f);

    if (cropXUnder || cropYUnder)
        return;

    float cropMult[4];
    cropMult[0] = curCropping[0]/totalSize.x;
    cropMult[1] = curCropping[1]/totalSize.y;
    cropMult[2] = curCropping[2]/totalSize.x;
    cropMult[3] = curCropping[3]/totalSize.y;

    Vect2 totalUVSize = Vect2(u2-u, v2-v);
    u  += cropMult[0] * totalUVSize.x;
    v  += cropMult[1] * totalUVSize.y;
    u2 -= cropMult[2] * totalUVSize.x;
    v2 -= cropMult[3] * totalUVSize.y;


    VBData *data = spriteVertexBuffer->GetData();
    data->VertList[0].Set(x,  y,  0.0f);
    data->VertList[1].Set(x,  y2, 0.0f);
    data->VertList[2].Set(x2, y,  0.0f);
    data->VertList[3].Set(x2, y2, 0.0f);

    if (!CloseFloat(degrees, 0.0f)) {
        List<Vect> &coords = data->VertList;

        Vect2 center(x+totalSize.x/2, y+totalSize.y/2);

        Matrix rotMatrix;
        rotMatrix.SetIdentity();
        rotMatrix.Rotate(AxisAngle(0.0f, 0.0f, 1.0f, RAD(degrees)));

        for (int i = 0; i < 4; i++) {
            Vect val = coords[i]-Vect(center);
            val.TransformVector(rotMatrix);
            coords[i] = val;
            coords[i] += Vect(center);
        }
    }

    List<UVWCoord> &coords = data->UVList[0];
    coords[0].Set(u,  v,0.0f);
    coords[1].Set(u,  v2,1.0f);
    coords[2].Set(u2, v,0.f);
    coords[3].Set(u2, v2,0.f);

    if (!CloseFloat(texDegrees, 0.0f)) {
        Matrix rotMatrix;
        rotMatrix.SetIdentity();
        rotMatrix.Rotate(AxisAngle(0.0f, 0.0f, 1.0f, -RAD(texDegrees)));

        Vect3 minVal = Vect3(0.0f, 0.0f,0.0f);
        for (int i = 0; i < 4; i++) {
            Vect val = Vect(coords[i]);
            val.TransformVector(rotMatrix);
            coords[i] = val;
            minVal.ClampMax(coords[i]);
        }

        for (int i = 0; i < 4; i++)
            coords[i] -= minVal;
    }

    spriteVertexBuffer->FlushBuffers(this);

    LoadVertexBuffer(spriteVertexBuffer);
    LoadTexture(texture);

    Draw(GS_TRIANGLESTRIP);
}

void D3D10System::DrawBox(const Vect2 &upperLeft, const Vect2 &size)
{
    VBData *data = boxVertexBuffer->GetData();
    Vect2 bottomRight = upperLeft+size;
    data->VertList[0] = upperLeft;
    data->VertList[1].Set(bottomRight.x, upperLeft.y);
    data->VertList[2].Set(bottomRight.x, bottomRight.y);
    data->VertList[3].Set(upperLeft.x, bottomRight.y);
    data->VertList[4] = upperLeft;

    boxVertexBuffer->FlushBuffers(this);
    LoadVertexBuffer(boxVertexBuffer);

    Draw(GS_LINESTRIP);
}

void D3D10System::ResetViewMatrix()
{
    Matrix4x4Convert(curViewMatrix, MatrixStack[curMatrix].GetTranspose());
	curViewMatrix[2] = -curViewMatrix[2];
	curViewMatrix[6] = -curViewMatrix[6];
	curViewMatrix[10] = -curViewMatrix[10];
	curViewMatrix[14] = -curViewMatrix[14];
    Matrix4x4Multiply(curViewProjMatrix, curViewMatrix, curProjMatrix);
    Matrix4x4Transpose(curViewProjMatrix, curViewProjMatrix);
}

void D3D10System::ResizeView(Texture *TextView)
{
    LPVOID nullVal = NULL;
    context->OMSetRenderTargets(1, (ID3D11RenderTargetView**)&nullVal, NULL);

	if (!TextView)
	{
		SafeRelease(swapRenderView);

		swap->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

		ID3D11Texture2D *backBuffer = NULL;
		HRESULT err = swap->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);

		if (backBuffer)
		{
			err = d3d->CreateRenderTargetView(backBuffer, NULL, &swapRenderView);
			if (FAILED(err))
				CrashError(TEXT("Unable to get render view from back buffer"));

			backBuffer->Release();
		}
	}
	else
	{
		D3D10Texture *Text = dynamic_cast<D3D10Texture*>(TextView);
		if (Text)
		{
			SafeRelease(Text->renderTarget);

			Text->SwapChain->ResizeBuffers(2, 0, 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0);

			ID3D11Texture2D *backBuffer = NULL;
			HRESULT err = Text->SwapChain->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer);

			if (backBuffer)
			{
				err = d3d->CreateRenderTargetView(backBuffer, NULL, &Text->renderTarget);
				if (FAILED(err))
					CrashError(TEXT("Unable to get render view from back buffer"));

				backBuffer->Release();
			}
		}
	}
}

void D3D10System::CopyTexture(Texture *texDest, Texture *texSrc)
{
    D3D10Texture *d3d10Dest = static_cast<D3D10Texture*>(texDest);
    D3D10Texture *d3d10Src  = static_cast<D3D10Texture*>(texSrc);

    context->CopyResource(d3d10Dest->texture, d3d10Src->texture);
}

HRESULT D3D10System::Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP map_type)
{
    HRESULT err;
    D3D11_MAPPED_SUBRESOURCE map;
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
	if (FAILED(err = context->Map(d3d10Tex->texture, 0, map_type, 0, &map)))
    {
        AppWarning(TEXT("D3D10Texture::Map: map failed, result = %08lX"), err);
        return err;
    }
	lpData = (BYTE*)map.pData;
    pitch = map.RowPitch;

    return err;
}

void D3D10System::Unmap(Texture *tex)
{
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
	context->Unmap(d3d10Tex->texture, 0);
}

void CopyPackedRGB(BYTE *lpDest, BYTE *lpSource, UINT nPixels)
{
	DWORD curComponent = 0;

	UINT totalBytes = (nPixels * 3);
	UINT alignedBytes = totalBytes & 0xFFFFFFFC;
	UINT nDWords = alignedBytes >> 2;

	DWORD *lpDWDest = (DWORD*)lpDest;
	DWORD *lpDWSrc = (DWORD*)lpSource;

	while (nDWords)
	{
		switch (curComponent)
		{
		case 0: *(lpDWDest++) = *lpDWSrc & 0xFFFFFF; *lpDWDest = *(lpDWSrc++) >> 24; break; //RBGR
		case 1: *(lpDWDest++) |= ((*lpDWSrc) << 8) & 0xFFFF00; *lpDWDest = *(lpDWSrc++) >> 16; break; //GRBG
		case 2: *(lpDWDest++) |= ((*lpDWSrc) << 16) & 0xFFFF00; *(lpDWDest++) = *(lpDWSrc++) >> 8;  break; //BGRB
		}

		if (curComponent == 2)
			curComponent = 0;
		else
			curComponent++;

		nDWords--;
	}

	totalBytes -= alignedBytes;
	lpSource = (LPBYTE)lpDWSrc;
	lpDest = (LPBYTE)lpDWDest;

	if (curComponent != 0)
		lpDest += curComponent;

	while (totalBytes--)
	{
		*(lpDest++) = *(lpSource++);

		if (curComponent == 2)
		{
			*(lpDest++) = 0;
			curComponent = 0;
		}
		else
			curComponent++;
	}
}

void D3D10System::SetImage(Texture* tex, void *lpData, GSImageFormat imageFormat, UINT pitch)
{
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
	if (!d3d10Tex->bDynamic)
    {
        AppWarning(TEXT("3D11Texture::SetImage: cannot call on a non-dynamic texture"));
        return;
    }

    bool bMatchingFormat = false;
    UINT pixelBytes = 0;   
    switch(d3d10Tex->format)
    {
        case GS_ALPHA:      bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_A8); pixelBytes = 1; break;
        case GS_GRAYSCALE:  bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_L8); pixelBytes = 1; break;
        case GS_RGB:        bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_RGB || imageFormat == GS_IMAGEFORMAT_RGBX); pixelBytes = 4; break;
        case GS_RGBA:       bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_RGBA); pixelBytes = 4; break;
        case GS_BGR:        bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_BGR || imageFormat == GS_IMAGEFORMAT_BGRX); pixelBytes = 4; break;
        case GS_BGRA:       bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_BGRA); pixelBytes = 4; break;
        case GS_RGBA16F:    bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_RGBA16F); pixelBytes = 8; break;
        case GS_RGBA32F:    bMatchingFormat = (imageFormat == GS_IMAGEFORMAT_RGBA32F); pixelBytes = 16; break;
    }

    if(!bMatchingFormat)
    {
        AppWarning(TEXT("D3D10Texture::SetImage: invalid or mismatching image format specified"));
        return;
    }
    HRESULT err;
    D3D11_MAPPED_SUBRESOURCE map;
	if (FAILED(err = context->Map(d3d10Tex->texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &map)))
    {
        AppWarning(TEXT("D3D10Texture::SetImage: map failed, result = %08lX"), err);
        return;
    }

	if ((d3d10Tex->format == GS_RGB || d3d10Tex->format == GS_BGR) && (imageFormat == GS_IMAGEFORMAT_BGR || imageFormat == GS_IMAGEFORMAT_RGB))
    {
		if (pitch == (d3d10Tex->width * 3) && map.RowPitch == (d3d10Tex->width * 4))
			CopyPackedRGB((BYTE*)map.pData, (BYTE*)lpData, d3d10Tex->width * d3d10Tex->height);
        else
        {
			for (UINT y = 0; y<d3d10Tex->height; y++)
            {
                LPBYTE curInput  = ((LPBYTE)lpData)    + (pitch*y);
                LPBYTE curOutput = ((LPBYTE)map.pData) + (map.RowPitch*y);
				CopyPackedRGB(curOutput, curInput, d3d10Tex->width);
            }
        }
    }
    else
    {
        if(pitch == map.RowPitch)
			mcpy(map.pData, lpData, pitch * d3d10Tex->height);
        else
        {
            UINT bestPitch = MIN(pitch, map.RowPitch);
			for (UINT y = 0; y < d3d10Tex->height; y++)
            {
                LPBYTE curInput  = ((LPBYTE)lpData)    + (pitch*y);
                LPBYTE curOutput = ((LPBYTE)map.pData) + (map.RowPitch*y);
                mcpy(curOutput, curInput, bestPitch);
            }
        }
    }
	context->Unmap(d3d10Tex->texture, 0);
}

void D3D10System::GetTextureWH(Texture* tex, DWORD& width, DWORD& height)
{
	if (tex)
	{
		D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
		width = d3d10Tex->width;
		height = d3d10Tex->height;
	}
}

bool D3D10System::GetTextureDC(Texture* tex, HDC &hDC)
{
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
    if(!d3d10Tex->bGDICompatible)
    {
        AppWarning(TEXT("D3D10Texture::GetDC: function was called on a non-GDI-compatible texture"));
        return false;
    }
    HRESULT err;
	if (FAILED(err = d3d10Tex->texture->QueryInterface(__uuidof(IDXGISurface1), (void**)&d3d10Tex->surface)))
    {
        AppWarning(TEXT("D3D10Texture::GetDC: could not query surface interface, result = %08lX"), err);
        return false;
    }
	if (FAILED(err = d3d10Tex->surface->GetDC(TRUE, &hDC)))
    {
        AppWarning(TEXT("D3D10Texture::GetDC: could not get DC, result = %08lX"), err);
		SafeRelease(d3d10Tex->surface);
        return false;
    }
    return true;
}

void  D3D10System::ReleaseTextureDC(Texture* tex)
{
	D3D10Texture *d3d10Tex = static_cast<D3D10Texture*>(tex);
    if(!d3d10Tex->surface)
    {
        AppWarning(TEXT("D3D10Texture::ReleaseDC: no DC to release"));
        return;
    }
	d3d10Tex->surface->ReleaseDC(NULL);
	SafeRelease(d3d10Tex->surface);
}

void D3D10System::Flush()
{
	context->Flush();
}

HRESULT D3D10System::GetDeviceRemovedReason()
{
	return  d3d->GetDeviceRemovedReason();
}

void D3D10System::Present(Texture *TextSwap)
{
	if (TextSwap)
	{
		D3D10Texture *Swap = dynamic_cast<D3D10Texture*>(TextSwap);
		if (Swap && Swap->SwapChain)
		{
			Swap->SwapChain->Present(0, 0);
		}
	}
	else
	{
		swap->Present(0, 0);
	}
	
}

Texture * D3D10System::CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height)
{
	bool bSuc = false;
	DXGI_SWAP_CHAIN_DESC swapDesc;
	zero(&swapDesc, sizeof(swapDesc));
	swapDesc.BufferCount = 2;
	swapDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapDesc.BufferDesc.Width = Width;
	swapDesc.BufferDesc.Height = Height;
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapDesc.OutputWindow = Hwnd;
	swapDesc.SampleDesc.Count = 1;
	swapDesc.Windowed = TRUE;

	ID3D11Texture2D *backBuffer = NULL;
	ID3D11RenderTargetView *target = NULL;
	IDXGISwapChain *Swap = NULL;

	if (FAILED(factory->CreateSwapChain(d3d, &swapDesc, &Swap))) {
		AppWarning(L"Could not create projector swap chain");
		goto exit;
	}

	if (FAILED(Swap->GetBuffer(0, IID_ID3D11Texture2D, (void**)&backBuffer))) {
		AppWarning(TEXT("Unable to get projector back buffer"));
		goto exit;
	}

	if (FAILED(d3d->CreateRenderTargetView(backBuffer, NULL, &target))) {
		AppWarning(TEXT("Unable to get render view from projector back buffer"));
		goto exit;
	}

	D3D10Texture *tex = new D3D10Texture();
	tex->width = Width;
	tex->height = Height;
	tex->format = GS_BGRA;
	tex->texture = NULL;
	tex->renderTarget = target;
	tex->SwapChain = Swap;

	backBuffer->Release();
	return tex;
exit:
	if (!bSuc) {
		SafeRelease(Swap);
		SafeRelease(backBuffer);
		
	}

	return NULL;
}
