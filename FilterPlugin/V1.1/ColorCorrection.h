#ifndef COLOR_CORRECTION_H
#define COLOR_CORRECTION_H
#include "BaseAfx.h"
#include "BaseFilter.h"

class FilterColorCorrection : public IIBaseFilter
{
	DYNIC_DECLARE();
public:
	FilterColorCorrection();
	~FilterColorCorrection();
	virtual bool InitFilter(UINT Width, UINT Height);
	virtual void GetDefaults(Value &JsonDefaults);
	virtual void UpDataSetting(Value &JsonDefaults);
	virtual Texture *GetRenderTarget();
	virtual void FilterRender(Texture *Target, const Vect2 &NewSize);
private:
	Texture *RenderTarget;
	Shader  *PixShader;
	HANDLE HHue;
	HANDLE HSatu;
	HANDLE HLight;

	float width;
	float hight;
	float Hue;// 色调-180~180
	float Saturation;// 饱和度-1~5
	float Lightness;// 亮度-1~1
	D3DAPI *D3DRender;
};

#endif