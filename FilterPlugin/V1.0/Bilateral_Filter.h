#ifndef BILATERAL_H
#define BILATERAL_H
#include "BaseAfx.h"
#include "BaseFilter.h"

class FilterBilateral : public IIBaseFilter
{
	DYNIC_DECLARE();
public:
	FilterBilateral();
	~FilterBilateral();
	virtual bool InitFilter(UINT Width, UINT Height);
	virtual void GetDefaults(Value &JsonDefaults);
	virtual void UpDataSetting(Value &JsonDefaults);
	virtual Texture *GetRenderTarget();
	virtual void FilterRender(Texture *Target, const Vect2 &NewSize);
private:
	Texture *RenderTarget;
	Shader  *PixShader;
	Shader  *VertexShader;
	IBaseVideo *BaseVideo;

	float width;
	float hight;
};

#endif