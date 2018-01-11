#include "NetInerface.h"
#include "base64.h"
#include "json/json.h"
//#include "HTTPClient.h"                   // http协议操作接口
#include "MD5.h"
#include <string>
CNetInerface *CNetInerface::m_pInstance = NULL;
#define  TEST_NO_NET
//Converting a Ansi string to WChar string

std::wstring Utf82WChar(char *pUtf8,int nLen)
{
    if (0 ==nLen)
    {
        return TEXT("");
    }
    int nSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUtf8, nLen, 0, 0);
    if (nSize <= 0) return NULL;

    WCHAR *pwszDst = new WCHAR[nSize + 1];
    if (NULL == pwszDst) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pUtf8, nLen, pwszDst, nSize);
    pwszDst[nSize] = 0;

    if (pwszDst[0] == 0xFEFF) // skip Oxfeff
    for (int i = 0; i < nSize; i++)
        pwszDst[i] = pwszDst[i + 1];

    std::wstring wcharString(pwszDst);
    delete pwszDst;
    return wcharString;
}

std::wstring Asic2WChar(std::string str)
{
    if (str.empty())
    {
        return TEXT("");
    }
    int nSize = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
    if (nSize <= 0) return NULL;

    WCHAR *pwszDst = new WCHAR[nSize + 1];
    if (NULL == pwszDst) return NULL;

    MultiByteToWideChar(CP_ACP, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
    pwszDst[nSize] = 0;

// 	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
// 	for (int i = 0; i < nSize; i++)
// 		pwszDst[i] = pwszDst[i + 1];

    std::wstring wcharString(pwszDst);
    delete pwszDst;
    return wcharString;
}

int WcharToUtf8(std::wstring strSrc,char *pUtf8, int &nLen)
{
    if (strSrc.empty())
    {
        nLen = 0;
        return 0;
    }
    int nSize =  WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), pUtf8, nLen,NULL,NULL);
    pUtf8[nSize] = 0;
    nLen = nSize;
    return 0;
}

int WcharToAnsi(std::wstring strSrc, char *pAnsi, int &nLen)
{
    if (strSrc.empty())
    {
        nLen = 0;
        return 0;
    }
    int nSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), pAnsi, nLen, NULL, NULL);
    pAnsi[nSize] = 0;
    nLen = nSize;
    return 0;
}

std::wstring Utf82WChar(std::string str)
{
    if (str.empty())
    {
        return TEXT("");
    }
    int nSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
    if (nSize <= 0) return NULL;

    WCHAR *pwszDst = new WCHAR[nSize + 1];
    if (NULL == pwszDst) return NULL;

    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
    pwszDst[nSize] = 0;

    if (pwszDst[0] == 0xFEFF) // skip Oxfeff
    for (int i = 0; i < nSize; i++)
        pwszDst[i] = pwszDst[i + 1];

    std::wstring wcharString(pwszDst);
    delete pwszDst;
    return wcharString;
}
// std::wstring WChar2Utf8(std::wstring str)
// {
// 	int nSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), 0, 0);
// 	if (nSize <= 0) return NULL;
//
// 	WCHAR *pwszDst = new WCHAR[nSize + 1];
// 	if (NULL == pwszDst) return NULL;
//
// 	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)str.c_str(), str.size(), pwszDst, nSize);
// 	pwszDst[nSize] = 0;
//
// 	if (pwszDst[0] == 0xFEFF) // skip Oxfeff
// 	for (int i = 0; i < nSize; i++)
// 		pwszDst[i] = pwszDst[i + 1];
//
// 	std::wstring wcharString(pwszDst);
// 	delete pwszDst;
// 	return wcharString;
// }

// 替换字符串中特征字符串为指定字符串
inline void CSreplace(std::string& strSrc, std::string strPlace, std::string strNew, int count = -1)
{
    std::string::size_type pos = 0;
    std::string::size_type a = strPlace.size();
    std::string::size_type b = strNew.size();

    if (count == -1){
        while ((pos = strSrc.find(strPlace, pos)) != std::string::npos)
        {
            strSrc.replace(pos, a, strNew);
            pos += b;
        }
    }
    else{
        int c = 0;
        while ((pos = strSrc.find(strPlace, pos)) != std::string::npos)
        {
            strSrc.replace(pos, a, strNew);
            pos += b;
            if (++c == count)
                return;
        }
    }
}

CNetInerface::CNetInerface()
{
    m_strIp = TEXT("");
    m_nPort = 80;
    m_hLock = OSCreateMutex();
}


CNetInerface::~CNetInerface()
{
    if (NULL != m_hLock)
    {
        OSCloseMutex(m_hLock);
    }
}

CNetInerface * CNetInerface::GetInstance()
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CNetInerface;
    }
    return m_pInstance;
}

void CNetInerface::Destroy()
{
    if (NULL != m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = NULL;
    }
}

int CNetInerface::SetSerAddr(std::wstring strHttpPre)
{
    if (strHttpPre.empty())
    {
        return -1;
    }
    m_httpPre = strHttpPre;
    #ifdef VERSION
    String IP = AppConfig->GetString(TEXT("Login"), TEXT("IP"), L"NULL");
    String httpGet = FormattedString(TEXT("http://%s/setsail/external/externalService?service="), IP.Array());
    m_httpGet = httpGet;
    #endif
    return 0;
}
#define  HTTP_BUFFER_LEN  (1024 * 10*10)
#define  VAR_LEN  (256)

#ifdef  VERSION
int  CNetInerface::Login(std::wstring strName, std::wstring strPwd, std::wstring token, int accouttype, VEC_LogIn *pLogIn)
{
    if (strName.empty() || strPwd.empty())
    {
        return -1;
    }
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String UrlPre = m_httpPre.c_str();
    UrlPre.FindReplace(TEXT("webapi/"), TEXT(""));
    char utf8Post[1024];
    int  utf8Len = 1024;

    //WcharToUtf8(strPwd.c_str(), utf8Post, utf8Len);
    //strPwd = MD5(utf8Post, utf8Len).toString();
    String &url = FormattedString(TEXT("%slogin?login=%s&password=%s&pushToken=%s&userType=%d&imei=imei&client=pc"), UrlPre.Array(), strName.c_str(), strPwd.c_str(), token.c_str(), accouttype);
    String post = FormattedString(TEXT("login=%s&password=%s&pushToken=%s&userType=%d&imei=imei&client=pc"), strName.c_str(), strPwd.c_str(), token.c_str(),accouttype);
    utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);
    //  	delete pPwd;
    //  	pPwd = NULL;

    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error =L"账号或密码错误" ;//Asic2WChar(root["message"].asString());
            Log(TEXT("login failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
        StLogIn oLogIn;
        oLogIn.uid = Asic2WChar((root["data"]["uid"].asString()).c_str());
        AppConfig->SetString(TEXT("Login"), TEXT("Uid"), oLogIn.uid.c_str());

        oLogIn.m_nickname = Asic2WChar((root["data"]["user_nickname"].asString()).c_str());
        AppConfig->SetString(TEXT("Login"), TEXT("NickName"), oLogIn.m_nickname.c_str());

        oLogIn.oauth_token = Asic2WChar((root["data"]["oauth_token"].asString()).c_str());
        AppConfig->SetString(TEXT("Login"), TEXT("Yanfantoken"), oLogIn.oauth_token.c_str());

        pLogIn->clear();

        pLogIn->push_back(oLogIn);

    }
    return RET_OK;
}

int CNetInerface::GetChannelDetail(std::wstring Token, std::wstring ChannelId, std::wstring UsrType, VEC_ChanelDetail * pChanelDetail)
{
    if (ChannelId.empty() || !pChanelDetail)
    {
        Log(TEXT("GetChannelDetail paramIn error!"));
        return -1;
    }

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(TEXT("%sgetChannelDetail&params="), m_httpGet.c_str());

    int nLen = VAR_LEN;
    char channelid_c[VAR_LEN];
    WcharToAnsi(ChannelId, channelid_c, nLen);

    nLen = VAR_LEN;
    char token_c[VAR_LEN];
    WcharToAnsi(Token, token_c, nLen);

    nLen = VAR_LEN;
    char usrtype_c[VAR_LEN];
    WcharToAnsi(UsrType, usrtype_c, nLen);

    Json::Value new_item;
    new_item["token"] = token_c;
    new_item["userType"] = usrtype_c;
    new_item["channelId"] = channelid_c;

    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error = Asic2WChar(root["message"].asString());
            Log(TEXT("login failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }

        pChanelDetail->clear();

        StChanelDetail oChanelDetail;
        oChanelDetail.channelid = Asic2WChar(root["data"]["id"].asString());
        oChanelDetail.channelName = Asic2WChar(root["data"]["name"].asString());
        oChanelDetail.pushUrl = Asic2WChar(root["data"]["pushUrl"].asString());
        oChanelDetail.channelcover = Asic2WChar(root["data"]["channelcover"].asString());
        oChanelDetail.channeldesc = Asic2WChar(root["data"]["channeldesc"].asString());

        oChanelDetail.channeltype = root["data"]["channeltype"].asInt();
        oChanelDetail.channeltype = root["data"]["populartype"].asInt();
        oChanelDetail.channeltype = root["data"]["isvip"].asInt();
        oChanelDetail.channeltype = root["data"]["onlinecount"].asInt();
        oChanelDetail.channeltype = root["data"]["participationcount"].asInt();
        oChanelDetail.channeltype = root["data"]["scancount"].asInt();
        oChanelDetail.channeltype = root["data"]["bookcount"].asInt();
        oChanelDetail.channeltype = root["data"]["status"].asInt();

        oChanelDetail.playurl = Asic2WChar(root["data"]["playUrl"].asString());
        AppConfig->SetString(TEXT("Login"), TEXT("PlayUrl"), oChanelDetail.playurl.c_str());

        oChanelDetail.pushurl = Asic2WChar(root["data"]["pushUrl"].asString());
        AppConfig->SetString(TEXT("Login"), TEXT("PushUrl"), oChanelDetail.pushurl.c_str());

        oChanelDetail.channelnumber = Asic2WChar(root["data"]["channelNumber"].asString());
        AppConfig->SetString(TEXT("Login"), TEXT("ChannelNumber"), oChanelDetail.channelnumber.c_str());

        oChanelDetail.interactivenumber = Asic2WChar(root["data"]["interactiveNumber"].asString());
        AppConfig->SetString(TEXT("Login"), TEXT("InteractiveNumber"), oChanelDetail.interactivenumber.c_str());


        oChanelDetail.sharepageurl = Asic2WChar(root["data"]["sharepageurl"].asString());
        oChanelDetail.coordinate = Asic2WChar(root["data"]["coordinate"].asString());
        oChanelDetail.templateconfig = Asic2WChar(root["data"]["templateconfig"].asString());
        oChanelDetail.accesstype = Asic2WChar(root["data"]["accesstype"].asString());
        oChanelDetail.accessconfig = Asic2WChar(root["data"]["accessconfig"].asString());
        Json::Value& channelsJson = root["data"]["menus"];
        for (int i = 0; i < channelsJson.size(); ++i)
        {
            StMenu omenu;
            omenu.menuid = Asic2WChar(channelsJson[i]["menuid"].asString());
            omenu.menuname = Asic2WChar(channelsJson[i]["menuname"].asString());
            omenu.menuurl = Asic2WChar(channelsJson[i]["menuurl"].asString());
            omenu.menutype = channelsJson[i]["menutype"].asInt();

            oChanelDetail.menu.push_back(omenu);
        }

        pChanelDetail->push_back(oChanelDetail);
    }

    return RET_OK;
}

int CNetInerface::GetProgramList(std::wstring token, std::wstring channelId, int pageidx, int pagesize, int status, std::wstring UsrType,
                                     VEC_ProgramList * pChannelProgramList)
{
    if (token.empty() || channelId.empty())
    {
        Log(TEXT("GetProgramList paramIn error!"));
        return -1;
    }

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(TEXT("%sgetProgramList&params="), m_httpGet.c_str());

    int nLen = VAR_LEN;
    char yangfantoken_c[VAR_LEN];
    WcharToAnsi(token, yangfantoken_c, nLen);

    nLen = VAR_LEN;
    char channelId_c[VAR_LEN];
    WcharToAnsi(channelId, channelId_c, nLen);

    nLen = VAR_LEN;
    char UsrType_c[VAR_LEN];
    WcharToAnsi(UsrType, UsrType_c, nLen);


    Json::Value new_item;
    new_item["token"] = yangfantoken_c;
    new_item["channelId"] = channelId_c;
    new_item["userType"] = UsrType_c;
    new_item["idx"] = pageidx;
    new_item["size"] = pagesize;
    new_item["status"] = status;


    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error = Asic2WChar(root["message"].asString());
            Log(TEXT("login failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }

        if (!pChannelProgramList)
        {
            return 0;
        }

        int TotalCount = root["data"]["totalCount"].asInt();

        AppConfig->SetInt(TEXT("ProGram"), TEXT("TotalCount"), 0);

        //pChannelProgramList->clear();

        Json::Value& channelsJson = root["data"]["rows"];

        if (channelsJson.size() > 0)
        {
            for (int i = 0; i < channelsJson.size(); ++i)
            {
                StChannelProgramList oChannelProgramList;
                oChannelProgramList.id = Asic2WChar(channelsJson[i]["id"].asString());
                oChannelProgramList.programname = Asic2WChar(channelsJson[i]["channelName"].asString());
                oChannelProgramList.starttime = Asic2WChar(channelsJson[i]["starttime"].asString());
                oChannelProgramList.endtime = Asic2WChar(channelsJson[i]["endtime"].asString());

                oChannelProgramList.status = channelsJson[i]["status"].asInt();
                oChannelProgramList.isrecord = channelsJson[i]["isrecord"].asInt();

                Json::Value& VideoList = channelsJson[i]["videoList"];

                if (VideoList.size() > 0)
                {

                    for (int index = 0; index < VideoList.size(); ++index)
                    {
                        StVideoList oVideoList;
                        oVideoList.liveName = Asic2WChar(VideoList[index]["liveName"].asString());
                        oVideoList.coordinate = Asic2WChar(VideoList[index]["coordinate"].asString());
                        oVideoList.stills = Asic2WChar(VideoList[index]["stills"].asString());
                        oVideoList.playUrl = Asic2WChar(VideoList[index]["playUrl"].asString());
                        oVideoList.programVideoId = Asic2WChar(VideoList[index]["programVideoId"].asString());

                        oChannelProgramList.VideoList.push_back(oVideoList);
                    }
                }

                pChannelProgramList->push_back(oChannelProgramList);
            }

            if (TotalCount - pagesize*(pageidx+1) > 0)
            {
                pageidx++;
                int code = GetProgramList(token, channelId, pageidx, pagesize, status, UsrType, pChannelProgramList);
                return code;
            }

        }
        else
        {
            std::wstring error = L"节目列表为空！";
            Log(TEXT("GetProgramList failue error %d %s "), -100, error.c_str());
            SetLastErrorDesc(error);
            return -100;
        }
    }
    return RET_OK;
}

int CNetInerface::GetParameterList(std::wstring key)
{
    if (key.empty())
    {
        Log(TEXT("GetParameterList paramIn error!"));
        return -1;
    }

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(TEXT("%sgetParameterList&params="), m_httpGet.c_str());

    int nLen = VAR_LEN;
    char key_c[VAR_LEN];
    WcharToAnsi(key, key_c, nLen);

    Json::Value new_item;
    new_item["keys"] = key_c;
    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);

    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);

    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (nResult != RET_OK)
        {
            std::wstring error = Asic2WChar(root["msg"].asString());
            Log(TEXT("GetParameterList failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
        Json::Value& channelsJson = root["data"]["rows"];

        if (channelsJson.size() > 0)
        {
            for (int i = 0; i < channelsJson.size(); ++i)
            {
                std::wstring usercenter_appkey = L"usercenter_appkey";
                std::wstring name = Asic2WChar((channelsJson[i]["name"].asString()).c_str());

                if (!name.compare(usercenter_appkey))
                {
                    std::wstring name_Value = Asic2WChar((channelsJson[i]["value"].asString()).c_str());
                    AppConfig->SetString(TEXT("Login"), TEXT("AppKey"), name_Value.c_str());
                }
            }
        }
    }
    return RET_OK;
}

int CNetInerface::GetProgramDetail(std::wstring Token, std::wstring ProgramId, std::wstring UsrType, VEC_ProgramDetail *pProgramDetail)
{
    if (ProgramId.empty() || Token.empty() || UsrType.empty())
    {
        Log(TEXT("GetProgramDetail paramIn error!"));
        return -1;
    }

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(TEXT("%sgetProgramDetail&params="), m_httpGet.c_str());
    int nLen = VAR_LEN;
    char programid_c[VAR_LEN];
    WcharToAnsi(ProgramId, programid_c, nLen);

    nLen = VAR_LEN;
    char token_c[VAR_LEN];
    WcharToAnsi(Token, token_c, nLen);

    nLen = VAR_LEN;
    char usrtype_c[VAR_LEN];
    WcharToAnsi(UsrType, usrtype_c, nLen);

    Json::Value new_item;
    new_item["token"] = token_c;
    new_item["userType"] = usrtype_c;
    new_item["programId"] = programid_c;

    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error = Asic2WChar(root["msg"].asString());
            Log(TEXT("GetProgramDetail failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }

        if (!pProgramDetail)
        {
            return 0;
        }

        StProgramDetail oProgramDetail;
        oProgramDetail.programName = Asic2WChar(root["data"]["programName"].asString());
        AppConfig->SetString(TEXT("Login"), TEXT("programName"), oProgramDetail.programName.c_str());

        oProgramDetail.liveType = root["data"]["liveType"].asInt();
        oProgramDetail.startTime = Asic2WChar(root["data"]["startTime"].asString());
        oProgramDetail.endTime = Asic2WChar(root["data"]["endTime"].asString());
        oProgramDetail.status = root["data"]["status"].asInt();
        oProgramDetail.isRecord = root["data"]["isRecord"].asInt();
        oProgramDetail.endStyle = root["data"]["endStyle"].asInt();
        oProgramDetail.interactive = root["data"]["interactive"].asInt();
        oProgramDetail.autoRecord = root["data"]["autoRecord"].asInt();
        //添加一个vector
        Json::Value& VideoList = root["data"]["VideoList"];

        if (VideoList.size() > 0)
        {
            for (int i = 0; i < VideoList.size(); ++i)
            {
                StVideoList oVideoList;
                oVideoList.liveName = Asic2WChar(VideoList[i]["liveName"].asString());
                oVideoList.coordinate = Asic2WChar(VideoList[i]["coordinate"].asString());
                oVideoList.stills = Asic2WChar(VideoList[i]["stills"].asString());
                oVideoList.playUrl = Asic2WChar(VideoList[i]["playUrl"].asString());
                oProgramDetail.VideoList.push_back(oVideoList);
            }
        }
        else
        {
            return -100;
        }

        pProgramDetail->clear();
        pProgramDetail->push_back(oProgramDetail);

    }
    else
    {
        return -100;
    }
    return 0;
}

int CNetInerface::GetChannelList(std::wstring yangfantoken, std::wstring type, int pageidx, int pagesize, VEC_ChannelList *pBrodcastChannelList)
{
    if (yangfantoken.empty() || type.empty())
    {
        Log(TEXT("GetChannelList paramIn error!"));
        return -1;
    }

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(TEXT("%sgetAnchorChannelList&params="), m_httpGet.c_str());

    int nLen = VAR_LEN;
    char yangfantoken_c[VAR_LEN];
    WcharToAnsi(yangfantoken, yangfantoken_c, nLen);

    nLen = VAR_LEN;
    char type_c[VAR_LEN];
    WcharToAnsi(type, type_c, nLen);

    Json::Value new_item;
    new_item["token"] = yangfantoken_c;
    new_item["type"] = type_c;
    new_item["idx"] = pageidx;
    new_item["size"] = pagesize;

    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);

    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);

    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (nResult != RET_OK)
        {
            std::wstring error = Asic2WChar(root["msg"].asString());
            Log(TEXT("GetChannelList failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }

        int totalcount = root["data"]["totalCount"].asInt();
        Json::Value& channelsJson = root["data"]["rows"];

        pBrodcastChannelList->clear();

        if (channelsJson.size() > 0)
        {
            for (int i = 0; i < channelsJson.size(); ++i)
            {

                StBrodcastChannelList oBrodcastChannelList;
                oBrodcastChannelList.id = Asic2WChar(channelsJson[i]["id"].asString());
                oBrodcastChannelList.name = Asic2WChar(channelsJson[i]["name"].asString());
                pBrodcastChannelList->push_back(oBrodcastChannelList);
            }
        }
        else
        {
            std::wstring error = L"频道列表为空！";
            Log(TEXT("GetProgramList failue error %d %s "), -100, error.c_str());
            SetLastErrorDesc(error);
            return -100;
        }
    }
    return RET_OK;
}

int CNetInerface::OverLive(StOverLive oOverLive)
{
    if (oOverLive.yangfantoken.empty() || oOverLive.channelid.empty() || oOverLive.programid.empty())
    {
        Log(TEXT("OverLive param error!"));
        return -1;
    }
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String UrlPre = m_httpPre.c_str();
    //UrlPre.FindReplace(TEXT("webapi/"), TEXT(""));
    char utf8Post[1024];
    int  utf8Len = 1024;
    String url = FormattedString(TEXT("%soverLive&params="), m_httpGet.c_str());

    int nLen = VAR_LEN;
    char token_c[VAR_LEN];
    WcharToAnsi(oOverLive.yangfantoken, token_c, nLen);
    nLen = VAR_LEN;
    char programId_c[VAR_LEN];
    WcharToAnsi(oOverLive.programid, programId_c, nLen);
    nLen = VAR_LEN;
    char channelId_c[VAR_LEN];
    WcharToAnsi(oOverLive.channelid, channelId_c, nLen);
    Json::Value new_item;
    new_item["token"] = token_c;
    new_item["channelId"] = channelId_c;
    new_item["programId"] = programId_c;
    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;
    utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);

    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }

    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error = Asic2WChar(root["msg"].asString());
            Log(TEXT("OverLive failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
    }
    return RET_OK;
}

int CNetInerface::Onlive(StInitiateLive oInitiateLive)
{
    if (oInitiateLive.token.empty() || oInitiateLive.programId.empty() || oInitiateLive.channelId.empty())
    {
        Log(TEXT("Onlive param error!"));
        return -1;
    }
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String UrlPre = m_httpPre.c_str();

    char utf8Post[1024];
    int  utf8Len = 1024;
    String url = FormattedString(TEXT("%sonLive&params="), m_httpGet.c_str());


    int nLen = VAR_LEN;
    char token_c[VAR_LEN];
    WcharToAnsi(oInitiateLive.token, token_c, nLen);

    nLen = VAR_LEN;
    char programId_c[VAR_LEN];
    WcharToAnsi(oInitiateLive.programId, programId_c, nLen);

    nLen = VAR_LEN;
    char channelId_c[VAR_LEN];
    WcharToAnsi(oInitiateLive.channelId, channelId_c, nLen);

    Json::Value new_item;
    new_item["token"] = token_c;
    new_item["channelId"] = channelId_c;
    new_item["programId"] = programId_c;


    std::wstring postjson = Asic2WChar(new_item.toStyledString());
    String post = postjson.c_str();
    url += post;

    utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);

    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);

    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }

    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"].asInt();
        if (RET_OK != nResult)
        {
            std::wstring error = Asic2WChar(root["message"].asString());
            Log(TEXT("InitiateLive failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
    }

    return RET_OK;
}

#else

int CNetInerface::Login(std::wstring strName, std::wstring strPwd)
{
    if (strName.empty() || strPwd.empty())
    {
        return -1;
    }
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String UrlPre = m_httpPre.c_str();
    UrlPre.FindReplace(TEXT("webapi/"), TEXT(""));
    String &url = FormattedString(TEXT("%sinternal/usergroup/login"), UrlPre.Array());

    char utf8Post[1024];
    int  utf8Len = 1024;
    WcharToUtf8(strPwd.c_str(), utf8Post, utf8Len);
    strPwd = MD5(utf8Post, utf8Len).toString();
    String post = FormattedString(TEXT("{'account':'%s','pwd':'%s'}"), strName.c_str(), strPwd.c_str());
     utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);
//  	delete pPwd;
//  	pPwd = NULL;
    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        bool bisInt = root["state"]["rc"].isInt();
        int nResult = -1;
        if (bisInt)
        {
            nResult = root["state"]["rc"].asInt();
        }
        else
        {
            nResult = atoi(root["state"]["rc"].asString().c_str());
        }
        if (0 != nResult)
        {
            std::wstring  error  = Asic2WChar(root["state"]["msg"].asString());
            Log(TEXT("login failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
        Json::Value &Enterprises = root["result"]["enterprises"];
        if (Enterprises.size() > 0)
        {
            m_AppId = Asic2WChar(Enterprises[Json::UInt(0)]["appid"].asString()).c_str();
        }
        else
        {
            std::wstring  error = TEXT("该用户不属于任何企业!");
            SetLastErrorDesc(error);
            return -22;
        }
        m_strNumber = Asic2WChar(root["result"]["nubernum"].asString()).c_str();
    }
    return Authorize(m_AppId);
}

#endif
int CNetInerface::Authorize(const std::wstring &AppId)
{
    if (AppId.empty())
    {
        return -1;
    }
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String &url = FormattedString(TEXT("%saccount/authorize"), m_httpPre.c_str());

    char utf8Post[1024] = {0};
    int  utf8Len = 1024;
    String post = FormattedString(TEXT("{'appid':'%s'}"), AppId.c_str());
    utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);

    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        return -21;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"]["rc"].asInt();
        if (0 != nResult)
        {
            std::wstring  error = Asic2WChar(root["state"]["msg"].asString());
            Log(TEXT("login failue error %d %s "), nResult, error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }

        m_strToken = Asic2WChar(root["result"]["token"].asString());
    }
    return 0;
}

int CNetInerface::GetChannelUrl(std::wstring url)
{
    //const unsigned long BUFF_LEN = 0;
    //char html[HTTP_BUFFER_LEN];
    //unsigned htmlBufLen = HTTP_BUFFER_LEN;
    //int nHttpCode = 0;
    //int bufLen = HTTP_BUFFER_LEN;
    url = FormattedString(TEXT("%slivechannel/getlist?"), m_httpPre.c_str());
    String post = FormattedString(TEXT("token=%s&creator=%s&startindex=0&pagecount=10"), m_strToken.c_str(), m_strNumber.c_str());
    url += post;

    //DWORD dwRet = HTTPGetFile(url, html, htmlBufLen, NULL, &nHttpCode);
    //if (0 != dwRet || 0 == htmlBufLen)
    //{
    //	wchar_t error[1024];
    //	wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
    //	SetLastErrorDesc(error);
    //	return -21;
    //}
    //std::wstring strHtml = Utf82WChar(html, htmlBufLen);
    //Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());

    //WcharToAnsi(strHtml, html, bufLen);
    //std::string ansihtml;
    //ansihtml.append(html, bufLen);
    ////CSreplace(ansihtml, "\\\"", "\"");
    //Json::Reader reader;
    //Json::Value root;
    //if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    //{
    //	int nResult = root["state"]["rc"].asInt();
    //	if (0 != nResult)
    //	{
    //		std::wstring error = Asic2WChar(root["state"]["msg"].asString());
    //		Log(TEXT("login failue error %d %s "), nResult, error.c_str());
    //		SetLastErrorDesc(error);
    //		return nResult;
    //	}
    //	Json::Value& channelsJson = root["pagingrows"]["rows"];
    //	channels.clear();
    //	PlayURL.clear();
    //	if (channelsJson.size() > 0)
    //	{
    //		for (int i = 0; i < channelsJson.size(); ++i)
    //		{
    //			StLiveChanel channel;
    //			channel.channelId = channelsJson[i]["id"].asUInt();
    //			channel.channelName = Asic2WChar(channelsJson[i]["name"].asString());
    //			channel.pushUrl = Asic2WChar(channelsJson[i]["pushstreamurl"].asString());

    //			Json::Value &PlayURLs = channelsJson[i]["playurls"];
    //			for (int j = 0; j < PlayURLs.size(); ++j)
    //			{
    //				PlayURL.push_back(Asic2WChar(PlayURLs[Json::Value::UInt(j)]["url"].asString()));
    //			}
    //			channels.push_back(channel);
    //		}
    //	}
    //	else
    //	{
    //		return -100;
    //	}
    //}
    return 0;
}

std::wstring CNetInerface::GetLastErrorDesc()
{
    std::wstring error;
    OSEnterMutex(m_hLock);
    error = m_strLastErrorDesc;
    OSLeaveMutex(m_hLock);
    return error;
}

int CNetInerface::StartOrStopRecord(int channelId,bool bStart /*= true*/)
{
    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;
    String url = FormattedString(bStart ? TEXT("%slivecontent/startrecord") : TEXT("%slivecontent/stoprecord"), m_httpPre.c_str());
    String post = FormattedString(TEXT("{'token':'%s','channelid':%u,'serverip':'%s','serverport':%u}"), m_strToken.c_str(), channelId,m_strRtmpIP.c_str(),m_nRtmpPort);
    char utf8Post[1024];
    int  utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);
    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        Log(bStart ? TEXT("发送开始录制命令失败！URL = %s,post = %s,错误代码%d") : TEXT("发送停止录制命令失败！URL = %s,post = %s,错误代码%d"),
            url.Array(), post.Array(), nHttpCode);
        return nHttpCode;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["rc"].asInt();
        if (nResult < 0)
        {
            std::wstring error = Asic2WChar(root["msg"].asString());
            Log(bStart ? TEXT("开始录制命令执行失败！错误描述：%s") : TEXT("停止录制命令执行失败！错误描述：%s"),
                error.c_str());
            SetLastErrorDesc(error);
            return nResult;
        }
    }
    return 0;
}

int CNetInerface::StartLive(int channelId, int nTypeStatus)
{
    return 0;//不发送录制命令

    char html[HTTP_BUFFER_LEN];
    unsigned htmlBufLen = HTTP_BUFFER_LEN;
    int nHttpCode = 0;
    int bufLen = HTTP_BUFFER_LEN;

    String url = L"";
    String strDesc = L"";
    if (0 == nTypeStatus)
    {
        url = FormattedString(TEXT("%slivecontent/start") , m_httpPre.c_str());
        strDesc = L"开始直播:";
    }
    else if (1 == nTypeStatus)
    {
        url = FormattedString(TEXT("%slivecontent/pause"), m_httpPre.c_str());
        strDesc = L"暂停直播:";
    }
    else if(2 == nTypeStatus)
    {
        url = FormattedString(TEXT("%slivecontent/stop"), m_httpPre.c_str());
        strDesc = L"停止直播:";
    }
    else
    {
        return -1;
    }
    String post = FormattedString(TEXT("{'token':'%s','channelid':%u}"), m_strToken.c_str(), channelId);
    char utf8Post[1024];
    int  utf8Len = 1024;
    WcharToUtf8(post.Array(), utf8Post, utf8Len);
    DWORD dwRet = HTTPPostFile(url, html, htmlBufLen, utf8Post, utf8Len, NULL, &nHttpCode);
    if (0 != dwRet || 0 == htmlBufLen)
    {
        wchar_t error[1024];
        wsprintf(error, TEXT("网络出现异常，错误代码%d"), dwRet);
        SetLastErrorDesc(error);
        Log(TEXT("发送%s命令失败！URL = %s,post = %s,错误代码%d"), strDesc.Array(),
            url.Array(), post.Array(), nHttpCode);
        return nHttpCode;
    }
    std::wstring strHtml = Utf82WChar(html, htmlBufLen);

    WcharToAnsi(strHtml, html, bufLen);
    std::string ansihtml;
    ansihtml.append(html, bufLen);
    //CSreplace(ansihtml, "\\\"", "\"");
    Log(TEXT("url %s htmp %s"), url.Array(), strHtml.c_str());
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(ansihtml, root))  // reader将Json字符串解析到root，root将包含Json里所有子元素
    {
        int nResult = root["state"]["rc"].asInt();
        if (nResult < 0)
        {
            String LastErrorDesc = Asic2WChar(root["state"]["msg"].asString()).c_str();
            LastErrorDesc += nResult;
            Log(TEXT("%s命令执行失败！错误描述：%s"), strDesc.Array(), LastErrorDesc.Array());
            SetLastErrorDesc(LastErrorDesc.Array());
            return nResult;
        }
    }
    return 0;
}

void CNetInerface::SetRtmpPort(std::wstring strRtmpIP, unsigned nRtmpPort)
{
    m_strRtmpIP = strRtmpIP;
    m_nRtmpPort = nRtmpPort;
    return ;
}

void CNetInerface::SetLastErrorDesc(std::wstring error)
{
    OSEnterMutex(m_hLock);
    m_strLastErrorDesc = error;
    OSLeaveMutex(m_hLock);
    return ;
}
