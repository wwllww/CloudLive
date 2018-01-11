#include "BitmapImage.h"

BitmapImage::BitmapImage()
{
	changeMonitor = NULL;
	lpGifData = NULL;
	texture = NULL;
}

BitmapImage::~BitmapImage()
{
    if(lpGifData)
        Free(lpGifData);

	EnableFileMonitor(false);

    delete texture;
}
//----------------------------------------------------------------------------

void BitmapImage::CreateErrorTexture(void)
{
	LPBYTE textureData = (LPBYTE)Allocate_Bak(32 * 32 * 4);
    msetd(textureData, 0xFF0000FF, 32*32*4);

    texture = CreateTexture(32, 32, GS_RGB, textureData, FALSE,TRUE);
    fullSize.Set(32.0f, 32.0f);

    Free(textureData);
}
//----------------------------------------------------------------------------

void BitmapImage::SetPath(String path)
{
    filePath = path;
}

void BitmapImage::EnableFileMonitor(bool bMonitor)
{
    if (changeMonitor)
    {
        OSMonitorFileDestroy(changeMonitor);
        changeMonitor = NULL;
    }

    if (bMonitor)
        changeMonitor = OSMonitorFileStart(filePath);
}

void BitmapImage::Init(void)
{
	bIsAnimatedGif = false;

    if(lpGifData)
    {
        Free(lpGifData);
        lpGifData = NULL;
    }

    animationTimes.Clear();

    delete texture;
    texture = NULL;

    CTSTR lpBitmap = filePath;
    if(!lpBitmap || !*lpBitmap)
    {
		Log::writeError(LOG_RTSPSERV, 1, "BitmapImage::Init: Empty path");
        CreateErrorTexture();
        return;
    }

    if(!bIsAnimatedGif)
    {
		UINT Width, Height;
		texture = CreateTextureFromFile(lpBitmap, TRUE, Width, Height);
        if(!texture)
        {
            Log::writeError(LOG_RTSPSERV,1,"BitmapImage::Init: could not create texture '%s'", WcharToAnsi(lpBitmap).c_str());
            CreateErrorTexture();
            return;
        }

		fullSize.x = Width;
		fullSize.y = Height;
    }
	
}

Vect2 BitmapImage::GetSize(void) const
{
    return fullSize;
}

Texture* BitmapImage::GetTexture(void) const
{
    return texture;
}

void BitmapImage::Tick(float fSeconds)
{
    if (updateImageTime)
    {
        updateImageTime -= fSeconds;
        if (updateImageTime <= 0.0f)
        {
            updateImageTime = 0.0f;
            Init();
        }
    }

    if (changeMonitor && OSFileHasChanged(changeMonitor))
        updateImageTime = 1.0f;
}
