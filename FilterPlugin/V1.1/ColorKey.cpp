#include "ColorKey.h"

IMPLEMENT_DYNIC_FILTER(FilterColorKey, "ɫֵ", true)

FilterColorKey::FilterColorKey()
{
	RenderTarget = NULL;
	PixShader = NULL;
	width = 1280;
	hight = 720;
	KeyColor = 0xFF00FF00;
	Similarity = 80.0f;
	Smoothness = 50.0f;
	HKeyColor = NULL;
	HSimilarity = NULL;
	HSmoothness = NULL;
	D3DRender = GetD3DRender();
}

FilterColorKey::~FilterColorKey()
{
	if (RenderTarget)
		delete RenderTarget;
	RenderTarget = NULL;

	if (PixShader)
		delete PixShader;
	PixShader = NULL;
}

bool FilterColorKey::InitFilter(UINT Width, UINT Height)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin! Width = %d,Height = %d", __FUNCTION__, Width, Height);

	PixShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders/ColorKey.pShader"));

	if (!PixShader)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreatePixelShaderFromFile failed! shaders/ColorKey.pShader" __FUNCTION__);
		return false;
	}

	RenderTarget = D3DRender->CreateRenderTarget(Width, Height, GS_BGRA, FALSE);
	if (!RenderTarget)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreateRenderTarget failed!" __FUNCTION__);
		return false;
	}

	HKeyColor = PixShader->GetParameterByName(TEXT("key_color"));
	HSimilarity = PixShader->GetParameterByName(TEXT("similarity"));
	HSmoothness = PixShader->GetParameterByName(TEXT("smoothness"));

	width = Width;
	hight = Height;

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);

	return true;
}

void FilterColorKey::GetDefaults(Value &JsonDefaults)
{
	JsonDefaults["KeyColor"] = 0xFF00FF00;
	JsonDefaults["Similarity"] = 80.0f;
	JsonDefaults["Smoothness"] = 50.0f;
}

void FilterColorKey::UpDataSetting(Value &JsonDefaults)
{
	KeyColor = JsonDefaults["KeyColor"].asUInt();
	Similarity = JsonDefaults["Similarity"].asDouble();
	Smoothness = JsonDefaults["Smoothness"].asDouble();
}

Texture * FilterColorKey::GetRenderTarget()
{
	return RenderTarget;
}

void FilterColorKey::FilterRender(Texture *Target, const Vect2 &NewSize)
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

		PixShader->SetColor(HKeyColor, KeyColor);
		PixShader->SetFloat(HSimilarity, Similarity / 1000);
		PixShader->SetFloat(HSmoothness, Smoothness / 1000);

		D3DRender->LoadPixelShader(PixShader);

		D3DRender->DrawSprite(Target, 0xFFFFFFFF, 0.0f, 0.0f, width, hight);
	}
}
