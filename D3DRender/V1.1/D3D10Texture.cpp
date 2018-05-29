
#include "D3DRender.h"


inline bool IsPow2(UINT num)
{
    return num >= 2 && (num & (num-1)) == 0;
}


const DXGI_FORMAT convertFormat[] = {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_A8_UNORM, DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_B5G5R5A1_UNORM, DXGI_FORMAT_B5G6R5_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC5_UNORM};
const UINT formatPitch[] = {0, 1, 1, 4, 4, 4, 4, 8, 16, 0, 0, 0};

const D3D11_TEXTURE_ADDRESS_MODE convertAddressMode[] = {D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MIRROR, D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_MIRROR_ONCE};
const D3D11_FILTER convertFilter[] = {D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_FILTER_ANISOTROPIC, D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR, D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT, D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR, D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT, D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR, D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT};

inline GSColorFormat GetGSFormatFromDXGIFormat(DXGI_FORMAT dxgiFormat)
{
    UINT numVals = sizeof(convertFormat)/sizeof(DXGI_FORMAT);

    for (UINT i=0; i<numVals; i++) {
        if (convertFormat[i] == dxgiFormat)
            return (GSColorFormat)i;
    }

    return GS_UNKNOWNFORMAT;
}

SamplerState* D3D10SamplerState::CreateSamplerState(SamplerInfo &info, D3D10System *System)
{
    D3D11_SAMPLER_DESC sampDesc;
    zero(&sampDesc, sizeof(sampDesc));
    sampDesc.AddressU       = convertAddressMode[(UINT)info.addressU];
    sampDesc.AddressV       = convertAddressMode[(UINT)info.addressV];
    sampDesc.AddressW       = convertAddressMode[(UINT)info.addressW];
    sampDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    sampDesc.Filter         = convertFilter[(UINT)info.filter];
    sampDesc.MaxAnisotropy  = 1;//info.maxAnisotropy;
    sampDesc.MaxLOD         = FLT_MAX;
    mcpy(sampDesc.BorderColor, info.borderColor.ptr, sizeof(Color4));

    ID3D11SamplerState *state;
	HRESULT err = System->GetDeviceInline()->CreateSamplerState(&sampDesc, &state);
    if(FAILED(err))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10SamplerState::CreateSamplerState: unable to create sampler state, result = %08lX", err);
        return NULL;
    }

    //-------------------------------------------

    D3D10SamplerState *samplerState = new D3D10SamplerState;
    samplerState->state = state;
    mcpy(&samplerState->info, &info, sizeof(SamplerInfo));

    return samplerState;
}

D3D10SamplerState::~D3D10SamplerState()
{
    SafeRelease(state);
}

Texture* D3D10Texture::CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic, D3D10System *D3DSystem)
{
    HRESULT err;

    if(colorFormat >= GS_DXT1)
    {
        Log::writeMessage(LOG_RTSPSERV,1,"D3D10Texture::CreateTexture: tried to create a blank DXT texture.  Use CreateTextureFromFile instead.");
        return NULL;
    }

    DXGI_FORMAT format = convertFormat[(UINT)colorFormat];

    if(bGenMipMaps && (!IsPow2(width) || !IsPow2(height)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTexture: Cannot generate mipmaps for a non-power-of-two sized texture.  Disabling mipmap generation.");
        bGenMipMaps = FALSE;
    }

    D3D11_TEXTURE2D_DESC td;
    zero(&td, sizeof(td));
    td.Width            = width;
    td.Height           = height;
	td.MipLevels        = bGenMipMaps ? 0 : 1;
    td.ArraySize        = 1;
	td.SampleDesc.Count = 1;
    td.Format           = format;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	td.Usage            = bStatic ? D3D11_USAGE_DEFAULT : D3D11_USAGE_DYNAMIC;
	td.CPUAccessFlags   = bStatic ? 0 : D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA srd;
    D3D11_SUBRESOURCE_DATA *lpSRD;
    if(lpData)
    {
        srd.pSysMem = lpData;
        srd.SysMemPitch = width*formatPitch[(UINT)colorFormat];
        srd.SysMemSlicePitch = 0;
        lpSRD = &srd;
    }
    else
        lpSRD = NULL;

    ID3D11Texture2D *texVal;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateTexture2D(&td, lpSRD, &texVal)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTexture: CreateTexture2D failed, result = 0x%08lX", err);
        return NULL;
    }

    //------------------------------------------

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    zero(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Format              = format;
    resourceDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceDesc.Texture2D.MipLevels = bGenMipMaps ? -1 : 1;

    ID3D11ShaderResourceView *resource;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateShaderResourceView(texVal, &resourceDesc, &resource)))
    {
        SafeRelease(texVal);
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTexture: CreateShaderResourceView failed, result = 0x%08lX", err);
        return NULL;
    }

    //------------------------------------------

    D3D10Texture *newTex = new D3D10Texture;
    newTex->format = colorFormat;
    newTex->resource = resource;
    newTex->texture = texVal;
    newTex->bDynamic = !bStatic;
    newTex->width = width;
    newTex->height = height;

    return newTex;
}

Texture* D3D10Texture::CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps, UINT &Width, UINT& Height, D3D10System *D3DSystem)
{
    HRESULT err;

    D3DX11_IMAGE_INFO ii;
    if(FAILED(D3DX11GetImageInfoFromFile(lpFile, NULL, &ii, NULL)))
    {
		Width = 0;
		Height = 0;
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTextureFromFile: Could not get information about texture file '%s'", Wchar2Ansi(lpFile).c_str());
        return NULL;
    }

    //------------------------------------------

	Width = ii.Width;
	Height = ii.Height;
// 	if (ii.Width * ii.Height > 20000000)
// 	{
// 		return NULL;
// 	}

    if(bBuildMipMaps && (!IsPow2(ii.Width) || !IsPow2(ii.Height)))
        bBuildMipMaps = FALSE;

    D3DX11_IMAGE_LOAD_INFO ili;
    ili.Width           = D3DX11_DEFAULT;
    ili.Height          = D3DX11_DEFAULT;
    ili.Depth           = D3DX11_DEFAULT;
    ili.FirstMipLevel   = D3DX11_DEFAULT;
    ili.MipLevels       = bBuildMipMaps ? 0 : 1;
    ili.Usage           = (D3D11_USAGE)D3DX11_DEFAULT;
    ili.BindFlags       = D3DX11_DEFAULT;
    ili.CpuAccessFlags  = D3DX11_DEFAULT;
    ili.MiscFlags       = D3DX11_DEFAULT;
    ili.Format          = (DXGI_FORMAT)D3DX11_DEFAULT;
    ili.Filter          = D3DX11_DEFAULT;
    ili.MipFilter       = D3DX11_DEFAULT;
    ili.pSrcInfo        = NULL;

    ID3D11Resource *texResource;
	if (FAILED(err = D3DX11CreateTextureFromFile(D3DSystem->GetDeviceInline(), lpFile, &ili, NULL, &texResource, NULL)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTextureFromFile: failed to load '%s'", Wchar2Ansi(lpFile).c_str());
        return NULL;
    }

    //------------------------------------------

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    zero(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Format              = ii.Format;
    resourceDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceDesc.Texture2D.MipLevels = bBuildMipMaps ? -1 : 1;

    ID3D11ShaderResourceView *resource;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateShaderResourceView(texResource, &resourceDesc, &resource)))
    {
        SafeRelease(texResource);
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTextureFromFile: CreateShaderResourceView failed, result = 0x%08lX", err);
        return NULL;
    }

    //------------------------------------------

    ID3D11Texture2D *tex2D;
    err = texResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex2D);
    if(FAILED(err))
    {
        SafeRelease(texResource);
        SafeRelease(resource);
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTextureFromFile: could not query texture interface");
        return NULL;
    }

    texResource->Release();

    //------------------------------------------

    D3D10Texture *newTex = new D3D10Texture;
    newTex->resource = resource;
    newTex->texture = tex2D;
    newTex->width = ii.Width;
    newTex->height = ii.Height;

    switch(ii.Format)
    {
        case DXGI_FORMAT_R8_UNORM:              newTex->format = GS_ALPHA;       break;
        case DXGI_FORMAT_A8_UNORM:              newTex->format = GS_GRAYSCALE;   break;
        case DXGI_FORMAT_B8G8R8X8_UNORM:        newTex->format = GS_BGR;         break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:        newTex->format = GS_BGRA;        break;
        case DXGI_FORMAT_R8G8B8A8_UNORM:        newTex->format = GS_RGBA;        break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:    newTex->format = GS_RGBA16F;     break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:    newTex->format = GS_RGBA32F;     break;
        case DXGI_FORMAT_BC1_UNORM:             newTex->format = GS_DXT1;        break;
        case DXGI_FORMAT_BC2_UNORM:             newTex->format = GS_DXT3;        break;
        case DXGI_FORMAT_BC3_UNORM:             newTex->format = GS_DXT5;        break;
        default:
            newTex->format = GS_UNKNOWNFORMAT;
    }

    return newTex;
}

Texture* D3D10Texture::CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps, D3D10System *D3DSystem)
{
    HRESULT err;
    if(colorFormat >= GS_DXT1)
        return NULL;

    DXGI_FORMAT format = convertFormat[(UINT)colorFormat];
    D3D11_TEXTURE2D_DESC td;
    zero(&td, sizeof(td));
    td.Width            = width;
    td.Height           = height;
    td.MipLevels        = bGenMipMaps ? 0 : 1;
    td.ArraySize        = 1;
    td.Format           = format;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    td.SampleDesc.Count = 1;
	td.Usage            = D3D11_USAGE_DEFAULT;
	
    ID3D11Texture2D *texVal;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateTexture2D(&td, NULL, &texVal)))
        return NULL;

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    zero(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Format              = format;
    resourceDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceDesc.Texture2D.MipLevels = bGenMipMaps ? -1 : 1;
    ID3D11ShaderResourceView *resource;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateShaderResourceView(texVal, &resourceDesc, &resource)))
    {
        SafeRelease(texVal);
        return NULL;
    }
    ID3D11RenderTargetView *view;
	err = D3DSystem->GetDeviceInline()->CreateRenderTargetView(texVal, NULL, &view);
    if(FAILED(err))
    {
        SafeRelease(texVal);
        SafeRelease(resource);
        return NULL;
    }
    D3D10Texture *newTex = new D3D10Texture;
    newTex->format = colorFormat;
    newTex->resource = resource;
    newTex->texture = texVal;
    newTex->renderTarget = view;
    newTex->width = width;
    newTex->height = height;

    return newTex;
}

Texture* D3D10Texture::CreateGDITexture(unsigned int width, unsigned int height, D3D10System *D3DSystem)
{
    HRESULT err;

    D3D11_TEXTURE2D_DESC td;
    zero(&td, sizeof(td));
    td.Width            = width;
    td.Height           = height;
    td.MipLevels        = 1;
    td.ArraySize        = 1;
    td.Format           = DXGI_FORMAT_B8G8R8A8_UNORM;
    td.BindFlags        = D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
    td.SampleDesc.Count = 1;
    td.Usage            = D3D11_USAGE_DEFAULT;
    td.MiscFlags        = D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

    D3D11_SUBRESOURCE_DATA srd;
    zero(&srd, sizeof(srd));

    ID3D11Texture2D *texVal;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateTexture2D(&td, NULL, &texVal)))
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateGDITexture: CreateTexture2D failed, result = 0x%08lX", err);
        return NULL;
    }

    //------------------------------------------

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
    zero(&resourceDesc, sizeof(resourceDesc));
    resourceDesc.Format              = DXGI_FORMAT_B8G8R8A8_UNORM;
    resourceDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceDesc.Texture2D.MipLevels = 1;

    ID3D11ShaderResourceView *resource;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateShaderResourceView(texVal, &resourceDesc, &resource)))
    {
        SafeRelease(texVal);
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateGDITexture: CreateShaderResourceView failed, result = 0x%08lX", err);
        return NULL;
    }

    //------------------------------------------

    D3D10Texture *newTex = new D3D10Texture;
    newTex->format = GS_BGRA;
    newTex->resource = resource;
    newTex->texture = texVal;
    newTex->width = width;
    newTex->height = height;
    newTex->bGDICompatible = true;

    return newTex;
}

Texture* D3D10Texture::CreateTextureRead(unsigned int width, unsigned int height,D3D10System *D3DSystem)
{
	HRESULT err;

	D3D11_TEXTURE2D_DESC td;
	zero(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.Usage = D3D11_USAGE_STAGING;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	ID3D11Texture2D *texVal;
	if (FAILED(err = D3DSystem->GetDeviceInline()->CreateTexture2D(&td, NULL, &texVal)))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "D3D10Texture::CreateTexture: CreateTexture2D failed, result = 0x%08lX", err);
		return NULL;
	}

	D3D10Texture *newTex = new D3D10Texture;
	newTex->resource = NULL;
	newTex->texture = texVal;
	newTex->width = width;
	newTex->height = height;

	return newTex;
}

D3D10Texture::~D3D10Texture()
{
    SafeRelease(renderTarget);
    SafeRelease(resource);
    SafeRelease(texture);
	SafeRelease(SwapChain);
	SafeRelease(surface);
}
