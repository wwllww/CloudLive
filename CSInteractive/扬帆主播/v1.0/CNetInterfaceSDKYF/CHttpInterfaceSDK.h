#ifndef CHTTPINTERFACESDK_H
#define CHTTPINTERFACESDK_H

#ifdef CNETINTERFACEAPI_EXPORTS
#include "CNetInterfaceSDK.h"
#endif

#ifdef CNETINTERFACEAPI_EXPORTS
#define API_EXPORT  __declspec(dllexport)
#else
#define API_EXPORT  __declspec(dllimport)
#endif


struct ChannelInfo
{
    unsigned int nChannelId;
    char* pChannelNumber;
    char* pChannelName;
    char* pInteractionNube;
    char* pAppKey;
    char* pPushUrl;
    char* pPlayUrl_flv;
    char* pPlayUrl_m3u8;
    unsigned int nDefaultAnchorFlag;
};

struct NodeInfo
{
    char   ip[16];
    char*  pdescribe;
    unsigned int    port;
};

enum StopLiveOper
{
    OperPause = 0,
    OperClose
};

#ifdef  __cplusplus
extern "C" {
#endif //  __cplusplus

        /************************************************************************/
        /* 函数功能：                  初始化
        /* 输入参数：                  pServerUrl 登录url
        /*                            pUploadUrl 上传剧照url pStrAppId 上传剧照appId（扬帆主播用）
        /* 输出参数：                  无

        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppInit(char* pServerUrl, char* pUploadUrl, char* pStrAppId);

        /************************************************************************/
        /* 函数功能：                  用户登录
        /* 输入参数：                  pUserName 用户名  pPassword 密码
        /*                            nUserType 用户类型（扬帆主播用）
        /* 输出参数：                  无

        /* 返 回 值：                  0 : 成功 -1 网络错误 -2 用户名密码错误
        /************************************************************************/

        API_EXPORT  int     BLiveAppLogin(char* pUserName, char* pPassword, int nUserType);

        /************************************************************************/
        /* 函数功能：                  鉴权
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppAuthorize();

        /************************************************************************/
        /* 函数功能：                  获取频道列表
        /* 输入参数：                  无
        /* 输出参数：                  pChannelInfo 频道列表 nChannelCount 频道个数
        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppGetChannelList(ChannelInfo** pChannelInfo, int* nChannelCount);

        /************************************************************************/
        /* 函数功能：                  释放频道列表内存
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppReleaseChannelList();

        /************************************************************************/
        /* 函数功能：                  设置当前频道
        /* 输入参数：                  pCurrentChannel 当前频道
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppSetChannel(ChannelInfo* pCurrentChannel);

        /************************************************************************/
        /* 函数功能：                  设置服务器节点列表
        /* 输入参数：                  无
        /* 输出参数：                  pNodeInfo 节点列表  nNodeCount 节点个数
        /* 返 回 值：                  0 : 成功 <0 失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppGetSeverNodeList(NodeInfo** pNodeInfo, int* nNodeCount);

        /************************************************************************/
        /* 函数功能：                  释放节点列表内存
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 ：失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppReleaseSeverNodeList();

        /************************************************************************/
        /* 函数功能：                  获取评论url （扬帆主播用）
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  成功返回评论url，失败返回NULL
        /************************************************************************/

        API_EXPORT  char*   BLiveAppGetChatUrl();

        /************************************************************************/
        /* 函数功能：                  开始直播推流
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 ：失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppBeforeStartPush();

        /************************************************************************/
        /* 函数功能：                  停止直播推流确认
        /* 输入参数：                  无
        /* 输出参数：                  bChannelClosed：true 关闭频道，false 不关闭频道
        /* 返 回 值：                  0 : 成功 <0 ：失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppBeforeStopPushConfirm(bool& bChannelClosed);

        /************************************************************************/
        /* 函数功能：                  停止直播推流
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  0 : 成功 <0 ：失败
        /************************************************************************/

        API_EXPORT  int     BLiveAppAfterStopPush();

        /************************************************************************/
        /* 函数功能：                  获取主程序标题
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  成功返回标题内容，失败返回NULL
        /************************************************************************/

        API_EXPORT  char*   BLiveAppGetTitle();

        /************************************************************************/
        /* 函数功能：                  获取关于界面信息，包括版本号、版权所有
        /* 输入参数：                  无
        /* 输出参数：                  无
        /* 返 回 值：                  成功返回版权信息内容，失败返回NULL
        /************************************************************************/

        API_EXPORT  char*   BLiveAppGetAbout();

        /************************************************************************/
        /* 函数功能：                  获取Appkey （扬帆主播用）
        /* 输入参数：                  key
        /* 输出参数：                  无
        /* 返 回 值：                  成功返回评论Appkey的值，互动会用，失败返回NULL
        /************************************************************************/

        API_EXPORT  char*   BLiveAppGetAppkey(char* key);

#ifdef  __cplusplus
}
#endif //  __cplusplus

#endif // CHTTPINTERFACESDK_H
