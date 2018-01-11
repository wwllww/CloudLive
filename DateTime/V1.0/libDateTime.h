#ifndef LIBDATETIME_H
#define LIBDATETIME_H

#include "BaseAfx.h"
#include ".\include\DateTimeProcess.h"
#include <Unknwn.h>
#include <gdiplus.h>
#include "BaseVideo.h"
using namespace Gdiplus;
inline DWORD GetAlphaVal(UINT opacityLevel)
{
	return ((opacityLevel * 255 / 100) & 0xFF) << 24;
}

#define ClampVal(val, minVal, maxVal) \
if (val < minVal) val = minVal; \
	else if (val > maxVal) val = maxVal;

class ProcessDateTime : public IBaseVideo
{
	DYNIC_DECLARE()
	CDateTime   m_DateTime;
	bool        bUpdateTexture;

	String      strCurrentText;
	Texture     *texture;
	float       scrollValue;
	float       showExtentTime;

	int         mode;
	String      strFormatString;

	String      strFont;
	int         size;
	DWORD       color;
	UINT        opacity;
	UINT        globalOpacity;
	int         scrollSpeed;
	bool        bBold, bItalic, bUnderline, bVertical;

 	UINT        backgroundOpacity;
// 	DWORD       backgroundColor;

	bool        bUseOutline;
	float       outlineSize;
	DWORD       outlineColor;
 	UINT        outlineOpacity;
// 
// 	bool        bUseExtents;
// 	UINT        extentWidth, extentHeight;

	bool        bWrap;
	bool        bScrollMode;
	int         align;

	Vect2       baseSize;
	Vect2       Resolution;
	SIZE        textureSize;

	bool        bDoUpdate;

	SamplerState *ss;

	Value    data;
	bool     bHasPre;

	HFONT    hFont;
	void DrawOutlineText(Graphics *graphics,
		Font &font,
		const GraphicsPath &path,
		const StringFormat &format,
		const Brush *brush);
	HFONT GetFont();
	void UpdateCurrentText();
	void SetStringFormat(StringFormat &format);
	float ProcessScrollMode(Graphics *graphics, 
							Font *font, 
							RectF &layoutBox, 
							StringFormat *format);
	void UpdateTexture();

protected:

public:
	ProcessDateTime();
	~ProcessDateTime();
	virtual bool Init(Value &JsonParam){ UpdateSettings(JsonParam);  return true; }
	void Preprocess();
	void Render(const Vect2 &pos, const Vect2 &size, Texture *FilterTexture = NULL, bool bScaleFull = true, bool bIsLiveC = false);
	Vect2 GetSize() const;
	void UpdateSettings(Value &data);
	void SetString(CTSTR lpName, CTSTR lpVal);
	void SetInt(CTSTR lpName, int iValue);
	void SetFloat(CTSTR lpName, float fValue);
	inline void ResetExtentRect() { showExtentTime = 0.0f; }
	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
};
struct ConfigDateTimeSourceInfo
{
	CTSTR lpName;
	Value &data;
	float cx, cy;
	ConfigDateTimeSourceInfo(Value &Data) :data(Data){}
	StringList fontNames;
	StringList fontFaces;
};
#endif // !LIBDATETIME_H
