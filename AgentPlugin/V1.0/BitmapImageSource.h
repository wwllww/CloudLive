
#ifndef BITMAPIMAGESOURCE_H
#define BITMAPIMAGESOURCE_H


#include "BaseAfx.h"
#include "BitmapImage.h"
#include "BaseVideo.h"
#include "resource.h"

struct ColorSelectionData
{
    HDC hdcDesktop;
    HDC hdcDestination;
    HBITMAP hBitmap;
    bool bValid;

    inline ColorSelectionData() : hdcDesktop(NULL), hdcDestination(NULL), hBitmap(NULL), bValid(false) {}
    inline ~ColorSelectionData() {Clear();}

    inline bool Init()
    {
        hdcDesktop = GetDC(NULL);
        if(!hdcDesktop)
            return false;

        hdcDestination = CreateCompatibleDC(hdcDesktop);
        if(!hdcDestination)
            return false;

        hBitmap = CreateCompatibleBitmap(hdcDesktop, 1, 1);
        if(!hBitmap)
            return false;

        SelectObject(hdcDestination, hBitmap);
        bValid = true;

        return true;
    }

    inline void Clear()
    {
        if(hdcDesktop)
        {
            ReleaseDC(NULL, hdcDesktop);
            hdcDesktop = NULL;
        }

        if(hdcDestination)
        {
            DeleteDC(hdcDestination);
            hdcDestination = NULL;
        }

        if(hBitmap)
        {
            DeleteObject(hBitmap);
            hBitmap = NULL;
        }

        bValid = false;
    }

    inline DWORD GetColor()
    {
        POINT p;
        if(GetCursorPos(&p))
        {
            BITMAPINFO data;
            zero(&data, sizeof(data));

            data.bmiHeader.biSize = sizeof(data.bmiHeader);
            data.bmiHeader.biWidth = 1;
            data.bmiHeader.biHeight = 1;
            data.bmiHeader.biPlanes = 1;
            data.bmiHeader.biBitCount = 24;
            data.bmiHeader.biCompression = BI_RGB;
            data.bmiHeader.biSizeImage = 4;

            if(BitBlt(hdcDestination, 0, 0, 1, 1, hdcDesktop, p.x, p.y, SRCCOPY|CAPTUREBLT))
            {
                DWORD buffer;
                if(GetDIBits(hdcDestination, hBitmap, 0, 1, &buffer, &data, DIB_RGB_COLORS))
                    return 0xFF000000|buffer;
            }
            else
            {
                int err = GetLastError();
                nop();
            }
        }

        return 0xFF000000;
    }
};


class BitmapImageSource : public IBaseVideo
{
	DYNIC_DECLARE()
    BitmapImage bitmapImage;

    Value data;

    bool     bUseColorKey;
    DWORD    keyColor;
    UINT     keySimilarity, keyBlend;

    DWORD opacity;
    DWORD color;

    Shader   *colorKeyShader, *alphaIgnoreShader;
	int Width;
	int Height;
	
	bool bPreProcess;

public:
    BitmapImageSource();

    ~BitmapImageSource();

	bool Init(Value &JsonParam);

    void Tick(float fSeconds);

	void Render(const Vect2 &pos, const Vect2 &size, Texture* Text, bool bScaleFull, bool bIsLiveC);

    void UpdateSettings(Value &Data);

    Vect2 GetSize() const {return bitmapImage.GetSize();}

	void SetInt(CTSTR lpName, int iValue);

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
};

#endif // !BITMAPIMAGESOURCE_H