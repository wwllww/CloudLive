#include "BaseAfx.h"
#include <thread>
#include <Psapi.h>
#include <DbgHelp.h>

#ifdef WIN32
#include <dshow.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>
#endif // WIN32

#include "SLiveApi.h"
#include "SLiveManager.h"
#include "Error.h"
#include "Instance.h"


#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


extern LARGE_INTEGER clockFre;
extern HINSTANCE hMain;
CInstanceProcess *G_LiveInstance = NULL;
HWND  G_MainHwnd = NULL;

typedef BOOL(WINAPI *ENUMERATELOADEDMODULES64) (HANDLE hProcess, PENUMLOADED_MODULES_CALLBACK64 EnumLoadedModulesCallback, PVOID UserContext);
typedef DWORD(WINAPI *SYMSETOPTIONS) (DWORD SymOptions);
typedef BOOL(WINAPI *SYMINITIALIZE) (HANDLE hProcess, PCTSTR UserSearchPath, BOOL fInvadeProcess);
typedef BOOL(WINAPI *SYMCLEANUP) (HANDLE hProcess);
typedef BOOL(WINAPI *STACKWALK64) (
	DWORD MachineType,
	HANDLE hProcess,
	HANDLE hThread,
	LPSTACKFRAME64 StackFrame,
	PVOID ContextRecord,
	PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine,
	PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine,
	PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine,
	PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress
	);

typedef PVOID(WINAPI *SYMFUNCTIONTABLEACCESS64) (HANDLE hProcess, DWORD64 AddrBase);
typedef DWORD64(WINAPI *SYMGETMODULEBASE64) (HANDLE hProcess, DWORD64 dwAddr);
typedef BOOL(WINAPI *SYMFROMADDR) (HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);
typedef BOOL(WINAPI *SYMGETMODULEINFO64) (HANDLE hProcess, DWORD64 dwAddr, PIMAGEHLP_MODULE64 ModuleInfo);

typedef DWORD64(WINAPI *SYMLOADMODULE64) (HANDLE hProcess, HANDLE hFile, PSTR ImageName, PSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll);

typedef BOOL(WINAPI *MINIDUMPWRITEDUMP) (
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);

typedef struct moduleinfo_s
{
	DWORD64 faultAddress;
	TCHAR   moduleName[MAX_PATH];
} moduleinfo_t;


int HasSSE2Support()
{
	int cpuInfo[4];

	__cpuid(cpuInfo, 1);

	return (cpuInfo[3] & (1 << 26)) != 0;
}

typedef struct MoudleStruct
{
	HMODULE HMoudle;
	std::wstring MoudleName;
}MoudleStruct;

BOOL LoadSeDebugPrivilege()
{
	DWORD   err;
	HANDLE  hToken;
	LUID    Val;
	TOKEN_PRIVILEGES tp;

	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	{
		err = GetLastError();
		return FALSE;
	}

	if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &Val))
	{
		err = GetLastError();
		CloseHandle(hToken);
		return FALSE;
	}

	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = Val;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof (tp), NULL, NULL))
	{
		err = GetLastError();
		CloseHandle(hToken);
		return FALSE;
	}

	CloseHandle(hToken);

	return TRUE;
}

BOOL CALLBACK EnumerateLoadedModulesProcInfo(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	moduleinfo_t *moduleInfo = (moduleinfo_t *)UserContext;
	if (moduleInfo->faultAddress >= ModuleBase && moduleInfo->faultAddress <= ModuleBase + ModuleSize)
	{
		scpy_n(moduleInfo->moduleName, ModuleName, _countof(moduleInfo->moduleName) - 1);
		return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK RecordAllLoadedModules(PCTSTR ModuleName, DWORD64 ModuleBase, ULONG ModuleSize, PVOID UserContext)
{
	String &str = *(String*)UserContext;

#ifdef _WIN64
	str << FormattedString(TEXT("%016I64X-%016I64X %s\r\n"), ModuleBase, ModuleBase + ModuleSize, ModuleName);
#else
	str << FormattedString(TEXT("%08.8I64X-%08.8I64X %s\r\n"), ModuleBase, ModuleBase + ModuleSize, ModuleName);
#endif
	return TRUE;
}

LONG CALLBACK SLiveExceptionHandler(PEXCEPTION_POINTERS exceptionInfo)
{
	HANDLE  hProcess;

	HMODULE hDbgHelp;

	MINIDUMP_EXCEPTION_INFORMATION miniInfo;

	STACKFRAME64        frame = { 0 };
	CONTEXT             context = *exceptionInfo->ContextRecord;
	SYMBOL_INFO         *symInfo;
	DWORD64             fnOffset;
	TCHAR               logPath[MAX_PATH];

	OSVERSIONINFOEX     osInfo;
	SYSTEMTIME          timeInfo;

	ENUMERATELOADEDMODULES64    fnEnumerateLoadedModules64;
	SYMSETOPTIONS               fnSymSetOptions;
	SYMINITIALIZE               fnSymInitialize;
	STACKWALK64                 fnStackWalk64;
	SYMFUNCTIONTABLEACCESS64    fnSymFunctionTableAccess64;
	SYMGETMODULEBASE64          fnSymGetModuleBase64;
	SYMFROMADDR                 fnSymFromAddr;
	SYMCLEANUP                  fnSymCleanup;
	MINIDUMPWRITEDUMP           fnMiniDumpWriteDump;
	SYMGETMODULEINFO64          fnSymGetModuleInfo64;

	DWORD                       i;
	DWORD64                     InstructionPtr;
	DWORD                       imageType;

	TCHAR                       searchPath[MAX_PATH], *p;

	static BOOL                 inExceptionHandler = FALSE;

	moduleinfo_t                moduleInfo;

	//always break into a debugger if one is present
	if (IsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;

	//exception codes < 0x80000000 are typically informative only and not crash worthy
	//0xe06d7363 indicates a c++ exception was thrown, let's just hope it was caught.
	//this is no longer needed since we're an unhandled handler vs a vectored handler

	/*if (exceptionInfo->ExceptionRecord->ExceptionCode < 0x80000000 || exceptionInfo->ExceptionRecord->ExceptionCode == 0xe06d7363 ||
	exceptionInfo->ExceptionRecord->ExceptionCode == 0x800706b5)
	return EXCEPTION_CONTINUE_SEARCH;*/

	//uh oh, we're crashing inside ourselves... this is really bad!
	if (inExceptionHandler)
		return EXCEPTION_CONTINUE_SEARCH;

	inExceptionHandler = TRUE;

	//load dbghelp dynamically
	hDbgHelp = LoadLibrary(TEXT("DBGHELP"));

	if (!hDbgHelp)
		return EXCEPTION_CONTINUE_SEARCH;

	fnEnumerateLoadedModules64 = (ENUMERATELOADEDMODULES64)GetProcAddress(hDbgHelp, "EnumerateLoadedModulesW64");
	fnSymSetOptions = (SYMSETOPTIONS)GetProcAddress(hDbgHelp, "SymSetOptions");
	fnSymInitialize = (SYMINITIALIZE)GetProcAddress(hDbgHelp, "SymInitialize");
	fnSymFunctionTableAccess64 = (SYMFUNCTIONTABLEACCESS64)GetProcAddress(hDbgHelp, "SymFunctionTableAccess64");
	fnSymGetModuleBase64 = (SYMGETMODULEBASE64)GetProcAddress(hDbgHelp, "SymGetModuleBase64");
	fnStackWalk64 = (STACKWALK64)GetProcAddress(hDbgHelp, "StackWalk64");
	fnSymFromAddr = (SYMFROMADDR)GetProcAddress(hDbgHelp, "SymFromAddrW");
	fnSymCleanup = (SYMCLEANUP)GetProcAddress(hDbgHelp, "SymCleanup");
	fnSymGetModuleInfo64 = (SYMGETMODULEINFO64)GetProcAddress(hDbgHelp, "SymGetModuleInfo64");
	fnMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");

	if (!fnEnumerateLoadedModules64 || !fnSymSetOptions || !fnSymInitialize || !fnSymFunctionTableAccess64 ||
		!fnSymGetModuleBase64 || !fnStackWalk64 || !fnSymFromAddr || !fnSymCleanup || !fnSymGetModuleInfo64)
	{
		FreeLibrary(hDbgHelp);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	hProcess = GetCurrentProcess();

	fnSymSetOptions(SYMOPT_UNDNAME | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_LOAD_ANYTHING);

	GetModuleFileName(NULL, searchPath, _countof(searchPath) - 1);
	p = srchr(searchPath, '\\');
	if (p)
		*p = 0;

	String DumpsPath = searchPath;
	DumpsPath += L"\\crashDumps";

	CreatePath(DumpsPath);

	//create a log file
	GetSystemTime(&timeInfo);
	for (i = 1;;)
	{
		tsprintf_s(logPath, _countof(logPath) - 1, TEXT("%s\\crashDumps\\BLiveCrashLog%.4d-%.2d-%.2d_%d.txt"), searchPath, timeInfo.wYear, timeInfo.wMonth, timeInfo.wDay, i);
		if (GetFileAttributes(logPath) == INVALID_FILE_ATTRIBUTES)
			break;
		i++;
	}

	XFile   crashDumpLog;

	if (!crashDumpLog.Open(logPath, XFILE_WRITE, XFILE_CREATENEW))
	{
		FreeLibrary(hDbgHelp);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	//initialize debug symbols
	fnSymInitialize(hProcess, NULL, TRUE);

#ifdef _WIN64
	InstructionPtr = context.Rip;
	frame.AddrPC.Offset = InstructionPtr;
	frame.AddrFrame.Offset = context.Rbp;
	frame.AddrStack.Offset = context.Rsp;
	imageType = IMAGE_FILE_MACHINE_AMD64;
#else
	InstructionPtr = context.Eip;
	frame.AddrPC.Offset = InstructionPtr;
	frame.AddrFrame.Offset = context.Ebp;
	frame.AddrStack.Offset = context.Esp;
	imageType = IMAGE_FILE_MACHINE_I386;
#endif

	frame.AddrFrame.Mode = AddrModeFlat;
	frame.AddrPC.Mode = AddrModeFlat;
	frame.AddrStack.Mode = AddrModeFlat;

	symInfo = (SYMBOL_INFO *)LocalAlloc(LPTR, sizeof(*symInfo) + 256);
	symInfo->SizeOfStruct = sizeof(SYMBOL_INFO);
	symInfo->MaxNameLen = 256;
	fnOffset = 0;

	//get os info
	memset(&osInfo, 0, sizeof(osInfo));
	osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	if (!GetVersionEx((OSVERSIONINFO *)&osInfo))
	{
		osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx((OSVERSIONINFO *)&osInfo);
	}

	String cpuInfo;
	HKEY key;

	// get cpu info
	if (RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), &key) == ERROR_SUCCESS)
	{
		DWORD dwSize = 1024;
		cpuInfo.SetLength(dwSize);
		if (RegQueryValueEx(key, TEXT("ProcessorNameString"), NULL, NULL, (LPBYTE)cpuInfo.Array(), &dwSize) != ERROR_SUCCESS)
			cpuInfo = TEXT("<unable to query>");
		RegCloseKey(key);
	}
	else
		cpuInfo = TEXT("<unable to query>");

	//determine which module the crash occured in
	scpy(moduleInfo.moduleName, TEXT("<unknown>"));
	moduleInfo.faultAddress = InstructionPtr;
	fnEnumerateLoadedModules64(hProcess, (PENUMLOADED_MODULES_CALLBACK64)EnumerateLoadedModulesProcInfo, (VOID *)&moduleInfo);
	slwr(moduleInfo.moduleName);

	BOOL isPlugin = FALSE;

	if (sstr(moduleInfo.moduleName, TEXT("plugins\\")))
		isPlugin = TRUE;

	String strModuleInfo;
	String crashMessage;

	fnEnumerateLoadedModules64(hProcess, (PENUMLOADED_MODULES_CALLBACK64)RecordAllLoadedModules, (VOID *)&strModuleInfo);

	crashMessage <<
		TEXT("BLive has encountered an unhandled exception and has terminated. If you are able to\r\n")
		TEXT("reproduce this crash, please submit this crash report on the forums at\r\n")
		TEXT("http://www.BLiveproject.com/ - include the contents of this crash log and the\r\n")
		TEXT("minidump .dmp file (if available) as well as your regular BLive log files and\r\n")
		TEXT("a description of what you were doing at the time of the crash.\r\n")
		TEXT("\r\n")
		TEXT("This crash appears to have occured in the '") << moduleInfo.moduleName << TEXT("' module.\r\n\r\n");

	crashDumpLog.WriteStr(crashMessage.Array());

	crashDumpLog.WriteStr(FormattedString(TEXT("**** UNHANDLED EXCEPTION: %x\r\nFault address: %I64p (%s)\r\n"), exceptionInfo->ExceptionRecord->ExceptionCode, InstructionPtr, moduleInfo.moduleName));

	crashDumpLog.WriteStr(TEXT("BLive version: ") L"1.0.0.1" TEXT("\r\n"));
	crashDumpLog.WriteStr(FormattedString(TEXT("Windows version: %d.%d (Build %d) %s\r\nCPU: %s\r\n\r\n"), osInfo.dwMajorVersion, osInfo.dwMinorVersion, osInfo.dwBuildNumber, osInfo.szCSDVersion, cpuInfo.Array()));

	crashDumpLog.WriteStr(TEXT("Crashing thread stack trace:\r\n"));
#ifdef _WIN64
	crashDumpLog.WriteStr(TEXT("Stack            EIP              Arg0             Arg1             Arg2             Arg3             Address\r\n"));
#else
	crashDumpLog.WriteStr(TEXT("Stack    EIP      Arg0     Arg1     Arg2     Arg3     Address\r\n"));
#endif
	crashDumpLog.FlushFileBuffers();

	while (fnStackWalk64(imageType, hProcess, GetCurrentThread(), &frame, &context, NULL, (PFUNCTION_TABLE_ACCESS_ROUTINE64)fnSymFunctionTableAccess64, (PGET_MODULE_BASE_ROUTINE64)fnSymGetModuleBase64, NULL))
	{
		scpy(moduleInfo.moduleName, TEXT("<unknown>"));
		moduleInfo.faultAddress = frame.AddrPC.Offset;
		fnEnumerateLoadedModules64(hProcess, (PENUMLOADED_MODULES_CALLBACK64)EnumerateLoadedModulesProcInfo, (VOID *)&moduleInfo);
		slwr(moduleInfo.moduleName);

		p = srchr(moduleInfo.moduleName, '\\');
		if (p)
			p++;
		else
			p = moduleInfo.moduleName;

#ifdef _WIN64
		if (fnSymFromAddr(hProcess, frame.AddrPC.Offset, &fnOffset, symInfo) && !(symInfo->Flags & SYMFLAG_EXPORT))
		{
			crashDumpLog.WriteStr(FormattedString(TEXT("%016I64X %016I64X %016I64X %016I64X %016I64X %016I64X %s!%s+0x%I64x\r\n"),
				frame.AddrStack.Offset,
				frame.AddrPC.Offset,
				frame.Params[0],
				frame.Params[1],
				frame.Params[2],
				frame.Params[3],
				p,
				symInfo->Name,
				fnOffset));
		}
		else
		{
			crashDumpLog.WriteStr(FormattedString(TEXT("%016I64X %016I64X %016I64X %016I64X %016I64X %016I64X %s!0x%I64x\r\n"),
				frame.AddrStack.Offset,
				frame.AddrPC.Offset,
				frame.Params[0],
				frame.Params[1],
				frame.Params[2],
				frame.Params[3],
				p,
				frame.AddrPC.Offset));
		}
#else
		if (fnSymFromAddr(hProcess, frame.AddrPC.Offset, &fnOffset, symInfo) && !(symInfo->Flags & SYMFLAG_EXPORT))
		{
			crashDumpLog.WriteStr(FormattedString(TEXT("%08.8I64X %08.8I64X %08.8X %08.8X %08.8X %08.8X %s!%s+0x%I64x\r\n"),
				frame.AddrStack.Offset,
				frame.AddrPC.Offset,
				(DWORD)frame.Params[0],
				(DWORD)frame.Params[1],
				(DWORD)frame.Params[2],
				(DWORD)frame.Params[3],
				p,
				symInfo->Name,
				fnOffset));
		}
		else
		{
			crashDumpLog.WriteStr(FormattedString(TEXT("%08.8I64X %08.8I64X %08.8X %08.8X %08.8X %08.8X %s!0x%I64x\r\n"),
				frame.AddrStack.Offset,
				frame.AddrPC.Offset,
				(DWORD)frame.Params[0],
				(DWORD)frame.Params[1],
				(DWORD)frame.Params[2],
				(DWORD)frame.Params[3],
				p,
				frame.AddrPC.Offset
				));
		}
#endif

		crashDumpLog.FlushFileBuffers();
	}


	//generate a minidump if possible
	if (fnMiniDumpWriteDump)
	{
		TCHAR     dumpPath[MAX_PATH];
		HANDLE    hFile;

		tsprintf_s(dumpPath, _countof(dumpPath) - 1, TEXT("%s\\crashDumps\\BLiveCrashDump%.4d-%.2d-%.2d_%d.dmp"), searchPath, timeInfo.wYear, timeInfo.wMonth, timeInfo.wDay, i);

		hFile = CreateFile(dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			MINIDUMP_TYPE dumpFlags = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithUnloadedModules | MiniDumpWithProcessThreadData);

			miniInfo.ClientPointers = TRUE;
			miniInfo.ExceptionPointers = exceptionInfo;
			miniInfo.ThreadId = GetCurrentThreadId();

			if (fnMiniDumpWriteDump(hProcess, GetCurrentProcessId(), hFile, dumpFlags, &miniInfo, NULL, NULL))
			{
				crashDumpLog.WriteStr(FormattedString(TEXT("\r\nA minidump was saved to %s.\r\nPlease include this file when posting a crash report.\r\n"), dumpPath));
			}
			else
			{
				CloseHandle(hFile);
				DeleteFile(dumpPath);
			}
		}
	}
	else
	{
		crashDumpLog.WriteStr(TEXT("\r\nA minidump could not be created. Please check dbghelp.dll is present.\r\n"));
	}

	crashDumpLog.WriteStr("\r\nList of loaded modules:\r\n");
#ifdef _WIN64
	crashDumpLog.WriteStr("Base Address                      Module\r\n");
#else
	crashDumpLog.WriteStr("Base Address      Module\r\n");
#endif
	crashDumpLog.WriteStr(strModuleInfo);

	crashDumpLog.Close();

	LocalFree(symInfo);

	fnSymCleanup(hProcess);

	if (BLiveMessageBox(G_MainHwnd, TEXT("Woops! BLive has crashed. Would you like to view a crash report?"), NULL, MB_ICONERROR | MB_YESNO) == IDYES)
		ShellExecute(NULL, NULL, logPath, NULL, searchPath, SW_SHOWDEFAULT);

	FreeLibrary(hDbgHelp);

	//we really shouldn't be returning here, if we're at the bottom of the VEH chain this is a pretty legitimate crash
	//and if we return we could end up invoking a second crash handler or other weird / annoying things
	//ExitProcess(exceptionInfo->ExceptionRecord->ExceptionCode);
	return EXCEPTION_CONTINUE_SEARCH;
}

typedef BOOL(WINAPI *getUserModeExceptionProc)(LPDWORD);
typedef BOOL(WINAPI *setUserModeExceptionProc)(DWORD);

void InitializeExceptionHandler()
{
	HMODULE k32;

	//standard app-wide unhandled exception filter
	SetUnhandledExceptionFilter(SLiveExceptionHandler);

	//fix for exceptions being swallowed inside callbacks (see KB976038)
	k32 = GetModuleHandle(TEXT("KERNEL32"));
	if (k32)
	{
		DWORD dwFlags;
		getUserModeExceptionProc procGetProcessUserModeExceptionPolicy;
		setUserModeExceptionProc procSetProcessUserModeExceptionPolicy;

		procGetProcessUserModeExceptionPolicy = (getUserModeExceptionProc)GetProcAddress(k32, "GetProcessUserModeExceptionPolicy");
		procSetProcessUserModeExceptionPolicy = (setUserModeExceptionProc)GetProcAddress(k32, "SetProcessUserModeExceptionPolicy");

		if (procGetProcessUserModeExceptionPolicy && procSetProcessUserModeExceptionPolicy)
		{
			if (procGetProcessUserModeExceptionPolicy(&dwFlags))
				procSetProcessUserModeExceptionPolicy(dwFlags & ~1);
		}
	}
}

CSLiveManager * CSLiveManager::m_Intances = NULL;
D3DAPI *CSLiveManager::m_D3DRender = NULL;
MemStack *CSLiveManager::mem_stack = new MemStack;

std::vector<MoudleStruct> ListPlugin;

CSLiveManager::CSLiveManager()
{
	bInit = false;
	Log::open(true, "-dGMfyds");
	QueryPerformanceFrequency(&clockFre);

	InitializeCriticalSection(&MapInstanceSec);
	bRunning = true;
	HVideoCapture = NULL;
	FPS = 0;
	mainVertexShader = NULL;
	mainPixelShader = NULL;
	solidVertexShader = NULL;
	solidPixelShader = NULL;
	yuvScalePixelShader = NULL;
	copyTextures = NULL;
	copyTextures_back = NULL;
	for (int i = 0; i < 2; ++i)
		mainRenderTextures[i] = NULL;

	transitionTexture = NULL;
	transNewTexture = NULL;
	yuvRenderTextures = NULL;
	yuvRenderTextures_back = NULL;
	PreRenderTexture = NULL;
	CurrentVideoTime = 0;
	HAudioCapture = NULL;
	HVideoEncoder = NULL;
	HVideoEncoder_back = NULL;
	HLittlePreview = NULL;
	transitionPixel = NULL;

	circleTransitionPixel = NULL;
	bStartLive = false;
	StartVideoTime = 0;
	bOutPicDel = false;
	bOutPicDel_Back = false;
	hVideoEvent = CreateEvent(NULL, false, false, NULL);
	hVideoEvent_back = CreateEvent(NULL, false, false, NULL);
	hVideoComplete = CreateEvent(NULL, false, false, NULL);
	Outpic = NULL;
	bFirstCreate = true;
	bTransDisSolving = false;
	TransFormTime = 1.0f;
	TransEscapeTime = 0.0f;
	bTransUpDown = false;
	bTransDiffuse = false;
	bRadius = false;
	ss = NULL;
	bUseBack = false;
	LiveInstance = NULL;

	desktopVol = 1.0f;
	leftdesktopVol = 1.0f;
	rightdesktopVol = 1.0f;
	m_quotietyVolume = 3.0f;
	m_bPlayPcmLocal = true;
	m_bPlayPcmLive = true;
	m_bProject = false;

	bPleaseEnableProjector = false;
	bPleaseDisableProjector = false;
	bEnableProjectorCursor = true;
	projectorX = 0; 
	projectorY = 0;
	projectorWidth = 0;
	projectorHeight = 0;
	projectorMonitorID = 0;
    hwndProjector = NULL;
	projectorTexture = NULL;
	LocalInstance = NULL;
	m_CheckTime = 2 * 60 * 1000;
	m_CheckEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	copyRGBTexture = NULL;
	SDIMutex = OSCreateMutex();
	SDITexture = NULL;
	HStatus = NULL;
	__SDIOutInfo = NULL;
	bStartView = false;
	Deinterlacer = NULL;
	DeinterlacerLocal = NULL;
	bNeedAgentInPGM = false;
}

CSLiveManager::~CSLiveManager()
{
	Log::writeMessage(LOG_RTSPSERV, 1, " LiveSDK_Log:%s CSLiveManager 0x%p 开始析构", __FUNCTION__, this);
	CoUninitialize();

	bRunning = false;
	SetEvent(hVideoEvent);
	SetEvent(hVideoEvent_back);
	if (HVideoEncoder)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待视频编码发送线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HVideoEncoder, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频编码发送线程等待退出超过5000ms,强杀!");
			TerminateThread(HVideoEncoder, 0);
		}
		CloseHandle(HVideoEncoder);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频编码发送线程退出!");
	}

	if (HVideoEncoder_back)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待视频back编码发送线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HVideoEncoder_back, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频编back码发送线程等待退出超过5000ms,强杀!");
			TerminateThread(HVideoEncoder_back, 0);
		}
		CloseHandle(HVideoEncoder_back);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频back编码发送线程退出!");
	}

	if (HVideoCapture)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待视频采集线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HVideoCapture, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频采集线程等待退出超过5000ms,强杀!");
			TerminateThread(HVideoCapture, 0);
		}
		CloseHandle(HVideoCapture);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:视频采集线程退出!");
	}

	if (HLittlePreview)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待小预览渲染线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HLittlePreview, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:小预览渲染线程等待退出超过5000ms,强杀!");
			TerminateThread(HLittlePreview, 0);
		}
		CloseHandle(HLittlePreview);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:小预览渲染线程退出!");
	}

	bool bForceKill = false;
	if (HAudioCapture)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:等待音频采集线程退出!");
		if (WAIT_TIMEOUT == WaitForSingleObject(HAudioCapture, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:音频采集线程等待退出超过5000,强杀!");
			TerminateThread(HAudioCapture, 0);
			bForceKill = true;
		}
		CloseHandle(HAudioCapture);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:音频采集线程退出!");
	}

	if (HStatus)
	{
		SetEvent(m_CheckEvent);

		if (WAIT_TIMEOUT == WaitForSingleObject(HStatus, 5000))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:HStatus等待退出超过5000,强杀!");
			TerminateThread(HStatus, 0);
		}

		CloseHandle(HStatus);
	}

	DisableProjector();

	if (mainVertexShader)
	{
		delete mainVertexShader;
		mainVertexShader = NULL;
	}

	if (mainPixelShader)
	{
		delete mainPixelShader;
		mainPixelShader = NULL;
	}
	if (yuvScalePixelShader)
	{
		delete yuvScalePixelShader;
		yuvScalePixelShader = NULL;
	}

	if (transitionPixel)
	{
		delete transitionPixel;
		transitionPixel = NULL;
	}


	if (circleTransitionPixel)
	{
		delete circleTransitionPixel;
		circleTransitionPixel = NULL;
	}


	if (solidVertexShader)
	{
		delete solidVertexShader;
		solidVertexShader = NULL;
	}

	if (solidPixelShader)
	{
		delete solidPixelShader;
		solidPixelShader = NULL;
	}

	if (copyTextures)
	{
		delete copyTextures;
		copyTextures = NULL;
	}

	if (copyRGBTexture)
	{
		delete copyRGBTexture;
		copyRGBTexture = NULL;
	}

	for (int i = 0; i < 2; ++i)
	{
		if (mainRenderTextures[i])
		{
			delete mainRenderTextures[i];
			mainRenderTextures[i] = NULL;
		}
	}

	if (transitionTexture)
	{
		delete transitionTexture;
		transitionTexture = NULL;
	}

	if (transNewTexture)
	{
		delete transNewTexture;
		transNewTexture = NULL;
	}

	if (yuvRenderTextures)
	{
		delete yuvRenderTextures;
		yuvRenderTextures = NULL;
	}

	if (PreRenderTexture)
	{
		delete PreRenderTexture;
		PreRenderTexture = NULL;
	}

	if (SDITexture)
	{
		delete SDITexture;
		SDITexture = NULL;
	}


	if (bUseBack)
	{
		if (yuvRenderTextures_back)
		{
			delete yuvRenderTextures_back;
			yuvRenderTextures_back = NULL;
		}
		if (copyTextures_back)
		{
			delete copyTextures_back;
			copyTextures_back = NULL;
		}
	}

	if (ss)
	{
		delete ss;
		ss = NULL;
	}

	if (hVideoEvent)
		CloseHandle(hVideoEvent);

	if (hVideoEvent_back)
		CloseHandle(hVideoEvent_back);

	if (hVideoComplete)
		CloseHandle(hVideoComplete);

	if (m_CheckEvent)
		CloseHandle(m_CheckEvent);

	if (SDIMutex)
		OSCloseMutex(SDIMutex);

	if (__SDIOutInfo)
		delete[] __SDIOutInfo;

	if (Deinterlacer)
		delete Deinterlacer;

	if (DeinterlacerLocal)
		delete DeinterlacerLocal;

	for (int i = 0; i < m_InstanceList.GetSize();)
	{
		CInstanceProcess *Process = m_InstanceList.GetAt(i);

		//先删除互动连接源
		if (Process && (!Process->bLittlePre || (Process->bLittlePre && Process->bNoPreView)))
		{
			if (!Process->bLittlePre && !Process->IsLiveInstance)
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "删除非直播实例%llu", (uint64_t)Process);
				LocalInstance = NULL;
			}
			else if (Process->IsLiveInstance)
			{
				if (bForceKill)
					Process->SetForceKillThread();

				Log::writeMessage(LOG_RTSPSERV, 1, "删除直播实例%llu", (uint64_t)Process);
				LiveInstance = NULL;
				G_LiveInstance = NULL;
			}
			else
			{
				Log::writeMessage(LOG_RTSPSERV, 1, "删除无预览源实例%llu", (uint64_t)Process);
			}
			m_InstanceList.earse((uint64_t)Process);
		}
		else
		{
			++i;
		}

	}

	for (int i = 0; i < m_InstanceList.GetSize();)//删除掉互动连接源
	{
		CInstanceProcess *Process = m_InstanceList.GetAt(i);
		bool bFind = false;
		if (Process)
		{
			if (Process->bLittlePre && !Process->bNoPreView)
			{
				for (int j = 0; j < Process->m_VideoList.Num(); ++j)
				{
					VideoStruct &OneVideo = Process->m_VideoList[j];

					if (0 == strcmp(OneVideo.VideoStream->GainClassName(), "PipeVideo"))//互动连接源
					{
						m_InstanceList.earse((uint64_t)Process);
						bFind = true;
						break;
					}
				}
			}
		}

		if (!bFind)
		{
			++i;
		}
	}

	DeleteCriticalSection(&MapInstanceSec);

	Log::writeMessage(LOG_RTSPSERV, 1, "%s 析构结束",__FUNCTION__);
}

CSLiveManager * CSLiveManager::GetInstance()
{
	if (m_Intances)
		return m_Intances;
	return m_Intances = new CSLiveManager;
}

int CSLiveManager::SLiveInit(const SLiveParam *Param)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!",__FUNCTION__);
	try
	{
		if (bInit)
		{
			BUTEL_THORWERROR("SDK已初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Param);

		// cpu必须支持sse,不支持就返回
		if (!HasSSE2Support())
		{
			BUTEL_THORWERROR("cpu 必需支持SSE!");
		}
		CoInitialize(NULL);
		// 如果有264编码，并且2013编译器
#if defined _M_X64 && _MSC_VER == 1800
		_set_FMA3_enable(0);
#endif

		// 设置调试权限
		LoadSeDebugPrivilege();

		EnableProfiling(TRUE);

		// 设置堆栈属性，堆破坏就结束
		HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

		// dep
		SetProcessDEPPolicy(PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);

		//设置异常处理
		InitializeExceptionHandler();
		
		// gdi+支持
		ULONG_PTR gdipToken;
		const Gdiplus::GdiplusStartupInput gdipInput;
		Gdiplus::GdiplusStartup(&gdipToken, &gdipInput, NULL);

		WNDCLASS wc;
		zero(&wc, sizeof(wc));
		wc.hInstance = hMain;
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);

		wc.lpszClassName = L"ProjectorFrame";
		wc.lpfnWndProc = (WNDPROC)ProjectorFrameProc;
		wc.hbrBackground = GetSysColorBrush(COLOR_HIGHLIGHT);

		if (!RegisterClass(&wc))
		{
			Log::writeError(LOG_RTSPSERV, 1, "注册 ProjectorFrame 窗口失败!");
			return -2;
		}

		hwndProjector = CreateWindow(L"ProjectorFrame",
			L"Projector Window",
			WS_POPUP, 0, 0, 0, 0,
			NULL, NULL, hMain, NULL);

		monitors.Clear();
		EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)MonitorInfoEnumProc, (LPARAM)&monitors);

		memcpy(&BSParam, Param, sizeof SLiveParam);
		if (BSParam.LiveSetting.FPS == 0)
			BSParam.LiveSetting.FPS = 25;
		if (BSParam.Advanced.BufferTime == 0)
			BSParam.Advanced.BufferTime = 200;

		G_MainHwnd = (HWND)Param->MainHwnd;

		if (BSParam.SDIOut && BSParam.SDICount > 0)
		{
			__SDIOutInfo = new SDIOutInfo[BSParam.SDICount];

			BUTEL_IFNULLRETURNERROR(__SDIOutInfo);

			for (int i = 0; i < BSParam.SDICount; ++i)
			{
				__SDIOutInfo[i].bEnable = BSParam.SDIOut[i].bEnable;
				__SDIOutInfo[i].Format = BSParam.SDIOut[i].Format;
				__SDIOutInfo[i].id = BSParam.SDIOut[i].Id;
				__SDIOutInfo[i].SourceName = BSParam.SDIOut[i].SourceName;
				__SDIOutInfo[i].AudioName = BSParam.SDIOut[i].AudioName;
			}
			BlackMagic::Instance()->SDI_Init(__SDIOutInfo, BSParam.BlackMagic.bOutSDI);

			if (BSParam.BlackMagic.bOutSDI)
			{
				BlackMagic::Instance()->ApplyOutOrInSettings(BSParam.BlackMagic.bOutSDI);
			}

		}
		else
		{
			BlackMagic::Instance()->SDI_Init(NULL, BSParam.BlackMagic.bOutSDI);
		}

		//加载所有插件

		WIN32_FIND_DATAW FindData;
		HANDLE HFile = FindFirstFile(L"./Plugins/*.dll", &FindData);

		if (HFile)
		{
			wstring PluginPath = L"./Plugins/";
			PluginPath += FindData.cFileName;
			HMODULE HModule = LoadLibrary(PluginPath.c_str());
			if (!HModule)
			{
				Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:加载 %s 模块失败! LastError = %d", WcharToAnsi(PluginPath).c_str(), GetLastError());
			}
			
			MoudleStruct MS;
			MS.HMoudle = HModule;
			MS.MoudleName = PluginPath;
			ListPlugin.push_back(MS);

			while(FindNextFile(HFile, &FindData))
			{
				PluginPath = L"./Plugins/";
				PluginPath += FindData.cFileName;
				HMODULE HModule = LoadLibrary(PluginPath.c_str());
				if (!HModule)
				{
					Log::writeError(LOG_RTSPSERV, 1, "LiveSDK_Log:加载 %s 模块失败! LastError = %d", WcharToAnsi(PluginPath).c_str(), GetLastError());
					continue;
				}
				MS.HMoudle = HModule;
				MS.MoudleName = PluginPath;
				ListPlugin.push_back(MS);
			}
		}
	}
	catch (CErrorBase& e)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		SLiveSetLastError(e.m_Error.c_str());
		return -1;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end, Init Sucess!", __FUNCTION__);
	bInit = true;
	return 0;
}

void CSLiveManager::SLiveRelese()
{
	Log::writeMessage(LOG_RTSPSERV, 1, " LiveSDK_Log:%s invoke begin!", __FUNCTION__);
	if (m_Intances)
	{
		DumpProfileData();
		FreeProfileData();
		m_Intances->ShutDown();
		delete m_Intances;
	}
	m_Intances = NULL;

	if (m_D3DRender)
		delete m_D3DRender;
	m_D3DRender = NULL;

	Log::writeMessage(LOG_RTSPSERV, 1, " LiveSDK_Log:BlackMagic::Destroy invoke begin!");
	BlackMagic::Destroy();
	Log::writeMessage(LOG_RTSPSERV, 1, " LiveSDK_Log:BlackMagic::Destroy invoke end!");
	for (int i = 0; i < ListPlugin.size(); ++i)
	{
		if (ListPlugin[i].HMoudle)
			FreeLibrary(ListPlugin[i].HMoudle);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, " LiveSDK_Log:%s CSLiveManager 结束析构", __FUNCTION__);
	mem_stack->bDelete = false;
	delete mem_stack;
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log: %s invoke end!", __FUNCTION__);
}

void CSLiveManager::ShutDown()
{
	EnterCriticalSection(&MapInstanceSec);
	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *Process = m_InstanceList.GetAt(i);
		
		if (Process)
			Process->bShutDown = true;
	}
	LeaveCriticalSection(&MapInstanceSec);
}

int CSLiveManager::SLiveSetParam(const SLiveParam *Param)
{
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Param);

		if (Param->Advanced.bChange)
		{
			memcpy(&BSParam.Advanced, &Param->Advanced, sizeof AdvancedParam);
			switch (BSParam.Advanced.DeinterlaceType)
			{
			case 0:
				DeinterConfig.type = DEINTERLACING_BLEND;
				break;
			case 1:
				DeinterConfig.type = DEINTERLACING_LINEAR;
				break;
			case 2:
				DeinterConfig.type = DEINTERLACING_YADIF;
				break;
			default:
				DeinterConfig.type = DEINTERLACING_LINEAR;
				break;
			}

			if (BSParam.Advanced.BufferTime == 0)
				BSParam.Advanced.BufferTime = 200;
			
		}

		if (Param->LiveSetting.bChange)
		{
			memcpy(&BSParam.LiveSetting, &Param->LiveSetting, sizeof LiveSettingParam);

			if (BSParam.LiveSetting.FPS <= 0)
			{
				BSParam.LiveSetting.FPS = 25;
			}

			if (BSParam.LiveSetting.FPSSec <= 0)
			{
				BSParam.LiveSetting.FPSSec = 25;
			}

			bUseBack = Param->LiveSetting.bUseLiveSec;
		}

		if (Param->DeviceSetting.bChange)
		{
			bool bMonitorChanged = strcmp(BSParam.DeviceSetting.MonitorDevice, Param->DeviceSetting.MonitorDevice) != 0;
			bool bScrProChanged = strcmp(BSParam.DeviceSetting.ScrProDevice, Param->DeviceSetting.ScrProDevice) != 0;
			if (bMonitorChanged || bScrProChanged)
			{
				String MonitorDevice = Asic2WChar(Param->DeviceSetting.MonitorDevice).c_str();
				String ScrProDevice = Asic2WChar(Param->DeviceSetting.ScrProDevice).c_str();
				if (LocalInstance)
				{
					for (UINT i = 0; i < LocalInstance->m_AudioList.Num(); ++i) {
						LocalInstance->m_AudioList[i].AudioStream->OnAudioDeviceChanged(MonitorDevice, ScrProDevice);
					}
				}

				if (LiveInstance)
				{
					//EnterCriticalSection(&LiveInstance->AudioSection);
					for (UINT i = 0; i < LiveInstance->m_AudioList.Num(); ++i) {
						LiveInstance->m_AudioList[i].AudioStream->OnAudioDeviceChanged(MonitorDevice, ScrProDevice);
					}
					//LeaveCriticalSection(&LiveInstance->AudioSection);
				}
			}
			memcpy(&BSParam.DeviceSetting, &Param->DeviceSetting, sizeof DeviceParam);

			//一定要在memcpy之后修改小预览的监听
			if (bMonitorChanged)
			{
				for (int i = 0; i < m_InstanceList.GetSize(); ++i)
				{
					CInstanceProcess *Process = m_InstanceList.GetAt(i);

					if (Process && Process->bLittlePre && !Process->bNoPreView)
					{
						if (Process->MultiRender)
						{
							Process->MultiRender->SetAudioRender(Process->MultiRender->WaveFormat.nChannels, Process->MultiRender->WaveFormat.nSamplesPerSec, Process->MultiRender->WaveFormat.wBitsPerSample);
						}
					}
				}
			}
		}
		
		if (Param->BlackMagic.bChange)
		{
			BlackMagic::Instance()->ApplyOutOrInSettings(Param->BlackMagic.bOutSDI);
		}

		EnterCriticalSection(&MapInstanceSec);
		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *Process = m_InstanceList.GetAt(i);

			if (Process && !Process->bLittlePre)
				Process->SetParam(Param);
		}
		LeaveCriticalSection(&MapInstanceSec);

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		return -1;
	}
	return 0;
}

int CSLiveManager::SLiveCreateInstance(uint64_t *iIntanceID, uint64_t hwnd, bool bLiveIntance, bool bLittlePre)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(iIntanceID);



		if (hwnd && bFirstCreate)
		{
			bFirstCreate = false;

			ManagerInit(hwnd);

			DeinterConfig.processor = DEINTERLACING_PROCESSOR_GPU;
			DeinterConfig.doublesFramerate = false;
			DeinterConfig.fieldOrder = FIELD_ORDER_TFF | FIELD_ORDER_BFF;
			DeinterConfig.type = DEINTERLACING_LINEAR;

			switch (BSParam.Advanced.DeinterlaceType)
			{
			case 0:
				DeinterConfig.type = DEINTERLACING_BLEND;
				break;
			case 1:
				DeinterConfig.type = DEINTERLACING_LINEAR;
				break;
			case 2:
				DeinterConfig.type = DEINTERLACING_YADIF;
				break;
			default:
				break;
			}


			Deinterlacer = new CDeinterlacer(DeinterConfig.type);
			DeinterlacerLocal = new CDeinterlacer(DeinterConfig.type);
			
		}

		CInstanceProcess *NewInstance = new CInstanceProcess(&BSParam);

		if (NewInstance)
		{
			NewInstance->IsLiveInstance = bLiveIntance;
			NewInstance->bLittlePre = bLittlePre;

			if (hwnd)
			{
				NewInstance->SetHwnd(hwnd);
				NewInstance->BulidD3D();

				if (hwnd && !bFirstCreate)
				{
					RECT Rect;
					GetClientRect((HWND)hwnd, &Rect);
					Texture * Swap = NULL;
					if (!bLittlePre)
					{
						Swap = m_D3DRender->CreateRenderTargetSwapChain((HWND)hwnd, Rect.right, Rect.bottom);
						BUTEL_IFNULLRETURNERROR(Swap);

						NewInstance->SwapRender = Swap;
					}
					
				}
			}
			else
			{
				NewInstance->bNoPreView = true;
			}

			if (bLittlePre && hwnd)
			{
				NewInstance->CreateLittleRenderTarget();
			}
			else if (!bLiveIntance && hwnd)
			{
				LocalInstance = NewInstance;
			}

			*iIntanceID = (uint64_t)NewInstance;
			EnterCriticalSection(&MapInstanceSec);
			m_InstanceList[*iIntanceID] = NewInstance;
			LeaveCriticalSection(&MapInstanceSec);

			if (bLiveIntance)
			{
				LiveInstance = NewInstance;
				G_LiveInstance = NewInstance;
			}

			Log::writeMessage(LOG_RTSPSERV, 1, "%s 成功,InstanceID %llu", __FUNCTION__, (uint64_t)NewInstance);
		}
		else
		{
			BUTEL_THORWERROR("创建实例失败--new失败");
		}
			
	}
	catch (CErrorBase& e)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end Error occur!", __FUNCTION__);
		SLiveSetLastError(e.m_Error.c_str());
		return -1;
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Success", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveDestroyInstance(uint64_t iIntanceID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		char Tem[50] = { 0 };
		sprintf(Tem, "%llu", iIntanceID);

		//这里不合理
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];

		if (Process->IsLiveInstance)
		{
			LeaveCriticalSection(&MapInstanceSec);
			BUTEL_THORWERROR("不允许删除直播实例");
		}

		m_InstanceList.Remove(Tem);
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			if (Process->bLittlePre) //删除小预览
			{
				for (int i = 0; i < Process->m_VideoList.Num(); ++i)
				{
					VideoStruct &VS = Process->m_VideoList[i];
					for (int j = 0; j < m_InstanceList.GetSize(); ++j)
					{
						CInstanceProcess *LiveProcess = m_InstanceList.GetAt(j);

						if (LiveProcess && LiveProcess->IsLiveInstance)
						{
							bool bFind = false;
							for (int k = 0; k < LiveProcess->m_VideoList.Num(); ++k)
							{
								VideoStruct &VSLive = LiveProcess->m_VideoList[k];
								if (VS.VideoStream.get() == VSLive.VideoStream.get())
								{
									LiveProcess->DeleteStream((uint64_t)VS.VideoStream.get());
									bFind = true;
									break;
								}

							}

							if (!bFind && VS.AudioStream && LiveProcess->m_VideoList.Num())
							{
								Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 没有找到视频流，强制删除音视流 %llu", __FUNCTION__, (uint64_t)VS.AudioStream);
								LiveProcess->DeleteStream((uint64_t)VS.AudioStream);//增加这个防止没有删除掉音视
							}
						}

						//置空所有用到该流的区域占位源
						if (!Process->bNoPreView && LiveProcess->bLittlePre && LiveProcess->bNoPreView)
						{
							for (int k = 0; k < LiveProcess->m_VideoList.Num(); ++k)
							{
								VideoStruct &VSAgent = LiveProcess->m_VideoList[k];

								if (0 == strcmp(VSAgent.VideoStream->GainClassName(), "AgentSource"))
								{
									if (!(*VSAgent.Config)["SecName"].isNull())
									{
										if (0 == strcmp((*VSAgent.Config)["SecName"].asString().c_str(), (*VS.Config)["Name"].asString().c_str()))
										{
											VSAgent.VideoStream->RestSetGlobalSource();
										}
									}
								}

							}
						}
					}
				}
			}

			delete Process;
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveAddStream(uint64_t iIntanceID, const char* cParamJson, VideoArea *Area, uint64_t *StreamID1, uint64_t *StreamID2 /*= NULL*/)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	CInstanceProcess *Proc = NULL;
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(cParamJson);
		BUTEL_IFNULLRETURNERROR(StreamID1);

		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s cParamJson = %s", __FUNCTION__,cParamJson);
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		Proc = Process;

		Value Jvalue;
		Reader JReader;

		if (!JReader.parse(cParamJson, Jvalue))
		{
			BUTEL_THORWERROR("Json %s 解析失败", cParamJson);
		}
		
		if (Process)
		{
			OSEnterMutex(SharedDevice::HLock);
			try
			{
				//音频捕捉设备也要有小预览(光音频)
				if (0 == strcmp(Jvalue["ClassName"].asString().c_str(), "DSource"))
				{
					Process->bNoPreView = false;
				}
				Process->CreateStream(Jvalue, Area, StreamID1, StreamID2);
			}
			catch (CErrorBase& e)
			{
				OSLeaveMutex(SharedDevice::HLock);
				BUTEL_THORWERROR(e.m_Error.c_str());
			}

			OSLeaveMutex(SharedDevice::HLock);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ",iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		if (Proc && Proc->bLittlePre)
		{
			EnterCriticalSection(&MapInstanceSec);
			m_InstanceList.earse((uint64_t)iIntanceID);
			LeaveCriticalSection(&MapInstanceSec);
		}
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveDelStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->DeleteStream(iStreamID);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveGetStreamInfo(uint64_t iIntanceID, uint64_t iStreamID, char **Info)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Info);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Value StreamInfo;
			Process->GetStreamInfo(iStreamID,StreamInfo);
			std::string &RetStr = StreamInfo.toStyledString();
			SLiveAllowMemory((void**)Info, RetStr.length() + 1);
			memcpy(*Info, RetStr.c_str(), RetStr.length());
			(*Info)[RetStr.length()] = 0;
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetStreamStatus(uint64_t iIntanceID, uint64_t iStreamID, DBOperation *Status)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Status);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->GetStreamStatus(iStreamID, Status);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT Pos)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! Pos = %d", __FUNCTION__,Pos);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->SetStreamPlayPos(iStreamID, Pos);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveGetStreamPlayPos(uint64_t iIntanceID, uint64_t iStreamID, UINT* Pos)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Pos);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			UINT RetPos;
			Process->GetStreamPlayPos(iStreamID, RetPos);
			*Pos = RetPos;
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveOperaterStream(uint64_t iIntanceID, uint64_t iStreamID, DBOperation OperType)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! OperType = %d", __FUNCTION__,OperType);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->OperaterStream(iStreamID, OperType);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveUpdateStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cJsonParam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(cJsonParam);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s CJsonParam = %s", __FUNCTION__,cJsonParam);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->UpdateStream(iStreamID, cJsonParam);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveUpdateStreamPosition(uint64_t iIntanceID, uint64_t iStreamID, VideoArea *Area, bool bScale)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Area);
		//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s left = %.3f,top = %.3f,width = %.3f,height = %.3f", __FUNCTION__,Area->left,Area->top,Area->width,Area->height);

		//EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		//LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->UpdateStreamPosition(iStreamID, Area, bScale);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveMoveStream(uint64_t iIntanceID, uint64_t iStreamID, StreamMoveOperation Type)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! Type = %d", __FUNCTION__,Type);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->MoveStream(iStreamID, Type);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveStartRenderVStream(uint64_t iIntanceID, uint64_t iStreamID, uint64_t hwnd)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}


	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveStopRenderVStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}


	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveStartRenderAStream(uint64_t iIntanceID, uint64_t iStreamID, const char* cRenderAudioDevice)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(cRenderAudioDevice);
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s cRenderAudioDevice = %s", __FUNCTION__,cRenderAudioDevice);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->StartRenderAStream(iStreamID, cRenderAudioDevice);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveSetAudioStreamDBCallBack(uint64_t iIntanceID, uint64_t iStreamID, AudioDBCallBack DBCallBack)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(DBCallBack);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->SetAudioStreamDBCallBack(iStreamID, DBCallBack);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}


int CSLiveManager::SLiveStopRenderAStream(uint64_t iIntanceID, uint64_t iStreamID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->StopRenderAStream(iStreamID);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveStartResize(uint64_t iIntanceID,bool bDragResize)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->StartResize(bDragResize);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveStopResize(uint64_t iIntanceID)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->StopResize();
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetRenderStream(uint64_t iIntanceID, uint64_t iStreamID, bool bRender)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! bRender = %s", __FUNCTION__,bRender ? "ture":"false");
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			VideoStruct __Video;
			bool bFind = false;
			bool bBreak = false;
			IBaseVideo *BaseVideo = dynamic_cast<IBaseVideo*>((CPObject*)iStreamID);

			bool bAgent = false;
			IBaseVideo* GlobalSource = NULL;
			if (BaseVideo)
			{
				//区域占位源
				if (strcmp(BaseVideo->GainClassName(), "AgentSource") == 0)
				{
					bAgent = true;
					GlobalSource = BaseVideo->GetGlobalSource();
				}
			}
			else
			{
				BUTEL_THORWERROR("BaseVideo 为空 iStreamID = %llu", iStreamID);
			}


			{
				//放在大括号里让__Audio不增加引用计	
				AudioStruct __Audio;
				const char *ClassName = BaseVideo->GainClassName();
				
				if (strcmp(ClassName, "VideoSource") == 0 ||
					strcmp(ClassName, "DeviceSource") == 0 ||
					strcmp(ClassName, "AgentSource") == 0 ||
					strcmp(ClassName, "VideoLiveSource") == 0 ||
					strcmp(ClassName, "PipeVideo") == 0 ||
					strcmp(ClassName, "DSource") == 0)
				for (int i = 0; i < m_InstanceList.GetSize(); ++i)
				{
					CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);
					if (OneProcess && OneProcess->bLittlePre && (!OneProcess->bNoPreView || strcmp(BaseVideo->GainClassName(), "DSource") == 0))
					{
						for (int j = 0; j < OneProcess->m_VideoList.Num(); ++j)
						{
							VideoStruct &OneVideo = OneProcess->m_VideoList[j];
							if (bAgent && GlobalSource)
							{
								if (OneVideo.VideoDevice)
								{
									if (OneVideo.VideoDevice.get() == GlobalSource)
									{
										//DeviceSource中没有声音源了
										bFind = true;
										__Video = OneVideo;
										break;
									}
								}
								else
								{
									if (OneVideo.VideoStream.get() == GlobalSource)
									{
										if (strcmp(OneVideo.VideoStream->GainClassName(), "DeviceSource") == 0)
										{
											//DeviceSource中没有声音源了
											bFind = true;
											__Video = OneVideo;
											break;
										}
										else
										{
											for (int k = 0; k < OneProcess->m_AudioList.Num(); ++k)
											{
												AudioStruct &OneAudio = OneProcess->m_AudioList[k];

												if (OneAudio.VideoStream == GlobalSource)
												{
													bFind = true;
													__Audio = OneAudio;
													__Video = OneVideo;
													break;
												}

											}
										}
										

										break;
									}
								}
								
							}
							else if (bAgent && !GlobalSource)
							{
								bBreak = true;
								break;
							}
							else
							{
								if ((uint64_t)OneVideo.VideoStream.get() == iStreamID)
								{
									if (strcmp(ClassName, "DeviceSource") == 0) //DeviceSource中没有声音源了
									{
										bFind = true;
										__Video = OneVideo;
										break;;
									}
									else
									{
										for (int k = 0; k < OneProcess->m_AudioList.Num(); ++k)
										{
											AudioStruct &OneAudio = OneProcess->m_AudioList[k];

											if ((uint64_t)OneAudio.VideoStream == iStreamID)
											{
												bFind = true;
												__Audio = OneAudio;
												__Video = OneVideo;
												break;
											}

										}
									}
									
									break;
								}
							}


						}
					}

					if (bFind || bBreak)
						break;
				}

				Process->SetRenderStream(iStreamID, bRender, __Audio);

				if (bAgent && bFind && GlobalSource && strcmp(GlobalSource->GainClassName(),"DeviceSource") != 0)
				{
					EnterCriticalSection(&Process->AudioSection);
					bool bFind = false;
					for (int j = 0; j < Process->m_AudioList.Num(); ++j)
					{
						AudioStruct &ASTemD = Process->m_AudioList[j];

						if (ASTemD.AudioStream.get() == __Audio.AudioStream.get())
						{
							bFind = true;
							break;
						}
					}
					if (!bFind && bRender)
					{
						Process->m_AudioList.SetSize(Process->m_AudioList.Num() + 1);
						AudioStruct &AS = Process->m_AudioList[Process->m_AudioList.Num() - 1];
						AS = __Audio;
						if (AS.AudioStream.use_count() == 2)
							AS.AudioStream->SetLiveInstance(false);
					}
					LeaveCriticalSection(&Process->AudioSection);
				}
			}
			
			if (bFind || bAgent)
			{
				if (bFind)
				{
					if (bRender)
					{
						if (bAgent)
						{
							BaseVideo->GlobalSourceEnterScene();
						}
						else if (BaseVideo->CanEnterScene())
						{
							BaseVideo->GlobalSourceEnterScene();
							BaseVideo->SetCanEnterScene(false);
							EnterCriticalSection(&Process->VideoSection);
							BaseVideo->BeginScene();
							LeaveCriticalSection(&Process->VideoSection);
						}
						else if (__Video.bGlobalStream)
						{
							__Video.VideoStream->SwitchSences();
							EnterCriticalSection(&Process->VideoSection);
							__Video.VideoStream->BeginScene();
							LeaveCriticalSection(&Process->VideoSection);
						}

					}
					else
					{
						if (bAgent)
						{
							BaseVideo->GlobalSourceLeaveScene();
// 							bool bBreak = false;
// 							for (int i = 0; i < m_InstanceList.GetSize(); ++i)
// 							{
// 								CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);
// 								if (OneProcess && OneProcess->bLittlePre && OneProcess->bNoPreView)
// 								{
// 									for (int j = 0; j < OneProcess->m_VideoList.Num(); ++j)
// 									{
// 										VideoStruct &OneVideo = OneProcess->m_VideoList[j];
// 
// 										if (OneVideo.VideoStream.get() == BaseVideo)
// 										{
// 											if (__Video.bGlobalStream && OneVideo.VideoStream.use_count() == 2)
// 											{
// // 												IBaseVideo *BaseVideo2 = __Video.VideoStream.get();
// 
// 												if (BaseVideo)
// 												{
// 													BaseVideo->GlobalSourceLeaveScene();
// 													if (OneVideo.VideoDevice)
// 													{
// 														OneVideo.VideoDevice->GlobalSourceLeaveScene();
// 														OneVideo.VideoDevice->SetCanEnterScene(true);
// 													}
// 													BaseVideo->SetCanEnterScene(true);
// 												}
// 											}
// 											else if (__Video.bGlobalStream)
// 											{
// 												__Video.VideoStream->SetHasSwitchSences(true);
// 											}
// 											bBreak = true;
// 											break;
// 										}
// 
// 									}
// 								}
// 
// 								if (bBreak)
// 									break;
// 							}
							
						}
						else
						{
							//这里_Vidio占了一个引用计数，所以为3时进入
							if (__Video.bGlobalStream && __Video.VideoStream.use_count() == 3)
							{
								IBaseVideo *BaseVideo = __Video.VideoStream.get();

								if (BaseVideo)
								{
									BaseVideo->GlobalSourceLeaveScene();
									if (__Video.VideoDevice)
									{
										__Video.VideoDevice->GlobalSourceLeaveScene();
										__Video.VideoDevice->SetCanEnterScene(true);
									}
									BaseVideo->SetCanEnterScene(true);
								}
							}
							else if (__Video.bGlobalStream)
							{
								__Video.VideoStream->SetHasSwitchSences(true);
							}
						}
						
						
					}
				}
			}
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveStartLive(uint64_t iIntanceID, bool bRecordOnly /*= false*/)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}


		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Outpic = NULL;
			Outpic_back = NULL;
			Process->StartLive(bRecordOnly);
			bStartLive = true;
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveStopLive(uint64_t iIntanceID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			bStartLive = false;
			Process->StopLive();
			StartVideoTime = 0;
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

const char* CSLiveManager::SLiveGetLastError()
{
	return ErrMsg.c_str();
}

int CSLiveManager::SLiveAllowMemory(void **Mem, size_t size)
{
//	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke begin!",__FUNCTION__);
	try
	{
// 		if (!bInit)
// 		{
// 			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
// 		}

		if (size == 0 || size > (1 << 31))
		{
			BUTEL_THORWERROR("分配内存失败 size = %llu 不合法!", size);
		}

		*Mem = new void *[size];

		if (!(*Mem))
		{
			BUTEL_THORWERROR("分配内存失败 size = %llu", size);
		}

	}
	catch (CErrorBase& e)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke begin! Error occur",__FUNCTION__);
		SLiveSetLastError(e.m_Error.c_str());
		return -1;
	}
//	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke end!",__FUNCTION__);
	return 0;
}

void CSLiveManager::SLiveFreeMemory(void *Mem)
{
//	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke begin!",__FUNCTION__);
	try
	{
// 		if (!bInit)
// 		{
// 			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
// 		}
		BUTEL_IFNULLRETURNERROR(Mem);
//		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s  要删除的内存地址 0x%08p",__FUNCTION__,Mem);
		delete[] Mem;

	}
	catch (CErrorBase& e)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke end! Error occured!",__FUNCTION__);
		SLiveSetLastError(e.m_Error.c_str());
	}
//	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s invoke end!",__FUNCTION__);
}

int CSLiveManager::SLiveGetVideoCaptureList(char** JsonVideoCaptureList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		BUTEL_IFNULLRETURNERROR(JsonVideoCaptureList);

		Json::Value JsonCaptureList;
		NameList.clear();
		if (FillOutListOfDevices(CLSID_VideoInputDeviceCategory, JsonCaptureList))
		{
			std::string &str = JsonCaptureList.toStyledString();

			*JsonVideoCaptureList = new char[str.length() + 1];
			(*JsonVideoCaptureList)[str.length()] = 0;
			memcpy(*JsonVideoCaptureList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("GetVideoCaptureList = 0");
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetAudioCaptureList(char** JsonAudioCaptureList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		BUTEL_IFNULLRETURNERROR(JsonAudioCaptureList);
		Json::Value JsonCaptureList;
		NameList.clear();
		if (FillOutListOfDevices(CLSID_AudioInputDeviceCategory, JsonCaptureList))
		{
			std::string &str = JsonCaptureList.toStyledString();

			*JsonAudioCaptureList = new char[str.length() + 1];
			(*JsonAudioCaptureList)[str.length()] = 0;
			memcpy(*JsonAudioCaptureList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("GetAudioCaptureList = 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetAudioRenderList(char** JsonAudioRenderList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		BUTEL_IFNULLRETURNERROR(JsonAudioRenderList);

		Json::Value JsonCaptureList;
		NameList.clear();
		if (GetAudioRenderDevices(JsonCaptureList))
		{
			std::string &str = JsonCaptureList.toStyledString();
			*JsonAudioRenderList = new char[str.length() + 1];
			(*JsonAudioRenderList)[str.length()] = 0;
			memcpy(*JsonAudioRenderList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("GetAudioRenderList = 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}
int CSLiveManager::SLiveGetProcessList(char** JsonProcessList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(JsonProcessList);

		Json::Value JsonProcessNameList;
		NameList.clear();
		if (GetProcessList(JsonProcessNameList))
		{
			std::string &str = JsonProcessNameList.toStyledString();

			*JsonProcessList = new char[str.length() + 1];
			(*JsonProcessList)[str.length()] = 0;
			memcpy(*JsonProcessList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("GetProcessNameList = 0");
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}
void CSLiveManager::SLiveSetLastError(const char *Error)
{
	ErrMsg = Error;
}

void CSLiveManager::WcharToChar(const WCHAR* wc, std::string &RetStr)
{
	int len = WideCharToMultiByte(CP_ACP, 0, wc, (int)wcslen(wc), NULL, 0, NULL, NULL);
	char *m_char = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wc, (int)wcslen(wc), m_char, len, NULL, NULL);
	m_char[len] = '\0';
	RetStr = m_char;
	delete[] m_char;
}

bool CSLiveManager::FillOutListOfDevices(GUID matchGUID, Json::Value& deviceList)
{
	ICreateDevEnum *deviceEnum;
	IEnumMoniker *videoDeviceEnum;

	HRESULT err;
	err = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&deviceEnum);
	if (FAILED(err))
	{
		BUTEL_THORWERROR("CoCreateInstance 失败 LastError = %d", GetLastError());
	}

	err = deviceEnum->CreateClassEnumerator(matchGUID, &videoDeviceEnum, 0);
	if (FAILED(err))
	{
		deviceEnum->Release();
		BUTEL_THORWERROR("CreateClassEnumerator 失败 LastError = %d", GetLastError());
	}

	if (deviceEnum)
		deviceEnum->Release();

	if (err == S_FALSE) //no devices
	{
		BUTEL_THORWERROR("没有该类型设备!");
	}

	//------------------------------------------

	IMoniker *deviceInfo;
	DWORD count;
	int iCount = 0;
	Json::Value &OneValue = deviceList["DeviceList"];
	while (videoDeviceEnum->Next(1, &deviceInfo, &count) == S_OK)
	{
		IPropertyBag *propertyData;
		err = deviceInfo->BindToStorage(0, 0, IID_IPropertyBag, (void**)&propertyData);
		if (SUCCEEDED(err))
		{
			VARIANT friendlyNameValue, devicePathValue;
			friendlyNameValue.vt = VT_BSTR;
			friendlyNameValue.bstrVal = NULL;
			devicePathValue.vt = VT_BSTR;
			devicePathValue.bstrVal = NULL;

			err = propertyData->Read(L"FriendlyName", &friendlyNameValue, NULL);
			propertyData->Read(L"DevicePath", &devicePathValue, NULL);

			if (SUCCEEDED(err))
			{
				IBaseFilter *filter;
				err = deviceInfo->BindToObject(NULL, 0, IID_IBaseFilter, (void**)&filter);
				if (SUCCEEDED(err))
				{
					std::string strName;
					if (devicePathValue.bstrVal)
					{
						WcharToChar(devicePathValue.bstrVal, strName);
						OneValue[Json::UInt(iCount)]["DeviceID"] = strName.c_str();
					}
					WcharToChar(friendlyNameValue.bstrVal, strName);

					std::string ShowName = strName;

					std::vector<std::string>::iterator It = NameList.begin();
					std::vector<std::string>::iterator ItEnd = NameList.end();

					int index = 1;
					for (It; It != ItEnd; ++It)
					{
						if (strcmp(ShowName.c_str(), (*It).c_str()) == 0)
						{
							char Name[256] = { 0 };
							sprintf(Name, "%s0%d", ShowName.c_str(), index++);
							ShowName = Name;
						}
					}

					OneValue[Json::UInt(iCount)]["ShowName"] = ShowName.c_str();
					OneValue[Json::UInt(iCount++)]["DeviceName"] = strName.c_str();

					NameList.push_back(ShowName);
				}
				if (filter)
					filter->Release();
			}

			if (propertyData)
				propertyData->Release();
		}

		if (deviceInfo)
			deviceInfo->Release();
	}

	if (videoDeviceEnum)
		videoDeviceEnum->Release();
	return OneValue.size() > 0 ? true : false;
}

bool CSLiveManager::GetAudioRenderDevices(Json::Value &deviceList)
{
	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	IMMDeviceEnumerator *mmEnumerator;
	HRESULT err;

	err = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, IID_IMMDeviceEnumerator, (void**)&mmEnumerator);
	if (FAILED(err))
	{
		BUTEL_THORWERROR("CoCreateInstance 失败 LastError = %d", GetLastError());
	}

	IMMDeviceCollection *collection;

	EDataFlow audioDeviceType;

	audioDeviceType = eRender;

	DWORD flags = DEVICE_STATE_ACTIVE;

	err = mmEnumerator->EnumAudioEndpoints(audioDeviceType, flags, &collection);
	if (FAILED(err))
	{
		if (mmEnumerator)
			mmEnumerator->Release();
		BUTEL_THORWERROR("EnumAudioEndpoints 失败 LastError = %d", GetLastError());
	}

	int iCount = 0;
	Json::Value &OneValue = deviceList["DeviceList"];

	UINT count;
	if (SUCCEEDED(collection->GetCount(&count)))
	{
		for (UINT i = 0; i < count; i++)
		{
			IMMDevice *device;
			if (SUCCEEDED(collection->Item(i, &device)))
			{
				LPWSTR wstrID;
				if (SUCCEEDED(device->GetId(&wstrID)))
				{
					IPropertyStore *store;
					if (SUCCEEDED(device->OpenPropertyStore(STGM_READ, &store)))
					{
						PROPVARIANT varName;

						PropVariantInit(&varName);
						if (SUCCEEDED(store->GetValue(PKEY_Device_FriendlyName, &varName)))
						{
							std::string StrName;
							WcharToChar(varName.bstrVal, StrName);
							OneValue[Json::UInt(iCount++)] = StrName.c_str();

						}
						PropVariantClear(&varName);
					}
					if (store)
						store->Release();
					CoTaskMemFree((LPVOID)wstrID);
				}

				if (device)
					device->Release();
			}
		}
	}

	//-------------------------------------------------------

	if (collection)
		collection->Release();
	if (mmEnumerator)
		mmEnumerator->Release();

	return OneValue.size() > 0 ? true : false;
}

bool CSLiveManager::GetProcessList(Json::Value& ProcessList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);

	int iCount = 0;
	Json::Value &OneValue = ProcessList["processList"];
	HWND hwndCurrent = GetWindow(GetDesktopWindow(), GW_CHILD);
	if (NULL == hwndCurrent)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! return false", __FUNCTION__);
		return false;
	}
	do
	{
		HWND owner = GetWindow(hwndCurrent, GW_OWNER);
		LONG exstyle = GetWindowLong(hwndCurrent, GWL_EXSTYLE);
		if (IsWindowVisible(hwndCurrent)
			&& !IsIconic(hwndCurrent)
			&& GetWindowTextLength(hwndCurrent) != 0)
		{
			if (owner && !(exstyle & WS_EX_APPWINDOW) || exstyle & WS_EX_LAYERED)
				continue;

			const size_t kClassLength = 256;
			WCHAR class_name[kClassLength];
			int class_name_length = GetClassName(hwndCurrent, class_name, kClassLength);
			if (GetNTDLLVersion() >= 602 &&
				(wcscmp(class_name, L"ApplicationFrameWindow") == 0 || wcscmp(class_name, L"Windows.UI.Core.CoreWindow") == 0))
				continue;

			RECT clientRect;
			GetClientRect(hwndCurrent, &clientRect);

			std::string strName;
			String strWindowName;
			strWindowName.SetLength(GetWindowTextLength(hwndCurrent));
			GetWindowText(hwndCurrent, strWindowName, strWindowName.Length() + 1);

			DWORD exStyles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_EXSTYLE);
			DWORD styles = (DWORD)GetWindowLongPtr(hwndCurrent, GWL_STYLE);

			if (strWindowName.IsValid() && sstri(strWindowName, L"battlefield") != nullptr)
				exStyles &= ~WS_EX_TOOLWINDOW;

			if ((exStyles & WS_EX_TOOLWINDOW) == 0 && (styles & WS_CHILD) == 0 &&
				clientRect.bottom != 0 && clientRect.right != 0)
			{
				DWORD processID;
				GetWindowThreadProcessId(hwndCurrent, &processID);
				if (processID == GetCurrentProcessId())
					continue;

				if (!strWindowName.IsEmpty())
				{
					WcharToChar(strWindowName, strName);
					OneValue[Json::UInt(iCount++)]["ProcessName"] = strName.c_str();
				}
			}
		}
	} while (hwndCurrent = GetNextWindow(hwndCurrent, GW_HWNDNEXT));

	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end! OneValue.size() = %d", __FUNCTION__, OneValue.size());

	return OneValue.size() > 0 ? true : false;
}

int CSLiveManager::SLiveGetImgInfo(const char* ImgPath, UINT *Width, UINT *Height,char **Format)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!,Path = %s", __FUNCTION__,ImgPath);

	try
	{
		BUTEL_IFNULLRETURNERROR(ImgPath);
		BUTEL_IFNULLRETURNERROR(Width);
		BUTEL_IFNULLRETURNERROR(Height);
		BUTEL_IFNULLRETURNERROR(Format);

		D3DX11_IMAGE_INFO ii;
		if (FAILED(D3DX11GetImageInfoFromFileA(ImgPath, NULL, &ii, NULL)))
		{
			BUTEL_THORWERROR("获取图片信息失败 pic = %s", ImgPath);
		}

		*Width = ii.Width;
		*Height = ii.Height;
		SLiveAllowMemory((void**)Format, 7);
		memset(*Format, 0, 7);
		switch (ii.ImageFileFormat)
		{
		case D3DX11_IFF_BMP:
			strcpy(*Format, "bmp");
			break;
		case D3DX11_IFF_JPG:
			strcpy(*Format, "jpg");
			break;
		case D3DX11_IFF_DDS:
			strcpy(*Format, "dds");
			break;
		case D3DX11_IFF_GIF:
			strcpy(*Format, "gif");
			break;
		case D3DX11_IFF_PNG:
			strcpy(*Format, "png");
			break;
		case D3DX11_IFF_TIFF:
			strcpy(*Format, "tiff");
			break;
		case D3DX11_IFF_WMP:
			strcpy(*Format, "wmp");
			break;
		default:
			strcpy(*Format, "unkown");
			break;
		}
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Width = %d,Height = %d,Format = %s", __FUNCTION__,ii.Width,ii.Height,*Format);
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSelectStream(uint64_t iIntanceID, uint64_t iStreamID, bool bSelect)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! bSelect = %s", __FUNCTION__, bSelect ? "ture" : "false");
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->SelectStream(iStreamID, bSelect);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSwitchInstance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, TransFormType Type, UINT TransTime)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! iIntanceID_S = %llu,iIntanceID_D = %llu,Type = %d,TransTime = %d", __FUNCTION__, iIntanceID_S, iIntanceID_D, Type,TransTime);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process_S = m_InstanceList[iIntanceID_S];
		CInstanceProcess *Process_D = m_InstanceList[iIntanceID_D];
		LeaveCriticalSection(&MapInstanceSec);

		if (!bTransDisSolving && !bTransUpDown && !bTransDiffuse && !bRadius)
		{
			if(Type == Cut)
				SwitchInstanceCut(Process_S, Process_D);
			else if (Type == DisSolve)
			{
				SwitchInstanceDisSolve(Process_S, Process_D, TransTime);
			}
			else if(Type == UpDown || Type == Diffuse || Type == Radius)
			{
				SwitchInstanceUpDownOrDiffuse(Process_S, Process_D, Type);
			}
		}
		else
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "正在进行切换直播操作bTransDisSolving = %s,bTransUpDown = %s,bTransDiffuse = %s,bRadius = %s", bTransDisSolving ? "true":"false",
				bTransUpDown ? "true" : "false",
				bTransDiffuse ? "true" : "false",
				bRadius ? "true" : "false");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

void CSLiveManager::SwitchInstanceCut(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	BUTEL_IFNULLRETURNERROR(InstanceS);
	BUTEL_IFNULLRETURNERROR(InstanceD);

	InstanceD->ClearVideo(InstanceS->GetRenderCount() == 0, true);
	InstanceD->ClearAudio();

	ProcessSwitch(InstanceS, InstanceD, Cut);

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CSLiveManager::SwitchInstanceDisSolve(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD, UINT TransTime)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	BUTEL_IFNULLRETURNERROR(InstanceS);
	BUTEL_IFNULLRETURNERROR(InstanceD);

	if (!InstanceD->IsLiveInstance)
	{
		BUTEL_THORWERROR("使用淡出模式iIntanceID_D必须为直播实例");
	}

	if (TransTime < 100)
		TransTime = 100;

	TransFormTime = TransTime;

	InstanceD->ClearAudio();
	InstanceD->ClearVideoTransForm();
	InstanceD->ClearFilterTransForm();

	ProcessSwitch(InstanceS, InstanceD, DisSolve);

	TransEscapeTime = 0.0f;
	bTransDisSolving = true;

	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CSLiveManager::SwitchInstanceUpDownOrDiffuse(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD,TransFormType Type)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	BUTEL_IFNULLRETURNERROR(InstanceS);
	BUTEL_IFNULLRETURNERROR(InstanceD);

	if (!InstanceD->IsLiveInstance)
	{
		BUTEL_THORWERROR("使用淡出模式iIntanceID_D必须为直播实例");
	}

	InstanceD->ClearAudio();
	InstanceD->ClearVideoTransForm();

	ProcessSwitch(InstanceS, InstanceD, Type);

	TransEscapeTime = 0.0f;

	if (Type == UpDown)
	{
		bTransUpDown = true;
		TransFormTime = 500;
	}
	else if (Type == Diffuse)
	{
		bTransDiffuse = true;
		TransFormTime = 1000;
	}
	else if (Type == Radius)
	{
		bRadius = true;
		TransFormTime = 1000;
	}


	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}


void CSLiveManager::ProcessSwitch(CInstanceProcess *InstanceS, CInstanceProcess *InstanceD, TransFormType type)
{
	std::vector<AudioStruct> vAudioList;
	EnterCriticalSection(&InstanceS->VideoSection);
	for (int i = 0; i < InstanceS->m_VideoList.Num(); ++i)
	{
		VideoStruct &VSTem = InstanceS->m_VideoList[i];

		if (VSTem.bRender)
		{
			if (type == Cut)
			{
// 				if (strcmp(VSTem.VideoStream->GainClassName(), "AgentSource") == 0)
// 				{
// 					//区域占位源重新生成一个
// 					IBaseVideo *AgentSource = dynamic_cast<IBaseVideo*>(CreatStreamObject("AgentSource"));
// 					if (AgentSource)
// 					{
// 						AgentSource->Init(*VSTem.Config);
// 						VideoStruct VSAgent;
// 
// 						VSAgent.bRender = true;
// 						VSAgent.AudioStream = VSTem.AudioStream;
// 						VSAgent.bGlobalStream = VSTem.bGlobalStream;
// 						VSAgent.bScale = VSTem.bScale;
// 						VSAgent.Config = VSTem.Config;
// 						VSAgent.Crop = VSTem.Crop;
// 						VSAgent.pos = VSTem.pos;
// 						VSAgent.size = VSTem.size;
// 						VSAgent.VideoStream = shared_ptr<IBaseVideo>(AgentSource);
// 
// 						EnterCriticalSection(&InstanceD->VideoSection);
// 						InstanceD->m_VideoList.SetSize(InstanceD->m_VideoList.Num() + 1);
// 						VideoStruct &VS = InstanceD->m_VideoList[InstanceD->m_VideoList.Num() - 1];
// 						VS = VSAgent;
// 						VS.bSelect = false;
// 						LeaveCriticalSection(&InstanceD->VideoSection);
// 
// 						VSAgent.VideoStream->GlobalSourceEnterScene();
// 					}
// 				}
// 				else
				{
					EnterCriticalSection(&InstanceD->VideoSection);
					InstanceD->m_VideoList.SetSize(InstanceD->m_VideoList.Num() + 1);
					VideoStruct &VS = InstanceD->m_VideoList[InstanceD->m_VideoList.Num() - 1];
					VS = VSTem;
					VS.bSelect = false;
					LeaveCriticalSection(&InstanceD->VideoSection);
				}
			}
			else
			{
				InstanceD->m_VideoListTransForm.SetSize(InstanceD->m_VideoListTransForm.Num() + 1);
				VideoStruct &VS = InstanceD->m_VideoListTransForm[InstanceD->m_VideoListTransForm.Num() - 1];
				VS = VSTem;
				VS.bSelect = false;
			}

			if (strcmp(VSTem.VideoStream->GainClassName(), "AgentSource") == 0)
			{
				VSTem.VideoStream->GlobalSourceEnterScene();
				IBaseVideo *BaseVideo = VSTem.VideoStream->GetGlobalSource();
				if (BaseVideo)
				{
					bool bFind = false;
					for (int i = 0; i < m_InstanceList.GetSize(); ++i)
					{
						CInstanceProcess *Process = m_InstanceList.GetAt(i);

						if (Process && Process->bLittlePre)
						{
							for (int j = 0; j < Process->m_VideoList.Num(); ++j)
							{
								VideoStruct &OneVideo = Process->m_VideoList[j];

								if (OneVideo.VideoStream.get() == BaseVideo)
								{
									for (int k = 0; k < Process->m_AudioList.Num(); ++k)
									{
										if (Process->m_AudioList[k].VideoStream == BaseVideo)
										{
											AudioStruct &ASTem = Process->m_AudioList[k];

											EnterCriticalSection(&InstanceD->AudioSection);

											bool bFind = false;
											for (int l = 0; l < InstanceD->m_AudioList.Num(); ++l)
											{
												AudioStruct &OneAudio = InstanceD->m_AudioList[l];

												if (OneAudio.AudioStream.get() == ASTem.AudioStream.get())
												{
													if (InstanceD->IsLiveInstance)
														OneAudio.AudioStream->SetLiveInstance(true);
													bFind = true;
													break;
												}

											}
											if (!bFind)
											{
												InstanceD->m_AudioList.SetSize(InstanceD->m_AudioList.Num() + 1);
												AudioStruct &AS = InstanceD->m_AudioList[InstanceD->m_AudioList.Num() - 1];
												AS = ASTem;
												if (InstanceD->IsLiveInstance)
													AS.AudioStream->SetLiveInstance(true);
											}

											LeaveCriticalSection(&InstanceD->AudioSection);
											break;
										}
									}

									bFind = true;
									break;
								}

							}
						}

						if (bFind)
							break;
					}

				}
			}
		}
		else
		{

			int iRefCount = 0;
			if (strcmp(VSTem.VideoStream->GainClassName(), "DeviceSource") == 0)
			{
				for (int i = 0; i < SharedDevice::VideoList.size(); ++i)
				{
					ShardVideo& OneVideo = SharedDevice::VideoList[i];

					bool bFind = false;
					if (OneVideo.VideoStream->GetDeviceID())
					{
						if (strcmp(OneVideo.VideoStream->GetDeviceName(), VSTem.VideoStream->GetDeviceName()) == 0 && strcmp(OneVideo.VideoStream->GetDeviceID(), VSTem.VideoStream->GetDeviceID()) == 0)
						{
							++iRefCount;
						}
					}
					else
					{
						if (strcmp(OneVideo.VideoStream->GetDeviceName(), VSTem.VideoStream->GetDeviceName()) == 0)
						{
							++iRefCount;
						}
					}
				}
			}

			int Ref = type == Cut ? 2 : 3;

			if (VSTem.bGlobalStream && (VSTem.VideoStream.use_count() - iRefCount) == Ref)
			{
				IBaseVideo *BaseVideo = VSTem.VideoStream.get();

				if (BaseVideo)
				{
					bool bAgent = strcmp(VSTem.VideoStream->GainClassName(), "AgentSource") == 0;
					if (!bAgent)
					{
						if (!BaseVideo->CanEnterScene())
						{
							BaseVideo->GlobalSourceLeaveScene();
							if (VSTem.VideoDevice)
							{
								VSTem.VideoDevice->GlobalSourceLeaveScene();
								VSTem.VideoDevice->SetCanEnterScene(true);
							}
							BaseVideo->SetCanEnterScene(true);
						}
					}

						//在这里将对应区域占位源中的音频删除掉
					if (bAgent)
					{
						IBaseVideo *BaseVideo = VSTem.VideoStream->GetGlobalSource();
						//如果在场景中有区域占位源所代理的真正的源则不删除
						bool bStreamFind = false;
						if (BaseVideo)
						{
							for (int m = 0; m < InstanceS->m_VideoList.Num(); ++m)
							{
								VideoStruct &VSTem = InstanceS->m_VideoList[m];

								if (VSTem.VideoStream && VSTem.VideoStream.get() == BaseVideo)
								{
									bStreamFind = true;
									break;
								}
							}
						}

						if (BaseVideo && !bStreamFind)
							{
								bool bFind = false;
								for (int i = 0; i < m_InstanceList.GetSize(); ++i)
								{
									CInstanceProcess *Process = m_InstanceList.GetAt(i);

									if (Process && Process->bLittlePre)
									{
										for (int j = 0; j < Process->m_VideoList.Num(); ++j)
										{
											VideoStruct &OneVideo = Process->m_VideoList[j];

											if (OneVideo.VideoStream.get() == BaseVideo)
											{
												for (int k = 0; k < Process->m_AudioList.Num(); ++k)
												{
													if (Process->m_AudioList[k].VideoStream == BaseVideo)
													{
														AudioStruct &ASTem = Process->m_AudioList[k];

														EnterCriticalSection(&InstanceS->AudioSection);

														bool bFind = false;
														for (int l = 0; l < InstanceS->m_AudioList.Num(); ++l)
														{
															AudioStruct &OneAudio = InstanceS->m_AudioList[l];

															if (OneAudio.AudioStream.get() == ASTem.AudioStream.get())
															{
																vAudioList.push_back(OneAudio);

																OneAudio.AudioStream.reset();

																if (OneAudio.Config)
																	OneAudio.Config.reset();

																InstanceS->m_AudioList.Remove(l);

																bFind = true;
																break;
															}

														}

														LeaveCriticalSection(&InstanceS->AudioSection);
														break;
													}
												}

												bFind = true;
												break;
											}

										}
									}

									if (bFind)
										break;
								}

							}
						}
				}
			}
		}

	}

	for (int i = 0; i < InstanceS->m_Filter.Num(); ++i)
	{
		Filter &VSTem = InstanceS->m_Filter[i];

		EnterCriticalSection(&InstanceD->VideoSection);
		if (type == Cut)
		{
			bool bFind = false;
			for (int j = 0; j < InstanceD->m_Filter.Num(); ++j)
			{
				Filter &ASTemD = InstanceD->m_Filter[j];

				if (ASTemD.IVideo == VSTem.IVideo)
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
			{
				InstanceD->m_Filter.SetSize(InstanceD->m_Filter.Num() + 1);
				Filter &VS = InstanceD->m_Filter[InstanceD->m_Filter.Num() - 1];
				VS = VSTem;
			}
		}
		else
		{
			bool bFind = false;
			for (int j = 0; j < InstanceD->m_FilterTransForm.Num(); ++j)
			{
				Filter &ASTemD = InstanceD->m_FilterTransForm[j];

				if (ASTemD.IVideo == VSTem.IVideo)
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
			{
				InstanceD->m_FilterTransForm.SetSize(InstanceD->m_FilterTransForm.Num() + 1);
				Filter &VS = InstanceD->m_FilterTransForm[InstanceD->m_FilterTransForm.Num() - 1];
				VS = VSTem;
			}
		}
		LeaveCriticalSection(&InstanceD->VideoSection);
	}

	LeaveCriticalSection(&InstanceS->VideoSection);


	EnterCriticalSection(&InstanceS->AudioSection);
	for (int i = 0; i < InstanceS->m_AudioList.Num(); ++i)
	{
		AudioStruct &ASTem = InstanceS->m_AudioList[i];

		EnterCriticalSection(&InstanceD->AudioSection);

		bool bFind = false;
		for (int l = 0; l < InstanceD->m_AudioList.Num(); ++l)
		{
			AudioStruct &OneAudio = InstanceD->m_AudioList[l];

			if (OneAudio.AudioStream.get() == ASTem.AudioStream.get())
			{
				if (InstanceD->IsLiveInstance)
					OneAudio.AudioStream->SetLiveInstance(true);
				bFind = true;
				break;
			}

		}
		if (!bFind)
		{
			InstanceD->m_AudioList.SetSize(InstanceD->m_AudioList.Num() + 1);
			AudioStruct &AS = InstanceD->m_AudioList[InstanceD->m_AudioList.Num() - 1];
			AS = ASTem;
			if (InstanceD->IsLiveInstance)
				AS.AudioStream->SetLiveInstance(true);
		}

		LeaveCriticalSection(&InstanceD->AudioSection);
	}


	for (auto Audio : vAudioList)//去除之后再加回来
	{
		InstanceS->m_AudioList.SetSize(InstanceS->m_AudioList.Num() + 1);
		AudioStruct &AS = InstanceS->m_AudioList[InstanceS->m_AudioList.Num() - 1];
		AS = Audio;
	}
	LeaveCriticalSection(&InstanceS->AudioSection);

	vAudioList.clear();
}

int CSLiveManager::SLiveAdd2Intance(uint64_t iIntanceID_S, uint64_t iIntanceID_D, VideoArea *Area, bool bRender)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! iIntanceID_S = %llu,iIntanceID_D = %llu", __FUNCTION__, iIntanceID_S, iIntanceID_D);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		
		if (!bStartView)
		{
			if (__SDIOutInfo)
				BlackMagic::Instance()->ApplySDISettings(__SDIOutInfo,true);
			bStartView = true;
		}	

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *InstanceS = m_InstanceList[iIntanceID_S];
		CInstanceProcess *InstanceD = m_InstanceList[iIntanceID_D];
		LeaveCriticalSection(&MapInstanceSec);

		BUTEL_IFNULLRETURNERROR(InstanceS);
		BUTEL_IFNULLRETURNERROR(InstanceD);

		//EnterCriticalSection(&InstanceS->VideoSection);

		Log::writeMessage(LOG_RTSPSERV, 1, "InstanceS.Num %d", InstanceS->m_VideoList.Num());

		for (int i = 0; i < InstanceS->m_VideoList.Num(); ++i)
		{
			VideoStruct &VSTem = InstanceS->m_VideoList[i];
			bool bFind = false;
			EnterCriticalSection(&InstanceD->VideoSection);
			for (int j = 0; j < InstanceD->m_VideoList.Num();++j)
			{
				VideoStruct &VSTemD = InstanceD->m_VideoList[j];

				if (VSTemD.VideoStream.get() == VSTem.VideoStream.get())
				{
					bFind = true;
					break;
				}
			}
			if (!bFind)
			{
				try
				{
					BUTEL_IFNULLRETURNERROR(Area);
				}
				catch (CErrorBase &e)
				{
					LeaveCriticalSection(&InstanceD->VideoSection);
					BUTEL_THORWERROR(e.m_Error.c_str());
				}
				


				Log::writeMessage(LOG_RTSPSERV, 1, "添加InstanceD.Num %d", InstanceD->m_VideoList.Num());

				InstanceD->m_VideoList.SetSize(InstanceD->m_VideoList.Num() + 1);
				VideoStruct &VS = InstanceD->m_VideoList[InstanceD->m_VideoList.Num() - 1];
				VS = VSTem;
				
				VS.pos.x = Area->left;
				VS.pos.y = Area->top;
				VS.size.x = Area->width;
				VS.size.y = Area->height;

				VS.Crop.x = Area->CropLeft;
				VS.Crop.y = Area->CropTop;
				VS.Crop.w = Area->CropRight;
				VS.Crop.z = Area->CropBottom;
				VS.bSelect = false;
				VS.bRender = bRender;

				if (bRender)
				{
					if (VS.bGlobalStream && VS.VideoStream->CanEnterScene())
					{
						VS.VideoStream->BeginScene();
					}
// 					else if (VS.VideoStream->GetGlobalSource())
// 					{
// 						if (VS.VideoStream->GetGlobalSource()->CanEnterScene())
// 						{
// 							VS.VideoStream->GetGlobalSource()->BeginScene();
// 						}
// 					}
					else if (VS.bGlobalStream)
					{
						VS.VideoStream->SwitchSences();
					}
				}

				LeaveCriticalSection(&InstanceD->VideoSection);
				if (bRender)
				{
					bool bAgent = strcmp(VS.VideoStream->GainClassName(), "AgentSource") == 0;

					if (bAgent)//区域占位源进入场景
					{
						if (VS.VideoStream)
						{
							VS.VideoStream->GlobalSourceEnterScene();
						}
					}	
					else
					{
						if (VS.bGlobalStream && VS.VideoStream->CanEnterScene())
						{
							VS.VideoStream->GlobalSourceEnterScene();
							if (VS.VideoDevice)
							{
								VS.VideoDevice->GlobalSourceEnterScene();
								VS.VideoDevice->SetCanEnterScene(false);
							}
							VS.VideoStream->SetCanEnterScene(false);
						}
						else if (VS.bGlobalStream)
						{
							VS.VideoStream->SwitchSences();
						}
					}
					

					if (bAgent)//打区域占位的源的音频
					{
						IBaseVideo *GlobalVideo = VS.VideoStream->GetGlobalSource();

						if (GlobalVideo && strcmp(GlobalVideo->GainClassName(),"DeviceSource") != 0) //不是摄像头（摄像头中没有音频）
						{

							for (int i = 0; i < m_InstanceList.GetSize(); ++i)
							{
								CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);
								bool bAdd = false;
								if (OneProcess->bLittlePre && !OneProcess->bNoPreView)
								{
									for (int j = 0; j < OneProcess->m_AudioList.Num(); ++j)
									{
										AudioStruct &OneAuido = OneProcess->m_AudioList[j];

										if (OneAuido.VideoStream == GlobalVideo)
										{
											EnterCriticalSection(&InstanceD->AudioSection);
											bool bFind = false;
											for (int j = 0; j < InstanceD->m_AudioList.Num(); ++j)
											{
												AudioStruct &ASTemD = InstanceD->m_AudioList[j];

												if (ASTemD.AudioStream.get() == OneAuido.AudioStream.get())
												{
													bFind = true;
													break;
												}
											}
											if (!bFind && bRender)
											{
												InstanceD->m_AudioList.SetSize(InstanceD->m_AudioList.Num() + 1);
												AudioStruct &AS = InstanceD->m_AudioList[InstanceD->m_AudioList.Num() - 1];
												AS = OneAuido;
												if (AS.AudioStream.use_count() == 2)
													AS.AudioStream->SetLiveInstance(false);
											}
											LeaveCriticalSection(&InstanceD->AudioSection);

											bAdd = true;
											break;
										}
										
									}
								}

								if (bAdd)
									break;
							}


						}
					}
				}
// 				else
// 				{
// 					if (strcmp(VS.VideoStream->GainClassName(), "AgentSource") == 0)
// 					{
// 						if (VS.VideoStream->GetGlobalSource() && !VS.VideoStream->CanEnterScene())
// 						{
// 							VS.VideoStream->GlobalSourceLeaveScene();
// 							VS.VideoStream->SetCanEnterScene(true);
// 						}
// 					}
// 				}
			}
			else
			{
				LeaveCriticalSection(&InstanceD->VideoSection);
			}

			

		}
		//LeaveCriticalSection(&InstanceS->VideoSection);


		for (int i = 0; i < InstanceS->m_Filter.Num(); ++i)
		{
			Filter &VSTem = InstanceS->m_Filter[i];

			EnterCriticalSection(&InstanceD->VideoSection);

			bool bFind = false;
			for (int j = 0; j < InstanceD->m_Filter.Num(); ++j)
			{
				Filter &ASTemD = InstanceD->m_Filter[j];

				if (ASTemD.IVideo == VSTem.IVideo)
				{
					bFind = true;
					break;
				}
			}

			if (!bFind)
			{
				InstanceD->m_Filter.SetSize(InstanceD->m_Filter.Num() + 1);
				Filter &VS = InstanceD->m_Filter[InstanceD->m_Filter.Num() - 1];
				VS = VSTem;
			}
			LeaveCriticalSection(&InstanceD->VideoSection);
		}

		//EnterCriticalSection(&InstanceS->AudioSection);


		for (int i = 0; i < InstanceS->m_AudioList.Num(); ++i)
		{
			AudioStruct &ASTem = InstanceS->m_AudioList[i];

			EnterCriticalSection(&InstanceD->AudioSection);
			bool bFind = false;
			for (int j = 0; j < InstanceD->m_AudioList.Num(); ++j)
			{
				AudioStruct &ASTemD = InstanceD->m_AudioList[j];

				if (ASTemD.AudioStream.get() == ASTem.AudioStream.get())
				{
					bFind = true;
					break;
				}
			}
			if (!bFind && bRender)
			{
				InstanceD->m_AudioList.SetSize(InstanceD->m_AudioList.Num() + 1);
				AudioStruct &AS = InstanceD->m_AudioList[InstanceD->m_AudioList.Num() - 1];
				AS = ASTem;
				if(AS.AudioStream.use_count() == 2) 
					AS.AudioStream->SetLiveInstance(false);
			}
			LeaveCriticalSection(&InstanceD->AudioSection);
		}
		//LeaveCriticalSection(&InstanceS->AudioSection);

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveConfigStream(uint64_t iIntanceID, uint64_t iStreamID, const char *cJson)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->ConfigStream(iStreamID,cJson);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveClearIntances(uint64_t iIntanceID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		char Tem[50] = { 0 };
		sprintf(Tem, "%llu", iIntanceID);

		//这里不合理
		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		m_InstanceList.Remove(Tem);
		LeaveCriticalSection(&MapInstanceSec);


		if (Process)
		{
			Process->ClearAudio();
			Process->ClearVideo();

			EnterCriticalSection(&MapInstanceSec);
			m_InstanceList[iIntanceID] = Process;
			LeaveCriticalSection(&MapInstanceSec);

			if (Process == LiveInstance)
			{
				DumpProfileData();
				bStartView = false;
			}

		}
		else
		{
			BUTEL_THORWERROR("没有找到IntanceID %llu", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

bool CSLiveManager::GetNameList(Value &data)
{
	Json::Value &ArryList = data["Namelist"];
	UINT iCount = 0;

	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *Process = m_InstanceList.GetAt(i);

		if (Process && Process->bLittlePre && !Process->bNoPreView)
		{
			for (int j = 0; j < Process->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneVideo = Process->m_VideoList[j];

				ArryList[iCount]["Name"] = (*OneVideo.Config)["Name"].asString().c_str();
				ArryList[iCount]["SourceID"] = (*OneVideo.Config)["SourceID"].asString().c_str();
				if (!(*OneVideo.Config)["DeviceSourceID"].isNull())
				{
					ArryList[iCount]["DeviceSourceID"] = (*OneVideo.Config)["DeviceSourceID"].asString().c_str();
				}

				++iCount;
			}
		}

	}
	return ArryList.size() > 0;
}

bool CSLiveManager::ScanSourceElmentByClassName(const char *ClassName, List<Value>& List)
{
	if (!ClassName)
		return false;

	UINT iCount = 0;
	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *Process = m_InstanceList.GetAt(i);

		if (Process && !Process->bLittlePre && (Process->IsLiveInstance == bNeedAgentInPGM))
		{
			for (int j = 0; j < Process->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneVideo = Process->m_VideoList[j];

				if (OneVideo.bRender && strcmp(OneVideo.VideoStream->GainClassName(), ClassName) == 0)
				{
					List.SetSize(List.Num() + 1);
					List[iCount]["Name"] = (*OneVideo.Config)["Name"].asString().c_str();
					List[iCount]["SecName"] = (*OneVideo.Config)["SecName"].asString().c_str();
					List[iCount]["SourceID"] = (*OneVideo.Config)["SourceID"].asString().c_str();
					if (!(*OneVideo.Config)["DeviceSourceID"].isNull())
					{
						List[iCount]["DeviceSourceID"] = (*OneVideo.Config)["DeviceSourceID"].asString().c_str();
					}

					++iCount;
				}
			}
			break;
		}

	}

	return List.Num() > 0;
}

int CSLiveManager::SLiveReNameStream(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! bSelect", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		BUTEL_IFNULLRETURNERROR(NewName);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			string &OldName = Process->ReNameStream(iStreamID, NewName);

			if (!OldName.empty())
			{
				//重命名所有用到该流的区域占位源
				for (int i = 0; i < m_InstanceList.GetSize(); ++i)
				{
					CInstanceProcess *Process = m_InstanceList.GetAt(i);

					if (Process && Process->bLittlePre && Process->bNoPreView)
					{
						for (int j = 0; j < Process->m_VideoList.Num(); ++j)
						{
							VideoStruct &OneVideo = Process->m_VideoList[j];
							//找到区域占位源
							if (0 == strcmp(OneVideo.VideoStream->GainClassName(), "AgentSource"))
							{
								if (0 == strcmp((*OneVideo.Config)["SecName"].asString().c_str(), OldName.c_str()))
								{
									(*OneVideo.Config)["SecName"] = NewName;
								}

							}
						}
					}

				}
			}
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveAdd2Agent(const char *StreamName, bool bAdd2PGM)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! bSelect", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		BUTEL_IFNULLRETURNERROR(StreamName);

		CONFINGFUN fp = GetConfigFunc("ConfigArea");
		if (fp)
		{
			Value data;
			data["Name"] = StreamName;

			bNeedAgentInPGM = bAdd2PGM;

// 			for (int i = 0; i < m_InstanceList.GetSize(); ++i)
// 			{
// 				CInstanceProcess *Process = m_InstanceList.GetAt(i);
// 
// 				if (Process && !Process->bLittlePre && !Process->IsLiveInstance)
// 				{
					char TemID[50] = { 0 };
					if (!bAdd2PGM)
						sprintf_s(TemID, "%llu", (uint64_t)LocalInstance);
					else
					{
						sprintf_s(TemID, "%llu", (uint64_t)LiveInstance);
					}
					data["IntanceID"] = TemID;
//					break;
// 				}
// 
// 			}

			fp(data, false);
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::ReNameStreamSec(uint64_t iIntanceID, uint64_t iStreamID, const char *NewName)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! bSelect", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}
		BUTEL_IFNULLRETURNERROR(NewName);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iIntanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			Process->ReNameStreamSec(iStreamID, NewName);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetStreamSize(uint64_t iIntanceID, uint64_t StreamID, UINT *Width, UINT *Height)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Width);
		BUTEL_IFNULLRETURNERROR(Height);

		CInstanceProcess *Process = m_InstanceList[iIntanceID];

		if (Process)
		{
			Process->GetStreamSize(StreamID, Width, Height);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iIntanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveDisplayDevices(char **DevicesList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		BUTEL_IFNULLRETURNERROR(DevicesList);

		Json::Value JsonDisplayDevicesList;

		DeviceOutputs OutPuts;
		GetDisplayDevices(OutPuts);

		if (OutPuts.devices.Num() > 0)
		{
			Value &ArryList = JsonDisplayDevicesList["NameList"];
			for (int i = 0; i < OutPuts.devices.Num(); ++i)
			{
				ArryList[i] = WcharToAnsi(OutPuts.devices[i].strDevice.Array()).c_str();
			}
			std::string &str = JsonDisplayDevicesList.toStyledString();
			*DevicesList = new char[str.length() + 1];
			(*DevicesList)[str.length()] = 0;
			memcpy(*DevicesList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("DevicesList = 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetCropping(uint64_t iInstanceID, uint64_t iStreamID, float left, float top, float right, float bottom)
{
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iInstanceID];
		LeaveCriticalSection(&MapInstanceSec);

		if (Process)
		{
			if (left < 0.0f)
				left = 0.0f;
			if (top < 0.0f)
				top = 0.0f;
			if (right < 0.0f)
				right = 0.0f;
			if (bottom < 0.0f)
				bottom = 0.0f;

			Process->SetCrop(iStreamID, left,top,right,bottom);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iInstanceID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	//Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetSoundAndLocalMinitorParam(SoundAndLocalMinitor *SoundParam)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(SoundParam);

		desktopVol = SoundParam->fMix;
		leftdesktopVol = SoundParam->fLeft;
		rightdesktopVol = SoundParam->fRight;
		m_quotietyVolume = SoundParam->fQuotietyVolume;
		m_bPlayPcmLocal = SoundParam->bPlayLocal;
		m_bPlayPcmLive = SoundParam->bPlayLocalLive;
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

void CSLiveManager::ActuallyEnableProjector()
{
	DisableProjector();

	SetWindowPos(hwndProjector, NULL, projectorX, projectorY, projectorWidth, projectorHeight, SWP_SHOWWINDOW);

	projectorTexture = m_D3DRender->CreateRenderTargetSwapChain(hwndProjector, projectorWidth, projectorHeight);

	m_bProject = true;

	bPleaseEnableProjector = false;
}

void CSLiveManager::EnableProjector(UINT monitorID)
{
	MonitorInfo mi;
	 if (monitorID >= monitors.Num())
	 {
		 mi = monitors[0];
	 }
	 else
	 {
		 mi = monitors[monitorID];
	 }

	projectorMonitorID = monitorID;
	projectorWidth = mi.rect.right - mi.rect.left;
	projectorHeight = mi.rect.bottom - mi.rect.top;
	projectorX = mi.rect.left;
	projectorY = mi.rect.top;

	bPleaseEnableProjector = true;

}

void CSLiveManager::DisableProjector()
{
	if (projectorTexture)
	{
		delete projectorTexture;
		projectorTexture = NULL;
	}

	m_bProject = false;
	bPleaseDisableProjector = false;

	ShowWindow(hwndProjector, SW_HIDE);
}

LRESULT CALLBACK CSLiveManager::ProjectorFrameProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			CSLiveManager::GetInstance()->bPleaseDisableProjector = true;
		break;

	case WM_CLOSE:
		CSLiveManager::GetInstance()->bPleaseDisableProjector = true;
		break;

	case WM_SETCURSOR:
		if (CSLiveManager::GetInstance()->bEnableProjectorCursor)
		{
			SetFocus(hwnd);
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
		else
		{
			SetFocus(hwnd);
			SetCursor(NULL);
		}
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}

	return 0;
}

void CSLiveManager::DrawPreviewProject(Texture* Prev, const Vect2 &renderSize, const Vect2 &renderOffset, const Vect2 &renderCtrlSize)
{
	m_D3DRender->SetRenderTarget(projectorTexture);
	m_D3DRender->Ortho(0.0f, renderCtrlSize.x, renderCtrlSize.y, 0.0f, -100.0f, 100.0f);
	m_D3DRender->SetViewport(0.0f, 0.0f, renderCtrlSize.x, renderCtrlSize.y);

	m_D3DRender->ClearRenderTarget(0x000000);

	m_D3DRender->DrawSprite(Prev, 0xFFFFFFFF,
		renderOffset.x, renderOffset.y,
		renderOffset.x + renderSize.x, renderOffset.y + renderSize.y);


	m_D3DRender->Present(projectorTexture);
	m_D3DRender->SetRenderTarget(NULL);
}

int CSLiveManager::SLiveGetMinitorNum(UINT *Num)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(Num);

		monitors.Clear();
		EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)MonitorInfoEnumProc, (LPARAM)&monitors);

		*Num = monitors.Num();

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Num = %d", __FUNCTION__, *Num);
	return 0;
}

int CSLiveManager::SLiveEnableProjector(UINT monitorID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin! monitorID = %d", __FUNCTION__, monitorID);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnableProjector(monitorID);
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveDisableProjector()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		DisableProjector();
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetPlayPreAudio(uint64_t iInstansID, uint64_t iStreamID, bool *bRet)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(bRet);

		CInstanceProcess *Process = m_InstanceList[iInstansID];

		if (Process)
		{
			if (Process->bLittlePre && !Process->bNoPreView)
			{
				Process->SetPlayPreAudio(iStreamID, bRet);
			}
			else
			{
				BUTEL_THORWERROR("实例ID为 %llu 不是为小预览源或是小预览但不带音视频 ", iInstansID);
			}
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iInstansID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetSenceToBackUp()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		if (LiveInstance)
		{
			LiveInstance->ChangeToBackup();
		}
		else
		{
			BUTEL_THORWERROR("LiveInstance 为空");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetPreConfig(char **ConfigList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(ConfigList);
		Json::Value JsonConfigList;

		Value &ArryList = JsonConfigList["ConfigList"];

		int iCount = 0;
		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *Process = m_InstanceList.GetAt(i);

			if (Process && Process->bLittlePre)
			{
				for (int j = 0; j < Process->m_VideoList.Num(); ++j)
				{
					VideoStruct &OneVideo = Process->m_VideoList[j];

					char Tem[50] = { 0 };
					sprintf_s(Tem, "%llu", (uint64_t)Process);
					ArryList[iCount]["InstanceID"] = Tem;
					sprintf_s(Tem, "%llu", (uint64_t)Process->m_VideoList[j].VideoStream.get());
					ArryList[iCount]["StreamID"] = Tem;
					ArryList[iCount]["data"] = *Process->m_VideoList[j].Config;
					++iCount;
				}
			}
		}

		if (ArryList.size() > 0)
		{
			std::string &str = JsonConfigList.toStyledString();
			*ConfigList = new char[str.length() + 1];
			(*ConfigList)[str.length()] = 0;
			memcpy(*ConfigList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("ArryList == 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetPluginsName(char **NameList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(NameList);
		Json::Value JsonNameList;

		GetPluginsName(JsonNameList);

		Value &ArryList = JsonNameList["NameList"];

		if (ArryList.size() > 0)
		{
			std::string &str = JsonNameList.toStyledString();
			*NameList = new char[str.length() + 1];
			(*NameList)[str.length()] = 0;
			memcpy(*NameList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("ArryList == 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

void CSLiveManager::RemoveLiveInstanceAudio(IBaseVideo *BaseVideo, bool bMustDel, IBaseVideo *NewGobal)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);

	if (!BaseVideo)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! BaseVideo为空", __FUNCTION__);
		return;
	}

	RemoveFilterFromPGMOrPVM(BaseVideo);


	if (!BaseVideo->GetAudioRender())
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Audio为空", __FUNCTION__);
		return;
	}

	if (LiveInstance)
	{
		bool bFind = false;
		bool bAgentFind = false;
		EnterCriticalSection(&LiveInstance->VideoSection);
		for (int i = 0; i < LiveInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LiveInstance->m_VideoList[i];

			if (OneVideo.VideoStream)
			{
				if (OneVideo.VideoStream.get() == BaseVideo || (OneVideo.VideoDevice && OneVideo.VideoDevice.get() == BaseVideo))
				{
					bFind = true;
					break;
				}
				IBaseVideo *Global = NULL;
				if (Global = OneVideo.VideoStream->GetGlobalSource())
				{
// 					if (NewGobal == Global)
// 					{
// 						BaseVideo->GlobalSourceLeaveScene();
// 					}

					//找到区域占位源
					if (Global == BaseVideo)
					{
						bAgentFind = true;
						break;
					}
				}
			}

		}
		LeaveCriticalSection(&LiveInstance->VideoSection);

		EnterCriticalSection(&LiveInstance->AudioSection);
		for (int i = 0; i < LiveInstance->m_AudioList.Num(); ++i)
		{
			AudioStruct &OneAudio = LiveInstance->m_AudioList[i];

			if (OneAudio.AudioStream.get() == BaseVideo->GetAudioRender())
			{
				if (!bAgentFind && !bFind)//在直播中没有找到
				{
					OneAudio.AudioStream->SetLiveInstance(false);
				}

				//这里不要删除，延后删除

				if (bMustDel)
				{
					OneAudio.AudioStream.reset();
					LiveInstance->m_AudioList.Remove(i);
				}
				
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s 找到Audio", __FUNCTION__);
				break;
			}
		}
		LeaveCriticalSection(&LiveInstance->AudioSection);
	}

	if (LocalInstance)
	{
		EnterCriticalSection(&LocalInstance->AudioSection);
		for (int i = 0; i < LocalInstance->m_AudioList.Num(); ++i)
		{
			AudioStruct &OneAudio = LocalInstance->m_AudioList[i];

			if (OneAudio.AudioStream.get() == BaseVideo->GetAudioRender())
			{
				bool bRemove = false;
				if (OneAudio.AudioStream.use_count() == 2)
				{
					OneAudio.AudioStream.reset();
					LocalInstance->m_AudioList.Remove(i);
					bRemove = true;
				}
				Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s LocalInstance找到Audio bRemove %s", __FUNCTION__,bRemove ? "true" : "false");
				break;
			}
		}
		LeaveCriticalSection(&LocalInstance->AudioSection);
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

void CSLiveManager::AddLiveInstanceAudio(IBaseVideo *Video,IBaseVideo *Agent)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	if (!Video && !Agent)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! Video为空", __FUNCTION__);
		return;
	}

	AddFilter2PGMOrPVM(Video);

	if (LiveInstance && LiveInstance->m_VideoList.Num())
	{
		bool bFind = false;
		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *Process = m_InstanceList.GetAt(i);

			if (Process && Process->bLittlePre)
			{
				for (int j = 0; j < Process->m_VideoList.Num(); ++j)
				{
					VideoStruct &OneVideo = Process->m_VideoList[j];

					if (OneVideo.VideoStream.get() == Video)
					{
						for (int k = 0; k < Process->m_AudioList.Num(); ++k)
						{
							if (Process->m_AudioList[k].VideoStream == Video)
							{
								AudioStruct &ASTem = Process->m_AudioList[k];

								for (int n = 0; n < LiveInstance->m_VideoList.Num(); ++n)
								{
									VideoStruct &LiveVideo = LiveInstance->m_VideoList[n];

									if (LiveVideo.VideoStream.get() == Agent)
									{
										//Video->GlobalSourceEnterScene();//直播中再进入一次
										EnterCriticalSection(&LiveInstance->AudioSection);

										bool bFind = false;
										for (int j = 0; j < LiveInstance->m_AudioList.Num(); ++j)
										{
											AudioStruct &ASTemD = LiveInstance->m_AudioList[j];

											if (ASTemD.AudioStream.get() == ASTem.AudioStream.get())
											{
												bFind = true;
												break;
											}
										}
										if (!bFind)
										{
											LiveInstance->m_AudioList.SetSize(LiveInstance->m_AudioList.Num() + 1);
											AudioStruct &AS = LiveInstance->m_AudioList[LiveInstance->m_AudioList.Num() - 1];
											AS = ASTem;
											AS.AudioStream->SetLiveInstance(true);
										}
										LeaveCriticalSection(&LiveInstance->AudioSection);
										break;
									}

								}
								//这里又放开了
								for (int n = 0; n < LocalInstance->m_VideoList.Num(); ++n)
								{
									VideoStruct &LiveVideo = LocalInstance->m_VideoList[n];

									if (LiveVideo.VideoStream.get() == Agent)
									{
										EnterCriticalSection(&LocalInstance->AudioSection);

										bool bFind = false;
										for (int j = 0; j < LocalInstance->m_AudioList.Num(); ++j)
										{
											AudioStruct &ASTemD = LocalInstance->m_AudioList[j];

											if (ASTemD.AudioStream.get() == ASTem.AudioStream.get())
											{
												bFind = true;
												break;
											}
										}
										if (!bFind)
										{
											LocalInstance->m_AudioList.SetSize(LocalInstance->m_AudioList.Num() + 1);
											AudioStruct &AS = LocalInstance->m_AudioList[LocalInstance->m_AudioList.Num() - 1];
											AS = ASTem;
										}
										LeaveCriticalSection(&LocalInstance->AudioSection);
										break;
									}

								}

								
								break;
							}
						}

						bFind = true;
						break;
					}

				}
			}

			if (bFind)
				break;
		}

	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
}

int CSLiveManager::SLiveCancelDelayPush()
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	if (LiveInstance)
	{
		LiveInstance->SetDelayCancel();
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetLogLevel(int Level)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke begin!", __FUNCTION__);
	LogFilter logfilter;
	memset(&logfilter, 0, sizeof(logfilter));
	logfilter.iSubTypeCount = 1;

	if (Level >= 1 && Level <= 15)
	{
		logfilter.i64SumOfErrorSubType[0] = (Level & 0x08) ? LOG_RTSPSERV : 0;
		logfilter.i64SumOfWarningSubTye[0] = (Level & 0x04) ? LOG_RTSPSERV : 0;
		logfilter.i64SumOfMsgSubType[0] = (Level & 0x02) ? LOG_RTSPSERV : 0;
		logfilter.i64SunOfDebugSubType[0] = (Level & 0x01) ? LOG_RTSPSERV : 0;
	}
	else
	{
		logfilter.i64SumOfErrorSubType[0] = LOG_RTSPSERV;
		logfilter.i64SumOfWarningSubTye[0] = LOG_RTSPSERV;
		logfilter.i64SumOfMsgSubType[0] = LOG_RTSPSERV;
		logfilter.i64SunOfDebugSubType[0] = 0;
	}

	Log::setNewLogFilter(&logfilter);
	Log::writeMessage(LOG_RTSPSERV, 1, "%s Invoke end!", __FUNCTION__);
	return 0;
}

DWORD CSLiveManager::CheckStatusThreadProc(LPVOID lParam)
{
	
	CSLiveManager::GetInstance()->CheckStatus();
	return 0;
}

float GetProcessOrThreadCpuPercent(const HANDLE hProcess, const DWORD dwElepsedTime, int CoreNum, bool bProcess, LARGE_INTEGER& OldTime)
{
	float nProcCpuPercent = 0;
	BOOL bRetCode = FALSE;

	FILETIME CreateTime, ExitTime, KernelTime, UserTime;
	LARGE_INTEGER lgKernelTime;
	LARGE_INTEGER lgUserTime;
	LARGE_INTEGER lgCurTime;

	if (bProcess)
		bRetCode = GetProcessTimes(hProcess, &CreateTime, &ExitTime, &KernelTime, &UserTime);
	else
	{
		bRetCode = GetThreadTimes(hProcess, &CreateTime, &ExitTime, &KernelTime, &UserTime);
	}
	if (bRetCode)
	{
		lgKernelTime.HighPart = KernelTime.dwHighDateTime;
		lgKernelTime.LowPart = KernelTime.dwLowDateTime;

		lgUserTime.HighPart = UserTime.dwHighDateTime;
		lgUserTime.LowPart = UserTime.dwLowDateTime;

		lgCurTime.QuadPart = (lgKernelTime.QuadPart + lgUserTime.QuadPart) / 10000;
		nProcCpuPercent = (float)((lgCurTime.QuadPart - OldTime.QuadPart) * 100 / dwElepsedTime);
		OldTime = lgCurTime;
		nProcCpuPercent = nProcCpuPercent / CoreNum;
	}
	else
	{
		nProcCpuPercent = -1;
	}

	return nProcCpuPercent;
}

BOOL SetPrivilege(HANDLE hProcess, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
	HANDLE hToken;
	if (!OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES, &hToken))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "OpenProcessToken 失败 LastError = %d", GetLastError());
		return FALSE;
	}
	LUID luid;
	if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "LookupPrivilegeValue 失败 LastError = %d", GetLastError());
		return FALSE;
	}
	TOKEN_PRIVILEGES tkp;
	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : FALSE;
	if (!AdjustTokenPrivileges(hToken, FALSE, &tkp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "AdjustTokenPrivileges 失败 LastError = %d", GetLastError());
		return FALSE;
	}
	return TRUE;
}

void CSLiveManager::CheckStatus()
{
	HANDLE hProcess = NULL;
	DWORD dwProcessId = GetCurrentProcessId();
	DWORD dwRetVal = 0;
	QWORD dwOldTickCount = GetQPCMS();
	QWORD dwCurrentTickCount = 0;
	DWORD dwElapsedTime = 0;
	DWORD HandleCount = 0;
	float nProcessCpuPercent = 0;
	float LastMemeryUse = 0;
	float CurrentMemory = 0;
	int Min = m_CheckTime / 60 / 1000;

	HANDLE ThreadHandle[] = { HVideoCapture, HVideoEncoder, HAudioCapture };
	float    ThreadCpuPercent[sizeof ThreadHandle / sizeof HANDLE];

	LARGE_INTEGER g_slgProcessTimeOld[sizeof ThreadHandle / sizeof HANDLE] = { 0 };

	static LARGE_INTEGER ProcessTimeOld = { 0 };

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

	if (!hProcess)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "检测线程 OpenProcess 失败, LastError = %d ", GetLastError());
	}

// 	if (!SetPrivilege(hProcess, SE_DEBUG_NAME, TRUE))
// 	{
// 		Log::writeMessage(LOG_RTSPSERV, 1, "检测线程 SetPrivilege 失败");
// 		CloseHandle(hProcess);
// 		return;
// 	}

	SYSTEM_INFO SysInfo;
	GetSystemInfo(&SysInfo);
	PROCESS_MEMORY_COUNTERS pmc = { 0 };
	Log::writeMessage(LOG_RTSPSERV, 1, "检测线程开始运行...");
	while (bRunning)
	{
		WaitForSingleObject(m_CheckEvent, INFINITE);

		if (!bRunning)
			break;
		dwCurrentTickCount = GetQPCMS();
		dwElapsedTime = dwCurrentTickCount - dwOldTickCount;
		dwOldTickCount = dwCurrentTickCount;

		nProcessCpuPercent = GetProcessOrThreadCpuPercent(hProcess, dwElapsedTime, SysInfo.dwNumberOfProcessors, true, ProcessTimeOld);

		for (int i = 0; i < sizeof ThreadHandle / sizeof HANDLE; ++i)
		{
			ThreadCpuPercent[i] = GetProcessOrThreadCpuPercent(ThreadHandle[i], dwElapsedTime, SysInfo.dwNumberOfProcessors, false, g_slgProcessTimeOld[i]);
		}


		GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));

		CurrentMemory = (float)pmc.PagefileUsage / 1024 / 1024;

		if (nProcessCpuPercent != -1 && GetProcessHandleCount(hProcess, &HandleCount))
		{
			Log::writeMessage(LOG_RTSPSERV, 1, "\r\n*************************************************************\r\n"
				"* 当前进程cpu占用                                      %.2f%% \r\n"
				"* 当前进程句柄个数                                     %d 个 \r\n"
				"* 当前内存占用                                         %.2f M \r\n"
				"* 内存增量                                             %.2f M \r\n"
				"* 视频采集线程cpu占用                                  %.2f%% \r\n"
				"* 编码线程cpu占用                                      %.2f%% \r\n"
				"* 音频采集线程cpu占用                                  %.2f%% \r\n"
				"* 在 %d 分钟内共采集了 %d 个视频帧 平均每帧用时        %0.2f ms \r\n"
				"* 在 %d 分钟内共编码了 %d 个视频帧 平均每帧编码用时    %0.2f ms \r\n"
				"* 在 %d 分钟内共编码采集了 %d 个音频帧 平均每帧用时    %0.2f ms \r\n"
				"*************************************************************"
				,
				nProcessCpuPercent,
				HandleCount,
				CurrentMemory,
				CurrentMemory - LastMemeryUse,
				ThreadCpuPercent[0], ThreadCpuPercent[1], ThreadCpuPercent[2],
				Min, m_FPSCount, (float)m_RealTime / m_FPSCount,
				Min, m_EncodeVideoCount, m_RealFrameTime,
				Min, m_EncodeAudioCount, m_RealAudioFrameTime);
		}

		LastMemeryUse = CurrentMemory;
	}
	Log::writeMessage(LOG_RTSPSERV,1,"检测线程退出");
	CloseHandle(hProcess);
}

bool CSLiveManager::HaveSDIOut()
{
	BlackMagic* blackMagic = BlackMagic::Instance();
	return (FirstSDIRender != -1000) && blackMagic->AllEnable();
}

void CSLiveManager::PushOrUpdateSIDId(const SDIID& id)
{
	if (FirstSDIRender == -1000 && id.enable)
	{
		FirstSDIRender = id.id;
	}
	else if (FirstSDIRender == id.id && (!id.enable))
	{
		FirstSDIRender = -1000;
	}

	OSEnterMutex(SDIMutex);

	bool find = false;
	for (auto& sid : SIDIDs)
	{
		if (sid.id == id.id)
		{
			sid.enable = id.enable;
			find = true;
		}

		if (FirstSDIRender == -1000 && sid.enable)
		{
			FirstSDIRender = sid.id;
		}
	}

	if (!find)
	{
		SIDIDs.push_back(id);
	}

	OSLeaveMutex(SDIMutex);
}

bool CSLiveManager::FindAndRemoveId(int id)
{
	OSEnterMutex(SDIMutex);
	SIDIDs.remove_if([&id](const SDIID& i){ return i.id == id; });

	if (FirstSDIRender == id)
	{
		FirstSDIRender = -1000;
		auto end = SIDIDs.end();
		auto pos = std::find_if(SIDIDs.begin(), end,
			[](const SDIID& i) { return i.enable; });
		if (pos != end)
		{
			FirstSDIRender = pos->id;
		}
	}
	OSLeaveMutex(SDIMutex);

	return 0;
}

int CSLiveManager::SLiveGetBlackMagicDevices(char **DevicesList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(DevicesList);
		Json::Value JsonConfigList;

		Value &ArryList = JsonConfigList["DevicesList"];

		BlackMagic *Magic = BlackMagic::Instance();

		for (int i = 0; i < Magic->nDeviceCount; ++i)
		{
			ArryList[i]["Name"] = Magic->pDevicelist[i].strName;
			ArryList[i]["ID"] = Magic->pDevicelist[i].strID;
		}

		if (ArryList.size() > 0)
		{
			std::string &str = JsonConfigList.toStyledString();
			*DevicesList = new char[str.length() + 1];
			(*DevicesList)[str.length()] = 0;
			memcpy(*DevicesList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("ArryList == 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveGetBlackMagicDisplayMode(char **DisplayModeList)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(DisplayModeList);
		Json::Value JsonConfigList;

		Value &ArryList = JsonConfigList["DisplayModeList"];

		BlackMagic *Magic = BlackMagic::Instance();

		for (int i = 0; i < Magic->nDisplayModeCount; ++i)
		{
			ArryList[i] = Magic->pModeList[i].strName;
		}

		if (ArryList.size() > 0)
		{
			std::string &str = JsonConfigList.toStyledString();
			*DisplayModeList = new char[str.length() + 1];
			(*DisplayModeList)[str.length()] = 0;
			memcpy(*DisplayModeList, str.data(), str.length());
		}
		else
		{
			BUTEL_THORWERROR("ArryList == 0");
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveBlackMagicOutputOn(bool bOn /*= true*/)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		if (bOn)
			BlackMagic::Instance()->ReStart();
		else
		{
			BlackMagic::Instance()->AllStop();
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveSetBlackMagicOut(__SDIOut * SDIOut)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(SDIOut);

		if (BlackMagic::Instance()->nDeviceCount > 0)
		{
			if (!__SDIOutInfo)
				__SDIOutInfo = new SDIOutInfo[BlackMagic::Instance()->nDeviceCount];

			BUTEL_IFNULLRETURNERROR(__SDIOutInfo);

			for (int i = 0; i < BlackMagic::Instance()->nDeviceCount; ++i)
			{
				__SDIOutInfo[i].bEnable = SDIOut[i].bEnable;
				__SDIOutInfo[i].Format = SDIOut[i].Format;
				__SDIOutInfo[i].id = SDIOut[i].Id;
				__SDIOutInfo[i].SourceName = SDIOut[i].SourceName;
				__SDIOutInfo[i].AudioName = SDIOut[i].AudioName;
			}

			BlackMagic::Instance()->ApplySDISettings(__SDIOutInfo);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

void CSLiveManager::ChangeLiveInstanceSameAsLocalInstance(IBaseVideo *Video)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);
	bool bFind = false;
	if (Video)
	{
		Vect2 NewSize;
		for (int i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneLocalVideo = LocalInstance->m_VideoList[i];
			if (OneLocalVideo.VideoStream.get() == Video)
			{
				NewSize = OneLocalVideo.size;
				bFind = true;
				break;
			}
		}

		if (bFind)
		{
			for (int i = 0; i < LiveInstance->m_VideoList.Num(); ++i)
			{
				VideoStruct &OneLivelVideo = LiveInstance->m_VideoList[i];
				if (OneLivelVideo.VideoStream.get() == Video)
				{
					OneLivelVideo.size = NewSize;
					break;
				}
			}
		}
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end! bFind = %s", __FUNCTION__,bFind ? "true": "false");
}

void CSLiveManager::ResetDevice(IBaseVideo *Video, shared_ptr<IBaseVideo>& ResetVideo,bool bNULL,IBaseVideo *PreVideo)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke begin!", __FUNCTION__);

	if (!Video)
	{
		Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end! Video is NULL", __FUNCTION__);
		return;
	}

	if (LocalInstance)
	{
		
		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);

			if (OneProcess && OneProcess->bLittlePre && OneProcess->bNoPreView)
			{
				EnterCriticalSection(&OneProcess->VideoSection);

				for (int j = 0;j < OneProcess->m_VideoList.Num(); ++j)
				{
					VideoStruct &OneVideo = OneProcess->m_VideoList[j];
					if (strcmp(OneVideo.VideoStream->GainClassName(), "AgentSource") == 0)
					{
						IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();
						if (Global != NULL)
						{
							if (Global == Video || Global == ResetVideo.get() || Global == PreVideo)
							{
								OneVideo.VideoStream->UpdateSettings(*OneVideo.Config);
							}
						}
					}
				}
				
				LeaveCriticalSection(&OneProcess->VideoSection);
			}
			
		}
		shared_ptr<IBaseVideo> TemDevice;//主要是为了摄像头不在锁内析构
		EnterCriticalSection(&LocalInstance->VideoSection);
		for (int i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LocalInstance->m_VideoList[i];
			if (OneVideo.VideoStream.get() == Video)
			{
				TemDevice = OneVideo.VideoDevice;
				if (bNULL)
				{
					if (OneVideo.VideoDevice)
						OneVideo.VideoDevice.reset();
				}
				else
				{
					OneVideo.VideoDevice = ResetVideo;
				}
				
				break;
			}
		}
		LeaveCriticalSection(&LocalInstance->VideoSection);
	}

	if (LiveInstance)
	{
		shared_ptr<IBaseVideo> TemDevice;//主要是为了摄像头不在锁内析构
		EnterCriticalSection(&LiveInstance->VideoSection);
		for (int i = 0; i < LiveInstance->m_VideoList.Num(); ++i) //先重置区域区占位源里的源
		{
			VideoStruct &OneVideo = LiveInstance->m_VideoList[i];
			if (OneVideo.VideoStream.get() == Video)
			{
				TemDevice = OneVideo.VideoDevice;
				if (bNULL)
				{
					if (OneVideo.VideoDevice)
						OneVideo.VideoDevice.reset();
				}
				else
				{
					OneVideo.VideoDevice = ResetVideo;
				}

				break;
			}

		}

		LeaveCriticalSection(&LiveInstance->VideoSection);
	}

	Log::writeMessage(LOG_RTSPSERV, 1, "%s invoke end!", __FUNCTION__);
}

int CSLiveManager::SLiveHasIntancesCanRecord(bool *bRecord)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(bRecord);

		*bRecord = BSParam.LiveSetting.bRecoder || (BSParam.LiveSetting.bUseLiveSec && BSParam.LiveSetting.bRecoderSec);

		if (*bRecord)
			return 0;

		for (int i = 0; i < m_InstanceList.GetSize(); ++i)
		{
			CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);

			if (OneProcess->bLittlePre && !OneProcess->bNoPreView)
			{
				if (OneProcess->bCanRecord)
				{
					*bRecord = true;
					return 0;
				}
			}
		}
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

bool CSLiveManager::IsHaveStream(IBaseVideo *Video)
{
	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);

		if (OneProcess && OneProcess->bLittlePre && OneProcess->bNoPreView)
		{
			EnterCriticalSection(&OneProcess->VideoSection);

			for (int j = 0; j < OneProcess->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneVideo = OneProcess->m_VideoList[j];
				if (strcmp(OneVideo.VideoStream->GainClassName(), "AgentSource") == 0)
				{
					IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();
					if (Global != NULL && Global == Video)
					{
						LeaveCriticalSection(&OneProcess->VideoSection);
						return true;
					}
				}
			}

			LeaveCriticalSection(&OneProcess->VideoSection);
		}

	}

	if (LiveInstance)
	{
		EnterCriticalSection(&LiveInstance->VideoSection);
		for (int i = 0; i < LiveInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LiveInstance->m_VideoList[i];
			if (OneVideo.bRender && (OneVideo.VideoStream.get() == Video || OneVideo.VideoDevice.get() == Video))
			{
				LeaveCriticalSection(&LiveInstance->VideoSection);
				return true;
			}

		}

		LeaveCriticalSection(&LiveInstance->VideoSection);
	}

	if (LocalInstance)
	{
		EnterCriticalSection(&LocalInstance->VideoSection);
		for (int i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LocalInstance->m_VideoList[i];
			if (OneVideo.bRender && (OneVideo.VideoStream.get() == Video || OneVideo.VideoDevice.get() == Video))
			{
				LeaveCriticalSection(&LocalInstance->VideoSection);
				return true;
			}

		}

		LeaveCriticalSection(&LocalInstance->VideoSection);
	}

	return false;
}

void CSLiveManager::RenderSDI(int Index)
{
	OSEnterMutex(SDIMutex);
	if (bStartView && SIDIDs.size())
	{
		BlackMagic* blackMagic = BlackMagic::Instance();

		bool bEnable = false;

		for (auto& id : SIDIDs)
		{
			if (id.enable)
			{
				bEnable = true;
				break;
			}
		}


		HRESULT result = S_FALSE;
		BYTE *lpData;
		UINT Pitch;

		if (bEnable)
		{
			D3D10Texture *d3dRGB = NULL;
			if (bTransDisSolving || bTransUpDown || bTransDiffuse || bRadius)
			{
				d3dRGB = dynamic_cast<D3D10Texture*>(transitionTexture);
			}
			else
			{
				d3dRGB = dynamic_cast<D3D10Texture*>(mainRenderTextures[Index]);
			}
			m_D3DRender->CopyTexture(copyRGBTexture, d3dRGB);
			result = m_D3DRender->Map(copyRGBTexture, lpData, Pitch);
		}

		if (bEnable)
		{
			for (auto& id : SIDIDs)
			{
				if (id.enable && result == S_OK)
				{
					blackMagic->SDI_RenderDevice(id, lpData, outputCX, outputCY, ColorFormat_RGBA32REVERSE, false, NULL, 0, false);
				}
			}

			if (result == S_OK)
			{
				m_D3DRender->Unmap(copyRGBTexture);
			}
		}
	}
	OSLeaveMutex(SDIMutex);
}

bool CSLiveManager::FindVideoInLocalIntance(IBaseAudio *Audio)
{
	bool bFind = false;
	if (LocalInstance)
	{
		EnterCriticalSection(&LocalInstance->AudioSection);
		for (int i = 0; i < LocalInstance->m_AudioList.Num(); ++i)
		{
			AudioStruct &OneAudio = LocalInstance->m_AudioList[i];

			if (OneAudio.AudioStream)
			{
				if (OneAudio.AudioStream.get() == Audio)
				{
					bFind = true;
					break;
				}

			}

		}
		LeaveCriticalSection(&LocalInstance->AudioSection);
	}

	return bFind;
}

bool CSLiveManager::FindVideoInLiveIntance(IBaseAudio *Audio)
{
	bool bFind = false;
	if (LiveInstance)
	{
		EnterCriticalSection(&LiveInstance->AudioSection);
		for (int i = 0; i < LiveInstance->m_AudioList.Num(); ++i)
		{
			AudioStruct &OneAudio = LiveInstance->m_AudioList[i];

			if (OneAudio.AudioStream)
			{
				if (OneAudio.AudioStream.get() == Audio)
				{
					bFind = true;
					break;
				}

			}

		}
		LeaveCriticalSection(&LiveInstance->AudioSection);
	}

	return bFind;
}

int CSLiveManager::SLiveAddFilter(uint64_t iInstansID, uint64_t iStreamID, const char *FilterName, uint64_t *iFilterID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(FilterName);
		BUTEL_IFNULLRETURNERROR(iFilterID);

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iInstansID];
		LeaveCriticalSection(&MapInstanceSec);


		if (Process)
		{
			Filter &RetFilter = Process->AddFilter(iStreamID, FilterName, iFilterID);

			//在PGM和PVW中添加
			if (Process->bLittlePre)
			{
				if (LocalInstance)
				{
					LocalInstance->AddFilter(RetFilter);
				}

				if (LiveInstance)
				{
					LiveInstance->AddFilter(RetFilter);
				}
			}

		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iInstansID);
		}
		
	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveDeleteFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID)
{
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke begin!", __FUNCTION__);
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iInstansID];
		LeaveCriticalSection(&MapInstanceSec);


		if (Process)
		{
			Process->DeleteFilter(iStreamID, iFilterID);

			if (Process->bLittlePre)
			{
				//把PGM和PVW中的都删除
				try
				{
					if (LocalInstance)
					{
						LocalInstance->DeleteFilter(iStreamID, iFilterID);
					}

					if (LiveInstance)
					{
						LiveInstance->DeleteFilter(iStreamID, iFilterID);
					}
				}
				catch (CErrorBase& e)
				{

				}
			}
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iInstansID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end!", __FUNCTION__);
	return 0;
}

int CSLiveManager::SLiveUpdateFilter(uint64_t iInstansID, uint64_t iStreamID, uint64_t iFilterID, const char *cJson)
{
	try
	{
		if (!bInit)
		{
			BUTEL_THORWERROR("SDK还未初始化,请先调用SLiveInit进行初始化!");
		}

		BUTEL_IFNULLRETURNERROR(cJson);

		Value Jvalue;
		Reader JReader;

		if (!JReader.parse(cJson, Jvalue))
		{
			BUTEL_THORWERROR("Json %s 解析失败", cJson);
		}

		EnterCriticalSection(&MapInstanceSec);
		CInstanceProcess *Process = m_InstanceList[iInstansID];
		LeaveCriticalSection(&MapInstanceSec);


		if (Process)
		{
			Process->UpdateFilter(iStreamID, iFilterID, Jvalue);
		}
		else
		{
			BUTEL_THORWERROR("没有找到实例ID为 %llu 的实例 ", iInstansID);
		}

	}
	catch (CErrorBase& e)
	{
		SLiveSetLastError(e.m_Error.c_str());
		Log::writeMessage(LOG_RTSPSERV, 1, "LiveSDK_Log:%s Invoke end! return -1", __FUNCTION__);
		return -1;
	}
	return 0;
}

void CSLiveManager::AddFilter2PGMOrPVM(IBaseVideo *Video)
{
	if (!Video)
		return;

	bool bFind = false;
	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);

		if (OneProcess && OneProcess->bLittlePre)
		{
			EnterCriticalSection(&OneProcess->VideoSection);

			for (UINT j = 0; j < OneProcess->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneVideo = OneProcess->m_VideoList[j];

				if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video))
				{
					if (OneProcess->m_Filter.Num())
					{
						AddFilter2PGMOrPVM(Video, OneProcess->m_Filter);
					}

					bFind = true;
					break;
				}
				
			}

			LeaveCriticalSection(&OneProcess->VideoSection);
		}

		if (bFind)
			break;
	}
}

void CSLiveManager::AddFilter2PGMOrPVM(IBaseVideo *Video, const List<Filter> &FilterList)
{
	if (LocalInstance)
	{
		EnterCriticalSection(&LocalInstance->VideoSection);

		for (UINT i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LocalInstance->m_VideoList[i];

			IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();

			if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video) || (Global != NULL && Global == Video))
			{
				for (UINT j = 0; j < FilterList.Num(); ++j)
				{
					bool bFind = false;
					Filter &ASTemD = FilterList[j];
					for (UINT k = 0; k < LocalInstance->m_Filter.Num(); ++k)
					{
						Filter &VSTem = LocalInstance->m_Filter[k];
						if (ASTemD.IVideo == VSTem.IVideo)
						{
							bFind = true;
							break;
						}
					}

					if (!bFind)
					{
						LocalInstance->m_Filter.SetSize(LocalInstance->m_Filter.Num() + 1);
						Filter &VS = LocalInstance->m_Filter[LocalInstance->m_Filter.Num() - 1];
						VS = ASTemD;
					}
				}
			
				break;
			}
		}

		LeaveCriticalSection(&LocalInstance->VideoSection);
	}



	if (LiveInstance)
	{
		EnterCriticalSection(&LiveInstance->VideoSection);

		for (UINT i = 0; i < LiveInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LiveInstance->m_VideoList[i];

			IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();

			if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video) || (Global != NULL && Global == Video))
			{
				for (UINT j = 0; j < FilterList.Num(); ++j)
				{
					bool bFind = false;
					Filter &ASTemD = FilterList[j];
					for (UINT k = 0; k < LiveInstance->m_Filter.Num(); ++k)
					{
						Filter &VSTem = LiveInstance->m_Filter[k];
						if (ASTemD.IVideo == VSTem.IVideo)
						{
							bFind = true;
							break;
						}
					}

					if (!bFind)
					{
						LiveInstance->m_Filter.SetSize(LiveInstance->m_Filter.Num() + 1);
						Filter &VS = LiveInstance->m_Filter[LiveInstance->m_Filter.Num() - 1];
						VS = ASTemD;
					}
				}

				break;
			}
		}

		LeaveCriticalSection(&LiveInstance->VideoSection);
	}
}

void CSLiveManager::RemoveFilterFromPGMOrPVM(IBaseVideo *Video)
{
	if (!Video)
		return;
	bool bFind = false;
	for (int i = 0; i < m_InstanceList.GetSize(); ++i)
	{
		CInstanceProcess *OneProcess = m_InstanceList.GetAt(i);

		if (OneProcess && OneProcess->bLittlePre)
		{
			EnterCriticalSection(&OneProcess->VideoSection);

			for (UINT j = 0; j < OneProcess->m_VideoList.Num(); ++j)
			{
				VideoStruct &OneVideo = OneProcess->m_VideoList[j];

				if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video))
				{
					if (OneProcess->m_Filter.Num())
					{
						RemoveFilterFromPGMOrPVM(Video, OneProcess->m_Filter);
					}

					bFind = true;
					break;
				}

			}

			LeaveCriticalSection(&OneProcess->VideoSection);
		}

		if (bFind)
			break;
	}
}

void CSLiveManager::RemoveFilterFromPGMOrPVM(IBaseVideo *Video, const List<Filter> &FilterList)
{
	if (LocalInstance)
	{
		EnterCriticalSection(&LocalInstance->VideoSection);

		for (UINT i = 0; i < LocalInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LocalInstance->m_VideoList[i];

			IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();

			if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video) || (Global != NULL && Global == Video))
			{
				for (UINT j = 0; j < FilterList.Num(); ++j)
				{
					Filter &ASTemD = FilterList[j];
					for (UINT k = 0; k < LocalInstance->m_Filter.Num(); ++k)
					{
						Filter &VSTem = LocalInstance->m_Filter[k];
						if (ASTemD.IVideo == VSTem.IVideo)
						{
							LocalInstance->m_Filter.Remove(k);
							break;
						}
					}

				}

				break;
			}
		}

		LeaveCriticalSection(&LocalInstance->VideoSection);
	}



	if (LiveInstance)
	{
		EnterCriticalSection(&LiveInstance->VideoSection);

		for (UINT i = 0; i < LiveInstance->m_VideoList.Num(); ++i)
		{
			VideoStruct &OneVideo = LiveInstance->m_VideoList[i];

			IBaseVideo *Global = OneVideo.VideoStream->GetGlobalSource();

			if ((OneVideo.VideoDevice && OneVideo.VideoDevice.get() == Video) || (OneVideo.VideoStream && OneVideo.VideoStream.get() == Video) || (Global != NULL && Global == Video))
			{
				for (UINT j = 0; j < FilterList.Num(); ++j)
				{
					Filter &ASTemD = FilterList[j];
					for (UINT k = 0; k < LiveInstance->m_Filter.Num(); ++k)
					{
						Filter &VSTem = LiveInstance->m_Filter[k];
						if (ASTemD.IVideo == VSTem.IVideo)
						{
							LiveInstance->m_Filter.Remove(k);
							break;
						}
					}

				}

				break;
			}
		}

		LeaveCriticalSection(&LiveInstance->VideoSection);
	}
}

D3DAPI * CSLiveManager::GetD3DRender() const
{
	return m_D3DRender;
}

