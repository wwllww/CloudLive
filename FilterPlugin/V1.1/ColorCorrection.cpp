#include "ColorCorrection.h"

IMPLEMENT_DYNIC_FILTER(FilterColorCorrection, "ÑÕÉ«Ð£Õý", true)

FilterColorCorrection::FilterColorCorrection()
{
	RenderTarget = NULL;
	PixShader = NULL;
	width = 1280;
	hight = 720;
	Hue = 0.0f;
	Saturation = 0.0f;
	Lightness = 0.0f;
	HHue = NULL;
	HSatu = NULL;
	HLight = NULL;
	D3DRender = GetD3DRender();
}

FilterColorCorrection::~FilterColorCorrection()
{
	if (RenderTarget)
		delete RenderTarget;
	RenderTarget = NULL;

	if (PixShader)
		delete PixShader;
	PixShader = NULL;
}

bool FilterColorCorrection::InitFilter(UINT Width, UINT Height)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin! Width = %d,Height = %d", __FUNCTION__, Width, Height);

	PixShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders/ColorCorrection.pShader"));

	if (!PixShader)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreatePixelShaderFromFile failed! shaders/ColorCorrection.pShader" __FUNCTION__);
		return false;
	}

	RenderTarget = D3DRender->CreateRenderTarget(Width, Height, GS_BGRA, FALSE);
	if (!RenderTarget)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreateRenderTarget failed!" __FUNCTION__);
		return false;
	}

	HHue = PixShader->GetParameterByName(TEXT("Hue_val"));
	HSatu = PixShader->GetParameterByName(TEXT("Satu_val"));
	HLight = PixShader->GetParameterByName(TEXT("HLight_val"));

	width = Width;
	hight = Height;

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);

	return true;
}

void FilterColorCorrection::GetDefaults(Value &JsonDefaults)
{
	JsonDefaults["Hue"] = 0.0f;
	JsonDefaults["Saturation"] = 0.0f;
	JsonDefaults["Lightness"] = 0.0f;
}

void FilterColorCorrection::UpDataSetting(Value &JsonDefaults)
{
	Hue = JsonDefaults["Hue"].asDouble();
	Saturation = JsonDefaults["Saturation"].asDouble();
	Lightness = JsonDefaults["Lightness"].asDouble();
}

Texture * FilterColorCorrection::GetRenderTarget()
{
	return RenderTarget;
}

void FilterColorCorrection::FilterRender(Texture *Target, const Vect2 &NewSize)
{
	if (Target && PixShader)
	{
		if (NewSize != Vect2(width, hight))
		{
			D3DRender->SetRenderTarget(NULL);

			if (RenderTarget)
			{
				delete RenderTarget;
			}
			width = NewSize.x;
			hight = NewSize.y;

			RenderTarget = D3DRender->CreateRenderTarget(width, hight, GS_BGRA, FALSE);

			D3DRender->SetRenderTarget(RenderTarget);
			D3DRender->ClearRenderTarget(0xFF000000);
		}

		PixShader->SetFloat(HHue, Hue);
		PixShader->SetFloat(HSatu, Saturation);
		PixShader->SetFloat(HLight, Lightness);

		D3DRender->LoadPixelShader(PixShader);

		D3DRender->DrawSprite(Target, 0xFFFFFFFF, 0.0f, 0.0f, width, hight);
	}
}
