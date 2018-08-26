// CloudLive.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include ".\CloudLive\stdafx.h"
#include <windows.h>
#include <iostream>
#include <io.h>
#include <DbgHelp.h>

#include "BaseAfx.h"
#include "CHttpNetWork.h"

//��������̨
void OpenConsole()
{
	AllocConsole();
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);      //��ȡ����̨������  
	INT hCrt = _open_osfhandle((INT)hCon, 0x4000);     //ת��ΪC�ļ�������  
	FILE * hf = _fdopen(hCrt, "w");           //ת��ΪC�ļ���  
	setvbuf(hf, NULL, _IONBF, 0);              //�޻���  
	*stdout = *hf;                      //�ض����׼���  


	hCon = GetStdHandle(STD_INPUT_HANDLE);      //��ȡ����̨������  
	hCrt = _open_osfhandle((INT)hCon, 0x4000);     //ת��ΪC�ļ�������  
	hf = _fdopen(hCrt, "r");           //ת��ΪC�ļ���  
	setvbuf(hf, NULL, _IONBF, 0);              //�޻���  
	*stdin = *hf;
}

BOOL WINAPI ConsoleHandler(DWORD msgType)
{
	if (msgType == CTRL_CLOSE_EVENT)
	{
		std::cout << "Close console window!" << std::endl;
		CHttpNetWork::GetInstance()->SetExit(true);
		CHttpNetWork::GetInstance()->UnInitLive();
		CHttpNetWork::HttpNetWorkRelese();
		FreeConsole();
		return TRUE;
	}

	/*
	Other messages:
	CTRL_BREAK_EVENT         Ctrl-Break pressed
	CTRL_LOGOFF_EVENT        User log off
	CTRL_SHUTDOWN_EVENT      System shutdown
	CTRL_C_EVENT
	*/

	return FALSE;
}

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	OpenConsole();
	Log::open(true, "-dGMfyds");

	std::cout << "��ǰ����ID " << GetCurrentProcessId() << std::endl;

	SetConsoleCtrlHandler(ConsoleHandler, TRUE);

	TCHAR FullName[1024] = { 0 };
	int RetLen = GetModuleFileName(NULL, FullName, sizeof FullName);

	for (int i = RetLen - 1; i >= 0; i--)
	{
		if (FullName[i] != '\\')
		{
			FullName[i] = 0;
		}
		else
		{
			break;
		}
	}

	SetCurrentDirectory(FullName);

	FunCall(CHttpNetWork::GetInstance()->InitLive());

	return CHttpNetWork::GetInstance()->HttpMsgLoop();
}