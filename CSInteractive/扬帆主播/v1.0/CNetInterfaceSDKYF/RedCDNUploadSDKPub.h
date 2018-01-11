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
	/* ��������  on_UploadLogin
	/* ���ܣ�    �ϴ���¼�ص�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           nRet		        -->�ص��Ĵ�����
	/*           pToken    	        -->��ȨToken
	/*           ctx  	            -->�����Ĳ���
	/************************************************************************/
	typedef void (*on_UploadLogin)(void* pHandle,int nRet,char* pToken,long long ctx);

	/************************************************************************/
	/* ��������  on_UploadGetAddr
	/* ���ܣ�    ��ȡ������Ϣ�ص�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           nRet		        -->�ص��Ĵ�����
	/*           pFileUploadUrl     -->�ļ��ϴ���������ַ
	/*           pFileProcUrl       -->ý�崦���������ַ
	/*           ctx  	            -->�����Ĳ���
	/************************************************************************/
	typedef void (*on_UploadGetAddr)(void* pHandle,int nRet,char* pFileUploadUrl,char* pFileProcUrl,long long ctx);

	/************************************************************************/
	/* ��������  on_FileUpload
	/* ���ܣ�    �ϴ��ص�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           nRet		        -->�ص��Ĵ����루Ϊ0��ʾ�ϴ��ɹ���
	/*           bUploadFinished    -->�Ƿ��ϴ��ɹ�
	/*           pUploadRate    	-->�ļ��ϴ����ȣ��ٷֱȣ�
	/*           pCreator           -->�ϴ���ʶ
	/*           pUploadSpeed       -->�ϴ��ٶ�
	/*           pFileDownloadUrl   -->�ϴ���ɺ��ļ������ص�ַ
	/*           pHost		        -->�ϵ��ļ��ϴ���ַ����Ӧ��httpͷ��host��Ϣ
	/*           pRange             -->�ϵ��ϴ�ʧ�ܷ��ص�Range
	/*           pCID               -->�ϵ��ϴ�ʧ�ܷ��ص�pCID
	/*           bAbortUpload       -->�Ƿ��������ж��ϴ�
	/*           ctx  	            -->�����Ĳ���
	/************************************************************************/
	//typedef void (*on_FileUpload)(void* pHandle,int nRet,bool bUploadFinished,char* pUploadRate,char* pCreator,char* pUploadSpeed,char* pFileDownloadUrl,char* pHost,char* pRange,char* pCID,bool bAbortUpload,long long ctx);
	
	typedef void (*on_FileUpload)(void* pHandle,int nRet,bool bUploadFinished,int nUploadRate,char* pCreator,char* pUploadSpeed,char* pFileDownloadUrl,char* pHost,char* pRange,char* pCID,bool bAbortUpload,long long ctx);

	
	/************************************************************************/
	/* ��������  on_AbortUpload
	/* ���ܣ�    �ж��ϴ��ص�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           nRet		        -->�ص��Ĵ�����
	/*           bAbortUploadSucc   -->�Ƿ��жϳɹ�
	/*           pCreator           -->�ϴ���ʶ
	/*           ctx   		        -->�����Ĳ���
	/************************************************************************/
	typedef void (*on_AbortUpload)(void* pHandle,int nRet,bool bAbortUploadSucc,char* pCreator,long long ctx);	

		
	/************************************************************************/
	/* ��������  on_VodFileProc
	/* ���ܣ�    �㲥�ļ�����ͳһ�ص�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           nRet		        -->�ص��Ĵ�����
	/*           bProcessSucc       -->�㲥�ļ������Ƿ�ɹ�
	/*           pTaskID            -->���������ѯ������ID
	/*           ctx  	            -->�����Ĳ���
	/************************************************************************/
	typedef void (*on_VodFileProc)(void* pHandle,int nRet,bool bProcessSucc,char* pTaskID,long long ctx);

	/************************************************************************/
	/* ��������  CreatUpload
	/* ���ܣ�    ����Handle
	/* ���������
	/*  
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void* CreatUpload();

	/************************************************************************/
	/* ��������  SetUploadFileLength
	/* ���ܣ�    ����Handle
	/* ���������
	/*  
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API int SetUploadFileLength(void* pHandle,long long nUploadLength);


	/************************************************************************/
	/* ��������  UploadLogin
	/* ���ܣ�    �ļ��ϴ���¼
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           pUrl		        -->�����URL
	/*           pAppid             -->Appid
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->��¼�ص�ָ��
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void UploadLogin(void* pHandle,char* pUrl,char* pAppid,long long ctx,on_UploadLogin cb);

	/************************************************************************/
	/* ��������  UploadGetAddr
	/* ���ܣ�    ��ȡ�ϴ���������ַ��ý�崦���������ַ
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��
	/*           pUrl		        -->�����URL  
	/*           pToken             -->��ȨToken
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ص�ָ��
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void UploadGetAddr(void* pHandle,char* pUrl,char* pToken,long long ctx,on_UploadGetAddr cb);

	/************************************************************************/
	/* ��������  FileUpload
	/* ���ܣ�    �ļ��ϴ�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ�� 
	/*           pFileUploadUrl		-->�ϴ���������ַURL
	/*           pToken             -->��ȨToken
	/*           pCreator           -->�ϴ���ʶ
	/*           pFileName		    -->�ϴ����ļ���
	/*           nTryTimes          -->�����쳣�����Դ���
	/*           pContext           -->�������ص���������Ϣ
	/*           pCallBack          -->�ص���������url��ַ
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ϴ��ص�ָ��	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void FileUpload(void* pHandle,char* pFileUploadUrl,char* pToken,char* pCreator,char* pFileName,int nTryTimes,char* pContext,char* pCallBack,long long ctx,on_FileUpload cb);

	/************************************************************************/
	/* ��������  FileUploadAgain
	/* ���ܣ�    �ļ��ϴ�ʧ���ٴ��ϴ�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ�� 
	/*           pHost		        -->�ϵ��ļ��ϴ���ַ����Ӧ��httpͷ��host��Ϣ
	/*           pRange             -->�ϵ��ϴ�ʧ�ܷ��ص�Range
	/*           pCID               -->�ϵ��ϴ�ʧ�ܷ��ص�pCID
	/*           pToken             -->��ȨToken
	/*           pCreator           -->�ϴ���ʶ
	/*           pFileName		    -->�ϴ����ļ���
	/*           nTryTimes          -->�����쳣�����Դ���
	/*           pContext           -->�������ص���������Ϣ
	/*           pCallBack          -->�ص���������url��ַ
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ϴ��ص�ָ��	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void FileUploadAgain(void* pHandle,char* pHost,char* pRange, char* pCID,char* pToken,char* pCreator,char* pFileName,int nTryTimes,char* pContext,char* pCallBack,long long ctx,on_FileUpload cb);


	/************************************************************************/
	/* ��������  AbortUpload
	/* ���ܣ�    �ж��ϴ�
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��         
	/*           cb    		        -->�ж��ϴ��ص�ָ��
	/*           pCreator           -->�ϴ���ʶ
	/*           ctx   		        -->�����Ĳ���
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void AbortUpload(void* pHandle,on_AbortUpload cb,char* pCreator,long long ctx);


	/************************************************************************/
	/* ��������  VODFileTransCode
	/* ���ܣ�    �㲥�ļ�ת�봦��
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ�� 
	/*           pFileProcUrl       -->ý�崦���������ַ	
	/*           pFileDownloadUrl   -->�ϴ���ɺ��ļ������ص�ַ
	/*           pToken             -->��ȨToken
	/*           pCallBack          -->�ص���������url��ַ
	/*           pContext           -->�������ص���������Ϣ
	/*           pArgs   		    -->ת�����
	/*           pFileName		    -->ת���ļ�����
	/*           bAddWaterMark		-->�Ƿ����ˮӡ
	/*           pPostContent_WM	-->���ˮӡ��Post����
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ϴ��ص�ָ��	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void VODFileTransCode(void* pHandle,char* pFileProcUrl,char* pFileDownloadUrl,char* pToken,char* pCallBack,char* pContext,char* pArgs,char* pFileName,bool bAddWaterMark,char* pPostContent_WM,long long ctx,on_VodFileProc cb);

	/************************************************************************/
	/* ��������  VODFileTransCode
	/* ���ܣ�    �㲥�ļ�ת�봦��
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ�� 
	/*           pFileProcUrl       -->ý�崦���������ַ	
	/*           pFileDownloadUrl   -->�ϴ���ɺ��ļ������ص�ַ
	/*           pToken             -->��ȨToken
	/*           pImageProcessArgs  -->ͼƬ����Ĳ�����json��ʽ��
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ϴ��ص�ָ��	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void ImageProcess(void* pHandle,char* pFileProcUrl,char* pFileDownloadUrl,char* pToken, char* pImageProcessArgs, long long ctx,on_VodFileProc cb);

	/************************************************************************/
	/* ��������  VODFileTransCode
	/* ���ܣ�    �㲥�ļ�ת�봦��
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ�� 
	/*           pFileProcUrl       -->ý�崦���������ַ	
	/*           pToken             -->��ȨToken
	/*           pVodVideoFrameArgs -->��Ƶ��֡������json��ʽ��
	/*           ctx   		        -->�����Ĳ���
	/*           cb    		        -->�ϴ��ص�ָ��	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void VodVideoFrame(void* pHandle,char* pFileProcUrl,char* pToken, char* pVodVideoFrameArgs, long long ctx,on_VodFileProc cb);



	/************************************************************************/
	/* ��������  DestoryUpload
	/* ���ܣ�    ����Handle
	/* ���������
	/*           pHandle		    -->��ǰ�ϴ��ļ��ľ��         
	
	/************************************************************************/
	REDCDNUPLOADSDK_PUB_API void DestoryUpload(void* pHandle);


#ifdef __cplusplus
}
#endif	
#endif	