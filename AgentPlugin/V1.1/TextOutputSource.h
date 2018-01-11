#ifndef TEXTOUTPUTSOURCE_H
#define TEXTOUTPUTSOURCE_H

#include "BaseAfx.h"
#include "BaseVideo.h"
#include <memory>
#include "resource.h"

#include <gdiplus.h>


class TextOutputSource : public IBaseVideo
{
	DYNIC_DECLARE()
    bool        bUpdateTexture;
	bool        bUpdateSubTitleTexture;
    String      strCurrentText;
    Texture     *texture;
    float       scrollValue;
    float       showExtentTime;

    int         mode;
    String      strText;
    String      strFile;

    String      strFont;
    int         size;
    DWORD       color;
    UINT        opacity;
    UINT        globalOpacity;
    int         scrollSpeed;
    bool        bBold, bItalic, bUnderline, bVertical;

    UINT        backgroundOpacity;
    DWORD       backgroundColor;

    bool        bUseOutline;
    float       outlineSize;
    DWORD       outlineColor;
    UINT        outlineOpacity;

    bool        bUseExtents;
	bool        bOldUseExtents;
    UINT        extentWidth, extentHeight;

    bool        bWrap;
    bool        bScrollMode;
    int         align;

	bool        bStop;
	bool        bUseSubTitle;
	bool        bRepeat;
	bool        bUpDown;
	int         iRepeatCount;
	int         iOldRepeatCount;
	int         iRealRepeatCount;
	bool        bCanStop;
	bool        bIsRepeat;
	bool        bHasAccess;
	bool        bChanged;

    Vect2       baseSize;
	Vect2       OldbaseSize;
	Vect2       OldSize;
	Vect2       OldNSize;
	Vect2       OldBSize;
	Vect2       OldextentVal;
	Vect2       RenderSize;
	Vect2       UseExtentSize;
	Vect2       OldMouseRenderPos;
    SIZE        textureSize;
	SIZE        OldtextureSize;
    bool        bUsePointFiltering;
	bool        bHasUpDataTexture;
	int         OldLen;

    bool        bMonitoringFileChanges;
    OSFileChangeData *fileChangeMonitor;

    std::unique_ptr<SamplerState> sampler;

    bool        bDoUpdate;

    SamplerState *ss;

    Value    data;
	Value    *Realdata;

	Shader *solidVertexShader;
	Shader *solidPixelShader;

	bool  bHasPreprocess;

	D3DAPI *D3DRender;

    void DrawOutlineText(Gdiplus::Graphics *graphics,
                         Gdiplus::Font &font,
                         const Gdiplus::GraphicsPath &path,
                         const Gdiplus::StringFormat &format,
                         const Gdiplus::Brush *brush);

    HFONT GetFont();

    void UpdateCurrentText();

    void SetStringFormat(Gdiplus::StringFormat &format);

    float ProcessScrollMode(Gdiplus::Graphics *graphics, Gdiplus::Font *font, Gdiplus::RectF &layoutBox, Gdiplus::StringFormat *format);

    void UpdateTexture();

	void DrawText(const Vect2 &pos, const SIZE&size);

public:
    TextOutputSource();

    ~TextOutputSource();

	bool Init(Value &JsonParam);

    void Preprocess();

    void Tick(float fSeconds);

	void Render(const Vect2 &pos, const Vect2 &size, Texture* tex, bool bScaleFull, bool bIsLiveC);

    Vect2 GetSize() const;

    void UpdateSettings(Value &data);

    void SetString(CTSTR lpName, CTSTR lpVal);

    void SetInt(CTSTR lpName, int iValue);

    void SetFloat(CTSTR lpName, float fValue);

	bool GetChanged() const;

	void SetChanged(bool bChange);
    inline void ResetExtentRect() {showExtentTime = 0.0f;}

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
};

struct ConfigTextSourceInfo
{
    CTSTR lpName;
	Value &data;
    float cx, cy;

    StringList fontNames;
    StringList fontFaces;
	ConfigTextSourceInfo(Value &Data) :data(Data){}
};

#endif
