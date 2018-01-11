#pragma once
#include "BaseAfx.h"

class BitmapImage{
    Texture *texture;
    Vect2 fullSize;

    bool bIsAnimatedGif;
    LPBYTE lpGifData;
    List<float> animationTimes;
    BYTE **animationFrameCache;
    BYTE *animationFrameData;
    UINT curFrame, curLoop, lastDecodedFrame;
    float curTime;
    float updateImageTime;

    String filePath;
    OSFileChangeData *changeMonitor;
    void CreateErrorTexture(void);

public:
    BitmapImage();
    ~BitmapImage();

    void SetPath(String path);
    void EnableFileMonitor(bool bMonitor);
    void Init(void);

    Vect2 GetSize(void) const;
    Texture* GetTexture(void) const;

    void Tick(float fSeconds);
};
