#include "D3DResize.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif

CD3DReszie::CD3DReszie(UINT AdpterID)
{
	m_SrcW = 0;
	m_SrcH = 0;
	m_ColorForamt = ColorType_I420;
	m_DesW = 0;
	m_DesH = 0;
	mainVertexShader = NULL;
	mainPixelShader = NULL;
	colorConvertShader = NULL;
	colorFieldConvertShader = NULL;
	RGBFieldShader = NULL;
	DesTexture = NULL;
	SrcTexture = NULL;
	copyTextures = NULL;
	SDILittleTexture = NULL;
	m_bFieldSignal = false;
	CopyData = NULL;
	D3DRender = new D3DAPI(AdpterID);
}

CD3DReszie::~CD3DReszie()
{
	if (mainVertexShader)
		delete mainVertexShader;
	if (mainPixelShader)
		delete mainPixelShader;
	if (colorConvertShader)
		delete colorConvertShader;
	if (colorFieldConvertShader)
		delete colorFieldConvertShader;
	if (DesTexture)
		delete DesTexture;
	if (SrcTexture)
		delete SrcTexture;
	if (copyTextures)
		delete copyTextures;

	if (SDILittleTexture)
		delete SDILittleTexture;

	if (RGBFieldShader)
		delete RGBFieldShader;

	if (CopyData)
		delete [] CopyData;

	if (D3DRender)
		delete D3DRender;
}

bool CD3DReszie::Init()
{
	if (!D3DRender)
		return false;

	mainVertexShader = D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader"));

	if (!mainVertexShader)
		return false;

	mainPixelShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawTexture.pShader"));

	if (!mainPixelShader)
		return false;

	ChangeShader();

	RGBFieldShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders/Field_RGB.pShader"));

	if (!RGBFieldShader)
		return false;

	UINT Width, Height;
	SDILittleTexture = D3DRender->CreateTextureFromFile(L"./img/SDIOUT.png", FALSE, Width, Height);

	if (SDILittleTexture)
		return false;

	return true;
}

unsigned char* CD3DReszie::Resize(unsigned char* SrcData, int SrcW, int SrcH, TColorType ColorFormat, int DesW, int DesH, bool bFieldSignal)
{
	if (SrcW == DesW && SrcH == DesH)
		return SrcData;

	m_bFieldSignal = bFieldSignal;

	ChangeSize(SrcW, SrcH, ColorFormat, DesW, DesH);

	return ResizeRender(SrcData);
}

String CD3DReszie::ChooseShader(bool bNeedField /*= true*/)
{
	if (m_ColorForamt == DeviceOutputType_RGB || m_ColorForamt == DeviceOutputType_RGB24)
		return String();

	String strShader;
	strShader << SHADER_PATH;

	if (bNeedField)
		strShader << L"Field_";

	if (m_ColorForamt == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
	else if (m_ColorForamt == DeviceOutputType_YV12)
		strShader << TEXT("YVUToRGB.pShader");
	else if (m_ColorForamt == DeviceOutputType_YVYU)
		strShader << TEXT("YVXUToRGB.pShader");
	else if (m_ColorForamt == DeviceOutputType_YUY2)
		strShader << TEXT("YUXVToRGB.pShader");
	else if (m_ColorForamt == DeviceOutputType_UYVY)
		strShader << TEXT("UYVToRGB.pShader");
	else if (m_ColorForamt == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
	{
		strShader.Clear();
	}

	return strShader;
}

void CD3DReszie::ChangeShader()
{
	String strShader = ChooseShader(false);

	if (strShader.IsValid() && (!strShaderOld.Compare(strShader)))
	{
		if (colorConvertShader)
		{
			delete colorConvertShader;
			colorConvertShader = NULL;
		}

		if (colorFieldConvertShader)
		{
			delete colorFieldConvertShader;
			colorFieldConvertShader = NULL;
		}

		colorConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);
		strShaderOld = strShader;


		strShader = ChooseShader();
		colorFieldConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);

	}
	else if (m_ColorForamt == DeviceOutputType_RGB || m_ColorForamt == DeviceOutputType_RGB24)
	{
		if (colorConvertShader)
		{
			delete colorConvertShader;
			colorConvertShader = NULL;
		}

		if (colorFieldConvertShader)
		{
			delete colorFieldConvertShader;
			colorFieldConvertShader = NULL;
		}
		strShaderOld.Clear();
	}
}

void CD3DReszie::ChangeSize(int SrcW, int SrcH, TColorType ColorFormat,int DesW, int DesH)
{
	if (SrcW != m_SrcW || SrcH != m_SrcH)
	{
		m_SrcW = SrcW;
		m_SrcH = SrcH;
		if (SrcTexture)
		{
			delete SrcTexture;
			SrcTexture = NULL;
		}

		if (m_ColorForamt == DeviceOutputType_RGB)
		{
			SrcTexture = D3DRender->CreateTexture(m_SrcW, m_SrcH, GS_BGR, NULL, FALSE, FALSE);
		}
		else
		{
			SrcTexture = D3DRender->CreateTexture(m_SrcW, m_SrcH, GS_RGB, NULL, FALSE, FALSE);
		}
	}

	if (DesW != m_DesW || DesH != m_DesH)
	{
		m_DesW = DesW;
		m_DesH = DesH;
		if (DesTexture)
		{
			delete DesTexture;
			DesTexture = NULL;
		}

		DesTexture = D3DRender->CreateRenderTarget(m_DesW, m_DesH, GS_BGRA, FALSE);

		if (copyTextures)
		{
			delete copyTextures;
			copyTextures = NULL;
		}

		copyTextures = D3DRender->CreateTextureRead(m_DesW, m_DesH);

		if (CopyData)
			delete[] CopyData;
		CopyData = new unsigned char[m_DesW * m_DesH * 4];

		memset(CopyData, 0, m_DesW * m_DesH * 4);
	}

	if (ColorFormat != m_ColorForamt)
	{
		m_ColorForamt = ColorFormat;

		ChangeShader();
	}
}

unsigned char* CD3DReszie::ResizeRender(unsigned char* SrcData)
{
	if (!CopyData)
		return NULL;

	D3DRender->SetRenderTarget(DesTexture);
	D3DRender->ClearRenderTarget(0xFF000000);

	D3DRender->LoadVertexShader(mainVertexShader);
	D3DRender->LoadPixelShader(mainPixelShader);
	D3DRender->Ortho(0.0f, m_DesW, m_DesH, 0.0f, -100.0f, 100.0f);
	D3DRender->SetViewport(0, 0, m_DesW, m_DesH);

	if (m_ColorForamt == DeviceOutputType_RGB || m_ColorForamt == DeviceOutputType_RGB24)
	{
		if (SrcTexture)
		{
			D3DRender->SetImage(SrcTexture, SrcData, GS_IMAGEFORMAT_BGRX, m_SrcW * 4);
		}
	}
	else if (m_ColorForamt == DeviceOutputType_I420 || m_ColorForamt == DeviceOutputType_YV12)
	{
		LPBYTE lpData;
		UINT pitch;

		if (S_OK == D3DRender->Map(SrcTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
		{
			PackPlanar(lpData, SrcData, m_SrcW, m_SrcH, pitch, 0, m_SrcH, m_SrcW, 0);
			D3DRender->Unmap(SrcTexture);
		}
		
	}
	else if (m_ColorForamt == DeviceOutputType_YVYU || m_ColorForamt == DeviceOutputType_YUY2)
	{
		LPBYTE lpData;
		UINT pitch;

		if (S_OK == D3DRender->Map(SrcTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
		{
			Convert422To444(lpData, SrcData, pitch, true);
			D3DRender->Unmap(SrcTexture);
		}

	}
	else if (m_ColorForamt == DeviceOutputType_UYVY || m_ColorForamt == DeviceOutputType_HDYC)
	{
		LPBYTE lpData;
		UINT pitch;

		if (S_OK == D3DRender->Map(SrcTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
		{
			Convert422To444(lpData, SrcData, pitch, false);
			D3DRender->Unmap(SrcTexture);
		}
	}

	if (m_bFieldSignal)
	{
		if (m_ColorForamt == DeviceOutputType_RGB || m_ColorForamt == DeviceOutputType_RGB24)
			D3DRender->LoadPixelShader(RGBFieldShader);
		else if (colorFieldConvertShader)
		{
			D3DRender->LoadPixelShader(colorFieldConvertShader);
			colorFieldConvertShader->SetFloat(colorFieldConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
		}
	}
	else if (colorConvertShader)
	{
		D3DRender->LoadPixelShader(colorConvertShader);
		colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), 1.0f);
	}

	D3DRender->DrawSprite(SrcTexture, 0xFFFFFFFF, 0, 0, m_DesW, m_DesH);

	D3DRender->CopyTexture(copyTextures, DesTexture);


	BYTE *lpData;
	UINT Pitch;
	HRESULT result;
	
	if (SUCCEEDED(result = D3DRender->Map(copyTextures, lpData, Pitch)))
	{
		
		//memcpy(CopyData, lpData, Pitch * m_DesH);
		return lpData;
		
	}
	
	return NULL;
}

void CD3DReszie::UnMap()
{
	D3DRender->Unmap(copyTextures);
}

void CD3DReszie::Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY)
{
	DWORD lineSize = m_SrcW * 2;
	DWORD size = lineSize;
	DWORD dwDWSize = size >> 2;

	if (bLeadingY)
	{
		for (UINT y = 0; y < m_SrcH; ++y)
		{
			LPDWORD output = (LPDWORD)(convertBuffer + (y*pitch));
			LPDWORD inputDW = (LPDWORD)(lp422 + (y*lineSize));
			LPDWORD inputDWEnd = inputDW + dwDWSize;

			while (inputDW < inputDWEnd)
			{
				register DWORD dw = *inputDW;

				output[0] = dw;
				dw &= 0xFFFFFF00;
				dw |= BYTE(dw >> 16);
				output[1] = dw;

				output += 2;
				inputDW++;
			}
		}
	}
	else
	{
		for (UINT y = 0; y < m_SrcH; ++y)
		{
			LPDWORD output = (LPDWORD)(convertBuffer + (y*pitch));
			LPDWORD inputDW = (LPDWORD)(lp422 + (y*lineSize));
			LPDWORD inputDWEnd = inputDW + dwDWSize;

			while (inputDW < inputDWEnd)
			{
				register DWORD dw = *inputDW;

				output[0] = dw;
				dw &= 0xFFFF00FF;
				dw |= (dw >> 16) & 0xFF00;
				output[1] = dw;

				output += 2;
				inputDW++;
			}
		}
	}
}

D3DAPI * CD3DReszie::GetD3DRender() const
{
	return D3DRender;
}

Shader * CD3DReszie::GetMainPixelShader() const
{
	return mainPixelShader;
}

Shader * CD3DReszie::GetMainVertexShader() const
{
	return mainVertexShader;
}

Texture * CD3DReszie::GetSDITexture() const
{
	return SDILittleTexture;
}

