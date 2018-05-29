#ifndef BITMAPTRANSITIONSOURCE_H
#define BITMAPTRANSITIONSOURCE_H
#include "BaseAfx.h"
#include "BaseVideo.h"

#define MIN_TRANSITION_TIME 1
#define MAX_TRANSITION_TIME 600

const float fadeTime = 1.5f;

extern "C" double round(double val);

class BitmapImage;

class BitmapTransitionSource : public IBaseVideo
{
	DYNIC_DECLARE()
private:
	List<BitmapImage*> bitmapImages;

	Vect2    fullSize;
	double   baseAspect;

	Value data;

	float transitionTime;

	UINT  curTexture;
	UINT  nextTexture;

	float curTransitionTime;
	float curFadeValue;
	bool  bTransitioning;

	bool  bFadeInOnly;
	bool  bDisableFading;
	bool  bRandomize;
	bool  bHasPreProcess;
	bool  bUpdate;
	D3DAPI *D3DRender;
	inline int lrand(int limit)
	{
		// return a random number in the interval [0 , limit)
		return int(((double)rand() / (RAND_MAX + 1)) * limit);
	}
public:
	BitmapTransitionSource();

	~BitmapTransitionSource();

	virtual bool Init(Value &JsonParam);

	virtual void Preprocess();

	void Tick(float fSeconds);

	void DrawBitmap(UINT texID, float alpha, const Vect2 &startPos, const Vect2 &startSize, bool bScaleFull, Texture *FilterTexture);

	void Render(const Vect2 &pos, const Vect2& size, Texture *FilterTexture = NULL, bool bScaleFull = true, bool bIsLiveC = false);

	void UpdateSettings(Value &JsonParam);

	void Update();

	Vect2 GetSize() const;

	virtual void SetHasPreProcess(bool bHasPre);
	virtual bool GetHasPreProcess() const;
};

#endif