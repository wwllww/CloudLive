#include "DrawFrame.h"
#include <VersionHelpers.h>

Title* Title::m_title = NULL;

Title* Title::Instance()
{
	if (m_title == NULL)
	{
		m_title = new Title;
		m_title->LoadTitlePis();
	}

	return m_title;
}

void Title::LoadTitlePis()
{
	//加载四个标题图片
	TCHAR *Path[] = {
		TEXT(".\\img\\Menu.png"),
		TEXT(".\\img\\MenuHover.png"),
		TEXT(".\\img\\Min.png"),
		TEXT(".\\img\\MinHover.png"),
		TEXT(".\\img\\Max.png"),
		TEXT(".\\img\\MaxHover.png"),
		TEXT(".\\img\\Enlarge.png"),
		TEXT(".\\img\\EnlargeHover.png"),
		TEXT(".\\img\\Close.png"),
		TEXT(".\\img\\CloseHover.png")
	};
	TitleType  Type[] = { TitleType_Setting, TitleType_Min, TitleType_Max, TitleType_Close };

	int CourrentRow = 0;

	for (int i = 0; i < 4; ++i)
	{
		int j = 2;

		if (i == 2)
			j = 4;
		TitlePicInfo PicInfo;
		bool bCanAdd = false;
		for (int k = 0; k < j; ++k)
		{
			Gdiplus::Image *BkImage = (Gdiplus::Image*)Gdiplus::Bitmap::FromFile(Path[CourrentRow]);
			if (BkImage)
			{
				bCanAdd = true;
				PicInfo.Img[k] = BkImage;
				PicInfo.Type = (TitleType)Type[i];
			}
			++CourrentRow;
		}
		if (bCanAdd)
			ListTitle.push_back(PicInfo);
	}
}

void Title::DrawFrame(HWND hwnd, int Type, bool IsOnlyColse)
{
	using namespace Gdiplus;
	HDC hdc;
	hdc = GetWindowDC(hwnd);

	RECT rtWnd, rtClientWnd;
	//获取窗口位置  
	GetWindowRect(hwnd, &rtWnd);
	rtWnd.right += -rtWnd.left;
	rtWnd.bottom += -rtWnd.top;
	rtWnd.left = 0;
	rtWnd.top = 0;
	GetClientRect(hwnd, &rtClientWnd);
	rtClientWnd.left += 8;
	rtClientWnd.right += 8;
	rtClientWnd.top += 30;
	rtClientWnd.bottom += 30;
	if (IsWindows8OrGreater())
	{
		ExcludeClipRect(hdc, rtClientWnd.left, rtClientWnd.top + 1, rtClientWnd.right, rtClientWnd.bottom);
	}
	else
	{
		ExcludeClipRect(hdc, rtClientWnd.left, rtClientWnd.top, rtClientWnd.right, rtClientWnd.bottom);
	}


	HDC hMemDC = CreateCompatibleDC(hdc);
	HBITMAP HBit = CreateCompatibleBitmap(hdc, rtWnd.right, rtWnd.bottom);

	HBITMAP oldHbmp = (HBITMAP)SelectObject(hMemDC, HBit);
	SetBkMode(hMemDC, TRANSPARENT);

	Graphics graphics(hMemDC);
	RectF destRect;
	graphics.Clear(Gdiplus::Color(0xFF232325));

	const int Width = 25;
	const int Height = 25;
	if (-1 == Type)
	{
		for (int i = ListTitle.size() - 1, j = 0; i >= 0; --i, ++j)
		{
			Image *Img = NULL;

			if (!IsZoomed(hwnd) && ListTitle[i].Type == TitleType_Max)
			{
				Img = reinterpret_cast<Image*>(ListTitle[i].Img[2]);
			}
			else
			{
				Img = reinterpret_cast<Image*>(ListTitle[i].Img[0]);
			}


			if (ListTitle[i].Type == TitleType_Close)
			{
				destRect.X = rtWnd.right - 40;
			}
			else
			{
				destRect.X = rtWnd.right - 40 - 25 * j;
			}

			destRect.Y = 3;
			destRect.Width = Width;
			destRect.Height = Height;

			RECT &Rect = ListTitle[i].Rect;
			Rect.left = destRect.X;
			Rect.top = destRect.Y;
			Rect.right = destRect.X + Width;
			Rect.bottom = destRect.Y + Height;
			graphics.DrawImage(Img, destRect, 0, 0, Img->GetWidth(), Img->GetHeight(), UnitPixel);

			if (IsOnlyColse)
				break;
		}
	}
	else
	{
		for (int i = ListTitle.size() - 1; i >= 0; --i)
		{
			Image *Img = NULL;
			if (ListTitle[i].Type == Type)
			{
				if (!IsZoomed(hwnd) && ListTitle[i].Type == TitleType_Max)
				{
					Img = reinterpret_cast<Image*>(ListTitle[i].Img[3]);
				}
				else
				{
					Img = reinterpret_cast<Image*>(ListTitle[i].Img[1]);
				}

			}
			else
			{
				if (!IsZoomed(hwnd) && ListTitle[i].Type == TitleType_Max)
				{
					Img = reinterpret_cast<Image*>(ListTitle[i].Img[2]);
				}
				else
				{
					Img = reinterpret_cast<Image*>(ListTitle[i].Img[0]);
				}
			}
			RECT &Rect = ListTitle[i].Rect;
			destRect.X = Rect.left;
			destRect.Y = Rect.top;
			destRect.Width = Rect.right - Rect.left;
			destRect.Height = Rect.bottom - Rect.top;

			graphics.DrawImage(Img, destRect, 0, 0, Img->GetWidth(), Img->GetHeight(), UnitPixel);

			if (IsOnlyColse)
				break;
		}
	}

	int xPos = 10;
	int yPos = 10;

	TCHAR strText[1024] = { 0 };
	GetWindowText(hwnd, strText, sizeof strText);
	if (wcscmp(strText, L"") != 0)
	{
		Gdiplus::FontFamily fontFamily(L"微软雅黑");
		Gdiplus::Font  font(&fontFamily, 10, Gdiplus::FontStyleRegular, Gdiplus::Unit::UnitPoint);

		StringFormat stringFormat;
		stringFormat.SetFormatFlags(StringFormatFlagsNoWrap);
		stringFormat.SetAlignment(StringAlignmentNear);
		stringFormat.SetLineAlignment(StringAlignmentNear);

		SolidBrush brush((ARGB)Color::White);
		PointF point;

		point.X = (Gdiplus::REAL)xPos;
		point.Y = (Gdiplus::REAL)yPos - 3;
		graphics.DrawString(strText, wcslen(strText), &font, point, &stringFormat, &brush);
	}

	BitBlt(hdc, 0, 0, rtWnd.right, rtWnd.bottom, hMemDC, 0, 0, SRCCOPY);

	SelectObject(hMemDC, oldHbmp);
	DeleteObject(HBit);
	DeleteObject(hMemDC);
	ReleaseDC(hwnd, hdc);
}