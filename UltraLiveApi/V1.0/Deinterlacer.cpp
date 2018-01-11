#include "Deinterlacer.h"
#include "OperatNew.h"


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


void CDeinterlacer::Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY)
{
	DWORD size = lineSize;
	DWORD dwDWSize = size >> 2;

	if (bLeadingY)
	{
		for (UINT y = 0; y < RenderCY; y++)
		{
			LPDWORD output = (LPDWORD)(convertBuffer + (y*pitch));
			LPDWORD inputDW = (LPDWORD)(lp422 + (y*linePitch) + /*lineShift*/0);
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
		for (UINT y = 0; y < RenderCY; y++)
		{
			LPDWORD output = (LPDWORD)(convertBuffer + (y*pitch));
			LPDWORD inputDW = (LPDWORD)(lp422 + (y*linePitch) + /*lineShift*/0);
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

CDeinterlacer::CDeinterlacer(int type) :RenderCX(0), RenderCY(0), linePitch(0),
lineSize(0), previousTexture(NULL), RendTexture(NULL), colorConvertShader(NULL),
bReadyToDraw(false), imageCX(0), imageCY(0)
{
	deinterlacer.isReady = true;
	deinterlacer.type = type;
	deinterlacer.vertexShader.reset(CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader")));
	deinterlacer.pixelShader.reset(CreatePixelShaderFromFile(ChooseDeinterlacingShader()));
}

CDeinterlacer::~CDeinterlacer()
{
	if (previousTexture)
		delete previousTexture;
	if (colorConvertShader)
		delete colorConvertShader;
	if (RendTexture)
		delete RendTexture;
}

void CDeinterlacer::ChangeSize(const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType ColorType)
{
	if (!Config || Config && imageCX == renderCX &&
		imageCY == renderCY &&
		this->ColorType == ColorType &&
		deinterlacer.type == Config->type)
		return;

	this->RenderCX = renderCX;
	this->RenderCY = renderCY;
	this->ColorType = ColorType;
	imageCX = renderCX;
	imageCY = renderCY;

	switch (ColorType) {
	case DeviceOutputType_RGB:
		lineSize = renderCX * 4;
		break;
	case DeviceOutputType_I420:
	case DeviceOutputType_YV12:
		lineSize = renderCX; //per plane
		break;
	case DeviceOutputType_YVYU:
	case DeviceOutputType_YUY2:
	case DeviceOutputType_UYVY:
	case DeviceOutputType_HDYC:
		lineSize = (renderCX * 2);
		break;
	}

	linePitch = lineSize;

	deinterlacer.imageCX = renderCX;
	deinterlacer.imageCY = renderCY;


	deinterlacer.doublesFramerate = Config->doublesFramerate;
	deinterlacer.fieldOrder = Config->fieldOrder;
	deinterlacer.processor = Config->processor;
	
	deinterlacer.needsPreviousFrame = false;

	if (deinterlacer.doublesFramerate)
	{
		deinterlacer.imageCX *= 2;
	}

	switch (Config->type) {
	case DEINTERLACING_DISCARD:
		deinterlacer.imageCY = RenderCY / 2;
		linePitch = lineSize * 2;
		RenderCY /= 2;
		break;

	case DEINTERLACING_RETRO:
		deinterlacer.imageCY = RenderCY / 2;
		if (deinterlacer.processor != DEINTERLACING_PROCESSOR_GPU)
		{
			lineSize *= 2;
			linePitch = lineSize;
			RenderCY /= 2;
			RenderCX *= 2;
		}
		break;

	case DEINTERLACING__DEBUG:
		deinterlacer.imageCX *= 2;
		deinterlacer.imageCY *= 2;
	case DEINTERLACING_BLEND2x:
		//case DEINTERLACING_MEAN2x:
	case DEINTERLACING_YADIF:
	case DEINTERLACING_YADIF2x:
		deinterlacer.needsPreviousFrame = true;
		break;
	}

	if (deinterlacer.type != DEINTERLACING_NONE && deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU && deinterlacer.type != Config->type)
	{
		deinterlacer.type = Config->type;
		//deinterlacer.vertexShader.reset(CreateVertexShaderFromFile(TEXT("shaders/DrawTexture.vShader")));//已经在构造中创建
		deinterlacer.pixelShader.reset(CreatePixelShaderFromFile(ChooseDeinterlacingShader()));
		deinterlacer.isReady = false;
	}
	
	if (colorConvertShader)
	{
		delete colorConvertShader;
		colorConvertShader = NULL;
	}

	String strShader = ChooseShader();

	if (strShader.IsValid())
	{
		colorConvertShader = CreatePixelShaderFromFile(strShader);
	}

	if (previousTexture)
	{
		delete previousTexture;
		previousTexture = NULL;
	}

	if (ColorType == DeviceOutputType_RGB) //you may be confused, but when directshow outputs RGB, it's actually outputting BGR
	{
		RendTexture = CreateTexture(RenderCX, RenderCY, GS_BGR, NULL, FALSE, FALSE);
		if (deinterlacer.needsPreviousFrame)
			previousTexture = CreateTexture(RenderCX, RenderCY, GS_BGR, NULL, FALSE, FALSE);
		if (deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU)
			deinterlacer.texture.reset(CreateRenderTarget(deinterlacer.imageCX, deinterlacer.imageCY, GS_BGRA, FALSE));
	}
	else //if we're working with planar YUV, we can just use regular RGB textures instead
	{
		RendTexture = CreateTexture(RenderCX, RenderCY, GS_RGB, NULL, FALSE, FALSE);
		if (deinterlacer.needsPreviousFrame)
			previousTexture = CreateTexture(RenderCX, RenderCY, GS_RGB, NULL, FALSE, FALSE);
		if (deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU)
			deinterlacer.texture.reset(CreateRenderTarget(deinterlacer.imageCX, deinterlacer.imageCY, GS_BGRA, FALSE));
	}
}

void CDeinterlacer::SetImage(LPVOID clpData, const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType colorType)
{
	if (previousTexture)
	{
		Texture *tmp = RendTexture;
		RendTexture = previousTexture;
		previousTexture = tmp;
	}

	if (clpData)
	{
		if (colorType == DeviceOutputType_RGB)
		{
			if (RendTexture)
			{
				ChangeSize(Config, renderCX, renderCY, colorType);
				::SetImage(RendTexture, clpData, GS_IMAGEFORMAT_BGRX, linePitch);
				bReadyToDraw = true;
			}
		}
		else if (colorType == DeviceOutputType_I420 || colorType == DeviceOutputType_YV12)
		{
		
			LPBYTE lpData;
			UINT pitch;

			ChangeSize(Config, renderCX, renderCY, colorType);

			if (S_OK == Map(RendTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				PackPlanar(lpData, (LPBYTE)clpData, RenderCX, RenderCY, pitch, 0, RenderCY, linePitch, 0);
				Unmap(RendTexture);
			}

			bReadyToDraw = true;

		}
		else if (colorType == DeviceOutputType_YVYU || colorType == DeviceOutputType_YUY2)
		{
			LPBYTE lpData;
			UINT pitch;

			ChangeSize(Config, renderCX, renderCY, colorType);

			if (S_OK == Map(RendTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				Convert422To444(lpData, (LPBYTE)clpData, pitch, true);
				Unmap(RendTexture);
			}

			bReadyToDraw = true;
		}
		else if (colorType == DeviceOutputType_UYVY || colorType == DeviceOutputType_HDYC)
		{
			LPBYTE lpData;
			UINT pitch;

			ChangeSize(Config, renderCX, renderCY, colorType);

			if (S_OK == Map(RendTexture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
			{
				Convert422To444(lpData, (LPBYTE)clpData, pitch, false);
				Unmap(RendTexture);
			}

			bReadyToDraw = true;
		}
	}

	deinterlacer.curField = deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU ? false : (deinterlacer.fieldOrder == FIELD_ORDER_BFF);
	deinterlacer.bNewFrame = true;

	if (RendTexture && deinterlacer.type != DEINTERLACING_NONE &&
		deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU &&
		deinterlacer.texture.get() &&
		deinterlacer.pixelShader.get())
	{
		SetRenderTarget(deinterlacer.texture.get());

		Shader *oldVertShader = GetCurrentVertexShader();
		LoadVertexShader(deinterlacer.vertexShader.get());

		Shader *oldShader = GetCurrentPixelShader();
		LoadPixelShader(deinterlacer.pixelShader.get());

		HANDLE hField = deinterlacer.pixelShader.get()->GetParameterByName(TEXT("field_order"));
		if (hField)
			deinterlacer.pixelShader.get()->SetBool(hField, deinterlacer.fieldOrder == FIELD_ORDER_BFF);

		Ortho(0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, -100.0f, 100.0f);
		SetViewport(0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY));

		if (previousTexture)
			LoadTexture(previousTexture, 1);

		DrawSpriteEx(RendTexture, 0xFFFFFFFF, 0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, 0.0f, 1.0f, 1.0f);

		if (previousTexture)
			LoadTexture(nullptr, 1);

		LoadPixelShader(oldShader);
		LoadVertexShader(oldVertShader);
		deinterlacer.isReady = true;
	}
}

void CDeinterlacer::SetImage(Texture *texture, const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType colorType)
{
	ChangeSize(Config, renderCX, renderCY, colorType);

	deinterlacer.curField = deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU ? false : (deinterlacer.fieldOrder == FIELD_ORDER_BFF);
	deinterlacer.bNewFrame = true;

	if (RendTexture && deinterlacer.type != DEINTERLACING_NONE &&
		deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU &&
		deinterlacer.texture.get() &&
		deinterlacer.pixelShader.get())
	{
		SetRenderTarget(deinterlacer.texture.get());

		Shader *oldVertShader = GetCurrentVertexShader();
		LoadVertexShader(deinterlacer.vertexShader.get());

		Shader *oldShader = GetCurrentPixelShader();
		LoadPixelShader(deinterlacer.pixelShader.get());

		HANDLE hField = deinterlacer.pixelShader.get()->GetParameterByName(TEXT("field_order"));
		if (hField)
			deinterlacer.pixelShader.get()->SetBool(hField, deinterlacer.fieldOrder == FIELD_ORDER_BFF);

		Ortho(0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, -100.0f, 100.0f);
		SetViewport(0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY));

		DrawSpriteEx(texture, 0xFFFFFFFF, 0.0f, 0.0f, float(deinterlacer.imageCX), float(deinterlacer.imageCY), 0.0f, 0.0f, 1.0f, 1.0f);

		LoadPixelShader(oldShader);
		LoadVertexShader(oldVertShader);
		bReadyToDraw = true;
		deinterlacer.isReady = true;
	}
}

void CDeinterlacer::RenderTexture(const Vect2 &pos, const Vect2 &size)
{
	if (RendTexture && bReadyToDraw && deinterlacer.isReady)
	{
		Shader *oldShader = GetCurrentPixelShader();
		if (colorConvertShader)
		{
			LoadPixelShader(colorConvertShader);
			colorConvertShader->SetFloat(colorConvertShader->GetParameterByName(TEXT("gamma")), 1);
		}

		Texture *tex = (deinterlacer.processor == DEINTERLACING_PROCESSOR_GPU && deinterlacer.texture.get()) ? deinterlacer.texture.get() : RendTexture;
		if (deinterlacer.doublesFramerate)
		{
			if (!deinterlacer.curField)
				DrawSpriteEx(tex, 0xFFFFFFFF, pos.x, pos.y, size.x, size.y, 0.f, 0.0f, 0.5f, 1.f);
			else
				DrawSpriteEx(tex, 0xFFFFFFFF, pos.x, pos.y, size.x, size.y, .5f, 0.0f, 1.f, 1.f);
		}
		else
			DrawSprite(tex, 0xFFFFFFFF, pos.x, pos.y, size.x, size.y);


		if (colorConvertShader)
			LoadPixelShader(oldShader);
	}
}

String CDeinterlacer::ChooseDeinterlacingShader()
{
	String shader;
	shader << SHADER_PATH << TEXT("Deinterlace_");

#ifdef _DEBUG
#define DEBUG__ _DEBUG
#undef _DEBUG
#endif
#define SELECT(x) case DEINTERLACING_##x: shader << String(TEXT(#x)).MakeLower(); break;
	switch (deinterlacer.type)
	{
		SELECT(RETRO)
			SELECT(BLEND)
			SELECT(BLEND2x)
			SELECT(LINEAR)
			SELECT(LINEAR2x)
			SELECT(YADIF)
			SELECT(YADIF2x)
			SELECT(_DEBUG)
	}
	return shader << TEXT(".pShader");
#undef SELECT
#ifdef DEBUG__
#define _DEBUG DEBUG__
#undef DEBUG__
#endif
}

String CDeinterlacer::ChooseShader()
{
	if (ColorType == DeviceOutputType_RGB)
		return String();

	String strShader;
	strShader << SHADER_PATH;

	if (ColorType == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
	else if (ColorType == DeviceOutputType_YV12)
		strShader << TEXT("YVUToRGB.pShader");
	else if (ColorType == DeviceOutputType_YVYU)
		strShader << TEXT("YVXUToRGB.pShader");
	else if (ColorType == DeviceOutputType_YUY2)
		strShader << TEXT("YUXVToRGB.pShader");
	else if (ColorType == DeviceOutputType_UYVY)
		strShader << TEXT("UYVToRGB.pShader");
	else if (ColorType == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
	{
		strShader.Clear();
	}

	return strShader;
}

Texture * CDeinterlacer::GetRenderTexture() const
{
	return deinterlacer.texture.get();
}



