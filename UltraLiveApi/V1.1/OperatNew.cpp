#include "BaseAfx.h"
#include "OperatNew.h"


///*__declspec(selectany) */MemStack mem_stack;
CRITICAL_SECTION MemStack::Section;

void* operator new(size_t size, const char* file, unsigned int line){
	void* ptr = malloc(size);
	if (CSLiveManager::mem_stack)
		CSLiveManager::mem_stack->Insert(ptr, file, line);
	return ptr;
}

void* operator new[](size_t size, const char* file, unsigned int line) {
	return operator new(size, file, line);
}

void operator delete(void* ptr) {
	if (ptr)
	{
		if (CSLiveManager::mem_stack->bDelete && CSLiveManager::mem_stack)
		{
			free(ptr);
			CSLiveManager::mem_stack->Delete(ptr);
		}
		else
		{
			free(ptr);
		}
	}
}

void operator delete[](void* ptr) {
	if (ptr)
		operator delete(ptr);
}

MemStack::MemStack() :head(NULL)
{
	bDelete = true;
	InitializeCriticalSection(&Section);
}

MemStack::~MemStack()
{
	Print();
	MemInfo* tmp;
	while (head != NULL) {
		free(head->ptr); // �ͷ�й©���ڴ� 
		tmp = head->link;
		free(head);
		head = tmp;
	}
	DeleteCriticalSection(&Section);
}

void MemStack::Insert(void* ptr, const char* file, unsigned int line)
{
	EnterCriticalSection(&Section);
	MemInfo* node = (MemInfo*)malloc(sizeof(MemInfo));
	node->ptr = ptr; node->file = file; node->line = line;
	node->link = head; head = node;
	LeaveCriticalSection(&Section);
}

void MemStack::Delete(void* ptr)
{
	EnterCriticalSection(&Section);
	MemInfo* node = head;
	MemInfo* pre = NULL;
	while (node != NULL && node->ptr != ptr) {
		pre = node;
		node = node->link;
	}
	if (node == NULL)
	{
		//OutputDebugString(L"ɾ��һ�������ڱ��������¿��ٵ��ڴ�\n");
	}
	else {
		if (pre == NULL) // ɾ������head 
			head = node->link;
		else
			pre->link = node->link;
		free(node);
	}
	LeaveCriticalSection(&Section);
}

void MemStack::Print()
{
	if (head == NULL) {
		OutputDebugString(L"�ڴ涼�ͷŵ���\n");
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:�ڴ涼�ͷŵ���");
		return;
	}
	OutputDebugString(L"���ڴ�й¶����\n");
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:���ڴ�й¶����");
	EnterCriticalSection(&Section);
	MemInfo* node = head;
	int iCount = 0;
	while (node != NULL) {
		char MSG[4096] = { 0 };
		sprintf_s(MSG, "LiveSDK_Log:�ļ�����%s,������%d,��ַ��%p\n", node->file, node->line, node->ptr);
		OutputDebugStringA(MSG);
		Log::writeMessage(LOG_RTSPSERV, 1, MSG);
		node = node->link;
		++iCount;
	}
	LeaveCriticalSection(&Section);

	char Msg[128] = { 0 };
	sprintf_s(Msg, "LiveSDK_Log:���� %d ��й¶\n", iCount);
	OutputDebugStringA(Msg);
	Log::writeMessage(LOG_RTSPSERV, 1, Msg);
}


