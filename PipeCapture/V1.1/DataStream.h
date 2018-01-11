#ifndef DATASTREAM_H
#define DATASTREAM_H

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

//��ʾ������
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

	HMONITOR hMonitor;  //��ʾ����� (��ʼ�������޸�)
	RECT rect;          //��ʾ�����ϽǺ����½�����(��ʼ�������޸�)
	volatile int tag;   //DLL �ڲ���Ӧ��tag������������Ӧ��
};

//��Ƶ����(Ŀǰֻ֧��VideoType_YUV)
enum VideoType
{
	//YUV420P
	VideoType_YUV = 0, //YUV��ƽ�����ݣ��ǽ�������
};


struct VideoData    //��Ƶ����
{
	int Width;	    //��Ƶ��
	int Height;     //��Ƶ��
	int FrameRate;  //֡��
	VideoType VType; //��Ƶ���� 
	int Rotate;			//YUV rotate 0, 90, 180,270
	long long Timestamp;	//RTP time stamp
	int Bpp;
	unsigned char* Buffer;    //����ָ�� (�������з���ռ䣬ֻ��ָ���������ݻ���������, ֻ�豣֤����SendVideoData�ڼ�ָ����Ч����)
	int Len;                  //���ݳ���
};

//��Ƶ����(Ŀǰֻ֧��PCM)
enum AudioType
{
	AudioType_PCM = 0,
};

struct AudioData  //��Ƶ����
{
	int channels;       //ͨ����
	int samplerate;     //������
	int sampleperbits;  //����λ��
	AudioType AType;     //��Ƶ����
	long long nTimestamp;

	unsigned char* Buffer;    //����ָ��(�������з���ռ䣬ֻ��ָ���������ݻ���������, ֻ�豣֤����SendAudioData�ڼ�ָ����Ч����)
	int Len;                  //���ݳ���
};

EXTERN_DLL_EXPORT int LoadInit(HINSTANCE hInstance);    //��ʼ��    ������Ϊ�ɹ�
EXTERN_DLL_EXPORT int UnLoadInit();  //����ʼ��  ������Ϊ�ɹ�

//�����ĸ�����û�е���˳֮�֣��첽��Ϣ�ڵ��ü���,Ҳ���ڶ��̻߳����ڵ��ã���Ȼ������LoadInit()����֮��UnLoadInit()����֮ǰ����)��

//���һ����ʾ��(���б�֤ͶӰ����֮ǰû�б�ͶӰ��)
EXTERN_DLL_EXPORT int AddMonitorRender(const MonitorInfo& monitor);      //������Ϊ�ɹ�
//����һ����ʾ��(���б�֤�Ƴ�����ʾ��֮ǰ�Ǵ��ڵ�)
EXTERN_DLL_EXPORT int RemoveMonitorRender(const MonitorInfo& monitor);      //������Ϊ�ɹ�

EXTERN_DLL_EXPORT int SendVideoData(const VideoData& info); //������Ƶ����    ������Ϊ�ɹ�
EXTERN_DLL_EXPORT int SendAudioData(const AudioData& info);  //������Ƶ����   ������Ϊ�ɹ�
#endif

/*����˳��˵��: ���к���������Ϊ�ɹ�����Ϊ������
�ȵ��� Init() ��ʼ��;

//�������Ƶ����һ֡��
SendVideoData
//�������Ƶ����һ֡��
SendAudioData

�����˳�ʱ����UnInit();
*/

/* MonitorInfo ��ȡʵ������
std::list<MonitorInfo*> monitors
EnumDisplayMonitors(NULL, NULL, (MONITORENUMPROC)MonitorInfoEnumProc, (LPARAM)&monitors);
BOOL CALLBACK MonitorInfoEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, std::list<MonitorInfo> &monitors)
{
monitors.push_back(new MonitorInfo(hMonitor, lprcMonitor));
return TRUE;
}
*/