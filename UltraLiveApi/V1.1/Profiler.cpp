//#include "XT.h"
#include "Profiler.h"

inline double MicroToMS(DWORD microseconds)
{
    return double(microseconds)*0.001;
}

float minPercentage, minTime;
HANDLE hProfilerMutex = NULL;


struct API_EXPORT ProfileNodeInfo
{
    ~ProfileNodeInfo()
    {
        FreeData();
    }

    void FreeData()
    {
        for(UINT i = 0; i < Children.Num(); i++)
            Children[i].FreeData();
        Children.Clear();
    }

    CTSTR lpName;

    DWORD numCalls;
    DWORD numParallelCalls;
    DWORD avgTimeElapsed;
    DWORD avgCpuTime;
    double avgPercentage;
    double childPercentage;
    double unaccountedPercentage;

    bool bSingular;

    QWORD totalTimeElapsed,
          lastTimeElapsed,
          cpuTimeElapsed,
          lastCpuTimeElapsed;

    DWORD lastCall;

    ProfileNodeInfo *parent;
    List<ProfileNodeInfo> Children;

    void calculateProfileData(int rootCallCount)
    {
        avgTimeElapsed = (DWORD)(totalTimeElapsed/(QWORD)numCalls);
        avgCpuTime = (DWORD)(cpuTimeElapsed/(QWORD)numCalls);


        if(parent)  avgPercentage = (double(avgTimeElapsed)/double(parent->avgTimeElapsed))*parent->avgPercentage;
        else        avgPercentage = 100.0f;

        childPercentage = 0.0;

        if(Children.Num())
        {
            for(unsigned int i=0; i<Children.Num(); i++)
            {
                Children[i].parent = this;
                Children[i].calculateProfileData(rootCallCount);

                if(!Children[i].bSingular)
                    childPercentage += Children[i].avgPercentage;
            }

            unaccountedPercentage = avgPercentage-childPercentage;
        }
    }

    void dumpData(int rootCallCount, int indent=0)
    {
        if(indent == 0)
        {
            rootCallCount = (int)floor(rootCallCount/(double)numParallelCalls+0.5);
            calculateProfileData(rootCallCount);
        }

        String indentStr;
        for(int i=0; i<indent; i++)
            indentStr << TEXT("| ");

        CTSTR lpIndent = indent == 0 ? TEXT("") : indentStr.Array();

        int perFrameCalls = (int)floor(numCalls/(double)rootCallCount+0.5);

        float fTimeTaken = (float)MicroToMS(avgTimeElapsed);
        float cpuTime = (float)MicroToMS(avgCpuTime);
        float totalCpuTime = (float)cpuTimeElapsed*0.001f;

        if(avgPercentage >= minPercentage && fTimeTaken >= minTime)
        {
            if(Children.Num())
                Log::writeMessage(LOG_RTSPSERV,1,"%s%s - [%.3g%%] [avg time: %g ms] [children: %.3g%%] [unaccounted: %.3g%%]", lpIndent, lpName, avgPercentage, fTimeTaken, childPercentage, unaccountedPercentage);
            else
				Log::writeMessage(LOG_RTSPSERV, 1, "%s%s - [%.3g%%] [avg time: %g ms]", lpIndent, lpName, avgPercentage, fTimeTaken);
        }

        for(unsigned int i=0; i<Children.Num(); i++)
            Children[i].dumpData(rootCallCount, indent+1);
    }

    void dumpCPUData(int rootCallCount, int indent=0)
    {
        if(indent == 0)
        {
            rootCallCount = (int)floor(rootCallCount/(double)numParallelCalls+0.5);
            calculateProfileData(rootCallCount);
        }

        String indentStr;
        for(int i=0; i<indent; i++)
            indentStr << TEXT("| ");

        CTSTR lpIndent = indent == 0 ? TEXT("") : indentStr.Array();

        int perFrameCalls = (int)floor(numCalls/(double)rootCallCount+0.5);

        float fTimeTaken = (float)MicroToMS(avgTimeElapsed);
        float cpuTime = (float)MicroToMS(avgCpuTime);
        float totalCpuTime = (float)cpuTimeElapsed*0.001f;

        if(avgPercentage >= minPercentage && fTimeTaken >= minTime)
        {
            if(Children.Num())
				Log::writeMessage(LOG_RTSPSERV, 1, "%s%s - [cpu time: avg %g ms, total %g ms] [avg calls per frame: %d]", lpIndent, lpName, cpuTime, totalCpuTime, perFrameCalls);
            else
				Log::writeMessage(LOG_RTSPSERV, 1, "%s%s - [cpu time: avg %g ms, total %g ms] [avg calls per frame: %d]", lpIndent, lpName, cpuTime, totalCpuTime, perFrameCalls);
        }

        for(unsigned int i=0; i<Children.Num(); i++)
            Children[i].dumpCPUData(rootCallCount, indent+1);
    }

    void dumpLastData(int callNum, int indent=0)
    {
        if(lastCall != callNum)
            return;

        String indentStr;
        for(int i=0; i<indent; i++)
            indentStr << TEXT("| ");

        CTSTR lpIndent = indent == 0 ? TEXT("") : indentStr.Array();

		Log::writeMessage(LOG_RTSPSERV, 1, "%s%s - [time: %g ms (cpu time: %g ms)]", lpIndent, lpName, MicroToMS((DWORD)lastTimeElapsed), MicroToMS((DWORD)lastCpuTimeElapsed));

        for(unsigned int i=0; i<Children.Num(); i++)
            Children[i].dumpLastData(callNum, indent+1);
    }

    ProfileNodeInfo* FindSubProfile(CTSTR lpName)
    {
        for(unsigned int i=0; i<Children.Num(); i++)
        {
            if(Children[i].lpName == lpName)
                return &Children[i];
        }

        return NULL;
    }

    static ProfileNodeInfo* FindProfile(CTSTR lpName)
    {
        for(unsigned int i=0; i<profilerData.Num(); i++)
        {
            if(profilerData[i].lpName == lpName)
                return profilerData+i;
        }

        return NULL;
    }

    static void MergeProfileInfo(ProfileNodeInfo &info)
    {
        OSEnterMutex(hProfilerMutex);
        ProfileNodeInfo *sum = FindProfile(info.lpName);
        if(!sum)
        {
            sum = profilerData.CreateNew();
            sum->lpName = info.lpName;
        }
        sum->MergeProfileInfo(&info, info.lastCall, sum->lastCall + info.lastCall);
        OSLeaveMutex(hProfilerMutex);
    }

    void MergeProfileInfo(ProfileNodeInfo *info, DWORD rootLastCall, DWORD updatedLastCall)
    {
        bSingular = info->bSingular;
        numCalls += info->numCalls;
        if(lastCall == rootLastCall) lastCall = updatedLastCall;
        totalTimeElapsed += info->totalTimeElapsed;
        lastTimeElapsed = info->lastTimeElapsed;
        cpuTimeElapsed += info->cpuTimeElapsed;
        lastCpuTimeElapsed = info->lastCpuTimeElapsed;
        numParallelCalls = info->numParallelCalls;
        for(UINT i = 0; i < info->Children.Num(); i++)
        {
            ProfileNodeInfo &child = info->Children[i];
            ProfileNodeInfo *sumChild = FindSubProfile(child.lpName);
            if(!sumChild)
            {
                sumChild = Children.CreateNew();
                sumChild->lpName = child.lpName;
            }
            sumChild->MergeProfileInfo(&child, rootLastCall, updatedLastCall);
        }
    }

    static List<ProfileNodeInfo> profilerData;
};


static __declspec(thread) ProfilerNode *__curProfilerNode = NULL;
BOOL bProfilingEnabled = FALSE;
List<ProfileNodeInfo> ProfileNodeInfo::profilerData;


void STDCALL EnableProfiling(BOOL bEnable, float pminPercentage, float pminTime)
{
    //if(engine && !engine->InEditor())
    bProfilingEnabled = bEnable;

    minPercentage = pminPercentage;
    minTime = pminTime;
}

void STDCALL DumpProfileData()
{
    if(ProfileNodeInfo::profilerData.Num())
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "\r\nProfiler time results:\r\n");
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================");
        for(unsigned int i=0; i<ProfileNodeInfo::profilerData.Num(); i++)
            ProfileNodeInfo::profilerData[i].dumpData(ProfileNodeInfo::profilerData[i].numCalls);
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================\r\n");
		Log::writeMessage(LOG_RTSPSERV, 1, "\r\nProfiler CPU results:\r\n");
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================");
        for(unsigned int i=0; i<ProfileNodeInfo::profilerData.Num(); i++)
            ProfileNodeInfo::profilerData[i].dumpCPUData(ProfileNodeInfo::profilerData[i].numCalls);
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================\r\n");
    }
}

void STDCALL DumpLastProfileData()
{
    if(ProfileNodeInfo::profilerData.Num())
    {
		Log::writeMessage(LOG_RTSPSERV, 1, "\r\nProfiler result for the last frame:");
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================");
        for(unsigned int i=0; i<ProfileNodeInfo::profilerData.Num(); i++)
            ProfileNodeInfo::profilerData[i].dumpLastData(ProfileNodeInfo::profilerData[i].lastCall);
		Log::writeMessage(LOG_RTSPSERV, 1, "==============================================================\r\n");
    }
}

void STDCALL FreeProfileData()
{
    for(unsigned int i=0; i<ProfileNodeInfo::profilerData.Num(); i++)
        ProfileNodeInfo::profilerData[i].FreeData();
    ProfileNodeInfo::profilerData.Clear();
}

ProfilerNode::ProfilerNode(CTSTR lpName, bool bSingularize) : lpName(nullptr), parent(nullptr), info(nullptr)
{
    parent = __curProfilerNode;

    if(bSingularNode = bSingularize)
    {
        if(!parent)
            return;

        while(parent->parent != NULL)
            parent = parent->parent;
    }
    else
        __curProfilerNode = this;

    if(parent)
    {
        if(!parent->lpName) return; //profiling was disabled when parent was created, so exit to avoid inconsistent results
        ProfileNodeInfo *parentInfo = parent->info;
        if(parentInfo)
        {
            info = parentInfo->FindSubProfile(lpName);
            if(!info)
            {
                info = parentInfo->Children.CreateNew();
                info->lpName = lpName;
                info->bSingular = bSingularNode;
            }
        }
    }
    else if(bProfilingEnabled)
    {
        info = new ProfileNodeInfo;
        info->lpName = lpName;
    }
    else
        return;

    if (info)
    {
        ++info->numCalls;
        if(!parent)
            info->lastCall = info->numCalls;
        else
            info->lastCall = parent->info->numCalls;
    }

    this->lpName = lpName;

    startTime = OSGetTimeMicroseconds();

    MonitorThread(OSGetCurrentThread());

    parallelCalls = 1;
}

ProfilerNode::~ProfilerNode()
{
    //profiling was diabled when created
    if(lpName)
    {
        QWORD newTime = OSGetTimeMicroseconds();

        DWORD curTime = (DWORD)(newTime-startTime);
        info->totalTimeElapsed += curTime;
        info->lastTimeElapsed = curTime;
        if(thread)
        {
            DWORD cpuTime = DWORD(OSGetThreadTime(thread) - cpuStartTime);
            info->cpuTimeElapsed += cpuTime;
            info->lastCpuTimeElapsed = cpuTime;
        }
        info->numParallelCalls = parallelCalls;
    }

    if(!bSingularNode)
        __curProfilerNode = parent;

    if(!parent && info)
    {
        ProfileNodeInfo::MergeProfileInfo(*info);
        delete info;
    }
}

void ProfilerNode::MonitorThread(HANDLE thread_)
{
    if(!thread_)
        return;
    thread = thread_;
    cpuStartTime = OSGetThreadTime(thread);
}

void ProfilerNode::SetParallelCallCount(DWORD num)
{
    parallelCalls = num;
}
