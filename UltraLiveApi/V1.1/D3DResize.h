#ifndef D3DRESIZE_H
#define D3DRESIZE_H
#include "BaseAfx.h"

class API_EXPORT CD3DReszie
{
public:
	CD3DReszie(UINT AdpterID);
	~CD3DReszie();
	bool Init();
	unsigned char* Resize(unsigned char* SrcData,int SrcW, int SrcH, TColorType ColorFormat, int DesW, int DesH,bool bFieldSignal);
	void UnMap();
	D3DAPI *GetD3DRender() const;
	Shader *GetMainVertexShader() const;
	Shader *GetMainPixelShader() const;
	Texture *GetSDITexture() const;
protected:
	void  ChangeSize(int SrcW, int SrcH, TColorType ColorFormat, int DesW, int DesH);
	void   ChangeShader();
	String ChooseShader(bool bNeedField = true);
	unsigned char* ResizeRender(unsigned char* SrcData);
	void Convert422To444(LPBYTE convertBuffer, LPBYTE lp422, UINT pitch, bool bLeadingY);
private:
	D3DAPI *D3DRender;
	int m_SrcW;
	int m_SrcH;
	TColorType m_ColorForamt;
	int m_DesW;
	int m_DesH;
	bool m_bFieldSignal;
	String strShaderOld;
	unsigned char* CopyData;

	Shader *mainVertexShader;
	Shader *mainPixelShader;
	Shader *colorConvertShader;
	Shader *colorFieldConvertShader;
	Shader *RGBFieldShader;

	Texture *DesTexture;
	Texture *SrcTexture;
	Texture *copyTextures;
	Texture *SDILittleTexture;

};


#endif