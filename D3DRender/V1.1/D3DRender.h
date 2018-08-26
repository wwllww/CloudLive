
#pragma once
#ifndef WINVER
#define WINVER         0x0600
#endif

#define _WIN32_WINDOWS 0x0600
#define _WIN32_WINNT   0x0600
#define NTDDI_VERSION  NTDDI_VISTASP1
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define ISOLATION_AWARE_ENABLED 1
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <commdlg.h>
#include <math.h>

#include <xmmintrin.h>
#include <emmintrin.h>

#define USE_AAC 1

#pragma warning(default : 4265)

#include <D3D11.h>
#include <D3DX11.h>
#include <DXGI.h>

#define USE_DXGI1_2 1

#ifdef USE_DXGI1_2
#include <dxgi1_2.h>
#endif


//#pragma comment(lib,"Common.lib")

#include "common.h"

#include "CodeTokenizer.h"

#include "D3D10System.h"
#include "LogDeliver.h"

#ifdef D3DRENDER_EXPORTS
#define EXTERN_DLLEXPORT  __declspec(dllexport)
#else
#define EXTERN_DLLEXPORT  __declspec(dllimport)
#endif // BUTELAPI_EXPORTS

#ifndef LOG_RTSPSERV
#define LOG_RTSPSERV  ((long long)1<<49)
#endif // !LOG_RTSPSERV

class EXTERN_DLLEXPORT D3DAPI
{
	GraphicsSystem *_GS = NULL;
public:
	explicit D3DAPI(UINT adapterID);
	D3DAPI(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID);
	~D3DAPI();

	Texture* CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic);
	Texture* CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps, UINT &Width, UINT& Height);
	Texture* CreateGDITexture(unsigned int width, unsigned int height);
	Texture* CreateTextureRead(unsigned int width, unsigned int height);

	SamplerState* CreateSamplerState(SamplerInfo &info);

	Shader* CreateVertexShaderFromFile(CTSTR lpFileName);
	Shader* CreatePixelShaderFromFile(CTSTR lpFileName);
	Shader* GetCurrentVertexShader();
	Shader* GetCurrentPixelShader();

	void  LoadTexture(Texture *texture, UINT idTexture);
	void  LoadSamplerState(SamplerState *sampler, UINT idSampler);
	void  LoadVertexShader(Shader *vShader);
	void  LoadPixelShader(Shader *pShader);

	void  EnableBlending(BOOL bEnabled);
	void  BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor);

	void  Ortho(float left, float right, float top, float bottom, float znear, float zfar);
	void  SetViewport(float x, float y, float width, float height);
	void  SetScissorRect(XRect *pRect);

	void  DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2);
	void  DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2);
	void  DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees);
	void  DrawBox(const Vect2 &upperLeft, const Vect2 &size);
	void  SetCropping(float top, float left, float bottom, float right);

	Texture* CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps);
	void  SetRenderTarget(Texture *texture);
	void  ClearRenderTarget(DWORD color);

	void  CopyTexture(Texture *texDest, Texture *texSrc);
	HRESULT  Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP map_type = D3D11_MAP_READ);
	void  Unmap(Texture *tex);
	void  SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch);
	void  GetTextureWH(Texture* tex, DWORD& width, DWORD& height);
	bool  GetTextureDC(Texture* tex, HDC &hDC);
	void  ReleaseTextureDC(Texture* tex);

	void  Flush();
	HRESULT GetDeviceRemovedReason();
	void  Present(Texture *TextSwap);

	OutputDuplicator* CreateOutputDuplicator(UINT outputID);
    void UnloadAllData();
	void ResizeView(Texture *TextView);

	Texture *CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height);
};