#pragma once
#include "BaseAfx.h"
struct ProfileNodeInfo;

class API_EXPORT ProfilerNode
{
    CTSTR lpName;
	QWORD startTime;
	QWORD cpuStartTime;
    DWORD parallelCalls;
    HANDLE thread;
    ProfilerNode *parent;
    bool bSingularNode;
    ProfileNodeInfo *info;

public:
    ProfilerNode(CTSTR name, bool bSingularize=false);
    ~ProfilerNode();
    void MonitorThread(HANDLE thread);
    void SetParallelCallCount(DWORD num);
};

//BASE_EXPORT extern ProfilerNode *__curProfilerNode;
API_EXPORT extern BOOL bProfilingEnabled;

#define ENABLE_PROFILING 1

#ifdef ENABLE_PROFILING
    #define profileSingularSegment(name)                ProfilerNode _curProfiler(TEXT(name), true);
    #define profileSingularIn(name)                     {ProfilerNode _curProfiler(TEXT(name), true);
    #define profileSegment(name)                        ProfilerNode _curProfiler(TEXT(name));
    #define profileParallelSegment(name, plural, num)   ProfilerNode _curProfiler(num == 1 ? TEXT(name) : TEXT(plural)); _curProfiler.SetParallelCallCount(num);
    #define profileIn(name)                             {ProfilerNode _curProfiler(TEXT(name));
    #define profileOut                                  }
#else
    #define profileSingularSegment(name)
    #define profileSingularIn(name)
    #define profileSegment(name)
    #define profileParallelSegment(name, plural, num)
    #define profileIn(name)
    #define profileOut
#endif

API_EXPORT void STDCALL EnableProfiling(BOOL bEnable, float minPercentage = 0.0f, float minTime = 0.0f);
API_EXPORT void STDCALL DumpProfileData();
API_EXPORT void STDCALL DumpLastProfileData();
API_EXPORT void STDCALL FreeProfileData();
