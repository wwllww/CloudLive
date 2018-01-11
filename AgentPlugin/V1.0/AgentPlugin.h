#include "resource.h"
#include "BaseAfx.h"
#include "BitmapImage.h"
#include "BaseVideo.h"

class AgentSource : public IBaseVideo
{
	DYNIC_DECLARE()
	IBaseVideo *globalSource;
	BitmapImage Image;
	Value data;
	bool bPreProcess;
	int enteredSceneCount = 0;

	HANDLE HLock;
	bool bCanEnter;
	bool bRunning;

public:
	AgentSource();
	bool Init(Value &data);

	void Preprocess() { if (globalSource) globalSource->Preprocess(); }
	void Tick(float fSeconds) { if (globalSource) globalSource->Tick(fSeconds); }

	IBaseVideo *GetGlobalSource() { return globalSource; }
	void Render(const Vect2 &pos, const Vect2 &size, Texture *texture, bool bScaleFull, bool bIsLiveC);
	Vect2 GetSize() const;
	Value& GetData(){ return data; }
	Texture *GetTexture();
	void UpdateSettings(Value &data);

	virtual void SetCanEnterScene(bool bCanEnter){ this->bCanEnter = bCanEnter; }
	virtual bool CanEnterScene() const { return bCanEnter; }

	virtual void GlobalSourceEnterScene();
	virtual void GlobalSourceLeaveScene();

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
	virtual bool IsFieldSignal() const;
	//-------------------------------------------------------------

	virtual void SetFloat(CTSTR lpName, float fValue)         { if (globalSource) globalSource->SetFloat(lpName, fValue); }
	virtual void SetInt(CTSTR lpName, int iValue)             { if (globalSource) globalSource->SetInt(lpName, iValue); }
	virtual void SetString(CTSTR lpName, CTSTR lpVal)         { if (globalSource) globalSource->SetString(lpName, lpVal); }
	virtual void SetVector(CTSTR lpName, const Vect &value)   { if (globalSource) globalSource->SetVector(lpName, value); }
	virtual void SetVector2(CTSTR lpName, const Vect2 &value) { if (globalSource) globalSource->SetVector2(lpName, value); }
	virtual void SetVector4(CTSTR lpName, const Vect4 &value) { if (globalSource) globalSource->SetVector4(lpName, value); }


	~AgentSource();

	void RestSetGlobalSource();
	static List<AgentSource*> AgentList;
};