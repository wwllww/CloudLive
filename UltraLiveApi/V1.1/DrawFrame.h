#ifndef DRAWFRAME_H
#define DRAWFRAME_H
#include "BaseAfx.h"

class Title
{
public:
	static Title* Instance();
	void LoadTitlePis();
	std::vector<TitlePicInfo> ListTitle;
	void DrawFrame(HWND hwnd,int Type = -1, bool IsOnlyColse = true);

private:
	Title(){}
	~Title();

	static Title* m_title;
};
#endif