
#pragma once

#define WINVER         0x0600
#define _WIN32_WINDOWS 0x0600
#define _WIN32_WINNT   0x0600
#define NTDDI_VERSION  NTDDI_VISTASP1
#define WIN32_LEAN_AND_MEAN
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

class d3dAPI
{
public:
	d3dAPI();
	~d3dAPI();

	static GraphicsSystem *GetInstance();
};

#define EXTERN_DLLEXPORT extern "C" __declspec(dllexport)

EXTERN_DLLEXPORT bool LoadPlugin();
EXTERN_DLLEXPORT void UnloadPlugin();

EXTERN_DLLEXPORT void initD3D(UINT renderFrameWidth, UINT renderFrameHeight, HWND hwndRenderFrame, UINT adapterID);
EXTERN_DLLEXPORT void unInitD3D();

EXTERN_DLLEXPORT Texture* CreateTexture(unsigned int width, unsigned int height, GSColorFormat colorFormat, void *lpData, BOOL bGenMipMaps, BOOL bStatic);
EXTERN_DLLEXPORT Texture* CreateTextureFromFile(CTSTR lpFile, BOOL bBuildMipMaps,UINT &Width, UINT& Height);
EXTERN_DLLEXPORT Texture* CreateGDITexture(unsigned int width, unsigned int height);
EXTERN_DLLEXPORT Texture* CreateTextureRead(unsigned int width, unsigned int height);

EXTERN_DLLEXPORT SamplerState* CreateSamplerState(SamplerInfo &info);

EXTERN_DLLEXPORT Shader* CreateVertexShaderFromFile(CTSTR lpFileName);
EXTERN_DLLEXPORT Shader* CreatePixelShaderFromFile(CTSTR lpFileName);
EXTERN_DLLEXPORT Shader* GetCurrentVertexShader();
EXTERN_DLLEXPORT Shader* GetCurrentPixelShader();

EXTERN_DLLEXPORT void  LoadTexture(Texture *texture, UINT idTexture);
EXTERN_DLLEXPORT void  LoadSamplerState(SamplerState *sampler, UINT idSampler);
EXTERN_DLLEXPORT void  LoadVertexShader(Shader *vShader);
EXTERN_DLLEXPORT void  LoadPixelShader(Shader *pShader);

EXTERN_DLLEXPORT void  EnableBlending(BOOL bEnabled);
EXTERN_DLLEXPORT void  BlendFunction(GSBlendType srcFactor, GSBlendType destFactor, float fFactor);

EXTERN_DLLEXPORT void  Ortho(float left, float right, float top, float bottom, float znear, float zfar);
EXTERN_DLLEXPORT void  SetViewport(float x, float y, float width, float height);
EXTERN_DLLEXPORT void  SetScissorRect(XRect *pRect);

EXTERN_DLLEXPORT void  DrawSprite(Texture *texture, DWORD color, float x, float y, float x2, float y2);
EXTERN_DLLEXPORT void  DrawSpriteEx(Texture *texture, DWORD color, float x, float y, float x2, float y2, float u, float v, float u2, float v2);
EXTERN_DLLEXPORT void  DrawSpriteExRotate(Texture *texture, DWORD color, float x, float y, float x2, float y2, float degrees, float u, float v, float u2, float v2, float texDegrees);
EXTERN_DLLEXPORT void  DrawBox(const Vect2 &upperLeft, const Vect2 &size);
EXTERN_DLLEXPORT void  SetCropping(float top, float left, float bottom, float right);

EXTERN_DLLEXPORT Texture* CreateRenderTarget(unsigned int width, unsigned int height, GSColorFormat colorFormat, BOOL bGenMipMaps);
EXTERN_DLLEXPORT void  SetRenderTarget(Texture *texture);
EXTERN_DLLEXPORT void  ClearRenderTarget(DWORD color);

EXTERN_DLLEXPORT void  CopyTexture(Texture *texDest, Texture *texSrc);
EXTERN_DLLEXPORT HRESULT  Map(Texture *tex, BYTE *&lpData, UINT &pitch, D3D11_MAP map_type = D3D11_MAP_READ);
EXTERN_DLLEXPORT void  Unmap(Texture *tex);
EXTERN_DLLEXPORT void  SetImage(Texture *tex, void *lpData, GSImageFormat imageFormat, UINT pitch);
EXTERN_DLLEXPORT void  GetTextureWH(Texture* tex, DWORD& width, DWORD& height);
EXTERN_DLLEXPORT bool  GetTextureDC(Texture* tex, HDC &hDC);
EXTERN_DLLEXPORT void  ReleaseTextureDC(Texture* tex);

EXTERN_DLLEXPORT void  Flush();
EXTERN_DLLEXPORT HRESULT GetDeviceRemovedReason();
EXTERN_DLLEXPORT void  Present(Texture *TextSwap);

EXTERN_DLLEXPORT OutputDuplicator* CreateOutputDuplicator(UINT outputID);
EXTERN_DLLEXPORT void UnloadAllData();
EXTERN_DLLEXPORT void ResizeView(Texture *TextView);
EXTERN_DLLEXPORT ID3D11Device*        GetD3D();
EXTERN_DLLEXPORT ID3D11DeviceContext* GetD3DCtx();

EXTERN_DLLEXPORT Texture *CreateRenderTargetSwapChain(HWND Hwnd, UINT Width, UINT Height);