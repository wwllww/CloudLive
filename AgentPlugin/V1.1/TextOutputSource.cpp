#include "TextOutputSource.h"
#include <sstream>

#define ClampVal(val, minVal, maxVal) \
    if(val < minVal) val = minVal; \
    else if(val > maxVal) val = maxVal;

const int Width = 10;

WNDPROC BrowerProc = NULL;
WNDPROC OK1Proc = NULL;
WNDPROC Cancel1Proc = NULL;
extern HINSTANCE hinstMain;
static HWND Hwnd = NULL;

LocaleStringLookup *pluginLocale = NULL;

inline DWORD GetAlphaVal(UINT opacityLevel)
{
    return ((opacityLevel*255/100)&0xFF) << 24;
}



IMPLEMENT_DYNIC(TextOutputSource, "文字源", "V1.0.0.1")

TextOutputSource::TextOutputSource()
{
	solidVertexShader = NULL;
	solidPixelShader = NULL;
	bHasPreprocess = false;

	texture = NULL;
	ss = NULL;
	bMonitoringFileChanges = NULL;
	bChanged = false;
	RenderSize = Vect2(0, 0);
	UseExtentSize = Vect2(0, 0);
	OldbaseSize = Vect2(0, 0);
	OldNSize = Vect2(0, 0);
	bHasUpDataTexture = false;
	OldBSize = Vect2(0, 0);
	OldMouseRenderPos = Vect2(0, 0);
	scrollValue = 0.0f;
	OldLen = 0;
	bOldUseExtents = false;
	D3DRender = GetD3DRender();
}

TextOutputSource::~TextOutputSource()
{
	if (texture)
	{
		delete texture;
		texture = NULL;
	}

	if (ss)
		delete ss;

	if (bMonitoringFileChanges)
	{
		OSMonitorFileDestroy(fileChangeMonitor);
	}
}

bool TextOutputSource::Init(Value &JsonParam)
{

	solidPixelShader = D3DRender->CreatePixelShaderFromFile(TEXT("shaders/DrawSolid.pShader"));
	solidVertexShader = D3DRender->CreateVertexShaderFromFile(TEXT("shaders/DrawSolid.vShader"));

	if (!solidVertexShader || !solidPixelShader)
		return false;

	UpdateSettings(JsonParam);

	SamplerInfo si;
	zero(&si, sizeof(si));
	si.filter = GS_FILTER_LINEAR;
	if (bUseSubTitle)
	{
		si.addressU = GS_ADDRESS_BORDER;
		si.addressV = GS_ADDRESS_BORDER;
		ss = D3DRender->CreateSamplerState(si);
	}
	else
	{
		si.addressU = GS_ADDRESS_REPEAT;
		si.addressV = GS_ADDRESS_REPEAT;
		ss = D3DRender->CreateSamplerState(si);
	}
	globalOpacity = 100;
	bStop = false;
	bCanStop = false;
	iRealRepeatCount = 0;
	RenderSize = baseSize;

	if (!JsonParam["UseExtentSize.x"].isNull())
	{
		UseExtentSize.x = JsonParam["UseExtentSize.x"].asDouble();
		UseExtentSize.y = JsonParam["UseExtentSize.y"].asDouble();
	}

	if (!JsonParam["OldbaseSize.x"].isNull())
	{
		OldbaseSize.x = JsonParam["OldbaseSize.x"].asDouble();
		OldbaseSize.y = JsonParam["OldbaseSize.y"].asDouble();
	}

	return true;
}

void TextOutputSource::Preprocess()
{
	if (bMonitoringFileChanges)
	{
		if (OSFileHasChanged(fileChangeMonitor))
			bUpdateTexture = true;
	}

	if (bUpdateTexture)
	{
		bUpdateTexture = false;
		UpdateTexture();
	}
}

void TextOutputSource::Tick(float fSeconds)
{
	if (scrollSpeed != 0 && texture)
	{
		scrollValue += fSeconds*float(scrollSpeed) / (bVertical ? (-1.0f)*float(textureSize.cy) : bUpDown ? float(textureSize.cy) : float(textureSize.cx));
		if (!bUseSubTitle)
		{
			while(scrollValue > 1.0f)
				scrollValue -= 1.0f;
			while (scrollValue < -1.0f)
				scrollValue += 1.0f;
		}
	}

	if (showExtentTime > 0.0f)
		showExtentTime -= fSeconds;

	if (bDoUpdate)
	{
		bDoUpdate = false;
		bUpdateTexture = true;
	}
}

void TextOutputSource::Render(const Vect2 &pos, const Vect2 &size, Texture* tex, bool bScaleFull, bool bIsLiveC)
{
	if (texture && (!bStop || !bUseSubTitle) && baseSize != Vect2(0,0))
	{
		
		Vect2 sizeMultiplier = size / baseSize;

		bool bCanWrite = false;
		if (OldLen != strCurrentText.Length() || bOldUseExtents != bUseExtents)//拖动的时候不改变RenderSize
		{
			OldLen = strCurrentText.Length();
			bCanWrite = true;
			bOldUseExtents = bUseExtents;
		}


		bool HasVaule = false;
		if (bUseExtents && UseExtentSize != Vect2(0, 0))
		{
			sizeMultiplier = UseExtentSize / OldbaseSize;
			HasVaule = true;
		}

		Vect2 newSize = Vect2(float(textureSize.cx), float(textureSize.cy)) * sizeMultiplier;

		static bool EnterSize = false;

		static bool bCanValue = true;
		static bool bChangedd = false;

		if ((bHasUpDataTexture || OldMouseRenderPos != size) && !bIsLiveC)
		{
			if (bHasUpDataTexture || bChangedd)
			{
				ChangeLiveInstanceSameAsLocalInstance(this);

				bChangedd = true;

				if (OldMouseRenderPos != size)
				{
					bChangedd = false;
				}
			}

			bCanValue = true;
			bHasUpDataTexture = false;
			OldMouseRenderPos = size;
		}

		if (!bUseExtents && sizeMultiplier != Vect2(1.0f, 1.0f) && !bIsLiveC) //换多字有问题
		{
			if (bCanValue)
			{
				OldNSize = newSize;
				OldBSize = baseSize;
			}

			EnterSize = true;
		}
		else if (!bIsLiveC)
		{
			EnterSize = false;
		}

		if (!bUseExtents && OldNSize != Vect2(0, 0) && OldBSize != Vect2(0, 0) && !bIsLiveC)
		{
			sizeMultiplier = OldNSize / OldBSize;
			newSize = Vect2(float(textureSize.cx), float(textureSize.cy)) * sizeMultiplier;

			if (!EnterSize)
				bCanValue = false;
		}
		else if (sizeMultiplier == Vect2(1.0f, 1.0f) && OldNSize != Vect2(0, 0) && OldBSize != Vect2(0, 0) && !bIsLiveC)//点自动大小两次
		{
			sizeMultiplier = OldNSize / OldBSize;
			newSize = Vect2(float(textureSize.cx), float(textureSize.cy)) * sizeMultiplier;

			if (!EnterSize)
				bCanValue = false;
		}


		if (OldSize != newSize && !bIsLiveC) //LiveCall不改变大小
		{
			OldSize = newSize;

			if (bUseExtents && bCanWrite)
			{
				RenderSize = newSize;
			}

			bUpdateSubTitleTexture = true;
			if (!HasVaule && bCanWrite)
			{
				if (OldNSize != Vect2(0, 0) && bCanValue)
				{
					RenderSize = OldNSize;
				}
				else
				{
					RenderSize = newSize;
				}
				
			}
			
		}
		else
		{
			//这里搬运停止的时候会进来
			if (!bCanWrite && !bIsLiveC)
			{
				if (OldNSize != Vect2(0, 0) && bCanValue)
				{
					RenderSize = OldNSize;
				}
				else
				{
					RenderSize = newSize;
				}
				
			}
		}


		if (bUseSubTitle && bUpdateSubTitleTexture)
		{
			SIZE New;
			New.cx = newSize.x;
			New.cy = newSize.y;
			//DrawText(pos, New);
		}

		if (bUseExtents)
		{
			Vect2 extentVal = Vect2(float(extentWidth), float(extentHeight))/** sizeMultiplier*/;

			if (OldextentVal != extentVal)
			{
				OldextentVal = extentVal;
				if (UseExtentSize == Vect2(0, 0))
				{
					UseExtentSize = size;
					(*Realdata)["UseExtentSize.x"] = UseExtentSize.x;
					(*Realdata)["UseExtentSize.y"] = UseExtentSize.y;
					
				}
				if (OldbaseSize == Vect2(0, 0))
				{
					OldbaseSize = baseSize;
					(*Realdata)["OldbaseSize.x"] = OldbaseSize.x;
					(*Realdata)["OldbaseSize.y"] = OldbaseSize.y;
				}
			}

			RenderSize = extentVal;
				
			if (showExtentTime > 0.0f)
			{
				Shader *pShader = D3DRender->GetCurrentPixelShader();
				Shader *vShader = D3DRender->GetCurrentVertexShader();

				Color4 rectangleColor = Color4(0.0f, 1.0f, 0.0f, 1.0f);
				if (showExtentTime < 1.0f)
					rectangleColor.w = showExtentTime;

				solidPixelShader->SetColor(solidPixelShader->GetParameter(0), rectangleColor);

				D3DRender->LoadVertexShader(solidVertexShader);
				D3DRender->LoadPixelShader(solidPixelShader);
				D3DRender->DrawBox(pos, extentVal);

				D3DRender->LoadVertexShader(vShader);
				D3DRender->LoadPixelShader(pShader);
			}

			if (!bWrap)
			{
				XRect rect = { int(pos.x), int(pos.y), int(extentVal.x), int(extentVal.y) };
				D3DRender->SetScissorRect(&rect);
			}
		}
		else
		{
			UseExtentSize = Vect2(0, 0);
			OldextentVal = Vect2(0, 0);
			OldbaseSize = Vect2(0, 0);
		}

		if (bUsePointFiltering) {
			if (!sampler) {
				SamplerInfo samplerinfo;
				samplerinfo.filter = GS_FILTER_POINT;
				std::unique_ptr<SamplerState> new_sampler(D3DRender->CreateSamplerState(samplerinfo));
				sampler = std::move(new_sampler);
			}

			D3DRender->LoadSamplerState(sampler.get(), 0);
		}

		DWORD alpha = DWORD(double(globalOpacity)*2.55);
		DWORD outputColor = (alpha << 24) | 0xFFFFFF;

		if (scrollSpeed != 0)
		{
			UVCoord ul;
			UVCoord lr;
			if (bUseSubTitle && iRepeatCount != 0)//启用字幕
			{
				if (bUpDown)//上下滚动
				{
					if (scrollSpeed > 0)
					{
						if (bUseExtents)
						{
							float MultiplierY = (float)extentHeight / (textureSize.cy * sizeMultiplier.y);
							ul.Set(0.0f, -1.0f * MultiplierY);
							lr.Set(1.0f, 1.0f - 1.0f * MultiplierY);
						}
						else
						{
							ul.Set(0.0f, -1.0f);
							lr.Set(1.0f, 0.0f);
						}

					}
					else
					{
						ul.Set(0.0f, 1.0f);
						lr.Set(1.0f, 2.0f);
					}

					ul.y += scrollValue;
					lr.y += scrollValue;


				}
				else //左右滚动
				{
					if (scrollSpeed > 0)
					{
						if (bUseExtents)
						{
							float MultiplierX = (float)extentWidth / (textureSize.cx * sizeMultiplier.x);
							ul.Set(-1.0f * MultiplierX, 0.0f);
							lr.Set(1 - 1.0f * MultiplierX, 1.0f);
						}
						else
						{
							ul.Set(-1.0f, 0.0f);
							lr.Set(0, 1.0f);
						}

					}
					else
					{
						ul.Set(1.0f, 0.0f);
						lr.Set(2.0f, 1.0f);
					}

					ul.x += scrollValue;
					lr.x += scrollValue;
				}

				if ((bCanStop || iRepeatCount == 1) &&
					(scrollValue >= 2.0f || scrollValue <= -2.0f))
				{
					bStop = true;
				}
				else if (iRepeatCount > 1 &&
					(scrollValue >= 1.0f || scrollValue <= -1.0f) && !bCanStop)
				{
					if (iRealRepeatCount++ == iRepeatCount - 1)
					{
						if (ss)
						{
							delete ss;
							ss = NULL;
						}
						SamplerInfo si;
						zero(&si, sizeof(si));
						si.filter = GS_FILTER_LINEAR;
						si.addressU = GS_ADDRESS_BORDER;
						si.addressV = GS_ADDRESS_BORDER;
						ss = D3DRender->CreateSamplerState(si);
						bCanStop = true;
					}
					else
					{
						if (bHasAccess)
						{
							if (ss)
							{
								delete ss;
								ss = NULL;
							}
							SamplerInfo si;
							zero(&si, sizeof(si));
							si.filter = GS_FILTER_LINEAR;
							si.addressU = GS_ADDRESS_REPEAT;
							si.addressV = GS_ADDRESS_REPEAT;
							ss = D3DRender->CreateSamplerState(si);
							bHasAccess = false;
						}
						while (scrollValue >= 1.0f)
						{
							scrollValue -= 1.0f;
						}
						while (scrollValue <= -1.0f)
							scrollValue += 1.0;
					}

				}
				else if (-1 == iRepeatCount &&
					(scrollValue >= 1.0f || scrollValue <= -1.0f))
				{
					if (bIsRepeat)
					{
						if (ss)
						{
							delete ss;
							ss = NULL;
						}
						SamplerInfo si;
						zero(&si, sizeof(si));
						si.filter = GS_FILTER_LINEAR;
						si.addressU = GS_ADDRESS_REPEAT;
						si.addressV = GS_ADDRESS_REPEAT;
						ss = D3DRender->CreateSamplerState(si);
						bIsRepeat = false;
					}
				}

				// 					XRect rect = { int(pos.x), int(pos.y), int(ScissorRect.x), int(ScissorRect.y) };
				// 					SetScissorRect(&rect);

				D3DRender->LoadSamplerState(ss, 0);
				D3DRender->DrawSpriteEx(texture, outputColor, pos.x, pos.y, pos.x + newSize.x, pos.y + newSize.y, ul.x, ul.y, lr.x, lr.y);
			}
			else if (bUseSubTitle && iRepeatCount == 0)
			{
				// 					XRect rect = { int(pos.x), int(pos.y), int(ScissorRect.x), int(ScissorRect.y) };
				// 					SetScissorRect(&rect);
				D3DRender->DrawSprite(texture, outputColor, pos.x, pos.y, pos.x + newSize.x, pos.y + newSize.y);
			}
			else if (!bUseSubTitle)
			{
				ul.Set(0.0f, 0.0f);
				lr.Set(1.0f, 1.0f);

				if (bVertical)
				{
					/*float sizeVal = float(textureSize.cy);
					float clampedVal = floorf(scrollValue*sizeVal)/sizeVal;*/
					ul.y += scrollValue;
					lr.y += scrollValue;
				}
				else
				{
					/*float sizeVal = float(textureSize.cx);
					float clampedVal = floorf(scrollValue*sizeVal)/sizeVal;*/
					ul.x += scrollValue;
					lr.x += scrollValue;

				}

				D3DRender->LoadSamplerState(ss, 0);
				D3DRender->DrawSpriteEx(texture, outputColor, pos.x, pos.y, pos.x + newSize.x, pos.y + newSize.y, ul.x, ul.y, lr.x, lr.y);
			}
		}
		else
			D3DRender->DrawSprite(texture, outputColor, pos.x, pos.y, pos.x + newSize.x, pos.y + newSize.y);

		if (bUsePointFiltering)
			D3DRender->LoadSamplerState(NULL, 0);

		if ((bUseExtents && !bWrap))
			D3DRender->SetScissorRect(NULL);
		//EnableBlending(TRUE);
	}
}

Vect2 TextOutputSource::GetSize() const
{
	return RenderSize == Vect2(0, 0) ? baseSize : RenderSize;// baseSize;
}

void TextOutputSource::UpdateSettings(Value &data)
{
	Realdata = &data;
	this->data = data;
	strFont = TEXT("Arial");

	if (!data["font"].isNull())
		strFont = Asic2WChar(data["font"].asString().c_str()).c_str();

	color = 0xFFFFFFFF;
	if (!data["color"].isNull())
		color = data["color"].asUInt();

	size = 48;
	if (!data["fontSize"].isNull())
	{
		size = data["fontSize"].asInt();
	}


	opacity = 100;
	if (!data["textOpacity"].isNull())
	{
		opacity -= data["textOpacity"].asUInt();
	}

	bBold = data["bold"].asInt() != 0;
	bItalic = data["italic"].asInt() != 0;
	bWrap = data["wrap"].asInt() != 0;
	bScrollMode = data["scrollMode"].asInt() != 0;
	bUnderline = data["underline"].asInt() != 0;
	bVertical = data["vertical"].asInt() != 0;
	bUseExtents = data["useTextExtents"].asInt() != 0;
	extentWidth = data["extentWidth"].asInt();
	extentHeight = data["extentHeight"].asInt();
	align = data["align"].asInt();

	strText = L"";
	
	if (!data["text"].isNull())
		strText = Asic2WChar(data["text"].asString().c_str()).c_str();
	mode = data["mode"].asInt();

	strFile = L"";

	if (!data["file"].isNull())
		strFile = Asic2WChar(data["file"].asString().c_str()).c_str();

	if (mode == 1 && !OSFileExists(strFile)) // 文件不存在
	{
		return;
	}

	
	bUsePointFiltering = data["pointFiltering"].asInt() != 0;

	baseSize.x = 100;

	if (!data["baseSizeCX"].isNull())
	{
		baseSize.x = data["baseSizeCX"].asInt();
	}

	baseSize.y = 100;

	if (!data["baseSizeCY"].isNull())
	{
		baseSize.y = data["baseSizeCY"].asInt();
	}

	bUseOutline = data["useOutline"].asInt() != 0;
	outlineColor = 0xFF000000;

	if (!data["outlineColor"].isNull())
	{
		outlineColor = data["outlineColor"].asUInt();
	}

	outlineSize = 2;

	if (!data["outlineSize"].isNull())
	{
		outlineSize = data["outlineSize"].asInt();
	}

	outlineOpacity = 100;

	if (!data["outlineOpacity"].isNull())
	{
		outlineOpacity -= data["outlineOpacity"].asUInt();
	}

	backgroundColor = 0xFF000000;

	if (!data["backgroundColor"].isNull())
	{
		backgroundColor = data["backgroundColor"].asUInt();
	}

	backgroundOpacity = 100;

	if (!data["backgroundOpacity"].isNull())
	{
		backgroundOpacity -= data["backgroundOpacity"].asInt();
	}

	bUseSubTitle = data["useSubTitle"].asInt() != 0;
	bRepeat = data["subTitleRepeat"].asInt() != 0;
	bUpDown = true;
	if (!data["subTitleUpDown"].isNull())
		bUpDown = data["subTitleUpDown"].asInt() != 0;

	iRepeatCount = -1;

	if (!data["SubTitleRepeatCount"].isNull())
	{
		iRepeatCount = data["SubTitleRepeatCount"].asInt();
	}

	scrollSpeed = data["scrollSpeed"].asInt();
	iOldRepeatCount = iRepeatCount;
	if (bUseSubTitle)
	{
		iRealRepeatCount = 0;
		bCanStop = false;
		scrollValue = 0;
		bStop = false;
	}
	else
	{
		scrollSpeed = 0;
	}

	if (iRepeatCount == -1)
		bIsRepeat = true;
	else
	{
		bIsRepeat = false;
		bHasAccess = true;
	}
	
	bUpdateTexture = true;
	bChanged = false;
}

void TextOutputSource::SetString(CTSTR lpName, CTSTR lpVal)
{
	if (scmpi(lpName, TEXT("font")) == 0)
		strFont = lpVal;
	else if (scmpi(lpName, TEXT("text")) == 0)
		strText = lpVal;
	else if (scmpi(lpName, TEXT("file")) == 0)
		strFile = lpVal;

	bUpdateTexture = true;
	bChanged = true;
}

void TextOutputSource::SetInt(CTSTR lpName, int iValue)
{
	if (scmpi(lpName, TEXT("color")) == 0)
		color = iValue;
	else if (scmpi(lpName, TEXT("fontSize")) == 0)
		size = iValue;
	else if (scmpi(lpName, TEXT("textOpacity")) == 0)
		opacity = iValue;
	else if (scmpi(lpName, TEXT("scrollSpeed")) == 0)
	{
		if (scrollSpeed == 0)
			scrollValue = 0.0f;
		scrollSpeed = iValue;
		iRealRepeatCount = 0;
		bCanStop = false;
		scrollValue = 0;
		bStop = false;
	}
	else if (scmpi(lpName, TEXT("bold")) == 0)
		bBold = iValue != 0;
	else if (scmpi(lpName, TEXT("italic")) == 0)
		bItalic = iValue != 0;
	else if (scmpi(lpName, TEXT("wrap")) == 0)
		bWrap = iValue != 0;
	else if (scmpi(lpName, TEXT("scrollMode")) == 0)
		bScrollMode = iValue != 0;
	else if (scmpi(lpName, TEXT("underline")) == 0)
		bUnderline = iValue != 0;
	else if (scmpi(lpName, TEXT("vertical")) == 0)
		bVertical = iValue != 0;
	else if (scmpi(lpName, TEXT("useTextExtents")) == 0)
		bUseExtents = iValue != 0;
	else if (scmpi(lpName, TEXT("extentWidth")) == 0)
	{
		showExtentTime = 2.0f;
		extentWidth = iValue;
	}
	else if (scmpi(lpName, TEXT("extentHeight")) == 0)
	{
		showExtentTime = 2.0f;
		extentHeight = iValue;
	}
	else if (scmpi(lpName, TEXT("align")) == 0)
		align = iValue;
	else if (scmpi(lpName, TEXT("mode")) == 0)
		mode = iValue;
	else if (scmpi(lpName, TEXT("useOutline")) == 0)
		bUseOutline = iValue != 0;
	else if (scmpi(lpName, TEXT("outlineColor")) == 0)
		outlineColor = iValue;
	else if (scmpi(lpName, TEXT("outlineOpacity")) == 0)
		outlineOpacity = iValue;
	else if (scmpi(lpName, TEXT("backgroundColor")) == 0)
		backgroundColor = iValue;
	else if (scmpi(lpName, TEXT("backgroundOpacity")) == 0)
		backgroundOpacity = iValue;
	else if (scmpi(lpName, TEXT("useSubTitle")) == 0)
	{
		bUseSubTitle = iValue != 0;
		if (!bUseSubTitle)
		{
			if (ss)
			{
				delete ss;
				ss = NULL;
			}

			SamplerInfo si;
			zero(&si, sizeof(si));
			si.addressU = GS_ADDRESS_REPEAT;
			si.addressV = GS_ADDRESS_REPEAT;
			si.borderColor = 0;
			si.filter = GS_FILTER_LINEAR;
			ss = D3DRender->CreateSamplerState(si);
		}
		else
		{
			if (ss)
			{
				delete ss;
				ss = NULL;
			}

			SamplerInfo si;
			zero(&si, sizeof(si));
			si.addressU = GS_ADDRESS_BORDER;
			si.addressV = GS_ADDRESS_BORDER;
			si.borderColor = 0;
			si.filter = GS_FILTER_LINEAR;
			ss = D3DRender->CreateSamplerState(si);

			iRealRepeatCount = 0;
			bCanStop = false;
			scrollValue = 0;
			bStop = false;
		}
	}
	else if (scmpi(lpName, TEXT("subTitleRepeat")) == 0)
	{
		bRepeat = iValue != 0;

		if (ss)
		{
			delete ss;
			ss = NULL;
		}
		SamplerInfo si;
		zero(&si, sizeof(si));
		si.filter = GS_FILTER_LINEAR;
		if (bRepeat)
		{
			si.addressU = GS_ADDRESS_REPEAT;
			si.addressV = GS_ADDRESS_REPEAT;
			ss = D3DRender->CreateSamplerState(si);
		}
		else
		{
			si.addressU = GS_ADDRESS_BORDER;
			si.addressV = GS_ADDRESS_BORDER;
			ss = D3DRender->CreateSamplerState(si);
		}
	}
	else if (scmpi(lpName, TEXT("subTitleUpDown")) == 0)
	{
		bUpDown = iValue != 0;
		iRealRepeatCount = 0;
		bCanStop = false;
		scrollValue = 0;
		bStop = false;
	}
	else if (scmpi(lpName, TEXT("SubTitleRepeatCount")) == 0)
	{
		iRepeatCount = iValue;

		if (iRepeatCount == -1)
			bIsRepeat = true;
		else
		{
			bIsRepeat = false;
			bHasAccess = true;
		}

		if (iRepeatCount != iOldRepeatCount)
		{
			iOldRepeatCount = iRepeatCount;
			iRealRepeatCount = 0;
			bCanStop = false;
			scrollValue = 0;
			bStop = false;
		}

		if (ss)
		{
			delete ss;
			ss = NULL;
		}

		SamplerInfo si;
		zero(&si, sizeof(si));
		si.filter = GS_FILTER_LINEAR;

		si.addressU = GS_ADDRESS_BORDER;
		si.addressV = GS_ADDRESS_BORDER;
		ss = D3DRender->CreateSamplerState(si);

	}
	bUpdateTexture = true;
	bChanged = true;
}

void TextOutputSource::SetFloat(CTSTR lpName, float fValue)
{
	if (scmpi(lpName, TEXT("outlineSize")) == 0)
		outlineSize = fValue;

	bUpdateTexture = true;
	bChanged = true;
}

void TextOutputSource::DrawText(const Vect2 &pos, const SIZE&size)
{
	HFONT hFont;
	Gdiplus::Status stat;
	Gdiplus::RectF layoutBox;

	Gdiplus::RectF boundingBox(0.0f, 0.0f, size.cx, size.cy);

	hFont = GetFont();
	if (!hFont)
		return;

	Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());

	SetStringFormat(format);

	//----------------------------------------------------------------------
	// write image

	{
		HDC hTempDC = CreateCompatibleDC(NULL);

		Gdiplus::Font font(hTempDC, hFont);

		BITMAPINFO bi;
		zero(&bi, sizeof(bi));

		void* lpBits;

		BITMAPINFOHEADER &bih = bi.bmiHeader;
		bih.biSize = sizeof(bih);
		bih.biBitCount = 32;
		bih.biWidth = size.cx;
		bih.biHeight = size.cy;
		bih.biPlanes = 1;

		HBITMAP hBitmap = CreateDIBSection(hTempDC, &bi, DIB_RGB_COLORS, &lpBits, NULL, 0);

		Gdiplus::Bitmap      bmp(size.cx, size.cy, 4 * size.cx, PixelFormat32bppARGB, (BYTE*)lpBits);

		Gdiplus::Graphics * graphics = new Gdiplus::Graphics(&bmp);
		graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

		Gdiplus::SolidBrush  *brush = new Gdiplus::SolidBrush(Gdiplus::Color(GetAlphaVal(opacity) | (color & 0x00FFFFFF)));

		DWORD bkColor;

		if (backgroundOpacity == 0 && scrollSpeed != 0)
			bkColor = 1 << 24 | (color & 0x00FFFFFF);
		else
			bkColor = ((strCurrentText.IsValid() || bUseExtents) ? GetAlphaVal(backgroundOpacity) : GetAlphaVal(0)) | (backgroundColor & 0x00FFFFFF);

		if ((size.cx > boundingBox.Width || size.cy > boundingBox.Height) && !bUseExtents)
		{
			stat = graphics->Clear(Gdiplus::Color(0x00000000));
			if (stat != Gdiplus::Ok)
				AppWarning(TEXT("TextSource::UpdateTexture: Graphics::Clear failed: %u"), (int)stat);

			Gdiplus::SolidBrush *bkBrush = new Gdiplus::SolidBrush(Gdiplus::Color(bkColor));

			graphics->FillRectangle(bkBrush, boundingBox);

			delete bkBrush;
		}
		else
		{
			stat = graphics->Clear(Gdiplus::Color(bkColor));
			if (stat != Gdiplus::Ok)
				AppWarning(TEXT("TextSource::UpdateTexture: Graphics::Clear failed: %u"), (int)stat);
		}

		graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		graphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
		graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (strCurrentText.IsValid())
		{
			if (bUseOutline)
			{
				boundingBox.Offset(outlineSize / 2, outlineSize / 2);

				Gdiplus::FontFamily fontFamily;
				Gdiplus::GraphicsPath path;

				font.GetFamily(&fontFamily);

				path.AddString(strCurrentText, -1, &fontFamily, font.GetStyle(), font.GetSize(), Gdiplus::PointF(0, 0), &format);

				DrawOutlineText(graphics, font, path, format, brush);
			}
			else
			{
				stat = graphics->DrawString(strCurrentText, -1, &font, Gdiplus::PointF(0, 0), &format, brush);
				if (stat != Gdiplus::Ok)
					AppWarning(TEXT("TextSource::UpdateTexture: Graphics::DrawString failed: %u"), (int)stat);
			}
		}

		delete brush;
		delete graphics;

		//----------------------------------------------------------------------
		// upload texture

		if (OldtextureSize.cx != size.cx || OldtextureSize.cy != size.cy)
		{
			if (texture)
			{
				delete texture;
				texture = NULL;
			}
			OldtextureSize = size;
			texture = D3DRender->CreateTexture(size.cx, size.cy, GS_BGRA, lpBits, FALSE, FALSE);
			bHasUpDataTexture = true;
		}
		else if (texture)
			D3DRender->SetImage(texture, lpBits, GS_IMAGEFORMAT_BGRA, 4 * size.cx);

		if (!texture)
		{
			AppWarning(TEXT("TextSource::UpdateTexture: could not create texture"));

		}

		DeleteDC(hTempDC);
		DeleteObject(hBitmap);
		DeleteObject(hFont);

		bUpdateSubTitleTexture = false;
	}
}

void TextOutputSource::UpdateTexture()
{
	HFONT hFont;
	Gdiplus::Status stat;
	Gdiplus::RectF layoutBox;
	SIZE textSize;
	float offset;

	Gdiplus::RectF boundingBox(0.0f, 0.0f, 32.0f, 32.0f);

	UpdateCurrentText();

	hFont = GetFont();
	if (!hFont)
		return;

	Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());

	SetStringFormat(format);

	HDC hdc = CreateCompatibleDC(NULL);

	Gdiplus::Font font(hdc, hFont);
	Gdiplus::Graphics *graphics = new Gdiplus::Graphics(hdc);

	graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

	if (strCurrentText.IsValid())
	{
		if (bUseExtents && bWrap)
		{
			layoutBox.X = layoutBox.Y = 0;
			layoutBox.Width = float(extentWidth);
			layoutBox.Height = float(extentHeight);

			if (bUseOutline)
			{
				//Note: since there's no path widening in DrawOutlineText the padding is half than what it was supposed to be.
				layoutBox.Width -= outlineSize;
				layoutBox.Height -= outlineSize;
			}

			if (!bVertical && bScrollMode)
			{
				offset = ProcessScrollMode(graphics, &font, layoutBox, &format);

				boundingBox = layoutBox;
				boundingBox.Y = offset;
				if (offset < 0)
					boundingBox.Height -= offset;
			}
			else
			{
				stat = graphics->MeasureString(strCurrentText, -1, &font, layoutBox, &format, &boundingBox);
				if (stat != Gdiplus::Ok)
				{
					AppWarning(TEXT("TextSource::UpdateTexture: Gdiplus::Graphics::MeasureString failed: %u"), (int)stat);
					delete graphics;

					DeleteDC(hdc);
					hdc = NULL;
					DeleteObject(hFont);
					return;
				}
			}
		}
		else
		{
			stat = graphics->MeasureString(strCurrentText, -1, &font, Gdiplus::PointF(0.0f, 0.0f), &format, &boundingBox);
			if (stat != Gdiplus::Ok)
			{
				AppWarning(TEXT("TextSource::UpdateTexture: Gdiplus::Graphics::MeasureString failed: %u else"), (int)stat);
				delete graphics;

				DeleteDC(hdc);
				hdc = NULL;
				DeleteObject(hFont);

				return;
			}
			if (bUseOutline)
			{
				//Note: since there's no path widening in DrawOutlineText the padding is half than what it was supposed to be.
				boundingBox.Width += outlineSize;
				boundingBox.Height += outlineSize;
			}
		}
	}

	delete graphics;

	DeleteDC(hdc);
	hdc = NULL;
	DeleteObject(hFont);

	if (bVertical)
	{
		if (boundingBox.Width < size)
		{
			textSize.cx = size;
			boundingBox.Width = float(size);
		}
		else
			textSize.cx = LONG(boundingBox.Width + EPSILON);

		textSize.cy = LONG(boundingBox.Height + EPSILON);
	}
	else
	{
		if (boundingBox.Height < size)
		{
			textSize.cy = size;
			boundingBox.Height = float(size);
		}
		else
			textSize.cy = LONG(boundingBox.Height + EPSILON);

		textSize.cx = LONG(boundingBox.Width + EPSILON);
	}

	if (bUseExtents)
	{
		if (bWrap)
		{
			textSize.cx = extentWidth;
			textSize.cy = extentHeight;
		}
		else
		{
			if (LONG(extentWidth) > textSize.cx)
				textSize.cx = extentWidth;
			if (LONG(extentHeight) > textSize.cy)
				textSize.cy = extentHeight;
		}
	}

	//textSize.cx &= 0xFFFFFFFE;
	//textSize.cy &= 0xFFFFFFFE;

	textSize.cx += textSize.cx % 2;
	textSize.cy += textSize.cy % 2;

	ClampVal(textSize.cx, 32, 16370);
	ClampVal(textSize.cy, 32, 16370);

	//textSize.cx += Width;
	//boundingBox.Width += Width;
	//baseSize.x += Width;//textSize.cx;
	//----------------------------------------------------------------------
	// write image

	{
		HDC hTempDC = CreateCompatibleDC(NULL);

		BITMAPINFO bi;
		zero(&bi, sizeof(bi));

		void* lpBits;

		BITMAPINFOHEADER &bih = bi.bmiHeader;
		bih.biSize = sizeof(bih);
		bih.biBitCount = 32;
		bih.biWidth = textSize.cx;
		bih.biHeight = textSize.cy;
		bih.biPlanes = 1;

		HBITMAP hBitmap = CreateDIBSection(hTempDC, &bi, DIB_RGB_COLORS, &lpBits, NULL, 0);

		Gdiplus::Bitmap      bmp(textSize.cx, textSize.cy, 4 * textSize.cx, PixelFormat32bppARGB, (BYTE*)lpBits);

		graphics = new Gdiplus::Graphics(&bmp);

		Gdiplus::SolidBrush  *brush = new Gdiplus::SolidBrush(Gdiplus::Color(GetAlphaVal(opacity) | (color & 0x00FFFFFF)));

		DWORD bkColor;

		if (backgroundOpacity == 0 && scrollSpeed != 0)
			bkColor = 1 << 24 | (color & 0x00FFFFFF);
		else
			bkColor = ((strCurrentText.IsValid() || bUseExtents) ? GetAlphaVal(backgroundOpacity) : GetAlphaVal(0)) | (backgroundColor & 0x00FFFFFF);

		if ((textSize.cx > boundingBox.Width || textSize.cy > boundingBox.Height) && !bUseExtents)
		{
			stat = graphics->Clear(Gdiplus::Color(0x00000000));
			if (stat != Gdiplus::Ok)
				AppWarning(TEXT("TextSource::UpdateTexture: Graphics::Clear failed: %u"), (int)stat);

			Gdiplus::SolidBrush *bkBrush = new Gdiplus::SolidBrush(Gdiplus::Color(bkColor));

			graphics->FillRectangle(bkBrush, boundingBox);

			delete bkBrush;
		}
		else
		{
			stat = graphics->Clear(Gdiplus::Color(bkColor));
			if (stat != Gdiplus::Ok)
				AppWarning(TEXT("TextSource::UpdateTexture: Graphics::Clear failed: %u"), (int)stat);
		}

		graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		graphics->SetCompositingMode(Gdiplus::CompositingModeSourceOver);
		graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (strCurrentText.IsValid())
		{
			if (bUseOutline)
			{
				boundingBox.Offset(outlineSize / 2, outlineSize / 2);

				Gdiplus::FontFamily fontFamily;
				Gdiplus::GraphicsPath path;

				font.GetFamily(&fontFamily);

				path.AddString(strCurrentText, -1, &fontFamily, font.GetStyle(), font.GetSize(), boundingBox, &format);

				DrawOutlineText(graphics, font, path, format, brush);
			}
			else
			{
				stat = graphics->DrawString(strCurrentText, -1, &font, boundingBox, &format, brush);
				if (stat != Gdiplus::Ok)
					AppWarning(TEXT("TextSource::UpdateTexture: Graphics::DrawString failed: %u"), (int)stat);
			}
		}

		delete brush;
		delete graphics;

		//----------------------------------------------------------------------
		// upload texture

		if (textureSize.cx != textSize.cx || textureSize.cy != textSize.cy || OldtextureSize.cx != textSize.cx || OldtextureSize.cy != textSize.cy)
		{
			if (texture)
			{
				delete texture;
				texture = NULL;
			}

			mcpy(&textureSize, &textSize, sizeof(textureSize));
			mcpy(&OldtextureSize, &textSize, sizeof(textureSize));
			texture = D3DRender->CreateTexture(textSize.cx, textSize.cy, GS_BGRA, lpBits, FALSE, FALSE);
			bHasUpDataTexture = true;
		}
		else if (texture)
			D3DRender->SetImage(texture, lpBits, GS_IMAGEFORMAT_BGRA, 4 * textSize.cx);

		if (!texture)
		{
			AppWarning(TEXT("TextSource::UpdateTexture: could not create texture"));
			DeleteObject(hFont);
		}

		DeleteDC(hTempDC);
		DeleteObject(hBitmap);
		bUpdateSubTitleTexture = true;
	}
}

float TextOutputSource::ProcessScrollMode(Gdiplus::Graphics *graphics, Gdiplus::Font *font, Gdiplus::RectF &layoutBox, Gdiplus::StringFormat *format)
{
	StringList strList;
	Gdiplus::RectF boundingBox;

	float offset = layoutBox.Height;

	Gdiplus::RectF l2(0.0f, 0.0f, layoutBox.Width, 32000.0f); // Really, it needs to be OVER9000

	strCurrentText.FindReplace(L"\n\r", L"\n");
	strCurrentText.GetTokenList(strList, '\n');

	if (strList.Num() != 0)
		strCurrentText.Clear();
	else
		return 0.0f;

	for (int i = strList.Num() - 1; i >= 0; i--)
	{
		strCurrentText.InsertString(0, TEXT("\n"));
		strCurrentText.InsertString(0, strList.GetElement((unsigned int)i).Array());

		if (strCurrentText.IsValid())
		{
			graphics->MeasureString(strCurrentText, -1, font, l2, &boundingBox);
			offset = layoutBox.Height - boundingBox.Height;
		}

		if (offset < 0)
			break;
	}

	return offset;
}

void TextOutputSource::SetStringFormat(Gdiplus::StringFormat &format)
{
	UINT formatFlags;

	formatFlags = Gdiplus::StringFormatFlagsNoFitBlackBox
		| Gdiplus::StringFormatFlagsMeasureTrailingSpaces;


	if (bVertical)
		formatFlags |= Gdiplus::StringFormatFlagsDirectionVertical
		| Gdiplus::StringFormatFlagsDirectionRightToLeft;


	format.SetFormatFlags(formatFlags);
	format.SetTrimming(Gdiplus::StringTrimmingWord);

	if (bUseExtents && bWrap)
		switch (align)
	{
		case 0:
			if (bVertical)
				format.SetLineAlignment(Gdiplus::StringAlignmentFar);
			else
				format.SetAlignment(Gdiplus::StringAlignmentNear);
			break;
		case 1:
			if (bVertical)
				format.SetLineAlignment(Gdiplus::StringAlignmentCenter);
			else
				format.SetAlignment(Gdiplus::StringAlignmentCenter);
			break;
		case 2:
			if (bVertical)
				format.SetLineAlignment(Gdiplus::StringAlignmentNear);
			else
				format.SetAlignment(Gdiplus::StringAlignmentFar);
			break;
	}
	else if (bUseExtents && bVertical && !bWrap)
		format.SetLineAlignment(Gdiplus::StringAlignmentFar);
	else if (bVertical)
		format.SetLineAlignment(Gdiplus::StringAlignmentFar);
}

void TextOutputSource::UpdateCurrentText()
{
	if (bMonitoringFileChanges)
	{
		OSMonitorFileDestroy(fileChangeMonitor);
		fileChangeMonitor = NULL;

		bMonitoringFileChanges = false;
	}

	if (mode == 0)
		strCurrentText = strText;

	else if (mode == 1 && strFile.IsValid())
	{
		XFile textFile;
		if (textFile.Open(strFile, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING))
		{
			textFile.ReadFileToString(strCurrentText);
		}
		else
		{
			strCurrentText = TEXT("");
			AppWarning(TEXT("TextSource::UpdateTexture: could not open specified file (invalid file name or access violation)"));
		}

		if (fileChangeMonitor = OSMonitorFileStart(strFile))
			bMonitoringFileChanges = true;
	}
	else
		strCurrentText = TEXT("");
}

HFONT TextOutputSource::GetFont()
{
	HFONT hFont = NULL;

	LOGFONT lf;
	zero(&lf, sizeof(lf));
	lf.lfHeight = size;
	lf.lfWeight = bBold ? FW_BOLD : FW_DONTCARE;
	lf.lfItalic = bItalic;
	lf.lfUnderline = bUnderline;
	lf.lfQuality = ANTIALIASED_QUALITY;

	if (strFont.IsValid())
	{
		scpy_n(lf.lfFaceName, strFont, 31);

		hFont = CreateFontIndirect(&lf);
	}

	if (!hFont)
	{
		scpy_n(lf.lfFaceName, TEXT("Arial"), 31);
		hFont = CreateFontIndirect(&lf);
	}

	return hFont;
}

void TextOutputSource::DrawOutlineText(Gdiplus::Graphics *graphics, Gdiplus::Font &font, const Gdiplus::GraphicsPath &path, const Gdiplus::StringFormat &format, const Gdiplus::Brush *brush)
{
	Gdiplus::GraphicsPath *outlinePath;

	outlinePath = path.Clone();

	// Outline color and size
	UINT tmpOpacity = (UINT)((((float)opacity * 0.01f) * ((float)outlineOpacity * 0.01f)) * 100.0f);
	Gdiplus::Pen pen(Gdiplus::Color(GetAlphaVal(tmpOpacity) | (outlineColor & 0xFFFFFF)), outlineSize);
	pen.SetLineJoin(Gdiplus::LineJoinRound);

	// Widen the outline
	// It seems that Widen has a huge performance impact on DrawPath call, screw it! We're talking about freaking seconds in some extreme cases...
	//outlinePath->Widen(&pen);

	// Draw the outline
	graphics->DrawPath(&pen, outlinePath);

	// Draw the text        
	graphics->FillPath(brush, &path);

	delete outlinePath;
}

void TextOutputSource::SetHasPreProcess(bool bHasPre)
{
	bHasPreprocess = bHasPre;
}

bool TextOutputSource::GetHasPreProcess() const
{
	return bHasPreprocess;
}

bool TextOutputSource::GetChanged() const
{
	return bChanged;
}

void TextOutputSource::SetChanged(bool bChange)
{
	bChanged = bChange;
}


int CALLBACK FontEnumProcThingy(ENUMLOGFONTEX *logicalData, NEWTEXTMETRICEX *physicalData, DWORD fontType, ConfigTextSourceInfo *configInfo)
{
    if(fontType == TRUETYPE_FONTTYPE) //HomeWorld - GDI+ doesn't like anything other than truetype
    {
        configInfo->fontNames << logicalData->elfFullName;
        configInfo->fontFaces << logicalData->elfLogFont.lfFaceName;
    }

    return 1;
}

void DoCancelStuff(HWND hwnd)
{
    ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);

	std::stringstream SourceId;
	uint64_t VideoId = 0;
	if (!configInfo->data["SourceID"].isNull())
	{
		SourceId << configInfo->data["SourceID"].asString().c_str();
		SourceId >> VideoId;
	}

	IBaseVideo *source = NULL;

	if (VideoId)
		source = (IBaseVideo*)VideoId;

	if (source)
	{
		TextOutputSource *TextSource = dynamic_cast<TextOutputSource*>(source);

		if (TextSource && TextSource->GetChanged())
		{
			TextSource->UpdateSettings(configInfo->data);
		}
	}
}

UINT FindFontFace(ConfigTextSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
    UINT id = configInfo->fontFaces.FindValueIndexI(lpFontFace);
    if(id == INVALID)
        return INVALID;
    else
    {
        for(UINT i=0; i<configInfo->fontFaces.Num(); i++)
        {
            UINT targetID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, i, 0);
            if(targetID == id)
                return i;
        }
    }

    return INVALID;
}

UINT FindFontName(ConfigTextSourceInfo *configInfo, HWND hwndFontList, CTSTR lpFontFace)
{
    return configInfo->fontNames.FindValueIndexI(lpFontFace);
}

CTSTR GetFontFace(ConfigTextSourceInfo *configInfo, HWND hwndFontList)
{
    UINT id = (UINT)SendMessage(hwndFontList, CB_GETCURSEL, 0, 0);
    if(id == CB_ERR)
        return NULL;

    UINT actualID = (UINT)SendMessage(hwndFontList, CB_GETITEMDATA, id, 0);
    return configInfo->fontFaces[actualID];
}


INT_PTR CALLBACK TextOutputButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	if (WM_PAINT == message)
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		RECT Rect;
		GetClientRect(hwnd, &Rect);

		HBRUSH HBrush = CreateSolidBrush(RGB(102, 102, 102));

		FillRect(hdc, &Rect, HBrush);
		DeleteObject(HBrush);

		if (IsWindowEnabled(hwnd))
		{
			SetTextColor(hdc, RGB(255, 255, 255));
		}
		else
		{
			SetTextColor(hdc, RGB(153, 153, 153));
		}
		SetBkColor(hdc, RGB(102, 102, 102));
		SetBkMode(hdc, TRANSPARENT);

		SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));

		TCHAR Title[MAX_PATH] = { 0 };

		GetWindowText(hwnd, Title, sizeof Title);

		RECT rtClient;
		GetClientRect(hwnd, &rtClient);

		if (wcslen(Title) > 0)
		{
			DrawText(hdc, Title, wcslen(Title), &rtClient, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
		}

		EndPaint(hwnd, &ps);
	}


	WNDPROC WndProcTem = NULL;
	if (hwnd == GetDlgItem(Hwnd, IDOK))
	{
		WndProcTem = OK1Proc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDC_BROWSE))
	{
		WndProcTem = BrowerProc;
	}
	else if (hwnd == GetDlgItem(Hwnd, IDCANCEL))
	{
		WndProcTem = Cancel1Proc;
	}

	return CallWindowProc(WndProcTem, hwnd, message, wParam, lParam);
}

INT_PTR CALLBACK ConfigureTextProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static bool bInitializedDialog = false;

	static bool _bMouseTrack = true;
	static HBRUSH HBrush = NULL;
	static HFONT g_Font = NULL;
	if (!g_Font)
	{
		g_Font = CreateFont(20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, ANSI_CHARSET, \
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"微软雅黑");
	}
	Hwnd = hwnd;
    switch(message)
    {
	case WM_NCACTIVATE:
	{
						  if (wParam == 0)
							  return FALSE;
	}
	case WM_NCPAINT:
	{
					   DrawFrame(hwnd,-1, true);
	}
		return TRUE;
	case WM_NCLBUTTONDOWN:
	{
							 POINTS Pts = MAKEPOINTS(lParam);
							 POINT Pt;
							 RECT rtWid;
							 GetWindowRect(hwnd, &rtWid);
							 Pt.x = Pts.x - rtWid.left;
							 Pt.y = Pts.y - rtWid.top;

							 for (int i = 0; i < GetListTitle().size(); ++i)
							 {
								 if (GetListTitle()[i].IsPointInRect(Pt))
								 {
									 switch (GetListTitle()[i].Type)
									 {
									 case TitleType_Close:
										 SendMessage(hwnd, WM_CLOSE, 0, 0);
										 return 0;
									 default:
										 break;
									 }
								 }

							 }

							 return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_PAINT:
	{
					 PAINTSTRUCT ps;
					 HDC hdc = BeginPaint(hwnd, &ps);

					 RECT Rect;
					 GetClientRect(hwnd, &Rect);

					 HBRUSH HBrush = CreateSolidBrush(RGB(153, 153, 153));

					 FillRect(hdc, &Rect, HBrush);
					 DeleteObject(HBrush);

					 EndPaint(hwnd, &ps);
	}
		break;
	case WM_NCMOUSEMOVE:
	{

						   if (_bMouseTrack)
						   {
							   TRACKMOUSEEVENT csTME;
							   csTME.cbSize = sizeof (csTME);
							   csTME.dwFlags = TME_LEAVE | TME_HOVER;
							   csTME.hwndTrack = hwnd;
							   csTME.dwHoverTime = 10;
							   ::_TrackMouseEvent(&csTME);


							   _bMouseTrack = FALSE;
						   }

						   POINTS Pts = MAKEPOINTS(lParam);
						   POINT Pt;
						   RECT rtWid;
						   GetWindowRect(hwnd, &rtWid);
						   Pt.x = Pts.x - rtWid.left;
						   Pt.y = Pts.y - rtWid.top;
						   static bool FirstFind = false;
						   bool bFind = false;
						   for (int i = 0; i < GetListTitle().size(); ++i)
						   {

							   if (GetListTitle()[i].IsPointInRect(Pt))
							   {
								   DrawFrame(hwnd, GetListTitle()[i].Type, true);
								   bFind = true;
								   FirstFind = false;

							   }

						   }

						   if (!bFind && !FirstFind)
						   {
							   DrawFrame(hwnd, -1, true);
							   FirstFind = true;
						   }

						   return DefWindowProc(hwnd, message, wParam, lParam);
	}
		break;
	case WM_NCMOUSELEAVE:
	{
							_bMouseTrack = true;
							DrawFrame(hwnd, -1, true);
	}
		break;

	case WM_CTLCOLORSTATIC:
	{
							  HDC hdc = (HDC)wParam;
							  SetTextColor(hdc, RGB(255, 255, 255));
							  SetBkColor(hdc, RGB(153, 153, 153));

							  if (HBrush)
							  {
								  DeleteObject(HBrush);
							  }
							  HBrush = CreateSolidBrush(RGB(153, 153, 153));

							  return (LRESULT)HBrush;
	}
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORLISTBOX:
	{
							   HDC hdc = (HDC)wParam;
							   SetTextColor(hdc, RGB(255, 255, 255));
							   SetBkColor(hdc, RGB(121, 121, 121));

							   if (HBrush)
							   {
								   DeleteObject(HBrush);
							   }

							   HBrush = CreateSolidBrush(RGB(121, 121, 121));
							   return (LRESULT)HBrush;
	}
		break;
        case WM_INITDIALOG:
            {
							  BrowerProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDC_BROWSE), GWLP_WNDPROC);
							  OK1Proc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC);
							  Cancel1Proc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC);

							  SetWindowLongPtr(GetDlgItem(hwnd, IDC_BROWSE), GWLP_WNDPROC, (LONG_PTR)TextOutputButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDOK), GWLP_WNDPROC, (LONG_PTR)TextOutputButtonProc);
							  SetWindowLongPtr(GetDlgItem(hwnd, IDCANCEL), GWLP_WNDPROC, (LONG_PTR)TextOutputButtonProc);

                ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)lParam;
                SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)configInfo);
				LocalizeWindow(hwnd, pluginLocale);

                Value &data = configInfo->data;

                //-----------------------------------------

                HDC hDCtest = GetDC(hwnd);

                LOGFONT lf;
                zero(&lf, sizeof(lf));
                EnumFontFamiliesEx(hDCtest, &lf, (FONTENUMPROC)FontEnumProcThingy, (LPARAM)configInfo, 0);

                HWND hwndFonts = GetDlgItem(hwnd, IDC_FONT);
                for(UINT i=0; i<configInfo->fontNames.Num(); i++)
                {
                    int id = (int)SendMessage(hwndFonts, CB_ADDSTRING, 0, (LPARAM)configInfo->fontNames[i].Array());
                    SendMessage(hwndFonts, CB_SETITEMDATA, id, (LPARAM)i);
                }

				String lpFont = L"";
				if (!data["font"].isNull())
				{
					lpFont = Asic2WChar(data["font"].asString().c_str()).c_str();
				}
                UINT id = FindFontFace(configInfo, hwndFonts, lpFont);
                if(id == INVALID)
                    id = (UINT)SendMessage(hwndFonts, CB_FINDSTRINGEXACT, -1, (LPARAM)TEXT("Arial"));

                SendMessage(hwndFonts, CB_SETCURSEL, id, 0);

                //-----------------------------------------

                SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_SETRANGE32, 5, 2048);

				int fontSize = 48;

				if (!data["fontSize"].isNull())
				{
					fontSize = data["fontSize"].asInt();
				}

				SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_SETPOS32, 0, fontSize);

                //-----------------------------------------

				DWORD color = 0xFFFFFFFF;

				if (!data["color"].isNull())
				{
					color = data["color"].asUInt();
				}

				CCSetColor(GetDlgItem(hwnd, IDC_COLOR), color);

                SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_SETPOS32, 0, data["textOpacity"].asInt());

                SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_SETRANGE32, -4095, 4095);

				int scrollSpeed = 100;

				if (!data["scrollSpeed"].isNull())
					scrollSpeed = data["scrollSpeed"].asInt();

				SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_SETPOS32, 0, scrollSpeed);

				SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT_COUNT), UDM_SETRANGE32, -1, 10000);


				scrollSpeed = -1;

				if (!data["SubTitleRepeatCount"].isNull())
					scrollSpeed = data["SubTitleRepeatCount"].asInt();

				SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT_COUNT), UDM_SETPOS32, 0, scrollSpeed);

                SendMessage(GetDlgItem(hwnd, IDC_BOLD), BM_SETCHECK, data["bold"].asInt() ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_ITALIC), BM_SETCHECK, data["italic"].asInt() ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_UNDERLINE), BM_SETCHECK, data["underline"].asInt() ? BST_CHECKED : BST_UNCHECKED, 0);

                SendMessage(GetDlgItem(hwnd, IDC_VERTICALSCRIPT), BM_SETCHECK, data["vertical"].asInt() ? BST_CHECKED : BST_UNCHECKED, 0);

                BOOL bUsePointFilter = data["pointFiltering"].asInt() != 0;
                SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_SETCHECK, bUsePointFilter ? BST_CHECKED : BST_UNCHECKED, 0);

                //-----------------------------------------

				color = 0xFF000000;

				if (!data["backgroundColor"].isNull())
					color = data["backgroundColor"].asUInt();

				CCSetColor(GetDlgItem(hwnd, IDC_BACKGROUNDCOLOR), color);

                SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_SETRANGE32, 0, 100);

				int backgroundOpacity = 100;

				if (!data["backgroundOpacity"].isNull())
				{
					backgroundOpacity = data["backgroundOpacity"].asInt();
				}

				SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_SETPOS32, 0, backgroundOpacity);

                //-----------------------------------------

                bool bChecked = data["useOutline"].asInt() != 0;
                SendMessage(GetDlgItem(hwnd, IDC_USEOUTLINE), BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS_EDIT), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINECOLOR), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY_EDIT), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), bChecked);

                SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_SETRANGE32, 1, 20);

				int outlineSize = 2;

				if (!data["outlineSize"].isNull())
					outlineSize = data["outlineSize"].asInt();

				SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_SETPOS32, 0, outlineSize);

				color = 0xFF000000;

				if (!data["outlineColor"].isNull())
					color = data["outlineColor"].asUInt();

				CCSetColor(GetDlgItem(hwnd, IDC_OUTLINECOLOR), color);

                SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_SETRANGE32, 0, 100);
                SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_SETPOS32, 0, data["outlineOpacity"].asInt());

                //-----------------------------------------

                bChecked = data["useTextExtents"].asInt() != 0;
                SendMessage(GetDlgItem(hwnd, IDC_USETEXTEXTENTS), BM_SETCHECK, bChecked ? BST_CHECKED : BST_UNCHECKED, 0);
				SendMessage(GetDlgItem(hwnd, IDC_AUTOSIZE), BM_SETCHECK, bChecked ? BST_UNCHECKED:BST_CHECKED, 0);
                ConfigureTextProc(hwnd, WM_COMMAND, MAKEWPARAM(IDC_USETEXTEXTENTS, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_USETEXTEXTENTS));

                EnableWindow(GetDlgItem(hwnd, IDC_EXTENTWIDTH_EDIT), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_EXTENTHEIGHT_EDIT), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_EXTENTWIDTH), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), bChecked);
                EnableWindow(GetDlgItem(hwnd, IDC_WRAP), bChecked);

                bool bVertical = data["vertical"].asInt() != 0;

                SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH),  UDM_SETRANGE32, 32, 2048);
                SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_SETRANGE32, 32, 2048);

				int extentWidth = 100, extentHeight = 50;

				if (!data["extentWidth"].isNull())
					extentWidth = data["extentWidth"].asInt();

				SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH), UDM_SETPOS32, 0, extentWidth);

				if (!data["extentHeight"].isNull())
					extentHeight = data["extentHeight"].asInt();

				SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_SETPOS32, 0, extentHeight);

                bool bWrap = data["wrap"].asInt() != 0;
                SendMessage(GetDlgItem(hwnd, IDC_WRAP), BM_SETCHECK, bWrap ? BST_CHECKED : BST_UNCHECKED, 0);

                bool bScrollMode = data["scrollMode"].asInt() != 0;
                SendMessage(GetDlgItem(hwnd, IDC_SCROLLMODE), BM_SETCHECK, bScrollMode ? BST_CHECKED : BST_UNCHECKED, 0);

                EnableWindow(GetDlgItem(hwnd, IDC_ALIGN), bChecked && bWrap);
                EnableWindow(GetDlgItem(hwnd, IDC_SCROLLMODE), bChecked && bWrap && !bVertical);
                
                HWND hwndAlign = GetDlgItem(hwnd, IDC_ALIGN);
                SendMessage(hwndAlign, CB_ADDSTRING, 0, (LPARAM)L"Sources.TextSource.Left");
                SendMessage(hwndAlign, CB_ADDSTRING, 0, (LPARAM)L"Sources.TextSource.Center");
                SendMessage(hwndAlign, CB_ADDSTRING, 0, (LPARAM)L"Sources.TextSource.Right");

                int align = data["align"].asInt();
                ClampVal(align, 0, 2);
                SendMessage(hwndAlign, CB_SETCURSEL, align, 0);

                //-----------------------------------------

                BOOL bUseFile = data["mode"].asInt() == 1;
                SendMessage(GetDlgItem(hwnd, IDC_USEFILE), BM_SETCHECK, bUseFile ? BST_CHECKED : BST_UNCHECKED, 0);
                SendMessage(GetDlgItem(hwnd, IDC_USETEXT), BM_SETCHECK, bUseFile ? BST_UNCHECKED : BST_CHECKED, 0);

				String Tem = L"";

				if (!data["text"].isNull())
					Tem = Asic2WChar(data["text"].asString().c_str()).c_str();

				SetWindowText(GetDlgItem(hwnd, IDC_TEXT), Tem.Array());

				Tem = L"";
				if (!data["file"].isNull())
				{
					Tem = Asic2WChar(data["file"].asString().c_str()).c_str();
				}

				
				SetWindowText(GetDlgItem(hwnd, IDC_FILE), Tem.Array());

                EnableWindow(GetDlgItem(hwnd, IDC_TEXT), !bUseFile);
                EnableWindow(GetDlgItem(hwnd, IDC_FILE), bUseFile);
                EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), bUseFile);

				bool bUseSubTitle = data["useSubTitle"].asInt() != 0;
				SendMessage(GetDlgItem(hwnd, IDC_USESUBTITLE), BM_SETCHECK, bUseSubTitle ? BST_CHECKED : BST_UNCHECKED, 0);
				EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEUPDOWN), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLERIGHTLEFT), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEATCOUNT_EDIT), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT_COUNT), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SCROLLSPEED_EDIT), bUseSubTitle);
				EnableWindow(GetDlgItem(hwnd, IDC_SCROLLSPEED), bUseSubTitle);

				bool bRepeat = data["subTitleRepeat"].asInt() != 0;
				SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT), BM_SETCHECK, bRepeat ? BST_CHECKED : BST_UNCHECKED, 0);

				bool bUpDown = data["subTitleUpDown"].asInt() != 0;
				if (bUpDown)
				{
					SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEUPDOWN), BM_SETCHECK, BST_CHECKED, 0);
				}
				else
				{
					SendMessage(GetDlgItem(hwnd, IDC_SUBTITLERIGHTLEFT), BM_SETCHECK, BST_CHECKED, 0);
				}


				SendMessage(GetDlgItem(hwnd, IDC_STATIC_SET), WM_SETFONT,(WPARAM)g_Font,TRUE);
				SendMessage(GetDlgItem(hwnd, IDC_STATIC_ROLLSET), WM_SETFONT, (WPARAM)g_Font, TRUE);
				SendMessage(GetDlgItem(hwnd, IDC_STATIC_TEXTFROM), WM_SETFONT, (WPARAM)g_Font, TRUE);


				HDC hdc = GetDC(GetDlgItem(hwnd, IDC_TEXTSIZE_EDIT));
				TEXTMETRIC tm;
				GetTextMetrics(hdc, &tm);
				int nFontHeight = tm.tmHeight + tm.tmExternalLeading;
				RECT rect;
				GetClientRect(GetDlgItem(hwnd, IDC_TEXTSIZE_EDIT), (LPRECT)&rect);
				OffsetRect((LPRECT)&rect, 0, (rect.bottom - nFontHeight) / 2);
				
				SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEATCOUNT_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED_EDIT), EM_SETRECT, 0, (LPARAM)&rect);
				

				ReleaseDC(GetDlgItem(hwnd, IDC_TEXTSIZE_EDIT), hdc);


                bInitializedDialog = true;

				InvalidateRect(hwnd, NULL, FALSE);
				SendMessage(hwnd, WM_NCPAINT, 0, 0);

                return TRUE;
            }

        case WM_DESTROY:
            bInitializedDialog = false;
			if (HBrush)
				DeleteObject(HBrush);
			if (g_Font)
			{
				DeleteObject(g_Font);
				g_Font = NULL;
			}
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_FONT:
                    if(bInitializedDialog)
                    {
                        if(HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == CBN_EDITCHANGE)
                        {
                            ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                            if(!configInfo) break;

                            String strFont;
                            if(HIWORD(wParam) == CBN_SELCHANGE)
                                strFont = GetFontFace(configInfo, (HWND)lParam);
                            else
                            {
                                UINT id = FindFontName(configInfo, (HWND)lParam, GetEditText((HWND)lParam));
                                if(id != INVALID)
                                    strFont = configInfo->fontFaces[id];
                            }

							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configInfo->data["SourceID"].isNull())
							{
								SourceId << configInfo->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *source = NULL;

							if (VideoId)
								source = (IBaseVideo*)VideoId;

                            if(source && strFont.IsValid())
                                source->SetString(TEXT("font"), strFont);
                        }
                    }
                    break;

                case IDC_OUTLINECOLOR:
                case IDC_BACKGROUNDCOLOR:
                case IDC_COLOR:
                    if(bInitializedDialog)
                    {
                        DWORD color = CCGetColor((HWND)lParam);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;
                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_OUTLINECOLOR:      source->SetInt(TEXT("outlineColor"), color); break;
                                case IDC_BACKGROUNDCOLOR:   source->SetInt(TEXT("backgroundColor"), color); break;
                                case IDC_COLOR:             source->SetInt(TEXT("color"), color); break;
                            }
                        }
                    }
                    break;

                case IDC_TEXTSIZE_EDIT:
                case IDC_EXTENTWIDTH_EDIT:
                case IDC_EXTENTHEIGHT_EDIT:
                case IDC_BACKGROUNDOPACITY_EDIT:
                case IDC_TEXTOPACITY_EDIT:
                case IDC_OUTLINEOPACITY_EDIT:
                case IDC_OUTLINETHICKNESS_EDIT:
                case IDC_SCROLLSPEED_EDIT:
				case IDC_SUBTITLEREPEATCOUNT_EDIT:
                    if(HIWORD(wParam) == EN_CHANGE && bInitializedDialog)
                    {
                        int val = (int)SendMessage(GetWindow((HWND)lParam, GW_HWNDNEXT), UDM_GETPOS32, 0, 0);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;

                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_TEXTSIZE_EDIT:             source->SetInt(TEXT("fontSize"), val); break;
                                case IDC_EXTENTWIDTH_EDIT:          source->SetInt(TEXT("extentWidth"), val); break;
                                case IDC_EXTENTHEIGHT_EDIT:         source->SetInt(TEXT("extentHeight"), val); break;
                                case IDC_TEXTOPACITY_EDIT:          source->SetInt(TEXT("textOpacity"), 100 - val); break;
								case IDC_OUTLINEOPACITY_EDIT:       source->SetInt(TEXT("outlineOpacity"), 100 - val); break;
								case IDC_BACKGROUNDOPACITY_EDIT:    source->SetInt(TEXT("backgroundOpacity"), 100 - val); break;
                                case IDC_OUTLINETHICKNESS_EDIT:     source->SetFloat(TEXT("outlineSize"), (float)val); break;
                                case IDC_SCROLLSPEED_EDIT:          source->SetInt(TEXT("scrollSpeed"), val); break;
								case IDC_SUBTITLEREPEATCOUNT_EDIT:  source->SetInt(TEXT("SubTitleRepeatCount"), val); break;
                            }
                        }
                    }
                    break;

                case IDC_BOLD:
                case IDC_ITALIC:
                case IDC_UNDERLINE:
                case IDC_VERTICALSCRIPT:
                case IDC_WRAP:
                case IDC_SCROLLMODE:
                case IDC_USEOUTLINE:
                case IDC_USETEXTEXTENTS:
				case IDC_USESUBTITLE:
				case IDC_SUBTITLEREPEAT:
				case IDC_SUBTITLEUPDOWN:
				case IDC_SUBTITLERIGHTLEFT:
				case IDC_AUTOSIZE:
                    if(HIWORD(wParam) == BN_CLICKED && bInitializedDialog)
                    {
                        BOOL bChecked = SendMessage((HWND)lParam, BM_GETCHECK, 0, 0) == BST_CHECKED;

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;

                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_BOLD:              source->SetInt(TEXT("bold"), bChecked); break;
                                case IDC_ITALIC:            source->SetInt(TEXT("italic"), bChecked); break;
                                case IDC_UNDERLINE:         source->SetInt(TEXT("underline"), bChecked); break;
                                case IDC_VERTICALSCRIPT:    source->SetInt(TEXT("vertical"), bChecked); break;
                                case IDC_WRAP:              source->SetInt(TEXT("wrap"), bChecked); break;
                                case IDC_SCROLLMODE:        source->SetInt(TEXT("scrollMode"), bChecked); break;
                                case IDC_USEOUTLINE:        source->SetInt(TEXT("useOutline"), bChecked); break;
                                case IDC_USETEXTEXTENTS:    source->SetInt(TEXT("useTextExtents"), bChecked); break;
								case IDC_USESUBTITLE:       source->SetInt(TEXT("useSubTitle"), bChecked);break;
								case IDC_SUBTITLEREPEAT:    source->SetInt(TEXT("subTitleRepeat"), bChecked); break;
								case IDC_SUBTITLEUPDOWN:    source->SetInt(TEXT("subTitleUpDown"), bChecked); break;
								case IDC_SUBTITLERIGHTLEFT: source->SetInt(TEXT("subTitleUpDown"), !bChecked); break;
								case IDC_AUTOSIZE:          source->SetInt(TEXT("useTextExtents"), !bChecked); break;
                            }
                        }

                        if(LOWORD(wParam) == IDC_VERTICALSCRIPT)
                        {
                            bool bWrap = SendMessage(GetDlgItem(hwnd, IDC_WRAP), BM_GETCHECK, 0, 0) == BST_CHECKED;
                            bool bUseExtents = SendMessage(GetDlgItem(hwnd, IDC_USETEXTEXTENTS), BM_GETCHECK, 0, 0) == BST_CHECKED;
                            
                            EnableWindow(GetDlgItem(hwnd, IDC_SCROLLMODE), bWrap && bUseExtents && !bChecked);
                        }
                        else if(LOWORD(wParam) == IDC_WRAP)
                        {
                            bool bVertical = SendMessage(GetDlgItem(hwnd, IDC_VERTICALSCRIPT), BM_GETCHECK, 0, 0) == BST_CHECKED;

                            EnableWindow(GetDlgItem(hwnd, IDC_ALIGN), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_SCROLLMODE), bChecked && !bVertical);
                        }
                        else if(LOWORD(wParam) == IDC_USETEXTEXTENTS)
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_EXTENTWIDTH_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_EXTENTHEIGHT_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_EXTENTWIDTH), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_WRAP), bChecked);

                            bool bWrap = SendMessage(GetDlgItem(hwnd, IDC_WRAP), BM_GETCHECK, 0, 0) == BST_CHECKED;
                            bool bVertical = SendMessage(GetDlgItem(hwnd, IDC_VERTICALSCRIPT), BM_GETCHECK, 0, 0) == BST_CHECKED;
                            
                            EnableWindow(GetDlgItem(hwnd, IDC_ALIGN), bChecked && bWrap);
                            EnableWindow(GetDlgItem(hwnd, IDC_SCROLLMODE), bChecked && bWrap && !bVertical);
                        }
                        else if(LOWORD(wParam) == IDC_USEOUTLINE)
                        {
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINECOLOR), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY_EDIT), bChecked);
                            EnableWindow(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), bChecked);
                        }
						else if (LOWORD(wParam) == IDC_USESUBTITLE)
						{
							EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEUPDOWN), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLERIGHTLEFT), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEATCOUNT_EDIT), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT_COUNT), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SCROLLSPEED_EDIT), bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_SCROLLSPEED), bChecked);

							if (!bChecked)
							{
								if (source)
									source->SetInt(TEXT("scrollSpeed"), 0);
							}
							else
							{
								if (source)
									source->SetInt(TEXT("scrollSpeed"), (int)SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_GETPOS32,0,0));
							}
						}
						else if (LOWORD(wParam) == IDC_AUTOSIZE)
						{
							EnableWindow(GetDlgItem(hwnd, IDC_EXTENTWIDTH_EDIT), !bChecked);
							EnableWindow(GetDlgItem(hwnd, IDC_EXTENTHEIGHT_EDIT), !bChecked);

							SendMessage(GetDlgItem(hwnd, IDC_USETEXTEXTENTS), BM_SETCHECK, bChecked ? BST_UNCHECKED:BST_CHECKED, 0);

							//SendMessage(hwnd, WM_COMMAND, MAKEWPARAM(IDC_USETEXTEXTENTS, BN_CLICKED), (LPARAM)GetDlgItem(hwnd, IDC_USETEXTEXTENTS));
						}
                    }
                    break;

                case IDC_ALIGN:
                    if(HIWORD(wParam) == CBN_SELCHANGE && bInitializedDialog)
                    {
                        int align = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
                        if(align == CB_ERR)
                            break;

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;

                        if(source)
                            source->SetInt(TEXT("align"), align);
                    }
                    break;
                case IDC_FILE:
                case IDC_TEXT:
                    if(HIWORD(wParam) == EN_CHANGE && bInitializedDialog)
                    {
                        String strText = GetEditText((HWND)lParam);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;

                        if(source)
                        {
                            switch(LOWORD(wParam))
                            {
                                case IDC_FILE: source->SetString(TEXT("file"), strText); break;
                                case IDC_TEXT: source->SetString(TEXT("text"), strText); break;
                            }
                        }
                    }
                    break;

                case IDC_USEFILE:
                    if(HIWORD(wParam) == BN_CLICKED && bInitializedDialog)
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_TEXT), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_FILE), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;

                        if(source)
                            source->SetInt(TEXT("mode"), 1);
                    }
                    break;

                case IDC_USETEXT:
                    if(HIWORD(wParam) == BN_CLICKED && bInitializedDialog)
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_TEXT), TRUE);
                        EnableWindow(GetDlgItem(hwnd, IDC_FILE), FALSE);
                        EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);

                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;

						std::stringstream SourceId;
						uint64_t VideoId = 0;
						if (!configInfo->data["SourceID"].isNull())
						{
							SourceId << configInfo->data["SourceID"].asString().c_str();
							SourceId >> VideoId;
						}

						IBaseVideo *source = NULL;

						if (VideoId)
							source = (IBaseVideo*)VideoId;
                        if(source)
                            source->SetInt(TEXT("mode"), 0);
                    }
                    break;

                case IDC_BROWSE:
                    {
                        TCHAR lpFile[MAX_PATH+1];
                        zero(lpFile, sizeof(lpFile));

                        OPENFILENAME ofn;
                        zero(&ofn, sizeof(ofn));
                        ofn.lStructSize = sizeof(ofn);
                        ofn.lpstrFile = lpFile;
                        ofn.hwndOwner = hwnd;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.lpstrFilter = TEXT("Text Files (*.txt)\0*.txt\0");
                        ofn.nFilterIndex = 1;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

                        TCHAR curDirectory[MAX_PATH+1];
                        GetCurrentDirectory(MAX_PATH, curDirectory);

                        BOOL bOpenFile = GetOpenFileName(&ofn);
                        SetCurrentDirectory(curDirectory);

                        if(bOpenFile)
                        {
                            SetWindowText(GetDlgItem(hwnd, IDC_FILE), lpFile);

                            ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
							std::stringstream SourceId;
							uint64_t VideoId = 0;
							if (!configInfo->data["SourceID"].isNull())
							{
								SourceId << configInfo->data["SourceID"].asString().c_str();
								SourceId >> VideoId;
							}

							IBaseVideo *source = NULL;

							if (VideoId)
								source = (IBaseVideo*)VideoId;

                            if(source)
                                source->SetString(TEXT("file"), lpFile);
                        }
                    }
                    break;

                case IDOK:
                    {
                        ConfigTextSourceInfo *configInfo = (ConfigTextSourceInfo*)GetWindowLongPtr(hwnd, DWLP_USER);
                        if(!configInfo) break;
                        Value &data = configInfo->data;

                        BOOL bUseTextExtents = SendMessage(GetDlgItem(hwnd, IDC_USETEXTEXTENTS), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bUseOutline = SendMessage(GetDlgItem(hwnd, IDC_USEOUTLINE), BM_GETCHECK, 0, 0) == BST_CHECKED;
						bool bUseSubTitle = SendMessage(GetDlgItem(hwnd, IDC_USESUBTITLE), BM_GETCHECK, 0, 0) == BST_CHECKED;
						bool bSubTitleRepeat = SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT), BM_GETCHECK, 0, 0) == BST_CHECKED;
						bool bSubTileUpDown = SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEUPDOWN), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        float outlineSize = (float)SendMessage(GetDlgItem(hwnd, IDC_OUTLINETHICKNESS), UDM_GETPOS32, 0, 0);

                        int mode = SendMessage(GetDlgItem(hwnd, IDC_USEFILE), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        String strText = GetEditText(GetDlgItem(hwnd, IDC_TEXT));
                        String strFile = GetEditText(GetDlgItem(hwnd, IDC_FILE));

                        UINT extentWidth  = (UINT)SendMessage(GetDlgItem(hwnd, IDC_EXTENTWIDTH),  UDM_GETPOS32, 0, 0);
                        UINT extentHeight = (UINT)SendMessage(GetDlgItem(hwnd, IDC_EXTENTHEIGHT), UDM_GETPOS32, 0, 0);

                        String strFont = GetFontFace(configInfo, GetDlgItem(hwnd, IDC_FONT));
                        UINT fontSize = (UINT)SendMessage(GetDlgItem(hwnd, IDC_TEXTSIZE), UDM_GETPOS32, 0, 0);

                        BOOL bBold = SendMessage(GetDlgItem(hwnd, IDC_BOLD), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bItalic = SendMessage(GetDlgItem(hwnd, IDC_ITALIC), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        BOOL bVertical = SendMessage(GetDlgItem(hwnd, IDC_VERTICALSCRIPT), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        BOOL pointFiltering = SendMessage(GetDlgItem(hwnd, IDC_POINTFILTERING), BM_GETCHECK, 0, 0) == BST_CHECKED;

                        String strFontDisplayName = GetEditText(GetDlgItem(hwnd, IDC_FONT));
                        if(strFont.IsEmpty())
                        {
                            UINT id = FindFontName(configInfo, GetDlgItem(hwnd, IDC_FONT), strFontDisplayName);
                            if(id != INVALID)
                                strFont = configInfo->fontFaces[id];
                        }

                        if(strFont.IsEmpty())
                        {
                            String strError = L"找不到字体 '$1'";
                            strError.FindReplace(TEXT("$1"), strFontDisplayName);
                            BLiveMessageBox(hwnd, strError, NULL, 0);
                            break;
                        }

                        if(bUseTextExtents)
                        {
                            configInfo->cx = float(extentWidth);
                            configInfo->cy = float(extentHeight);
                        }
                        else
                        {
                            String strOutputText;
                            if(mode == 0)
                                strOutputText = strText;
                            else if(mode == 1)
                            {
                                XFile textFile;
                                if(strFile.IsEmpty() || !textFile.Open(strFile, XFILE_READ | XFILE_SHARED, XFILE_OPENEXISTING))
                                {
									String strError = L"找不到字体 '$1'";
                                    strError.FindReplace(TEXT("$1"), strFile);

                                    BLiveMessageBox(hwnd, strError, NULL, 0);
                                    break;
                                }

                                textFile.ReadFileToString(strOutputText);
                            }

                            LOGFONT lf;
                            zero(&lf, sizeof(lf));
                            lf.lfHeight = fontSize;
                            lf.lfWeight = bBold ? FW_BOLD : FW_DONTCARE;
                            lf.lfItalic = bItalic;
                            lf.lfQuality = ANTIALIASED_QUALITY;
                            if(strFont.IsValid())
                                scpy_n(lf.lfFaceName, strFont, 31);
                            else
                                scpy_n(lf.lfFaceName, TEXT("Arial"), 31);

                            HDC hDC = CreateCompatibleDC(NULL);

                            Gdiplus::Font font(hDC, &lf);

                            if(!font.IsAvailable())
                            {
								String strError = L"找不到字体 '$1'";
                                strError.FindReplace(TEXT("$1"), strFontDisplayName);
                                BLiveMessageBox(hwnd, strError, NULL, 0);
                                DeleteDC(hDC);
                                break;
                            }

                            {
                                Gdiplus::Graphics graphics(hDC);
                                Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());

                                UINT formatFlags;

                                formatFlags = Gdiplus::StringFormatFlagsNoFitBlackBox
                                            | Gdiplus::StringFormatFlagsMeasureTrailingSpaces;

                                if(bVertical)
                                    formatFlags |= Gdiplus::StringFormatFlagsDirectionVertical
                                                 | Gdiplus::StringFormatFlagsDirectionRightToLeft;

                                format.SetFormatFlags(formatFlags);
                                format.SetTrimming(Gdiplus::StringTrimmingWord);

                                Gdiplus::RectF rcf;
                                graphics.MeasureString(strOutputText, -1, &font, Gdiplus::PointF(0.0f, 0.0f), &format, &rcf);

                                if(bUseOutline)
                                {
                                    rcf.Height += outlineSize;
                                    rcf.Width  += outlineSize;
                                }

                                if(bVertical)
                                {
                                    if(rcf.Width<fontSize)
                                        rcf.Width = (float)fontSize;
                                }
                                else
                                {
                                    if(rcf.Height<fontSize)
                                        rcf.Height = (float)fontSize;
                                }
                                configInfo->cx = MAX(rcf.Width,  32.0f);
                                configInfo->cy = MAX(rcf.Height, 32.0f);
                            }

                            DeleteDC(hDC);
                        }

                        data["baseSizeCX"] = configInfo->cx;
                        data["baseSizeCY"] = configInfo->cy;

                        data["font"] = WcharToAnsi(strFont.Array()).c_str();
                        data["color"] = (UINT)CCGetColor(GetDlgItem(hwnd, IDC_COLOR));
                        data["fontSize"] = fontSize;
                        data["textOpacity"] = (UINT)SendMessage(GetDlgItem(hwnd, IDC_TEXTOPACITY), UDM_GETPOS32, 0, 0);
                        data["scrollSpeed"] = (int)SendMessage(GetDlgItem(hwnd, IDC_SCROLLSPEED), UDM_GETPOS32, 0, 0);
                        data["bold"] = bBold;
                        data["italic"] = bItalic;
                        data["vertical"] = bVertical;
                        data["wrap"] = SendMessage(GetDlgItem(hwnd, IDC_WRAP), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        data["scrollMode"] = SendMessage(GetDlgItem(hwnd, IDC_SCROLLMODE), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        data["underline"] = SendMessage(GetDlgItem(hwnd, IDC_UNDERLINE), BM_GETCHECK, 0, 0) == BST_CHECKED;
                        data["pointFiltering"] = pointFiltering;

                        data["backgroundColor"] =  (UINT)CCGetColor(GetDlgItem(hwnd, IDC_BACKGROUNDCOLOR));
                        data["backgroundOpacity"] =  (UINT)SendMessage(GetDlgItem(hwnd, IDC_BACKGROUNDOPACITY), UDM_GETPOS32, 0, 0);

                        data["useOutline"] = bUseOutline;
                        data["outlineColor"] =  (UINT)CCGetColor(GetDlgItem(hwnd, IDC_OUTLINECOLOR));
                        data["outlineSize"] = outlineSize;
						data["outlineOpacity"] = (UINT)SendMessage(GetDlgItem(hwnd, IDC_OUTLINEOPACITY), UDM_GETPOS32, 0, 0);

                        data["useTextExtents"] = bUseTextExtents;
                        data["extentWidth"] = extentWidth;
                        data["extentHeight"] = extentHeight;
                        data["align"] = (int)SendMessage(GetDlgItem(hwnd, IDC_ALIGN), CB_GETCURSEL, 0, 0);

						if (strFile.Array())
						 data["file"] = WcharToAnsi(strFile.Array()).c_str();

						if (strText.Array())
							data["text"] =  WcharToAnsi(strText.Array()).c_str();
                        data["mode"] = mode;
						data["useSubTitle"] = bUseSubTitle;
						data["subTitleRepeat"] = bSubTitleRepeat;
						data["subTitleUpDown"] = bSubTileUpDown;
						data["SubTitleRepeatCount"] = (int)SendMessage(GetDlgItem(hwnd, IDC_SUBTITLEREPEAT_COUNT), UDM_GETPOS32, 0, 0);
                    }
					
                case IDCANCEL:
                    if(LOWORD(wParam) == IDCANCEL)
                        DoCancelStuff(hwnd);

                    EndDialog(hwnd, LOWORD(wParam));
            }
            break;

        case WM_CLOSE:
            DoCancelStuff(hwnd);
            EndDialog(hwnd, IDCANCEL);
    }
    return 0;
}

static bool bFirst = true;

bool STDCALL ConfigureTextSource(Value& element, bool bCreating)
{
	if (bFirst)
	{
		bFirst = false;
		InitColorControl(hinstMain);

		if (!pluginLocale)
		{
			pluginLocale = new LocaleStringLookup;

			if (!pluginLocale->LoadStringFile(AGENTLOCALPATH))
				Log::writeError(LOG_RTSPSERV, 1, "Could not open locale string file '%s'", WcharToAnsi(AGENTLOCALPATH).c_str());
		}
	}

    ConfigTextSourceInfo configInfo(element);


    if(BLiveDialogBox(hinstMain, MAKEINTRESOURCE(IDD_CONFIGURETEXTSOURCE), GetMainWindow(), ConfigureTextProc, (LPARAM)&configInfo) == IDOK)
    {
        element["cx"] =  configInfo.cx + Width;
        element["cy"] = configInfo.cy;

        return true;
    }

    return false;
}

REGINST_CONFIGFUNC(TextOutputSource, ConfigureTextSource)
