#include "FilterPlugin.h"

IMPLEMENT_DYNIC_FILTER(FilterColor_Key,"ÑÕÉ«Öµ",true)

FilterColor_Key::FilterColor_Key()
{
	RenderTarget = NULL;
	PixShader = NULL;
	Color = Vect4(1.f, 1.0f, 1.0f, 1.f);
	Similarity = 1.0f;
	Blend = 1.0f;
	Gamma = 1.0f;
	width = 640;
	hight = 480;
}

FilterColor_Key::~FilterColor_Key()
{
	if (RenderTarget)
		delete RenderTarget;
	RenderTarget = NULL;

	if (PixShader)
		delete PixShader;
	PixShader = NULL;
}

bool FilterColor_Key::InitFilter(UINT Width, UINT Height)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin! Width = %d,Height = %d", __FUNCTION__,Width,Height);

	PixShader = CreatePixelShaderFromFile(TEXT("shaders/ColorKey_RGB.pShader"));

	if (!PixShader)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreatePixelShaderFromFile failed! shaders/ColorKey_RGB.pShader" __FUNCTION__);
		return false;
	}
		
	RenderTarget = CreateRenderTarget(Width, Height, GS_BGRA, FALSE);
	if (!RenderTarget)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreateRenderTarget failed!" __FUNCTION__);
		return false;
	}
	width = Width;
	hight = Height;

	HColorKey = PixShader->GetParameterByName(TEXT("colorKey"));
	hSimilarity = PixShader->GetParameterByName(TEXT("similarity"));
	hBlend = PixShader->GetParameterByName(TEXT("blend"));
	hGamma = PixShader->GetParameterByName(TEXT("gamma"));

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!" ,__FUNCTION__);
	return false;
}

void FilterColor_Key::GetDefaults(Value &JsonDefaults)
{
	JsonDefaults["colorKeyR"] = 1;
	JsonDefaults["colorKeyG"] = 1;
	JsonDefaults["colorKeyB"] = 1;
	JsonDefaults["colorKeyA"] = 1;
	JsonDefaults["similarity"] = 1;
	JsonDefaults["blend"] = 1;
	JsonDefaults["gamma"] = 1.0f;

}

void FilterColor_Key::UpDataSetting(Value &JsonDefaults)
{
	Color.x = JsonDefaults["colorKeyR"].asDouble();
	Color.y = JsonDefaults["colorKeyG"].asDouble();
	Color.z = JsonDefaults["colorKeyB"].asDouble();
	Color.w = JsonDefaults["colorKeyA"].asDouble();
	Similarity = JsonDefaults["similarity"].asDouble();
	Blend = JsonDefaults["blend"].asDouble();
	Gamma = JsonDefaults["gamma"].asDouble();
}

Texture * FilterColor_Key::GetRenderTarget()
{
	return RenderTarget;
}

void FilterColor_Key::FilterRender(Texture *Target, const Vect2 &NewSize)
{
	if (Target && PixShader)
	{
		LoadPixelShader(PixShader);

		PixShader->SetVector4(HColorKey, Color);
		PixShader->SetFloat(hSimilarity, Similarity);
		PixShader->SetFloat(hBlend, Blend);
		PixShader->SetFloat(hGamma, Gamma);

		DrawSprite(Target, 0xFFFFFFFF, 0.0f, 0.0f, width, hight);
	}
}


