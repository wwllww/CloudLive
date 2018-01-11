#ifndef DATASTREAM_H
#define DATASTREAM_H

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

//显示器参数
struct PipeMonitorInfo
{
	inline PipeMonitorInfo() { memset(this, 0x00, sizeof(PipeMonitorInfo)); }
	inline PipeMonitorInfo(HMONITOR hMonitor, RECT *lpRect) :tag(0)
	{
		this->hMonitor = hMonitor;
		this->rect.left = lpRect->left;
		this->rect.top = lpRect->top;
		this->rect.right = lpRect->right;
		this->rect.bottom = lpRect->bottom;
	}

	PipeMonitorInfo& operator = (const PipeMonitorInfo& other)
	{
		this->hMonitor = other.hMonitor;
		this->rect.left = other.rect.left;
		this->rect.top = other.rect.top;
		this->rect.right = other.rect.right;
		this->rect.bottom = other.rect.bottom;
		this->tag = other.tag;

		return *this;
	}

	inline PipeMonitorInfo(const PipeMonitorInfo& other)
	{
		this->hMonitor = other.hMonitor;
		this->rect.left = other.rect.left;
		this->rect.top = other.rect.top;
		this->rect.right = other.rect.right;
		this->rect.bottom = other.rect.bottom;
		this->tag = other.tag;
	}

	bool operator == (const PipeMonitorInfo& other) const
	{
		return 	(this->hMonitor == other.hMonitor) &&
			    (this->rect.left == other.rect.left) &&
			    (this->rect.top == other.rect.top) &&
			    (this->rect.right == other.rect.right) &&
			    (this->rect.bottom == other.rect.bottom);
	}

	HMONITOR hMonitor;  //显示器句柄 (初始化后不再修改)
	RECT rect;          //显示器左上角和右下角坐标(初始化后不再修改)
	volatile int tag;   //DLL 内部不应用tag，开发者自行应用
};

//视频类型(目前只支持VideoType_YUV)
enum VideoType
{
	//YUV420P
	VideoType_YUV = 0, //YUV是平面数据，非交叉数据
};


struct VideoData    //视频数据
{
	int Width;	    //视频宽
	int Height;     //视频高
	int FrameRate;  //帧率
	VideoType VType; //视频类型 
	int Rotate;			//YUV rotate 0, 90, 180,270
	long long Timestamp;	//RTP time stamp
	int Bpp;
	unsigned char* Buffer;    //数据指针 (不必再行分配空间，只需指向现有数据缓冲区即可, 只需保证调用SendVideoData期间指针有效即可)
	int Len;                  //数据长度
};

//音频类型(目前只支持PCM)
enum AudioType
{
	AudioType_PCM = 0,
};

struct AudioData  //音频参数
{
	int channels;       //通道数
	int samplerate;     //采样率
	int sampleperbits;  //采样位数
	AudioType AType;     //音频类型
	long long nTimestamp;

	unsigned char* Buffer;    //数据指针(不必再行分配空间，只需指向现有数据缓冲区即可, 只需保证调用SendAudioData期间指针有效即可)
	int Len;                  //数据长度
};

EXTERN_DLL_EXPORT int LoadInit(HINSTANCE hInstance);    //初始化    返回零为成功
EXTERN_DLL_EXPORT int UnLoadInit();  //反初始化  返回零为成功

//以下四个函数没有调用顺之分，异步消息内调用即可,也可在多线程环境内调用（当然必须在LoadInit()函数之后，UnLoadInit()函数之前调用)。

//添加一个显示区(自行保证投影区域之前没有被投影过)
EXTERN_DLL_EXPORT int AddMonitorRender(const MonitorInfo& monitor);      //返回零为成功
//减少一个显示区(自行保证移除的显示区之前是存在的)
EXTERN_DLL_EXPORT int RemoveMonitorRender(const MonitorInfo& monitor);      //返回零为成功

EXTERN_DLL_EXPORT int SendVideoData(const VideoData& info); //传输视频数据    返回零为成功
EXTERN_DLL_EXPORT int SendAudioData(const AudioData& info);  //传输音频数据   返回零为成功
#endif

/*调用顺序说明: 所有函数返回零为成功其他为错误码
先调用 Init() 初始化;

//在你的视频解析一帧后
SendVideoData
//在你的视频解析一帧后
SendAudioData

程序退出时调用UnInit();
*/

/* MonitorInfo 获取实例代码
std::list<MonitorInfo*> monitors
EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)MonitorInfoEnumProc, (LPARAM)&monitors);
BOOL CALLBACK MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, std::list<MonitorInfo> &monitors)
{
monitors.push_back(new MonitorInfo(hMonitor, lprcMonitor));
return TRUE;
}
*/