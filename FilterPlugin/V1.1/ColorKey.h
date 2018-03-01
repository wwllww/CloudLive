#ifndef COLORKEY_H
#define COLORKEY_H

#include "BaseAfx.h"
#include "BaseFilter.h"

class FilterColorKey : public IIBaseFilter
{
	DYNIC_DECLARE();
public:
	FilterColorKey();
	~FilterColorKey();
	virtual bool InitFilter(UINT Width, UINT Height);
	virtual void GetDefaults(Value &JsonDefaults);
	virtual void UpDataSetting(Value &JsonDefaults);
	virtual Texture *GetRenderTarget();
	virtual void FilterRender(Texture *Target, const Vect2 &NewSize);
private:
	Texture *RenderTarget;
	Shader  *PixShader;
	HANDLE HKeyColor; //关键颜色值
	HANDLE HSimilarity; //相似度
	HANDLE HSmoothness; //光华度

	float width;
	float hight;
	DWORD KeyColor;
	float Similarity; //(0,1]
	float Smoothness; //(0,1]
	D3DAPI *D3DRender;
};

#endif