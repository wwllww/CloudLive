// D3DRender.cpp : 定义 DLL 应用程序的导出函数。
//

#include "D3DRender.h"

//static GraphicsSystem *GS = NULL;

// bool LoadPlugin()
// {
// 	GS = new D3D10System;
// 	return true;
// }
// 
// void UnloadPlugin()
// {
// 	unInitD3D();
// }


D3DAPI::D3DAPI(UINT adapterID)
{
	_GS = new D3D10System;
	_GS->initD3D(adapterID);
}

D3DAPI::D3DAPI(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID)
{
	_GS = new D3D10System;
	_GS->initD3D(renderFrameWidth, renderFrameHeight, hwndRenderFrame, adapterID);
}

D3DAPI::~D3DAPI()
{
	if (_GS)
	{
		_GS->unInitD3D();
		delete _GS;
		_GS = NULL;
	}
}

Texture* D3DAPI::CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic)
{
	return _GS->CreateTexture(width, height, colorFormat, lpData, bGenMipMaps, bStatic);
}

Texture* D3DAPI::CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps, UINT &Width, UINT& Height)
{
	return _GS->CreateTextureFromFile(lpFile, bBuildMipMaps,Width,Height);
}

Texture* D3DAPI::CreateGDITexture(unsigned int width, unsigned int height)
{
	return _GS->CreateGDITexture(width, height);
}

Texture* D3DAPI::CreateTextureRead(unsigned int width, unsigned int height)
{
	return _GS->CreateTextureRead(width, height);
}

SamplerState* D3DAPI::CreateSamplerState(SamplerInfo &info)
{ 
	return _GS->CreateSamplerState(info); 
}

Shader* D3DAPI::CreateVertexShaderFromFile(CTSTR lpFileName)
{ 
	return _GS->CreateVertexShaderFromFile(lpFileName); 
}

Shader* D3DAPI::CreatePixelShaderFromFile(CTSTR lpFileName)
{ 
	return _GS->CreatePixelShaderFromFile(lpFileName); 
}

Shader* D3DAPI::GetCurrentVertexShader()
{ 
	return _GS->GetCurrentVertexShader(); 
}

Shader* D3DAPI::GetCurrentPixelShader()
{ 
	return _GS->GetCurrentPixelShader(); 
}

void  D3DAPI::LoadTexture(Texture *texture, UINT idTexture)
{ 
	_GS->LoadTexture(texture, idTexture); 
}

void  D3DAPI::LoadSamplerState(SamplerState *sampler, UINT idSampler)
{ 
	_GS->LoadSamplerState(sampler, idSampler); 
}

void  D3DAPI::LoadVertexShader(Shader *vShader)
{ 
	_GS->LoadVertexShader(vShader); 
}

void  D3DAPI::LoadPixelShader(Shader *pShader)
{ 
	_GS->LoadPixelShader(pShader); 
}

void  D3DAPI::EnableBlending(BOOL bEnabled)
{ 
	_GS->EnableBlending(bEnabled); 
}

void  D3DAPI::BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor)
{ 
	_GS->BlendFunction(srcFactor, destFactor, fFactor); 
}

void  D3DAPI::Ortho(float left, float right, float top, float bottom, float znear, float zfar)
{
	_GS->Ortho(left, right, top, bottom, znear, zfar);
}

void  D3DAPI::SetViewport(float x, float y, float width, float height)
{ 
	_GS->SetViewport(x, y, width, height); 
}

void  D3DAPI::SetScissorRect(XRect *pRect)
{
	_GS->SetScissorRect(pRect);
}

void D3DAPI::DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2)
{
	_GS->DrawSprite(texture, color, x, y, x2, y2);
}

void D3DAPI::DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2)
{
	_GS->DrawSpriteEx(texture, color, x, y, x2, y2, u, v, u2, v2);
}

void  D3DAPI::DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees)
{
	_GS->DrawSpriteExRotate(texture, color, x, y, x2, y2, degrees, u, v, u2, v2, texDegrees);
}

void D3DAPI::DrawBox(const Vect2 &upperLeft, const Vect2 &size)
{
	_GS->DrawBox(upperLeft, size);
}

void D3DAPI::SetCropping(float top, float left, float bottom, float right)
{
	_GS->SetCropping(top, left, bottom, right);
}

Texture* D3DAPI::CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps)
{
	return _GS->CreateRenderTarget(width, height, colorFormat, bGenMipMaps);
}

void D3DAPI::SetRenderTarget(Texture *texture)
{
	_GS->SetRenderTarget(texture);
}

void  D3DAPI::ClearRenderTarget(DWORD color)
{
	_GS->ClearRenderTarget(color);
}

void  D3DAPI::CopyTexture(Texture *texDest, Texture *texSrc)
{
	_GS->CopyTexture(texDest, texSrc);
}

HRESULT D3DAPI::Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP  map_type)
{
	return _GS->Map(tex, lpData, pitch, map_type);
}

void D3DAPI::Unmap(Texture *tex)
{
	_GS->Unmap(tex);
}

void D3DAPI::SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch)
{
	_GS->SetImage(tex, lpData, imageFormat, pitch);
}

void D3DAPI::GetTextureWH(Texture* tex, DWORD& width, DWORD& height)
{
	_GS->GetTextureWH(tex, width, height);
}

bool  D3DAPI::GetTextureDC(Texture* tex, HDC &hDC)
{
	return _GS->GetTextureDC(tex, hDC);
}

void  D3DAPI::ReleaseTextureDC(Texture* tex)
{
	_GS->ReleaseTextureDC(tex);
}

void D3DAPI::Flush()
{
	_GS->Flush();
}

HRESULT D3DAPI::GetDeviceRemovedReason()
{
	return _GS->GetDeviceRemovedReason();
}

void D3DAPI::Present(Texture *TextSwap)
{
	_GS->Present(TextSwap);
}

OutputDuplicator* D3DAPI::CreateOutputDuplicator(UINT outputID)
{
	return _GS->CreateOutputDuplicator(outputID);
}

void D3DAPI::UnloadAllData()
{
	_GS->UnloadAllData();
}

void D3DAPI::ResizeView(Texture *TextView)
{
	_GS->ResizeView(TextView);
}

Texture * D3DAPI::CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height)
{
	return _GS->CreateRenderTargetSwapChain(Hwnd, Width, Height);
}

HINSTANCE hinstMain = NULL;

BOOL CALLBACK DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpBla)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
#if defined _M_X64 && _MSC_VER == 1800
		_set_FMA3_enable(0);
#endif
		hinstMain = hInst;
	}

	return TRUE;
}

