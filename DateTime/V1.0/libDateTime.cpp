// libDateTime.cpp : 定义 DLL 应用程序的导出函数。
//


#include "libDateTime.h"

IMPLEMENT_DYNIC(ProcessDateTime,"时间源","V1.0.0.1")

ProcessDateTime::ProcessDateTime()
{
	SamplerInfo si;
	zero(&si, sizeof(si));
	si.addressU = GS_ADDRESS_REPEAT;
	si.addressV = GS_ADDRESS_REPEAT;
	si.borderColor = 0;
	si.filter = GS_FILTER_LINEAR;
	ss = CreateSamplerState(si);
	globalOpacity = 100;
	bHasPre = false;
	texture = NULL;
	hFont = NULL;
}

ProcessDateTime::~ProcessDateTime()
{
	if (texture)
	{
		delete texture;
		texture = NULL;
	}

	delete ss;

	if (hFont)
	{
		DeleteObject(hFont);
		hFont = NULL;
	}
}

void ProcessDateTime::Preprocess()
{
		UpdateTexture();
}

void ProcessDateTime::Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture, bool bScaleFull, bool bIsLiveC)
{
	bScaleFull = false;
	if (texture)
	{
		//EnableBlending(FALSE);
		Vect2 sizeMultiplier = size / Resolution;

		sizeMultiplier.x += 0.1;
		Vect2 newSize = Vect2(float(textureSize.cx), float(textureSize.cy)) * sizeMultiplier;

		DWORD alpha = DWORD(double(globalOpacity)*2.55);
		DWORD outputColor = (alpha << 24) | 0xFFFFFF;

		DrawSprite(texture, outputColor, pos.x, pos.y, pos.x + newSize.x, pos.y + newSize.y);

		//EnableBlending(TRUE);
	}
}

Vect2 ProcessDateTime::GetSize() const
{
	return Resolution;
}

void ProcessDateTime::UpdateSettings(Value &data)
{
	this->data = data;
	strFont = TEXT("Arial");
	if (!data["Font"].isNull())
	{
		strFont = Asic2WChar(data["Font"].asString()).c_str();
	}
	color = 0xFFFFFFFF;
	if (!data["FontColor"].isNull())
	{
		color = data["FontColor"].asUInt();
	}

	size = 48;

	align = data["align"].asInt();
	opacity = 100;

	if (!data["textOpacity"].isNull())
	{
		opacity = data["textOpacity"].asInt();
	}

	strFormatString = Asic2WChar(data["FormatString"].asString()).c_str();

	bUseOutline = data["useOutline"].asUInt() != 0;
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
		outlineOpacity = data["outlineOpacity"].asInt();
	}

	//新增
	baseSize.x = 330;
	if (!data["baseSizeCX"].isNull())
	{
		baseSize.x = data["baseSizeCX"].asDouble();
	}
	baseSize.y = 50;

	if (!data["baseSizeCY"].isNull())
	{
		baseSize.y = data["baseSizeCY"].asDouble();
	}

	Resolution.x = 330;

	if (!data["cx"].isNull())
	{
		Resolution.x = data["cx"].asDouble();
	}

	Resolution.y = 50;

	if (!data["cy"].isNull())
	{
		Resolution.y = data["cy"].asDouble();
	}

	bUpdateTexture = true;
}

void ProcessDateTime::SetString(CTSTR lpName, CTSTR lpVal)
{
	if (scmpi(lpName, TEXT("Font")) == 0)
	{
		strFont = lpVal;
	}
	else if (scmpi(lpName, TEXT("FormatString")) == 0)
		strFormatString = lpVal;

	bUpdateTexture = true;
}

void ProcessDateTime::SetInt(CTSTR lpName, int iValue)
{
	if (scmpi(lpName, TEXT("Fontcolor")) == 0)
		color = iValue;
	else if (scmpi(lpName, TEXT("fontSize")) == 0)
		size = iValue;
	else if (scmpi(lpName, TEXT("underline")) == 0)
		bUnderline = iValue != 0;
	else if (scmpi(lpName, TEXT("align")) == 0)
		align = iValue;
	else if (scmpi(lpName, TEXT("useOutline")) == 0)
		bUseOutline = iValue != 0;
	else if (scmpi(lpName, TEXT("outlineColor")) == 0)
		outlineColor = iValue;

	bUpdateTexture = true;
}

void ProcessDateTime::SetFloat(CTSTR lpName, float fValue)
{
	if (scmpi(lpName, TEXT("outlineSize")) == 0)
		outlineSize = fValue;
	else if (scmpi(lpName, TEXT("cx")) == 0)
	{
		Resolution.x = fValue;
	}
	else if (scmpi(lpName, TEXT("cy")) == 0)
	{
		Resolution.y = fValue;
	}

	bUpdateTexture = true;
}


void ProcessDateTime::DrawOutlineText(Graphics *graphics,
	Font &font,
	const GraphicsPath &path,
	const StringFormat &format,
	const Brush *brush)
{

	GraphicsPath *outlinePath;

	outlinePath = path.Clone();

	// Outline color and size
	UINT tmpOpacity = (UINT)((((float)opacity * 0.01f) * ((float)outlineOpacity * 0.01f)) * 100.0f);
	Pen pen(Gdiplus::Color(GetAlphaVal(tmpOpacity) | (outlineColor & 0xFFFFFF)), outlineSize);
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

HFONT ProcessDateTime::GetFont()
{
	HFONT hFont = NULL;
	bBold = false;
	LOGFONT lf;
	zero(&lf, sizeof(lf));
	lf.lfHeight = size;
	lf.lfWeight = bBold ? FW_BOLD : FW_DONTCARE;
	lf.lfItalic = false;
	lf.lfUnderline = false;
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

void ProcessDateTime::UpdateCurrentText()
{
	SYSTEMTIME LocalTime;
	GetLocalTime(&LocalTime);

	if (strFormatString.IsEmpty())
	{
		strFormatString = "HH:mm:ss";
	}
	
	LPTSTR pf = (LPTSTR)(LPCTSTR)strFormatString;
	if (pf)
	{
		char *pFileName = new char[2 * wcslen(pf) + 1];
		wcstombs(pFileName, pf, 2 * wcslen(pf) + 1);

		char cLocalTime[100] = { 0 };
		sprintf(cLocalTime, "%4d-%2d-%2d %2d:%2d:%2d", LocalTime.wYear,
			LocalTime.wMonth,
			LocalTime.wDay,
			LocalTime.wHour,
			LocalTime.wMinute,
			LocalTime.wSecond);
		m_DateTime.LoadFormatTime(cLocalTime, pFileName);

// 		string sFormatTime = m_DateTime.GetTimeString();
// 		size_t len = sFormatTime.length();
// 		size_t wlen = MultiByteToWideChar(CP_ACP, 0, (const char*)sFormatTime.c_str(), int(len), NULL, 0);
// 		wchar_t *wBuf = new wchar_t[wlen + 1];
// 		MultiByteToWideChar(CP_ACP, 0, (const char*)sFormatTime.c_str(), int(len), wBuf, int(wlen));
// 		wBuf[len] = '\0';

		strCurrentText = Asic2WChar(m_DateTime.GetTimeString().c_str()).c_str();

		if (strCurrentText.IsEmpty())
		{
			strFormatString = "HH:mm:ss";

			pf = L"HH:mm:ss";
			char *pFileName = new char[2 * wcslen(pf) + 1];
			wcstombs(pFileName, pf, 2 * wcslen(pf) + 1);

			char cLocalTime[100] = { 0 };
			sprintf(cLocalTime, "%4d-%2d-%2d %2d:%2d:%2d", LocalTime.wYear,
				LocalTime.wMonth,
				LocalTime.wDay,
				LocalTime.wHour,
				LocalTime.wMinute,
				LocalTime.wSecond);
			m_DateTime.LoadFormatTime(cLocalTime, pFileName);

			strCurrentText = Asic2WChar(m_DateTime.GetTimeString().c_str()).c_str();
		}

		delete[] pFileName;
		//delete[] wBuf;
	}
}

void ProcessDateTime::SetStringFormat(StringFormat &format)
{
	UINT formatFlags;

	formatFlags = Gdiplus::StringFormatFlagsNoFitBlackBox
		| Gdiplus::StringFormatFlagsMeasureTrailingSpaces;


	format.SetFormatFlags(formatFlags);
	format.SetTrimming(Gdiplus::StringTrimmingWord);

	format.SetAlignment(Gdiplus::StringAlignmentNear);


}

float ProcessDateTime::ProcessScrollMode(Graphics *graphics, Font *font, RectF &layoutBox, StringFormat *format)
{
	StringList strList;
	RectF boundingBox;

	float offset = layoutBox.Height;

	RectF l2(0.0f, 0.0f, layoutBox.Width, 32000.0f); // Really, it needs to be OVER9000

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

void ProcessDateTime::UpdateTexture()
{
	Status stat;
	RectF layoutBox;
	SIZE textSize;

	RectF boundingBox(0.0f, 0.0f, 32.0f, 32.0f);

	UpdateCurrentText();

	if (bUpdateTexture)
	{
		bUpdateTexture = false;

		if (hFont)
		{
			DeleteObject(hFont);
			hFont = NULL;
		}

		hFont = GetFont();

		if (!hFont)
			return;
	}

	StringFormat format(Gdiplus::StringFormat::GenericTypographic());

	SetStringFormat(format);

	HDC hdc = CreateCompatibleDC(NULL);

	Font font(hdc, hFont);
	Graphics graphics(hdc);
	graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
	if (strCurrentText.IsValid())
	{

		stat = graphics.MeasureString(strCurrentText, -1, &font, PointF(0.0f, 0.0f), &format, &boundingBox);
		if (stat != Ok)
			Log::writeError(LOG_RTSPSERV, 1, "TextSource::UpdateTexture: Gdiplus::Graphics::MeasureString failed: %u", (int)stat);
		if (bUseOutline)
		{
			//Note: since there's no path widening in DrawOutlineText the padding is half than what it was supposed to be.
			boundingBox.Width += outlineSize;
			boundingBox.Height += outlineSize;
		}
	}

	DeleteDC(hdc);
	hdc = NULL;

	if (boundingBox.Height < size)
	{
		textSize.cy = size;
		boundingBox.Height = float(size);
	}
	else
		textSize.cy = LONG(boundingBox.Height + EPSILON);

	textSize.cx = LONG(boundingBox.Width + EPSILON);
	//textSize.cx &= 0xFFFFFFFE;
	//textSize.cy &= 0xFFFFFFFE;

	textSize.cx += textSize.cx % 2;
	textSize.cy += textSize.cy % 2;

	ClampVal(textSize.cx, 32, 8192);
	ClampVal(textSize.cy, 32, 8192);
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

		Bitmap      bmp(textSize.cx, textSize.cy, 4 * textSize.cx, PixelFormat32bppARGB, (BYTE*)lpBits);

		Graphics graphics(&bmp);

		SolidBrush  brush(Gdiplus::Color(GetAlphaVal(opacity) | (color & 0x00FFFFFF)));

		DWORD bkColor;

		
		bkColor = ((strCurrentText.IsValid()) ? GetAlphaVal(0) : GetAlphaVal(0));

		if (textSize.cx > boundingBox.Width || textSize.cy > boundingBox.Height)
		{
			stat = graphics.Clear(Gdiplus::Color(0x00000000));
			if (stat != Gdiplus::Ok)
				Log::writeError(LOG_RTSPSERV, 1, "TextSource::UpdateTexture: Graphics::Clear failed: %u", (int)stat);

			SolidBrush *bkBrush = new SolidBrush(Gdiplus::Color(bkColor));

			graphics.FillRectangle(bkBrush, boundingBox);

			delete bkBrush;
		}
		else
		{
			stat = graphics.Clear(Gdiplus::Color(bkColor));
			if (stat != Ok)
				Log::writeError(LOG_RTSPSERV, 1, "TextSource::UpdateTexture: Graphics::Clear failed: %u", (int)stat);
		}

		graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
		graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

		if (strCurrentText.IsValid())
		{
			if (bUseOutline)
			{
				boundingBox.Offset(outlineSize / 2, outlineSize / 2);

				FontFamily fontFamily;
				GraphicsPath path;

				font.GetFamily(&fontFamily);

				path.AddString(strCurrentText, -1, &fontFamily, font.GetStyle(), font.GetSize(), boundingBox, &format);

				DrawOutlineText(&graphics, font, path, format, &brush);
			}
			else
			{
				stat = graphics.DrawString(strCurrentText, -1, &font, boundingBox, &format, &brush);
				if (stat != Gdiplus::Ok)
					Log::writeError(LOG_RTSPSERV, 1, "TextSource::UpdateTexture: Graphics::DrawString failed: %u", (int)stat);
			}
		}

		//----------------------------------------------------------------------
		// upload texture

		if (textureSize.cx != textSize.cx || textureSize.cy != textSize.cy)
		{
			if (texture)
			{
				delete texture;
				texture = NULL;
			}

			mcpy(&textureSize, &textSize, sizeof(textureSize));
			texture = CreateTexture(textSize.cx, textSize.cy, GS_BGRA, lpBits, FALSE, FALSE);
		}
		else if (texture)
			SetImage(texture,lpBits, GS_IMAGEFORMAT_BGRA, 4 * textSize.cx);

		if (!texture)
		{
			Log::writeError(LOG_RTSPSERV,1,"TextSource::UpdateTexture: could not create texture");
			DeleteObject(hFont);
		}

		DeleteDC(hTempDC);
		DeleteObject(hBitmap);
	}
}

void ProcessDateTime::SetHasPreProcess(bool bHasPre)
{
	this->bHasPre = bHasPre;
}

bool ProcessDateTime::GetHasPreProcess() const
{
	return bHasPre;
}
