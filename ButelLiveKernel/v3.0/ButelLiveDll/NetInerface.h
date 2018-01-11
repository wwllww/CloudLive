#ifndef  _NET_INTERFACE_H_
#define  _NET_INTERFACE_H_
#include <string>
#include <vector>
#include <String>
using namespace std;
//#define  VERSION
static int RET_OK = 1000;
// 直播列表
typedef struct tagLiveChanel
{
    unsigned     channelId;		//	频道编号
    std::wstring channelName;    //	频道名称
    std::wstring pushUrl;  		//	播放地址
    std::wstring interactivenumber; //互动号
    std::wstring appkey;             //key
 }StLiveChanel;
typedef std::vector<StLiveChanel>  VEC_LIVECHANNEL;


typedef struct tagLogIn
{
    std::wstring          uid;          //       杨帆用户ID
    std::wstring          m_nickname;     //     用户昵称
    std::wstring          user_mobile;    //     用户手机号
    std::wstring          user_email;     //     邮箱
    std::wstring          user_url;  //          用户个人网站
    int                   sex;
    std::wstring          birthday;       //生日
    std::wstring          signature;      //个性签名
    int                   user_status;    //用户状态
    std::wstring           avatar_auto;    //用户自定义截取头像
    std::wstring           avatar_large;   //头像200*200
    std::wstring          avatar_big;     //头像100*100
    std::wstring          avatar_middle;   //头像50*50
    std::wstring          avatar_small;    //头像 30*30
    std::wstring          oauth_token;     //杨帆用户中心token
}StLogIn;

typedef std::vector<StLogIn>  VEC_LogIn;


typedef struct tagMenu
{
    std::wstring menuid;
    std::wstring menuname;
    int menutype;
    std::wstring menuurl;
}StMenu;
typedef std::vector<StMenu>  VEC_Menu;

typedef struct tagChanelDetail            //3.17
{
    std::wstring channelid;		//	频道id
    std::wstring channelName;    //	频道名称
    std::wstring pushUrl;  		//	播放地址
    std::wstring channelcover;  //  频道剧照
    std::wstring channeldesc;   //  频道描述
    int       channeltype;
    int       populartype;
    int       isvip;
    int       onlinecount;
    int       participationcount;
    int       scancount;
    int       bookcount;
    int       status;

    std::wstring playurl;
    std::wstring pushurl;
    std::wstring channelnumber;
    std::wstring interactivenumber;
    std::wstring sharepageurl;
    std::wstring coordinate;
    std::wstring templateconfig;
    std::wstring accesstype;
    std::wstring accessconfig;
    VEC_Menu menu;

}StChanelDetail;
typedef std::vector<StChanelDetail>  VEC_ChanelDetail;

typedef struct tagVideoList        //3.19
{

    std::wstring liveName;
    std::wstring coordinate;
    std::wstring stills;
    std::wstring playUrl;
    std::wstring programVideoId;

}StVideoList;

typedef std::vector<StVideoList>  VEC_VideoList;

typedef struct tagProgramList        //3.19
{

    std::wstring id;
    std::wstring programname;
    int          liveType;
    std::wstring starttime;
    std::wstring endtime;
    int status;
    int isrecord;
    VEC_VideoList VideoList;


}StChannelProgramList;
typedef std::vector<StChannelProgramList>  VEC_ProgramList;

typedef struct tagProgramSubMedias
{
    std::wstring name;
    std::wstring picurl;
    std::wstring recordplayurl;
    std::wstring coordinate;
    std::wstring playurl;
    int isrecord;
}StProgramSubMedias;

typedef std::vector<StProgramSubMedias>  VEC_ProgramSubMedias;
typedef struct tagProgramDetail             //3.22
{
    int id;
    std::wstring programName;
    int liveType;
    std::wstring startTime;
    std::wstring endTime;
    int status;
    int isRecord;
    int endStyle;
    int interactive;
    int autoRecord;
    VEC_VideoList VideoList;
}StProgramDetail;
typedef std::vector<StProgramDetail>   VEC_ProgramDetail;

typedef struct tagBrodcastChannelList                      //3.33
{
    std::wstring id;
    std::wstring name;

}StBrodcastChannelList;

typedef std::vector<StBrodcastChannelList> VEC_ChannelList;

// 直播列表
typedef struct tagLiveSchedule
{
    unsigned     scheduleId;  //		节目单编号
    std::wstring scheduleName;//		节目单名称
    std::wstring starttime;   //		节目开始时间
    std::wstring endtime;     //		节目结束时间
    std::wstring playUrl;     //		播放地址
    std::wstring artist;      //		播放地址

}StLiveSchedule;
typedef std::vector<StLiveSchedule>  VEC_LIVE_SCHEDULE;

typedef struct tagInitiateLive     //3.36
{
    std::wstring token;
    std::wstring programId;
    std::wstring channelId;
}StInitiateLive;

typedef struct tagOverLive     //3.35
{
    std::wstring uid;
    std::wstring yangfantoken;
    std::wstring channelid;
    std::wstring programid;

}StOverLive;

// 网络操作类，包括登陆，获取频道，获取节目单,都是同步接口
class CNetInerface
{
public:
    // 单例接口
    static CNetInerface *GetInstance();

    // 释放
    static void Destroy();

    // 设置服务器地址
    int  SetSerAddr(std::wstring strHttpPre);

    // 设置汇报关键字
    void  SetRtmpPort(std::wstring  strRtmpIP,unsigned  nRtmpPort);

    // 登陆操作
    int  Login(std::wstring strName, std::wstring strPwd);

    int  Login(std::wstring strName, std::wstring strPwd, std::wstring token,int accouttype, VEC_LogIn *pLogIn);                        //3.5

    int GetChannelDetail(std::wstring Token, std::wstring ChannelId, std::wstring UsrType, VEC_ChanelDetail * pChanelDetail);												  //3.17

    int GetProgramList(std::wstring token, std::wstring channelId, int pageidx, int pagesize, int status, std::wstring UsrType,
         VEC_ProgramList * pChannelProgramList);    //3.19

    int GetProgramDetail(std::wstring Token, std::wstring ProgramId, std::wstring UsrType, VEC_ProgramDetail *pProgramDetail);                      //3.22

    int GetChannelList(std::wstring yangfantoken, std::wstring type, int pageidx, int pagesize, VEC_ChannelList *pBrodcastChannelList); //3.33

    int GetParameterList(std::wstring key);

    int OverLive(StOverLive oOverLive);   //3.35

    int Onlive(StInitiateLive oInitiateLive);     //3.36

    // 接口授权
    int Authorize(const std::wstring &Appid);

    // 获取频道列表
    int  GetChannelUrl(std::wstring &url);

    // 获取错误描述
    std::wstring GetLastErrorDesc();

    // 获取错误描述
    void SetLastErrorDesc(std::wstring error);

    //开始或停止直播录制
    int StartOrStopRecord(int channelId,bool bStart = true);

    // 直播操作 nTypeStatus = 0 开始直播 nTypeStatus =1 暂停直播  nTypeStatus =2 停止直播
    int StartLive(int channelId, int nTypeStatus);

    //获取直播URL;
    std::vector<std::wstring> GetPlayUrls() const { return PlayURL; }

    const std::wstring&  GetToken(){ return m_strToken; }
    const std::wstring&  GetNumber(){ return m_strNumber; }
    const std::wstring&  GethttpPre(){ return m_httpPre; }

private:
    // 标准析构函数和构造函数
    CNetInerface();
    virtual ~CNetInerface();
private:
    static CNetInerface  *m_pInstance;
    std::wstring          m_strIp;
    std::wstring          m_strToken;
    std::wstring          m_AppId;
    std::wstring          m_strNumber;
    std::wstring          m_strLastErrorDesc;
    unsigned              m_nPort;

    std::wstring          m_strRtmpIP;
    unsigned              m_nRtmpPort;
    std::wstring          m_httpPre;
    std::wstring          m_httpGet;
   // HANDLE                m_hLock;
    std::vector<std::wstring> PlayURL;

    VEC_ChannelList m_BrodcastChannelList;

    //add 2016.7.25

};

std::wstring Asic2WChar(std::string str);
std::wstring Utf82WChar(char *pUtf8, int nLen);
int WcharToAnsi(std::wstring strSrc, char *pAnsi, int &nLen);
int WcharToAnsi(std::wstring strSrc, char *pAnsi, int &nLen);
#endif // _NET_INTERFACE_H_


