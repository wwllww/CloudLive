// D3DRender.cpp : 定义 DLL 应用程序的导出函数。
//

#include "D3DRender.h"

GraphicsSystem* d3dAPI::GetInstance()
{
	return GS;
}

bool LoadPlugin()
{
	GS = new D3D10System;
	return true;
}

void UnloadPlugin()
{
	unInitD3D();
}

void initD3D(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID)
{
	GS->initD3D(renderFrameWidth, renderFrameHeight, hwndRenderFrame,adapterID);
}

void unInitD3D()
{
	if (GS)
	{
		GS->unInitD3D();
		delete GS;
		GS = NULL;
	}
}

Texture* CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic)
{
	return GS->CreateTexture(width, height, colorFormat, lpData, bGenMipMaps, bStatic);
}

Texture* CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps,UINT &Width, UINT& Height)
{
	return GS->CreateTextureFromFile(lpFile, bBuildMipMaps,Width,Height);
}

Texture* CreateGDITexture(unsigned int width, unsigned int height)
{
	return GS->CreateGDITexture(width, height);
}

Texture* CreateTextureRead(unsigned int width, unsigned int height)
{
	return GS->CreateTextureRead(width, height);
}

SamplerState* CreateSamplerState(SamplerInfo &info)          
{ 
	return GS->CreateSamplerState(info); 
}

Shader* CreateVertexShaderFromFile(CTSTR lpFileName)         
{ 
	return GS->CreateVertexShaderFromFile(lpFileName); 
}

Shader* CreatePixelShaderFromFile(CTSTR lpFileName)          
{ 
	return GS->CreatePixelShaderFromFile(lpFileName); 
}

Shader* GetCurrentVertexShader()                         
{ 
	return GS->GetCurrentVertexShader(); 
}

Shader* GetCurrentPixelShader()                         
{ 
	return GS->GetCurrentPixelShader(); 
}

void  LoadTexture(Texture *texture, UINT idTexture)    
{ 
	GS->LoadTexture(texture, idTexture); 
}

void  LoadSamplerState(SamplerState *sampler, UINT idSampler) 
{ 
	GS->LoadSamplerState(sampler, idSampler); 
}

void  LoadVertexShader(Shader *vShader)                  
{ 
	GS->LoadVertexShader(vShader); 
}

void  LoadPixelShader(Shader *pShader)                   
{ 
	GS->LoadPixelShader(pShader); 
}

void  EnableBlending(BOOL bEnabled)                      
{ 
	GS->EnableBlending(bEnabled); 
}

void  BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor) 
{ 
	GS->BlendFunction(srcFactor, destFactor, fFactor); 
}

void  Ortho(float left, float right, float top, float bottom, float znear, float zfar)
{
	GS->Ortho(left, right, top, bottom, znear, zfar);
}

void  SetViewport(float x, float y, float width, float height) 
{ 
	GS->SetViewport(x, y, width, height); 
}

void  SetScissorRect(XRect *pRect)                        
{
	GS->SetScissorRect(pRect);
}

void DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2)
{
	GS->DrawSprite(texture, color, x, y, x2, y2);
}

void DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2)
{
	GS->DrawSpriteEx(texture, color, x, y, x2, y2, u, v, u2, v2);
}

void  DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees)
{
	GS->DrawSpriteExRotate(texture, color, x, y, x2, y2, degrees, u, v, u2, v2, texDegrees);
}

void DrawBox(const Vect2 &upperLeft, const Vect2 &size)
{
	GS->DrawBox(upperLeft, size);
}

void SetCropping(float top, float left, float bottom, float right)
{
	GS->SetCropping(top, left, bottom, right);
}

Texture* CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps)
{
	return GS->CreateRenderTarget(width, height, colorFormat, bGenMipMaps);
}

void SetRenderTarget(Texture *texture)
{
	GS->SetRenderTarget(texture);
}

void  ClearRenderTarget(DWORD color)
{
	GS->ClearRenderTarget(color);
}

void  CopyTexture(Texture *texDest, Texture *texSrc)
{
	GS->CopyTexture(texDest, texSrc);
}

HRESULT  Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP  map_type)
{
	return GS->Map(tex, lpData, pitch, map_type);
}

void Unmap(Texture *tex)
{
	GS->Unmap(tex);
}

void SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch)
{
	GS->SetImage(tex, lpData, imageFormat, pitch);
}

void GetTextureWH(Texture* tex, DWORD& width, DWORD& height)
{
	GS->GetTextureWH(tex, width, height);
}

bool  GetTextureDC(Texture* tex, HDC &hDC)
{
	return GS->GetTextureDC(tex, hDC);
}

void  ReleaseTextureDC(Texture* tex)
{
	GS->ReleaseTextureDC(tex);
}

void Flush()
{
	GS->Flush();
}

HRESULT GetDeviceRemovedReason()
{
	return GS->GetDeviceRemovedReason();
}

void Present(Texture *TextSwap)
{
	GS->Present(TextSwap);
}

OutputDuplicator* CreateOutputDuplicator(UINT outputID)
{
	return GS->CreateOutputDuplicator(outputID);
}

void UnloadAllData()
{
	GS->UnloadAllData();
}

void ResizeView(Texture *TextView)
{
	GS->ResizeView(TextView);
}

ID3D11Device* GetD3D()
{
	return static_cast<D3D10System*>(GS)->GetDeviceInline();
}

ID3D11DeviceContext* GetD3DCtx()
{
	return static_cast<D3D10System*>(GS)->GetContextInline();
}

Texture * CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height)
{
	return GS->CreateRenderTargetSwapChain(Hwnd, Width, Height);
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


