#pragma once
#include <gdiplus.h>
using namespace Gdiplus;
class CGdiPlusImage
{
public:
	Gdiplus::Image* m_pImage;

public:
	CGdiPlusImage()							{ m_pImage = NULL; }
	CGdiPlusImage(LPCWSTR pFile)				{ m_pImage = NULL; LoadFromFile(pFile); }
	virtual ~CGdiPlusImage()					{ Empty(); }

	void Empty()								{ delete m_pImage; m_pImage = NULL; }

	bool LoadFromFile(LPCWSTR pFile)
	{
		Empty();
		m_pImage = Gdiplus::Image::FromFile(pFile);
		return m_pImage->GetLastStatus() == Gdiplus::Ok;
	}

	int GetWidth(){ return m_pImage->GetWidth(); }
	int GetHeight(){ return m_pImage->GetHeight(); }
	Gdiplus::Image* GetImage() const { return m_pImage; }
	operator Gdiplus::Image*() { return m_pImage; }
};

