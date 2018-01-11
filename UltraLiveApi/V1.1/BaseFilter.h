#ifndef BASEFILTER_H
#define BASEFILTER_H
#include "BaseAfx.h"
#include "BaseVideo.h"


class API_EXPORT IIBaseFilter : public CPObject
{
public:
	IIBaseFilter(){}
	virtual ~IIBaseFilter(){}

public:
	virtual bool InitFilter(UINT Width, UINT Height){ return false; }
	virtual void GetDefaults(Value &JsonDefaults){}
	virtual void UpDataSetting(Value &JsonDefaults){}
	virtual Texture *GetRenderTarget() { return NULL; }
	virtual void FilterRender(Texture *Target,const Vect2 &NewSize) = 0;

	virtual const char* GainClassName()
	{
		return "IBaseFilter";
	}
};

#endif // !BASEFILTER_H
