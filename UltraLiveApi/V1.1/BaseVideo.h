#ifndef BASEVIDEO_H
#define BASEVIDEO_H
#include "BaseAfx.h"

class IBaseAudio;
class IBaseVideo : public CPObject
{
public:
	IBaseVideo(){ m_bEnterScene = true; }
	virtual ~IBaseVideo(){}
	virtual bool Init(Value &JsonParam){ return false; }
	virtual void Preprocess(){}
	virtual void Render(const Vect2 &pos, const Vect2& size, Texture *FilterTexture = NULL, bool bScaleFull = true,bool bIsLiveC = false) = 0;
	virtual void Tick(float fSeconds) {}
	virtual void UpdateSettings(Value &JsonParam) {}
	virtual void BeginScene() {}
	virtual void EndScene() {}
	virtual void GlobalSourceLeaveScene() {}
	virtual void GlobalSourceEnterScene() {}
	virtual void RestSetGlobalSource() {}
	virtual Vect2 GetSize() const = 0;
	virtual const char* GetAduioClassName() const { return NULL; }
	virtual IBaseAudio * GetAudioRender(){ return NULL; }
	virtual bool GetStreamPos(UINT& Pos){ return false; }
	virtual bool SetStreamPos(UINT Pos){ return false; }
	virtual bool OperaterStream(DBOperation OperType){ return false; }
	virtual DBOperation GetStreamStatus() { return Play; }
	virtual bool GetStreamInfo(Value &JsonInfo){ return false; }
	virtual const char* GetDeviceName() const { return NULL; }
	virtual const char* GetDeviceID() const { return NULL; }
	virtual Texture *GetLastTextureFrame() const { return NULL; }

	virtual void SetHasPreProcess(bool bHasPre) = 0;
	virtual bool GetHasPreProcess() const = 0;

	virtual Texture *GetTexture(){ return NULL; }
	virtual IBaseVideo *GetGlobalSource() { return NULL; }

	//µã²¥ÌØ¼Ó
	virtual void SetHasSwitchSences(bool bSwitch) {}
	virtual void SwitchSences() {}
	virtual void SetCanEnterScene(bool bCanEnter){}
	virtual bool CanEnterScene() const { return false; }
	virtual void SetD3DRender(D3DAPI *D3DRender) { }


	virtual void RegisterDataCallBack(void *Context, DataCallBack pCb){}
	virtual void UnRegisterDataCallBack(void *Context){}

	virtual bool IsFieldSignal() const { return false; }
	virtual void RenameSource(const char *NewName) {}

	virtual void SetOpacity(DWORD Opacity) {}


	virtual void SetFloat(CTSTR lpName, float fValue) {}
	virtual void SetInt(CTSTR lpName, int iValue) {}
	virtual void SetString(CTSTR lpName, CTSTR lpVal) {}
	virtual void SetVector(CTSTR lpName, const Vect &value) {}
	virtual void SetVector2(CTSTR lpName, const Vect2 &value) {}
	virtual void SetVector4(CTSTR lpName, const Vect4 &value) {}

	virtual const char* GainClassName()
	{
		return "IBaseVideo";
	}
protected:
	List<__DataCallBack> m_ListCallBack;
	bool m_bEnterScene;
};

#endif