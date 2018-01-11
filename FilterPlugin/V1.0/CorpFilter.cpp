#include "CorpFilter.h"

IMPLEMENT_DYNIC_FILTER(FilterCorp, "²Ã¼ô", true)
FilterCorp::FilterCorp()
{
	RenderTarget = NULL;
	PixShader = NULL;
	VertexShader = NULL;

	Mul_val = Vect2(1.0f,1.0f);
	Add_val = Vect2(0.0f,0.0f);
	width = 640;
	hight = 480;
}

FilterCorp::~FilterCorp()
{
	if (RenderTarget)
		delete RenderTarget;
	RenderTarget = NULL;

	if (PixShader)
		delete PixShader;
	PixShader = NULL;

	if (VertexShader)
		delete VertexShader;
	VertexShader = NULL;
}

bool FilterCorp::InitFilter(UINT Width, UINT Height)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin! Width = %d,Height = %d", __FUNCTION__, Width, Height);

	PixShader = CreatePixelShaderFromFile(TEXT("shaders/Crop_filter.pShader"));

	if (!PixShader)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreatePixelShaderFromFile failed! shaders/Crop_filter.pShader" __FUNCTION__);
		return false;
	}

	VertexShader = CreateVertexShaderFromFile(TEXT("shaders/Crop_filter.vShader"));

	if (!VertexShader)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!CreateVertexShaderFromFile failed! shaders/Crop_filter.vShader" __FUNCTION__);
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

	HMul = VertexShader->GetParameterByName(TEXT("mul_val"));
	HAdd = VertexShader->GetParameterByName(TEXT("add_val"));

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
	return true;
}

void FilterCorp::GetDefaults(Value &JsonDefaults)
{
	JsonDefaults["Mul_valx"] = 1.0f;
	JsonDefaults["Mul_valy"] = 1.0f;

	JsonDefaults["Add_valx"] = 0.0f;
	JsonDefaults["Add_valy"] = 0.0f;
}

void FilterCorp::UpDataSetting(Value &JsonDefaults)
{
	Mul_val.x = JsonDefaults["Mul_valx"].asDouble(); //ÓÒÏÂ¼ôÇÐ
	Mul_val.y = JsonDefaults["Mul_valy"].asDouble();

	Add_val.x = JsonDefaults["Add_valx"].asDouble();//×óÉÏ¼ôÇÐ
	Add_val.y = JsonDefaults["Add_valy"].asDouble();
}

Texture * FilterCorp::GetRenderTarget()
{
	return RenderTarget;
}

void FilterCorp::FilterRender(Texture *Target, const Vect2 &NewSize)
{
	if (Target && VertexShader && PixShader)
	{
		VertexShader->SetVector2(HMul, Mul_val);
		VertexShader->SetVector2(HAdd, Add_val);

		LoadVertexShader(VertexShader);
		LoadPixelShader(PixShader);

		Ortho(0.0f, width * Mul_val.x, hight * Mul_val.y, 0.0f, -100.0f, 100.0f);
		SetViewport(0, 0, width * Mul_val.x, hight * Mul_val.y);

		DrawSprite(Target, 0xFFFFFFFF, width * Add_val.x, hight * Add_val.y, width * Mul_val.x, hight * Mul_val.y);
	}
}
