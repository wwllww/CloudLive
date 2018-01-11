#ifndef FILTERPLUGIN_H
#define FILTERPLUGIN_H
#include "BaseAfx.h"
#include "BaseFilter.h"

class FilterColor_Key : public IIBaseFilter
{
	DYNIC_DECLARE();
public:
	FilterColor_Key();
	~FilterColor_Key();
	virtual bool InitFilter(UINT Width, UINT Height);
	virtual void GetDefaults(Value &JsonDefaults);
	virtual void UpDataSetting(Value &JsonDefaults);
	virtual Texture *GetRenderTarget();
	virtual void FilterRender(Texture *Target, const Vect2 &NewSize);
private:
	Texture *RenderTarget;
	Shader  *PixShader;

	Vect4 Color;
	float Similarity;
	float Blend;
	float Gamma;
	float width;
	float hight;
	HANDLE HColorKey = NULL;
	HANDLE hSimilarity = NULL;
	HANDLE hBlend = NULL;
	HANDLE hGamma = NULL;
};



#endif // !FILTERPLUGIN_H
