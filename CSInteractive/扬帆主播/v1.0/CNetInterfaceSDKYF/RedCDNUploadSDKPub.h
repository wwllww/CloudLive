#ifndef __REDCDNUPLOADSDKPUB_H__
#define __REDCDNUPLOADSDKPUB_H__

#ifdef WIN32
#ifdef _REDCDNUPLOADSDK_PUB
#define REDCDNUPLOADSDK_PUB_API __declspec(dllexport)
#else
#define REDCDNUPLOADSDK_PUB_API __declspec(dllimport)
#endif
#else
#define REDCDNUPLOADSDK_PUB_API extern "C" __attribute__ ((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" 
{	
#endif

	/************************************************************************/
	/* 函数名：  on_UploadLogin
	/* 功能：    上传登录回调
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           nRet		        -->回调的错误码
	/*           pToken    	        -->鉴权Token
	/*           ctx  	            -->上下文参数
	/************************************************************************/
	typedef void (*on_UploadLogin)(void* pHandle,int nRet,char* pToken,long long ctx);

	/************************************************************************/
	/* 函数名：  on_UploadGetAddr
	/* 功能：    获取配置信息回调
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           nRet		        -->回调的错误码
	/*           pFileUploadUrl     -->文件上传服务器地址
	/*           pFileProcUrl       -->媒体处理服务器地址
	/*           ctx  	            -->上下文参数
	/************************************************************************/
	typedef void (*on_UploadGetAddr)(void* pHandle,int nRet,char* pFileUploadUrl,char* pFileProcUrl,long long ctx);

	/************************************************************************/
	/* 函数名：  on_FileUpload
	/* 功能：    上传回调
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           nRet		        -->回调的错误码（为0表示上传成功）
	/*           bUploadFinished    -->是否上传成功
	/*           pUploadRate    	-->文件上传进度（百分比）
	/*           pCreator           -->上传标识
	/*           pUploadSpeed       -->上传速度
	/*           pFileDownloadUrl   -->上传完成后文件的下载地址
	/*           pHost		        -->断点文件上传地址，对应于http头部host信息
	/*           pRange             -->断点上传失败返回的Range
	/*           pCID               -->断点上传失败返回的pCID
	/*           bAbortUpload       -->是否是主动中断上传
	/*           ctx  	            -->上下文参数
	/************************************************************************/
	//typedef void (*on_FileUpload)(void* pHandle,int nRet,bool bUploadFinished,char* pUploadRate,char* pCreator,char* pUploadSpeed,char* pFileDownloadUrl,char* pHost,char* pRange,char* pCID,bool bAbortUpload,long long ctx);
	
	typedef void (*on_FileUpload)(void* pHandle,int nRet,bool bUploadFinished,int nUploadRate,char* pCreator,char* pUploadSpeed,char* pFileDownloadUrl,char* pHost,char* pRange,char* pCID,bool bAbortUpload,long long ctx);

	
	/************************************************************************/
	/* 函数名：  on_AbortUpload
	/* 功能：    中断上传回调
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           nRet		        -->回调的错误码
	/*           bAbortUploadSucc   -->是否中断成功
	/*           pCreator           -->上传标识
	/*           ctx   		        -->上下文参数
	/************************************************************************/
	typedef void (*on_AbortUpload)(void* pHandle,int nRet,bool bAbortUploadSucc,char* pCreator,long long ctx);	

		
	/************************************************************************/
	/* 函数名：  on_VodFileProc
	/* 功能：    点播文件处理统一回调
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           nRet		        -->回调的错误码
	/*           bProcessSucc       -->点播文件处理是否成功
	/*           pTaskID            -->用于任务查询的任务ID
	/*           ctx  	            -->上下文参数
	/************************************************************************/
	typedef void (*on_VodFileProc)(void* pHandle,int nRet,bool bProcessSucc,char* pTaskID,long long ctx);

	/************************************************************************/
	/* 函数名：  CreatUpload
	/* 功能：    创建Handle
	/* 输入参数：
	/*  
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void* CreatUpload();

	/************************************************************************/
	/* 函数名：  SetUploadFileLength
	/* 功能：    创建Handle
	/* 输入参数：
	/*  
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API int SetUploadFileLength(void* pHandle,long long nUploadLength);


	/************************************************************************/
	/* 函数名：  UploadLogin
	/* 功能：    文件上传登录
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           pUrl		        -->请求的URL
	/*           pAppid             -->Appid
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->登录回调指针
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void UploadLogin(void* pHandle,char* pUrl,char* pAppid,long long ctx,on_UploadLogin cb);

	/************************************************************************/
	/* 函数名：  UploadGetAddr
	/* 功能：    获取上传服务器地址和媒体处理服务器地址
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄
	/*           pUrl		        -->请求的URL  
	/*           pToken             -->鉴权Token
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->回调指针
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void UploadGetAddr(void* pHandle,char* pUrl,char* pToken,long long ctx,on_UploadGetAddr cb);

	/************************************************************************/
	/* 函数名：  FileUpload
	/* 功能：    文件上传
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄 
	/*           pFileUploadUrl		-->上传服务器地址URL
	/*           pToken             -->鉴权Token
	/*           pCreator           -->上传标识
	/*           pFileName		    -->上传的文件名
	/*           nTryTimes          -->网络异常的重试次数
	/*           pContext           -->第三方回调上下文信息
	/*           pCallBack          -->回调第三方的url地址
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->上传回调指针	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void FileUpload(void* pHandle,char* pFileUploadUrl,char* pToken,char* pCreator,char* pFileName,int nTryTimes,char* pContext,char* pCallBack,long long ctx,on_FileUpload cb);

	/************************************************************************/
	/* 函数名：  FileUploadAgain
	/* 功能：    文件上传失败再次上传
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄 
	/*           pHost		        -->断点文件上传地址，对应于http头部host信息
	/*           pRange             -->断点上传失败返回的Range
	/*           pCID               -->断点上传失败返回的pCID
	/*           pToken             -->鉴权Token
	/*           pCreator           -->上传标识
	/*           pFileName		    -->上传的文件名
	/*           nTryTimes          -->网络异常的重试次数
	/*           pContext           -->第三方回调上下文信息
	/*           pCallBack          -->回调第三方的url地址
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->上传回调指针	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void FileUploadAgain(void* pHandle,char* pHost,char* pRange, char* pCID,char* pToken,char* pCreator,char* pFileName,int nTryTimes,char* pContext,char* pCallBack,long long ctx,on_FileUpload cb);


	/************************************************************************/
	/* 函数名：  AbortUpload
	/* 功能：    中断上传
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄         
	/*           cb    		        -->中断上传回调指针
	/*           pCreator           -->上传标识
	/*           ctx   		        -->上下文参数
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void AbortUpload(void* pHandle,on_AbortUpload cb,char* pCreator,long long ctx);


	/************************************************************************/
	/* 函数名：  VODFileTransCode
	/* 功能：    点播文件转码处理
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄 
	/*           pFileProcUrl       -->媒体处理服务器地址	
	/*           pFileDownloadUrl   -->上传完成后文件的下载地址
	/*           pToken             -->鉴权Token
	/*           pCallBack          -->回调第三方的url地址
	/*           pContext           -->第三方回调上下文信息
	/*           pArgs   		    -->转码参数
	/*           pFileName		    -->转码文件名字
	/*           bAddWaterMark		-->是否添加水印
	/*           pPostContent_WM	-->添加水印的Post内容
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->上传回调指针	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void VODFileTransCode(void* pHandle,char* pFileProcUrl,char* pFileDownloadUrl,char* pToken,char* pCallBack,char* pContext,char* pArgs,char* pFileName,bool bAddWaterMark,char* pPostContent_WM,long long ctx,on_VodFileProc cb);

	/************************************************************************/
	/* 函数名：  VODFileTransCode
	/* 功能：    点播文件转码处理
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄 
	/*           pFileProcUrl       -->媒体处理服务器地址	
	/*           pFileDownloadUrl   -->上传完成后文件的下载地址
	/*           pToken             -->鉴权Token
	/*           pImageProcessArgs  -->图片处理的参数（json格式）
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->上传回调指针	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void ImageProcess(void* pHandle,char* pFileProcUrl,char* pFileDownloadUrl,char* pToken, char* pImageProcessArgs, long long ctx,on_VodFileProc cb);

	/************************************************************************/
	/* 函数名：  VODFileTransCode
	/* 功能：    点播文件转码处理
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄 
	/*           pFileProcUrl       -->媒体处理服务器地址	
	/*           pToken             -->鉴权Token
	/*           pVodVideoFrameArgs -->视频抽帧参数（json格式）
	/*           ctx   		        -->上下文参数
	/*           cb    		        -->上传回调指针	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void VodVideoFrame(void* pHandle,char* pFileProcUrl,char* pToken, char* pVodVideoFrameArgs, long long ctx,on_VodFileProc cb);



	/************************************************************************/
	/* 函数名：  DestoryUpload
	/* 功能：    销毁Handle
	/* 输入参数：
	/*           pHandle		    -->当前上传文件的句柄         
	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void DestoryUpload(void* pHandle);


#ifdef __cplusplus
}
#endif	
#endif	