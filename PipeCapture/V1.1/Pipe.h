//设置参数参数, 导播音视频数据请求, 主持人音视频数据请求, 视频数据， 音频数据, 主持人模式, 回应
enum MsgType{ ParamSet = 0, DirectorAVRequest, HostAVRequest, CommandCenterAVRequest, VideoData, AudioData, HostMode, Response, HeartBeat };

struct MSGHeader
{
    unsigned int len;   //消息内容长度
    MsgType type;     //消息类型
};

//以下是消息内容
//回应
struct strResponse
{
	int code;                 //回应码, 回应值为0正常， 其值他待定
	char description[256];    //描述
};

//设置导播主持人采集设备
struct DeviceParam
{
	int nWidth;        //视频宽度
	int nHeight;        //视频高度
	int  nChannelNum; //最大互动通道数
	char DirectorAudioCapture[128]; //导播音频采集设备名称
	char DirectorAudioRender[128];  //导播音频放音设备名称
	char DirectorVideoCapture[128]; //导播视频采集设备名称
	char DirectorVideoCaptureID[256];
	char PresenterAudioCapture[128];//主持人音频采集设备名称
	char PresenterAudioRender[128]; //主持人音频放音设备名称
	char PresenterVideoCapture[128];//主持人视频采集设备名称
	char PresenterVideoCaptureID[256];
	int ZCRWidth;      //主持人视频宽度
	int ZCRHeight;     //主持人视频高度
	int nSamplesRate;   // 采样率
	int nAudioChannel;  // 音频声道数
	int nBitPerSample;       // 采样精度
	char CommandCenterAudioCapture[128]; //第三个设备音频采集设备名称
	char CommandCenterVideoCapture[128]; //第三个设备导播视频采集设备名称
	char CommandCenterVideoCaptureID[256];
	int CCRWidth; //指挥中心视频宽度
	int CCRHeight;         //指挥中心视频高度
};

//错误回应码解释
//0   //正常, 参数设置成功
//1   //导播音频采集设备名称设置失败
//2   //导播音频放音设备名称设置失败
//3   //导播视频采集设备名称设置失败
//4   //主持人音频采集设备名称设置失败
//5   //主持人音频放音设备名称设置失败
//6   //主持人视频采集设备名称设置失败
//7   //其他错误， 见描述

//注意条件：
/*
在发送互动机参数前一定要先执行 关闭导播、主持人请求的管道句柄
(详细见 导播、主持人请求)
收到回应码后立即关闭该管道
异常请自行处理
*/

// DirectorAVRequest HostAVRequest
//导播与主持人音视频请求,消息内容为空
//错误码解释
//0 //正常
//1 //导播音频采集设备未开启
//2 //导播视频采集设备未开启
//3 //主持人音频采集设备为开启
//4 //主持人视频采集设备为开启
//5 //

//注意条件：
/*
客户端发送请求后，服务器会发回应码，客户如果收到正常码，服务器后续会继续发送相应主持人或者导播的音视频数据。
客户端在收到正常回应后，应该继续保持管道打开，接收音视频数据。当客户不在需要该数据时只需要客户自行关闭该管道即可
异常请自行处理
*/

//HostMode 主持人模式设置
struct strHostMode
{
	char ChannelNumber[24];      //客户视讯号
	char Nickname[64];           //客户昵称
};

//回应码解释
//0   //正常
//1   //具体看描述

//条件
/*
客户端发送主持人模式设置请求，如果服务器应答正常，客户端在收到应答后应该继续在该管道上发送该用户的音视频数据。
当导播将该客户置于导播模式时关闭该请求的管道
*/

enum ColorType{
	COLOR_YUV420P = 0,
	COLOR_RGB24,

};

//视频传输YUV420数据
//音频传输PCM数据
//原始数据传输包
struct VideoRawData
{
	unsigned long long timeStamp; //时间戳
	unsigned long len;            //数据长度
	int width;
	int height;
	ColorType color;
    int 	nFramePerSec;         //帧率
};

struct AudioRawData
{
	unsigned long long timeStamp; //时间戳
	WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
	unsigned long len;            //数据长度
    DWORD       nSamplesPerSec;     /* sample rate */
	WORD       wBitsPerSample;     /* number of bits per sample of mono data */
	
};

//在DirectorAVRequest， HostAVRequest请求中，收到应答码后，需要继续从服务器下载音视频原始数据
//在HostMode主持人模式设置请求中，收到服务器的正常应答码后，客户需上传该客户的音视频数据
