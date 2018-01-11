#ifndef DEINTERLACER_H
#define DEINTERLACER_H
#include "BaseAfx.h"

class API_EXPORT CDeinterlacer
{
public:
	CDeinterlacer(int type);
	~CDeinterlacer();

	void SetImage(LPVOID data, const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType colorType);

	//只能使用GPU且不需要前一帧的情况
	void SetImage(Texture *texture, const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType colorType);

	void RenderTexture(const Vect2 &pos, const Vect2 &size);

	Texture *GetRenderTexture() const;
	struct {
		int                         type; //DeinterlacingType
		char                        fieldOrder; //DeinterlacingFieldOrder
		char                        processor; //DeinterlacingProcessor
		bool                        curField, bNewFrame;
		bool                        doublesFramerate;
		bool                        needsPreviousFrame;
		bool                        isReady;
		std::unique_ptr<Texture>    texture;
		UINT                        imageCX, imageCY;
		std::unique_ptr<Shader>     vertexShader;
		FuturePixelShader           pixelShaderAsync;
		std::unique_ptr<Shader>     pixelShader;
	} deinterlacer;
	
protected:
	void Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY);
	String ChooseDeinterlacingShader();
	String ChooseShader();
	void ChangeSize(const DeinterlacerConfig* Config, UINT renderCX, UINT renderCY, DeviceColorType ColorType);

private:
	
	UINT RenderCX;
	UINT RenderCY;
	UINT linePitch, lineSize;
	DeviceColorType ColorType;
	Texture *previousTexture;
	Texture *RendTexture;
	Shader  *colorConvertShader;
	bool    bReadyToDraw;
	UINT    imageCX, imageCY;
};

#endif