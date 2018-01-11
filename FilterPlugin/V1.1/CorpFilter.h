#ifndef CORPFILTER_H
#define CORPFILTER_H
#include "BaseAfx.h"
#include "BaseFilter.h"

class FilterCorp : public IIBaseFilter
{
	DYNIC_DECLARE();
public:
	FilterCorp();
	~FilterCorp();
	virtual bool InitFilter(UINT Width, UINT Height);
	virtual void GetDefaults(Value &JsonDefaults);
	virtual void UpDataSetting(Value &JsonDefaults);
	virtual Texture *GetRenderTarget();
	virtual void FilterRender(Texture *Target, const Vect2 &NewSize);
private:
	Texture *RenderTarget;
	Shader  *PixShader;
	Shader  *VertexShader;

	Vect2 Mul_val;
	Vect2 Add_val;
	float width;
	float hight;
	HANDLE HMul = NULL;
	HANDLE HAdd = NULL;

	D3DAPI *D3DRender;
};

#endif