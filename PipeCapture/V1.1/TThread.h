#pragma once
#include "windows.h"

template<typename Class> class TThread //简单的多线程实现
{
public:
	typedef void(Class::*MemberFun)();

	TThread(Class* owner, MemberFun fun, bool Suspend = false) : Owner(owner), m_fun(fun), m_closed(false)
	{
		m_hThread = ::CreateThread(NULL, 0, Agent, this, Suspend ? CREATE_SUSPENDED : 0, &m_threadID);
		//assert(m_hThread != NULL);
	}

	~TThread()
	{
		if (m_closed == false)
		{
			Close();
		}
	}

	DWORD GetThreadId()
	{
		return m_threadID;
	}

	HANDLE GetThreadHandle()
	{
		return m_hThread;
	}

	DWORD ResumeThread()//唤醒
	{
		return ::ResumeThread(m_hThread);
	}

	DWORD SuspendThread()//暂停
	{
		return ::SuspendThread(m_hThread);
	}

	bool TerminateThread(DWORD exitCode)
	{
		return ::TerminateThread(m_hThread, exitCode);
	}

	DWORD WaitForSingleObjectd(DWORD dwMilliseconds = INFINITE)
	{
		return ::WaitForSingleObject(m_hThread, dwMilliseconds);
	}

	BOOL Close()
	{
		m_closed = true;
		return ::CloseHandle(m_hThread);
	}

private:
	static DWORD WINAPI Agent(PVOID param) //这是个不错的处理方式
	{
		TThread<Class>* thread = (TThread<Class>*)param;
		(thread->Owner->*(thread->m_fun))();

		return 0;
	}

private:
	DWORD m_threadID;
	HANDLE m_hThread;
	BOOL m_closed;

	Class *Owner;
	MemberFun m_fun;
};

class TThreadBase :public TThread<TThreadBase>
{
public:
	TThreadBase(bool Suspend = false) :TThread(this, &TThreadBase::Exec, Suspend){}
	virtual ~TThreadBase(){};

protected:
	virtual void Exec() = 0;
};

class TLock
{
public:
	TLock(){ InitializeCriticalSection(&m_cs); }
	void Lock(){ EnterCriticalSection(&m_cs); }
	void UnLock(){ LeaveCriticalSection(&m_cs); }

private:
	CRITICAL_SECTION m_cs;
};