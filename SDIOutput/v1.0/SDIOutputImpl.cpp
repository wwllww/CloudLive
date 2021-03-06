
#include "SDIOutputImpl.h"

char r_table[65536] = { 0 };
char g_table[65536] = { 0 };
char b_table[65536] = { 0 };
_BMDDisplayMode getDisplayMode(ModeMapping* mappings, SDIOUT_DISPLAYMODE modeIndex)
{
	while (mappings->mode != NULL)
	{
		if (mappings->modeIndex == modeIndex)
			return mappings->mode;
		++mappings;
	}
	return bmdModePAL;
}

void CopyRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int pixformat)
{
	if (pixformat == ColorFormat_RGBA32REVERSE)
		memcpy(pDstImg, pSrcImg, srcW * srcH * 4);
	else
	{
		for (int i = 0; i < srcH; i++)
		{
			for (int j = 0; j < srcW; j++)
			{
				if (pixformat == ColorFormat_RGB24)
				{
					pDstImg[i * srcW * 4 + j * 4] = pSrcImg[i * srcW * 3 + j * 3];
					pDstImg[i * srcW * 4 + j * 4 + 1] = pSrcImg[i * srcW * 3 + j * 3 + 1];
					pDstImg[i * srcW * 4 + j * 4 + 2] = pSrcImg[i * srcW * 3 + j * 3 + 2];
					pDstImg[i * srcW * 4 + j * 4 + 3] = 0;
				}
				else if (pixformat == ColorFormat_RGB32)
				{
					pDstImg[(srcH - i - 1) * srcW * 4 + j * 4] = pSrcImg[i * srcW * 4 + j * 4];
					pDstImg[(srcH - i - 1) * srcW * 4 + j * 4 + 1] = pSrcImg[i * srcW * 4 + j * 4 + 1];
					pDstImg[(srcH - i - 1) * srcW * 4 + j * 4 + 2] = pSrcImg[i * srcW * 4 + j * 4 + 2];
					pDstImg[(srcH - i - 1) * srcW * 4 + j * 4 + 3] = pSrcImg[i * srcW * 4 + j * 4 + 3];
				}
			}
		}
	}
}

void ResizeRGB720(unsigned char* pSrcImg, unsigned char* pDstImg)
{
	int tSrcH;
	int* pdst = (int*)pDstImg;
	LONGLONG* psrc = (LONGLONG*)pSrcImg;

	for (int i = 0; i < 1080; i += 3)
	{
		for (int j = 0; j < 640; j += 2)
		{
			*((LONGLONG*)pdst) = *(psrc + j);
			*((LONGLONG*)(pdst + 1)) = *(psrc + j);
			*((LONGLONG*)(pdst + 3)) = *(psrc + j + 1);
			*((LONGLONG*)(pdst + 4)) = *(psrc + j + 1);
			pdst += 6;
		}
		psrc += 640;
		memcpy(pdst, pdst - 1920, 1920 * 4);
		pdst += 1920;
		for (int j = 0; j < 640; j += 2)
		{
			*((LONGLONG*)pdst) = *(psrc + j);
			*((LONGLONG*)(pdst + 1)) = *(psrc + j);
			*((LONGLONG*)(pdst + 3)) = *(psrc + j + 1);
			*((LONGLONG*)(pdst + 4)) = *(psrc + j + 1);
			pdst += 6;
		}
		psrc += 640;
	}
}

void ResizeRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH, int pixformat)
{
	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	unsigned  int * tSrcW = new  unsigned  int[dstW];
	for (unsigned int j = 0; j < dstW; j++)
		tSrcW[j] = (int)(rateW * double(j));

	int tSrcH;
	if (pixformat == ColorFormat_RGBA32REVERSE)
	{
		for (int i = 0; i < dstH; i++)
		{
			tSrcH = (int)(rateH * double(i));
			for (int j = 0; j < dstW; j++)
			{
				int d = (i * dstW +j) * 4;
				int s = (tSrcH * srcW + tSrcW[j]) * 4;
				*((int*)(pDstImg + d)) = *((int*)(pSrcImg + s));
			}
		}
	}
	else if (pixformat == ColorFormat_RGB32)
	{
		for (int i = 0; i < dstH; i++)
		{
			tSrcH = (int)(rateH * double(i));
			for (int j = 0; j < dstW; j++)
			{
				int d = ((dstH - i - 1) * dstW + j) * 4;
				int s = (tSrcH * srcW + tSrcW[j]) * 4;
				*((int*)(pDstImg + d)) = *((int*)(pSrcImg + s));
			}
		}
	}
	else if (pixformat == ColorFormat_RGB24)
	{
		for (int i = 0; i < dstH; i++)
		{
			tSrcH = (int)(rateH * double(i));
			for (int j = 0; j < dstW; j++)
			{
				int d = (i * dstW + j)* 4;
				int s = (tSrcH * srcW + tSrcW[j]) * 3;
				pDstImg[d] = pSrcImg[s];
				pDstImg[d + 1] = pSrcImg[s + 1];
				pDstImg[d + 2] = pSrcImg[s + 2];
				pDstImg[d + 3] = 0;
			}
		}
	}
	delete[] tSrcW;
	tSrcW = NULL;
}

void ResizeYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;
	unsigned char *VSrcImg = pSrcImg + srcW*srcH + srcW*srcH / 4;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int UsrcW = srcW / 2;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	unsigned  int * tSrcW = new  unsigned  int[dstW];
	for (unsigned int j = 0; j < dstW; j++)
	{
		tSrcW[j] = (int)(rateW * double(j));
	}

	int tSrcH;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH * double(i));
		for (int j = 0; j < dstW; j++)
			pDstImg[i*dstW + j] = pSrcImg[tSrcH * srcW + tSrcW[j]];
	}
	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH * double(i));
		for (int j = 0; j < UdstW; j++)
		{
			UDstImg[i*UdstW + j] = USrcImg[tSrcH * UsrcW + tSrcW[j]];
			VDstImg[i*UdstW + j] = VSrcImg[tSrcH * UsrcW + tSrcW[j]];
		}
	}
	if (tSrcW)
	{
		delete[] tSrcW;
		tSrcW = NULL;
	}
}

void ResizeYUV422(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;
	unsigned char *VSrcImg = pSrcImg + srcW*srcH + srcW*srcH / 2;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	int tSrcH, tSrcW;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j));
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j));
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW * 2 + tSrcW];
			VDstImg[i*UdstW + j] = VSrcImg[tSrcH*UsrcW * 2 + tSrcW];
		}
	}
}

void ResizeYUY2ToYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *YSrcImg = pSrcImg;
	unsigned char *USrcImg = pSrcImg + 1;
	unsigned char *VSrcImg = pSrcImg + 3;

	unsigned char *UDstImg = pDstImg + dstW*dstH;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j));
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j));
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
			VDstImg[i*UdstW + j] = VSrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}
}

void ResizeUYVYToYUV420(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg;
	unsigned char *YSrcImg = pSrcImg + 1;
	unsigned char *VSrcImg = pSrcImg + 2;

	unsigned char *UDstImg = pDstImg + dstW*dstH;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j));
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i));
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j));
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
			VDstImg[i*UdstW + j] = VSrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}
}

void ResizeUYVYToRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int width, int height)
{
	unsigned char* p = pDstImg;
	int j, i;
	for (j = height - 1; j >= 0; j--)
	{
		for (i = 0; i < width / 2; i++)
		{
			int r, g, b;
			unsigned char u, v, y1, y2;
			y1 = pSrcImg[j*width * 2 + i * 4 + 1];
			y2 = pSrcImg[j*width * 2 + i * 4 + 3];
			u = pSrcImg[j*width * 2 + i * 4 + 0];
			v = pSrcImg[j*width * 2 + i * 4 + 2];

#if 1  
			r = (int)(y1 + 1.402*(u - 128) + 0.5);
			g = (int)(y1 - 0.344*(u - 128) - 0.714*(v - 128) + 0.5);
			b = (int)(y1 + 1.772*(v - 128) + 0.5);
			if (r > 255)
				r = 255;
			if (r<0)
				r = 0;
			if (g>255)
				g = 255;
			if (g<0)
				g = 0;
			if (b>255)
				b = 255;
			if (b<0)
				b = 0;
			p[0] = r;
			p[1] = g;
			p[2] = b;
			p[3] = 255;

			r = (int)(y2 + 1.375*(u - 128) + 0.5);
			g = (int)(y2 - 0.344*(u - 128) - 0.714*(v - 128) + 0.5);
			b = (int)(y2 + 1.772*(v - 128) + 0.5);
			if (r>255)
				r = 255;
			if (r<0)
				r = 0;
			if (g>255)
				g = 255;
			if (g<0)
				g = 0;
			if (b>255)
				b = 255;
			if (b < 0)
				b = 0;
			p[4] = r;
			p[5] = g;
			p[6] = b;
			p[7] = 255;
			p += 8;
#else   

			//为提高性能此处用移位运算；  
			rgb[3 * i] = (unsigned char)(y1 + (u - 128) + ((104 * (u - 128)) >> 8));
			rgb[3 * i + 1] = (unsigned char)(y1 - (89 * (v - 128) >> 8) - ((183 * (u - 128)) >> 8));
			rgb[3 * i + 2] = (unsigned char)(y1 + (v - 128) + ((199 * (v - 128)) >> 8));

			rgb[3 * i + 3] = (unsigned char)(y2 + (u - 128) + ((104 * (u - 128)) >> 8));
			rgb[3 * i + 4] = (unsigned char)(y2 - (89 * (v - 128) >> 8) - ((183 * (u - 128)) >> 8));
			rgb[3 * i + 5] = (unsigned char)(y2 + (v - 128) + ((199 * (v - 128)) >> 8));
#endif
		}
	}

// 	int size = width*height;
// 	unsigned char u, v, y1, y2;
// 	int i;
// 	for (i = 0; i < size; i += 2)
// 	{
// 		y1 = pSrcImg[2 * i + 1];
// 		y2 = pSrcImg[2 * i + 3];
// 		u = pSrcImg[2 * i];
// 		v = pSrcImg[2 * i + 2];
// 
// #if 1  
// 		pDstImg[4 * i] = (unsigned char)(y1 + 1.402*(u - 128));
// 		pDstImg[4 * i + 1] = (unsigned char)(y1 - 0.344*(u - 128) - 0.714*(v - 128));
// 		pDstImg[4 * i + 2] = (unsigned char)(y1 + 1.772*(v - 128));
// 		pDstImg[4 * i + 3] = 255;
// 
// 		pDstImg[4 * i + 4] = (unsigned char)(y2 + 1.375*(u - 128));
// 		pDstImg[4 * i + 5] = (unsigned char)(y2 - 0.344*(u - 128) - 0.714*(v - 128));
// 		pDstImg[4 * i + 6] = (unsigned char)(y2 + 1.772*(v - 128));
// 		pDstImg[4 * i + 7] = 255;
// #else   
// 
// 		//为提高性能此处用移位运算；  
// 		pDstImg[3 * i] = (unsigned char)(y1 + (u - 128) + ((104 * (u - 128)) >> 8));
// 		pDstImg[3 * i + 1] = (unsigned char)(y1 - (89 * (v - 128) >> 8) - ((183 * (u - 128)) >> 8));
// 		pDstImg[3 * i + 2] = (unsigned char)(y1 + (v - 128) + ((199 * (v - 128)) >> 8));
// 
// 		pDstImg[3 * i + 3] = (unsigned char)(y2 + (u - 128) + ((104 * (u - 128)) >> 8));
// 		pDstImg[3 * i + 4] = (unsigned char)(y2 - (89 * (v - 128) >> 8) - ((183 * (u - 128)) >> 8));
// 		pDstImg[3 * i + 5] = (unsigned char)(y2 + (v - 128) + ((199 * (v - 128)) >> 8));
// #endif
// 	}
}

void YUV420ToRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int width, int height)
{
	int Y, U, V, R, G, B;
	unsigned char* bufY = pSrcImg;
	unsigned char* bufU = pSrcImg + width * height;
	unsigned char* bufV = bufU + (width * height * 1 / 4);

	unsigned char* pRGB = pDstImg;
	int w = width / 2;
	for (int i = height - 1; i >= 0; i--)
	{
		for (int j = 0; j < width; j++)
		{
			Y = *(bufY + i * width + j);
			U = *(bufU + (i / 2 * w + j / 2)) - 128;
			V = *(bufV + (i / 2 * w + j / 2)) - 128;
#ifdef USE_FLOAT
			R = (int)(Y + 1.403f * V);
			G = (int)(Y - 0.344f * U - 0.714f * V);
			B = (int)(Y + 1.770f * U);

			B = min(255, max(0, B));
			*(pRGB++) = B;
			G = min(255, max(0, G));
			*(pRGB++) = G;
			R = min(255, max(0, R));
			*(pRGB++) = R;

			*pRGB++;
#else
			R = (int)(Y + ((359 * V) >> 8));
			G = (int)(Y - (89 * U >> 8) - ((183 * V) >> 8));
			B = (int)(Y + ((453 * U) >> 8));
			*(pRGB++) = b_table[B + 32768];
			*(pRGB++) = g_table[G + 32768];
			*(pRGB++) = r_table[R + 32768];
			*(pRGB++) = 255;
#endif
		}
	}
}

void YUV420ToUYVY(unsigned char* pSrcImg, unsigned char* pDstImg, int width, int height, bool bYV12 = false)
{
	unsigned char* output = pDstImg;
	unsigned char* input = pSrcImg;
	unsigned char* input2 = nullptr;
	unsigned char* input3 = nullptr;
	if (bYV12)
	{
		input3 = input + (width*height);
		input2 = input3 + (width*height / 4);
	}
	else
	{
		input2 = input + (width*height);
		input3 = input2 + (width*height / 4);
	}

	unsigned int halfStartY = 0;
	unsigned int halfX = width / 2;
	unsigned int halfY = height / 2;
	unsigned int linePitch = width;
	unsigned int pitch = width * 2;

	for (unsigned int y = 0; y < halfY; y++)
	{
		unsigned char* lpLum1 = input + y * 2 * linePitch;
		unsigned char* lpLum2 = lpLum1 + linePitch;
		unsigned char* lpChroma1 = input2 + y*(linePitch / 2);
		unsigned char* lpChroma2 = input3 + y*(linePitch / 2);
		unsigned long* output1 = (unsigned long*)(output + ((halfY - y) * 2 - 1)*pitch);
		unsigned long*  output2 = (unsigned long*)(((unsigned char*)output1) - pitch);

		for (unsigned int x = 0; x < halfX; x++)
		{
			unsigned long out = (*(lpChroma1++)) | (*(lpChroma2++) << 16);
			unsigned long out1 = out;

			out |= (*(lpLum1++) << 8);
			out |= (*(lpLum1++) << 24);
			*(output1++) = out;

			out1 |= (*(lpLum2++) << 8);
			out1 |= (*(lpLum2++) << 24);
			*(output2++) = out1;
		}
	}
	return;
}

int WCHARToAnsi(std::wstring strSrc, char *pAnsi, int &nLen)
{
	int nSize = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)strSrc.c_str(), strSrc.length(), pAnsi, nLen, NULL, NULL);
	pAnsi[nSize] = 0;
	nLen = nSize;
	return 0;
}

void mono_to_stereo(short *output, short *input, int n)
{
	short *p, *q;
	p = input;
	q = output;
	while (n >= 4) {
		q[0] = p[0]; q[1] = p[0];
		q[2] = p[1]; q[3] = p[1];
		q[4] = p[2]; q[5] = p[2];
		q[6] = p[3]; q[7] = p[3];
		q += 8;
		p += 4;
		n -= 4;
	}
	while (n > 0) {
		q[0] = p[0]; q[1] = p[0];
		q += 2;
		p += 1;
		n--;
	}
}

SDIOutput::SDIOutput()
{
	Log::writeMessage(LOG_SDI, 1, "SDIOutput Begin!");
	InitializeCriticalSection(&lock);
	m_pDevicelist = NULL;
	m_nDeviceCount = 0;
	m_pDisplayModeList = NULL;
	m_nModeCount = 0;
}

SDIOutput::~SDIOutput()
{
	Log::writeMessage(LOG_SDI, 1, "~SDIOutput Begin!");
	DeleteCriticalSection(&lock);
	if (listChannelInfo.size())
	{
		std::list<ChannelInfo*>::iterator pos = listChannelInfo.begin();
		std::list<ChannelInfo*>::iterator end = listChannelInfo.end();
		for (; pos != end; ++pos)
		{
			if ((*pos)->pDLOutput)
			{
				(*pos)->pDLOutput->SetScheduledFrameCompletionCallback(NULL);
				(*pos)->pDLOutput->SetAudioCallback(NULL);
				SAFE_RELEASE((*pos)->pDLOutput);
			}
			SAFE_RELEASE((*pos)->pDL);
			DeleteCriticalSection(&(*pos)->pMutex);
		}
	}
}

int SDIOutput::SDI_Init()
{
	Log::open(true);
	for (int c = 0; c < 256; c++)
	{
		for (int i = 0; i < 256; i++)
		{
			for (int j = 0; j<256; j++)
			{
				int r, g, b;
				r = (int)(c + (i - 128) + ((104 * (i - 128)) >> 8));
				if (r>255)
					r_table[r + 32768] = 255;
				else if (r >= 0)
					r_table[r + 32768] = r;

				g = (int)(c - (89 * (j - 128) >> 8) - ((183 * (i - 128)) >> 8));
				if (g > 255)
					g_table[g + 32768] = 255;
				else if (g >= 0)
					g_table[g + 32768] = g;

				b = (int)(c + (j - 128) + ((199 * (j - 128)) >> 8));
				if (b > 255)
					b_table[b + 32768] = 255;
				else if (b >= 0)
					b_table[b + 32768] = b;
			}
		}
	}
	return S_OK;
}

FILE* fp1;
FILE* fp2;
FILE* fp3;
FILE* fp4;
FILE* fp5;

int SDIOutput::SDI_StartOut(int nDeviceID, SDIOUT_DISPLAYMODE mode, SDIOUT_COLORFORMAT nColorFormat, int nInnerBufferCount, int nOutBufferCount)
{
// 	fp1 = fopen("1.pcm", "wb");
// 	fp2 = fopen("2.pcm", "wb");
// 	fp3 = fopen("3.pcm", "wb");
// 	fp4 = fopen("4.pcm", "wb");
// 	fp5 = fopen("5.pcm", "wb");

	Log::writeMessage(LOG_SDI, 1, "SDI_StartOut Begin 设备ID=%d, 通道数=%d!", nDeviceID, listChannelInfo.size());

	if (nDeviceID < 1 || nDeviceID > m_nDeviceCount)
	{
		Log::writeError(LOG_SDI, 1, "SDI_StartOut nDeviceID Error!");
		return errInvalidParameter;
	}
	if (mode < NTSC || mode > HD720p60)
	{
		Log::writeError(LOG_SDI, 1, "SDI_StartOut mode Error!");
		return errInvalidParameter;
	}

	{
		std::list<ChannelInfo*>::iterator pos = listChannelInfo.begin();
		std::list<ChannelInfo*>::iterator end = listChannelInfo.end();
		for (; pos != end; ++pos)
		{
			if ((*pos)->nDeviceID == nDeviceID)
			{
				Log::writeError(LOG_SDI, 1, "SDI_StartOut errDeviceAlwaysStart Error!");
				return errDeviceAlwaysStart;
			}
		}

		ChannelInfo* channelInfo = new ChannelInfo;
		IDeckLinkIterator*	pDLIterator = NULL;
		HRESULT result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDLIterator);
		if (FAILED(result))
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut CoCreateInstance IID_IDeckLinkIterator 1 Error! result = %d", result);
			return errDeckLinkDrivers;
		}

		int nCurDeviceID = 1;
		bool bFound = false;
		IDeckLink* pCurDL = NULL;
		while (pDLIterator->Next(&pCurDL) == S_OK)
		{
			if (nCurDeviceID == nDeviceID)
			{
				channelInfo->nDeviceID = nCurDeviceID;
				channelInfo->pDL = pCurDL;
				bFound = true;
				break;
			}
			else
				SAFE_RELEASE(pCurDL);
			nCurDeviceID++;
		}
		if (!bFound)
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut IDeckLinkIterator Next Error!");
			SAFE_RELEASE(pDLIterator);
			return errIDeckLink;
		}

		IDeckLinkOutput* pCurDLOutput = NULL;
		if (pCurDL->QueryInterface(IID_IDeckLinkOutput, (void**)&pCurDLOutput) == S_OK)
			channelInfo->pDLOutput = pCurDLOutput;
		else
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut QueryInterface IID_IDeckLinkOutput Error!");
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDL);
			return errIDeckLinkOutput;
		}

		IDeckLinkDisplayModeIterator*		pDLDisplayModeIterator = NULL;
		IDeckLinkDisplayMode*				pDLDisplayMode = NULL;
		_BMDDisplayMode DisplayMode = bmdModePAL;
		if (m_nDeviceCount == 1)
			DisplayMode = getDisplayMode(kDisplayModeMappingsOne, mode);
		else if (m_nDeviceCount == 4)
			DisplayMode = getDisplayMode(kDisplayModeMappings, mode);

		Log::writeError(LOG_SDI, 1, "SDI_StartOut DisplayMode = %d", mode);

		if (pCurDLOutput->GetDisplayModeIterator(&pDLDisplayModeIterator) == S_OK)
		{
			while (pDLDisplayModeIterator->Next(&pDLDisplayMode) == S_OK)
			{
				if (pDLDisplayMode->GetDisplayMode() == DisplayMode)
				{
	// 				BMDFieldDominance filedDominance = pDLDisplayMode->GetFieldDominance();
	// 				HRESULT hr;
	// 				BMDDisplayModeSupport support;
	// 				BMDDisplayMode mode = pDLDisplayMode->GetDisplayMode();
	// 				hr = pCurDLOutput->DoesSupportVideoMode(pDLDisplayMode->GetDisplayMode(), bmdFormat8BitYUV, bmdVideoOutputFlagDefault, &support, NULL);
	// 				int a = 0;
					break;
				}
			}
			SAFE_RELEASE(pDLDisplayModeIterator);
		}
		if (pDLDisplayMode == NULL)
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut IDeckLinkDisplayModeIterator Next Error!");
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDLOutput);
			SAFE_RELEASE(pCurDL);
			return errDisplayMode;
		}

		EnterCriticalSection(&lock);

		channelInfo->uiFrameWidth = pDLDisplayMode->GetWidth();
		channelInfo->uiFrameHeigh = pDLDisplayMode->GetHeight();
		pDLDisplayMode->GetFrameRate(&channelInfo->frameDuration, &channelInfo->frameTimescale);
		channelInfo->uiFPS = ((channelInfo->frameTimescale + (channelInfo->frameDuration - 1)) / channelInfo->frameDuration);
		if (pCurDLOutput->EnableVideoOutput(pDLDisplayMode->GetDisplayMode(), bmdVideoOutputFlagDefault) != S_OK)
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut EnableVideoOutput Error!");
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDLOutput);
			SAFE_RELEASE(pCurDL);
			LeaveCriticalSection(&lock);
			return errEnableVideoOutput;
		}

		if (pCurDLOutput->EnableAudioOutput(bmdAudioSampleRate48kHz, bmdAudioSampleType16bitInteger, 2, /*bmdAudioOutputStreamTimestamped*/bmdAudioOutputStreamContinuous/*bmdAudioOutputStreamContinuousDontResample*/) != S_OK)
		{
			Log::writeError(LOG_SDI, 1, "SDI_StartOut EnableAudioOutput Error!");
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDLOutput);
			SAFE_RELEASE(pCurDL);
			LeaveCriticalSection(&lock);
			return errEnableAudioOutput;
		}

		channelInfo->colorFormat = nColorFormat;
		channelInfo->displayMode = mode;

		float fScale = 1.0;//(float)channelInfo->uiFPS / 25.0f;
		if (nInnerBufferCount > 0)
		{
			channelInfo->nInnerBufFrameCount = nInnerBufferCount * fScale;
			channelInfo->uiTotalFrames = nInnerBufferCount * fScale;
		}
		else
		{
			channelInfo->nInnerBufFrameCount = FRAMECOUNT * fScale;
			channelInfo->uiTotalFrames = FRAMECOUNT * fScale;
		}
		channelInfo->nOutBufFrameCount = nOutBufferCount * fScale;

		Log::writeMessage(LOG_SDI, 1, "SDI_StartOut 输出帧率=%d, SDI内部缓冲大小=%d， SDI外部缓冲大小=%d!", channelInfo->uiFPS, channelInfo->nInnerBufFrameCount, channelInfo->nOutBufFrameCount);

		channelInfo->bStop = false;
		listChannelInfo.push_back(channelInfo);

		OutputCallback*  pOutputCallback = NULL;
		pOutputCallback = new OutputCallback(pCurDLOutput, &listChannelInfo, nDeviceID);
		if (pOutputCallback == NULL)
		{
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDLOutput);
			SAFE_RELEASE(pCurDL);
			LeaveCriticalSection(&lock);
			Log::writeError(LOG_SDI, 1, "SDI_StartOut new OutputCallback Error!");

			return errCreateOutputCallback;
		}

		pCurDLOutput->SetAudioCallback(pOutputCallback);

		if (pCurDLOutput->SetScheduledFrameCompletionCallback(pOutputCallback) != S_OK)
		{
			SAFE_RELEASE(pDLIterator);
			SAFE_RELEASE(pCurDLOutput);
			SAFE_RELEASE(pCurDL);
			if (pOutputCallback)
			{
				delete pOutputCallback;
				pOutputCallback = NULL;
			}
			LeaveCriticalSection(&lock);
			Log::writeError(LOG_SDI, 1, "SDI_StartOut SetScheduledFrameCompletionCallback Error!");

			return errSetOutputCallbackToDevice;
		}
		channelInfo->pOutputCallback = pOutputCallback;

		SetPreroll(nDeviceID, nColorFormat);

		pCurDLOutput->StartScheduledPlayback(0, 100, 1.0);
		SAFE_RELEASE(pDLIterator);
		LeaveCriticalSection(&lock);
	}
	Log::writeMessage(LOG_SDI, 1, "SDI_StartOut End 设备ID=%d, 通道数=%d!", nDeviceID, listChannelInfo.size());

	return errNone;
}

static LARGE_INTEGER nFreq1;
static LARGE_INTEGER nLastTime11;
static LARGE_INTEGER nLastTime12;
static LARGE_INTEGER nLastTime21;
static LARGE_INTEGER nLastTime22;
static LARGE_INTEGER nLastTime31;
static LARGE_INTEGER nLastTime32;
static LARGE_INTEGER nLastTime41;
static LARGE_INTEGER nLastTime42;

static BMDTimeValue	hardwareTime2;
static BMDTimeValue	hardwareTime;

static int count = 0;
long long t1, t2, t3, t4, detalRender, detal2;
int SDIOutput::SDI_RenderDevice(int nDeviceID, void* pData, int nWidth, int nHeight, SDIOUT_COLORFORMAT colorFormat, bool bAudio, void* pAudioFormat, int nAudioLength, bool bPGM)
{
	if (pData == NULL || listChannelInfo.size() == 0)
	{
		Log::writeError(LOG_SDI, 1, "SDI_RenderDevice pData = NULL!");
		return S_OK;
	}

// 	if (count == 0)
// 	{
// 		t1 = GetTickCount64();
// 	}
// 	count++;
// 	if (count == 100)
// 	{
// 		t2 = GetTickCount64();
// 		detal = t2 - t1;
// 	}

	EnterCriticalSection(&lock);

	ChannelInfo* pChannelInfo = NULL;

	unsigned int nSampleWritten = 0;
	WAVEFORMATEX* pFormat = NULL;
	double resampleRatio = 0.0f;
	int nBytePerSample = 0;
	int nChannel = 0;
	int nSample = 0;
	UINT nSampleCount = 0;
	UINT totalSamples = 0;;
	UINT nAdjustSampleCount = 0;
	UINT newFrameSize = 0;
	int errVal;
	bool bParaChange = false;

	if (bAudio)
	{
		pFormat = (WAVEFORMATEX*)(pAudioFormat);
		nBytePerSample = pFormat->wBitsPerSample / 8;
		nChannel = pFormat->nChannels;
		nSample = pFormat->nSamplesPerSec;
		nSampleCount = nAudioLength / (nChannel * nBytePerSample);
		totalSamples = nAudioLength / nBytePerSample;
		resampleRatio = double(48000) / double(nSample);
		nAdjustSampleCount = UINT((double(nSampleCount) * resampleRatio) + 1.0);
		newFrameSize = nAdjustSampleCount * nChannel;
	}

	if (listChannelInfo.size())
	{
		std::list<ChannelInfo*>::iterator pos = listChannelInfo.begin();
		std::list<ChannelInfo*>::iterator end = listChannelInfo.end();
		for (; pos != end; ++pos)
		{
			if ((*pos)->nDeviceID == nDeviceID)
			{
				EnterCriticalSection(&(*pos)->pMutex);
				pChannelInfo = (*pos);
				bParaChange = Restart(bAudio, pChannelInfo, nChannel, nSample, nBytePerSample, nAudioLength, bParaChange, colorFormat, nWidth, nHeight);				
				if (bParaChange)
				{
					if (bAudio && pFormat->nSamplesPerSec == 48000 && pFormat->nChannels == 2 && pFormat->wBitsPerSample == 16 && (!bPGM))
						pChannelInfo->pDLOutput->ScheduleAudioSamples(pData, nSampleCount, 0, 0, &nSampleWritten);
					LeaveCriticalSection(&(*pos)->pMutex);
					break;
				}

				if (bAudio)
				{
					if (pFormat->nSamplesPerSec != 48000)
					{
						if (NULL == (*pos)->resampler)
						{
							(*pos)->resampler = src_new(SRC_SINC_FASTEST, nChannel, &errVal);
						}
						if (NULL == (*pos)->pResampleBuffer)
						{
							(*pos)->pResampleBuffer = new float[newFrameSize];
						}
						if ((!bPGM) && NULL == (*pos)->convertBufferFloat)
						{
							(*pos)->convertBufferFloat = new float[totalSamples];
						}
						if (NULL == (*pos)->convertBufferShort)
						{
							(*pos)->convertBufferShort = new short[newFrameSize];
						}
						if (nChannel == 1 && NULL == (*pos)->doubleChannelBuffer)
						{
							(*pos)->doubleChannelBuffer = new short[newFrameSize * 2];
						}
					}
					else
					{
						if (bPGM)
						{
							int tempFrameSize = nSampleCount * nChannel;
							if (NULL == (*pos)->convertBufferShort)
							{
								(*pos)->convertBufferShort = new short[tempFrameSize];
							}
						}
						else
						{
							if (4 == nBytePerSample)
							{
								int tempFrameSize = nSampleCount * nChannel;
								if (NULL == (*pos)->convertBufferShort)
								{
									(*pos)->convertBufferShort = new short[tempFrameSize];
								}
							}
						}
						if (nChannel == 1 && NULL == (*pos)->doubleChannelBuffer)
						{
							(*pos)->doubleChannelBuffer = new short[nAudioLength];
						}
					}
				}
				else
				{
					if (NULL == (*pos)->pConvertBuf)
					{
						int size = (*pos)->uiFrameWidth * (*pos)->uiFrameHeigh * 1.5;
						(*pos)->pConvertBuf = new unsigned char[size];
					}
				}
				LeaveCriticalSection(&(*pos)->pMutex);
				break;
			}
		}
	}
	LeaveCriticalSection(&lock);

	if (bParaChange)
		return errNone;

	if (pChannelInfo)
	{
		EnterCriticalSection(&pChannelInfo->pMutex);
		if (bAudio)
		{
			if (pFormat->nSamplesPerSec != 48000)
			{
// 							fwrite(pData, nAudioLength, 1, fp1);
				if (!bPGM)
				{
					float *tempConvert = pChannelInfo->convertBufferFloat;
					short *tempShort = (short*)pData;
					while (totalSamples--)
					{
						*(tempConvert++) = float(*(tempShort++)) / 32767.0f;
					}
// 							fwrite(pos->convertBufferFloat, nAudioLength, 2, fp2);
				}

				SRC_DATA data;
				data.src_ratio = resampleRatio;
				if (bPGM)
					data.data_in = (float*)pData;
				else
					data.data_in = (float*)pChannelInfo->convertBufferFloat;
				data.input_frames = nSampleCount;
				data.output_frames = nAdjustSampleCount;
				data.data_out = pChannelInfo->pResampleBuffer;
				data.end_of_input = 0;
				int err = src_process(pChannelInfo->resampler, &data);
// 							fwrite(pos->pResampleBuffer, data.output_frames_gen * nChannel * sizeof(float), 1, fp3);

				int tempFrameSize = data.output_frames_gen * nChannel;
				short* tempConvert1 = pChannelInfo->convertBufferShort;
				float* tempFloat1 = (float*)pChannelInfo->pResampleBuffer;
				while (tempFrameSize--)
				{
					if (*tempFloat1 > 1.0f)
					{
						*tempFloat1 = 1.0f;
					}
					else if (*tempFloat1 < -1.0f)
					{
						*tempFloat1 = -1.0f;
					}
					short tempShort = (*tempFloat1) * 32767.0f;
					*tempConvert1 = tempShort;
					tempFloat1++;
					tempConvert1++;
				}
// 							fwrite(pos->convertBufferShort, data.output_frames_gen * nChannel * sizeof(short), 1, fp4);

				if (nChannel == 1)
				{
// 								mono_to_stereo(pos->doubleChannelBuffer, pos->convertBufferShort, data.output_frames_gen);
					int tempAudioLength = data.output_frames_gen * nChannel * 2;
					short *tempConvert = pChannelInfo->doubleChannelBuffer;
					short *tempShort = (short*)pChannelInfo->convertBufferShort;
					while (tempAudioLength)
					{
						*tempConvert = *tempShort;
						tempConvert++;
						*tempConvert = *tempShort;
						tempConvert++;
						tempShort++;
						tempAudioLength -= 2;
					}
// 								fwrite(pos->doubleChannelBuffer, data.output_frames_gen * nChannel * sizeof(short)* 2, 1, fp5);
					pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->doubleChannelBuffer, data.output_frames_gen, 0, 0, &nSampleWritten);
				}
				else
				{
					pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->convertBufferShort, data.output_frames_gen, 0, 0, &nSampleWritten);
				}
			}
			else
			{
				if (bPGM)
				{
					int tempFrameSize = nSampleCount * nChannel;
					short* tempConvert1 = pChannelInfo->convertBufferShort;
					float* tempFloat1 = (float*)pData;
					while (tempFrameSize--)
					{
						short tempShort = (*tempFloat1) * 32767.0f;
						*tempConvert1 = tempShort;
						tempFloat1++;
						tempConvert1++;
					}
					if (nChannel == 1)
					{
						int tempAudioLength = nAudioLength;
						short *tempConvert = pChannelInfo->doubleChannelBuffer;
						short *tempShort = pChannelInfo->convertBufferShort;
						while (tempAudioLength)
						{
							*tempConvert = *tempShort;
							tempConvert++;
							*tempConvert = *tempShort;
							tempConvert++;
							tempShort++;
							tempAudioLength -= 2;
						}
						pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->doubleChannelBuffer, nSampleCount, 0, 0, &nSampleWritten);
					}
					else
					{
// 						fwrite(pChannelInfo->convertBufferShort, nSampleCount * nChannel * sizeof(short), 1, fp4);
						pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->convertBufferShort, nSampleCount, 0, 0, &nSampleWritten);
					}
				}
				else
				{
					if (4 == nBytePerSample)
					{
						int tempFrameSize = nSampleCount * nChannel;
						short* tempConvert1 = pChannelInfo->convertBufferShort;
						float* tempFloat1 = (float*)pData;
						while (tempFrameSize--)
						{
							short tempShort = (*tempFloat1) * 32767.0f;
							*tempConvert1 = tempShort;
							tempFloat1++;
							tempConvert1++;
						}
					}
					if (nChannel == 1)
					{
						int tempAudioLength = nAudioLength;
						short *tempConvert = pChannelInfo->doubleChannelBuffer;
						short *tempShort = (short*)pData;
						if (4 == nBytePerSample)
							tempShort = pChannelInfo->convertBufferShort;
						while (tempAudioLength)
						{
							*tempConvert = *tempShort;
							tempConvert++;
							*tempConvert = *tempShort;
							tempConvert++;
							tempShort++;
							tempAudioLength -= 2;
						}
						pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->doubleChannelBuffer, nSampleCount, 0, 0, &nSampleWritten);
					}
					else
					{
// 						fwrite(pData, nSampleCount * nChannel * sizeof(short), 1, fp5);
						if (4 == nBytePerSample)
							pChannelInfo->pDLOutput->ScheduleAudioSamples(pChannelInfo->convertBufferShort, nSampleCount, 0, 0, &nSampleWritten);
						else
							pChannelInfo->pDLOutput->ScheduleAudioSamples(pData, nSampleCount, 0, 0, &nSampleWritten);
					}
				}
			}
// 			if (bPGM)
// 			{
// 				unsigned int nCount = 0;
// 				pChannelInfo->pDLOutput->GetBufferedAudioSampleFrameCount(&nCount);
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice,PGM_SDI缓冲音频数=%d!", nCount);
// 			}
// 			else
// 			{
// 				unsigned int nCount = 0;
// 				pChannelInfo->pDLOutput->GetBufferedAudioSampleFrameCount(&nCount);
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice,SDI缓冲音频帧数=%d!", nCount);
// 			}
		}
		else
		{
			int curWidth = pChannelInfo->uiFrameWidth;
			int curHeight = pChannelInfo->uiFrameHeigh;
			unsigned char* pFrame = nullptr;

			if (pChannelInfo->m_BufferList.unsafe_size() < pChannelInfo->nOutBufFrameCount)
			{
				if (colorFormat == ColorFormat_RGB24 || colorFormat == ColorFormat_RGBA32REVERSE || colorFormat == ColorFormat_RGB32)
				{
					pFrame = new unsigned char[curWidth * curHeight * 4];
					if (nWidth != curWidth || nHeight != curHeight)
					{
						if (1280 == nWidth && 720 == nHeight && curWidth == 1920 && curHeight == 1080)
							ResizeRGB720((unsigned char*)pData, (unsigned char*)pFrame);
						else
							ResizeRGB((unsigned char*)pData, (unsigned char*)pFrame, nWidth, nHeight, curWidth, curHeight, colorFormat);
					}
					else
						CopyRGB((unsigned char*)pData, (unsigned char*)pFrame, nWidth, nHeight, colorFormat);
				}
				else
				{
					pFrame = new unsigned char[curWidth * curHeight * 2];
					if (nWidth != curWidth || nHeight != curHeight)
					{
						if (colorFormat == ColorFormat_I420 || colorFormat == ColorFormat_YV12)
							ResizeYUV420((unsigned char*)pData, (unsigned char*)pChannelInfo->pConvertBuf, nWidth, nHeight, curWidth, curHeight);
						else if (colorFormat == ColorFormat_YUY2)
							ResizeYUY2ToYUV420((unsigned char*)pData, (unsigned char*)pChannelInfo->pConvertBuf, nWidth, nHeight, curWidth, curHeight);
						else if (colorFormat == ColorFormat_UYVY)
							ResizeUYVYToYUV420((unsigned char*)pData, (unsigned char*)pChannelInfo->pConvertBuf, nWidth, nHeight, curWidth, curHeight);

						if (colorFormat == ColorFormat_YV12)
							YUV420ToUYVY((unsigned char*)pChannelInfo->pConvertBuf, (unsigned char*)pFrame, curWidth, curHeight, true);
						else
							YUV420ToUYVY((unsigned char*)pChannelInfo->pConvertBuf, (unsigned char*)pFrame, curWidth, curHeight);
					}
					else
					{
						if (colorFormat == ColorFormat_I420)
							YUV420ToUYVY((unsigned char*)pData, (unsigned char*)pFrame, curWidth, curHeight);
						else if (colorFormat == ColorFormat_YV12)
							YUV420ToUYVY((unsigned char*)pData, (unsigned char*)pFrame, curWidth, curHeight, true);
						else if (colorFormat == ColorFormat_UYVY)
							memcpy(pFrame, pData, curWidth * curHeight * 2);
						else if (colorFormat == ColorFormat_YUY2)
						{
							ResizeYUY2ToYUV420((unsigned char*)pData, (unsigned char*)pChannelInfo->pConvertBuf, curWidth, curHeight, curWidth, curHeight);
							YUV420ToUYVY((unsigned char*)pChannelInfo->pConvertBuf, (unsigned char*)pFrame, curWidth, curHeight);
						}
					}
				}

				pChannelInfo->m_BufferList.push(pFrame);
				pChannelInfo->AddRef();
			}

// 			Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice%d AddRef=%d", nDeviceID, pChannelInfo->ref);

// 			unsigned int nCount = 0;
// 			pChannelInfo->pDLOutput->GetBufferedVideoFrameCount(&nCount);
// 
// 			QueryPerformanceFrequency(&nFreq1);
// 			if (nDeviceID == 1)
// 			{
// 				QueryPerformanceCounter(&nLastTime12);
// 				float fInterval = (nLastTime12.QuadPart - nLastTime11.QuadPart) * 1000;
// 				detalRender = fInterval / (float)nFreq1.QuadPart;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice%d间隔=%d, 自己缓冲帧数=%d, SDI缓冲帧数=%d!", nDeviceID, detalRender, pChannelInfo->m_BufferList.unsafe_size(), nCount);
// 				QueryPerformanceCounter(&nLastTime11);
// 			}
// 			else if (nDeviceID == 2)
// 			{
// 				QueryPerformanceCounter(&nLastTime22);
// 				float fInterval = (nLastTime22.QuadPart - nLastTime21.QuadPart) * 1000;
// 				detalRender = fInterval / (float)nFreq1.QuadPart;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice%d间隔=%d, 自己缓冲帧数=%d, SDI缓冲帧数=%d!", nDeviceID, detalRender, pChannelInfo->m_BufferList.unsafe_size(), nCount);
// 				QueryPerformanceCounter(&nLastTime21);
// 			}
// 			else if (nDeviceID == 3)
// 			{
// 				QueryPerformanceCounter(&nLastTime32);
// 				float fInterval = (nLastTime32.QuadPart - nLastTime31.QuadPart) * 1000;
// 				detalRender = fInterval / (float)nFreq1.QuadPart;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice%d间隔=%d, 自己缓冲帧数=%d, SDI缓冲帧数=%d!", nDeviceID, detalRender, pChannelInfo->m_BufferList.unsafe_size(), nCount);
// 				QueryPerformanceCounter(&nLastTime31);
// 			}
// 			else if (nDeviceID == 4)
// 			{
// 				QueryPerformanceCounter(&nLastTime42);
// 				float fInterval = (nLastTime42.QuadPart - nLastTime41.QuadPart) * 1000;
// 				detalRender = fInterval / (float)nFreq1.QuadPart;
// 				Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice%d间隔=%d, 自己缓冲帧数=%d, SDI缓冲帧数=%d!", nDeviceID, detalRender, pChannelInfo->m_BufferList.unsafe_size(), nCount);
// 				QueryPerformanceCounter(&nLastTime41);
// 			}

// 			BMDTimeValue	timeInFrame;
// 			BMDTimeValue	ticksPerFrame;
// 			pChannelInfo->pDLOutput->GetHardwareReferenceClock(pChannelInfo->frameTimescale, &hardwareTime, &timeInFrame, &ticksPerFrame);
// 			detalRender = (hardwareTime - hardwareTime2) * ticksPerFrame / pChannelInfo->frameTimescale;
// 			Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice中间缓冲大小=%d_间隔=%d_缓冲帧数=%d!", pChannelInfo->m_BufferList.unsafe_size(), detalRender, nCount);
// 			pChannelInfo->pDLOutput->GetHardwareReferenceClock(pChannelInfo->frameTimescale, &hardwareTime2, &timeInFrame, &ticksPerFrame);
	
// 			t2 = GetTickCount64();
// 			detalRender = t2 - t1;
// 			Log::writeMessage(LOG_SDI, 1, "SDI_RenderDevice detalRender = %d!", detalRender);
// 			t1 = GetTickCount64();
		}
		LeaveCriticalSection(&pChannelInfo->pMutex);
	}

	return errNone;
}

int SDIOutput::SDI_StopOut(int nDeviceID)
{
	Log::writeMessage(LOG_SDI, 1, "SDI_StopOut Begin 设备ID=%d, 通道数=%d!", nDeviceID, listChannelInfo.size());

	if (nDeviceID < 1 || nDeviceID > m_nDeviceCount)
	{
		Log::writeError(LOG_SDI, 1, "SDI_StopOut nDeviceID Error!");
		return errInvalidParameter;
	}

	EnterCriticalSection(&lock);
	if (listChannelInfo.size())
	{
		std::list<ChannelInfo*>::iterator pos = listChannelInfo.begin();
		std::list<ChannelInfo*>::iterator end = listChannelInfo.end();
		for (; pos != end; ++pos)
		{
			if ((*pos)->nDeviceID == nDeviceID)
			{
// 				int nCount = 0;
// 				while ((*pos)->ref > 0 && nCount < (*pos)->m_BufferList.unsafe_size())
// 				{
// 					Log::writeMessage(LOG_SDI, 1, "ref = %d!", (*pos)->ref);
// 					Sleep(40);
// 					nCount++;
// 				}
// 				(*pos)->bStop = true;

				(*pos)->bStop = true;

				int nCount = 0;
				while ((*pos)->ref > 0 && nCount < 100)
				{
					Log::writeMessage(LOG_SDI, 1, "ref = %d!", (*pos)->ref);
					Sleep(1);
					nCount++;
				}
				EnterCriticalSection(&(*pos)->pMutex);

				(*pos)->pDLOutput->StopScheduledPlayback(0, NULL, 0);
				(*pos)->pDLOutput->DisableVideoOutput();
				(*pos)->pDLOutput->DisableAudioOutput();
				(*pos)->pDLOutput->EndAudioPreroll();
				SAFE_RELEASE((*pos)->pDLOutput);
				SAFE_RELEASE((*pos)->pDL);
				if ((*pos)->pOutputCallback)
				{
					delete (*pos)->pOutputCallback;
					(*pos)->pOutputCallback = nullptr;
				}

				if ((*pos)->pConvertBuf)
				{
					delete[](*pos)->pConvertBuf;
					(*pos)->pConvertBuf = NULL;
				}
				if ((*pos)->resampler)
				{
					src_delete((*pos)->resampler);
					(*pos)->resampler = NULL;
				}
				if ((*pos)->pResampleBuffer)
				{
					delete[](*pos)->pResampleBuffer;
					(*pos)->pResampleBuffer = NULL;
				}
				if ((*pos)->convertBufferFloat)
				{
					delete[](*pos)->convertBufferFloat;
					(*pos)->convertBufferFloat = NULL;
				}
				if ((*pos)->convertBufferShort)
				{
					delete[](*pos)->convertBufferShort;
					(*pos)->convertBufferShort = NULL;
				}
				if ((*pos)->doubleChannelBuffer)
				{
					delete[](*pos)->doubleChannelBuffer;
					(*pos)->doubleChannelBuffer = NULL;
				}
				if ((*pos)->pPreBuffer)
				{
					delete[](*pos)->pPreBuffer;
					(*pos)->pPreBuffer = NULL;
				}

				// 				(*pos)->pDLOutput->SetScheduledFrameCompletionCallback(NULL);
				// 				(*pos)->pDLOutput->SetAudioCallback(NULL);

				tbb::concurrent_queue<unsigned char*>::iterator it = (*pos)->m_BufferList.unsafe_begin();
				for (; it != (*pos)->m_BufferList.unsafe_end(); it++)
				{
					delete[] * it;
					*it = NULL;
				}
				(*pos)->m_BufferList.clear();

				LeaveCriticalSection(&(*pos)->pMutex);
				if ((*pos))
				{
					delete *pos;
					*pos = NULL;
				}

				listChannelInfo.erase(pos);
				break;
			}
		}
	}

	LeaveCriticalSection(&lock);
	Log::writeMessage(LOG_SDI, 1, "SDI_StopOut End 设备ID=%d, 通道数=%d!", nDeviceID, listChannelInfo.size());

	return errNone;
}

void SDIOutput::SetPreroll(int nDeviceID, SDIOUT_COLORFORMAT nColorFormat)
{
	IDeckLinkMutableVideoFrame* pDLVideoFrame;

	// Set 1 second preroll
	if (listChannelInfo.size())
	{
		std::list<ChannelInfo*>::iterator pos = listChannelInfo.begin();
		std::list<ChannelInfo*>::iterator end = listChannelInfo.end();
		for (; pos != end; ++pos)
		{
// 			Log::writeMessage(LOG_SDI, 1, "SetPreroll%d缓冲帧数=%d!", nDeviceID, bufferFrameCount);
			if ((*pos)->nDeviceID == nDeviceID)
			{
				for (unsigned __int32 i = 0; i < (*pos)->nInnerBufFrameCount; i++)
				{
					if (nColorFormat == ColorFormat_RGB32 || nColorFormat == ColorFormat_RGBA32REVERSE || nColorFormat == ColorFormat_RGB24)
					{
						if ((*pos)->pDLOutput->CreateVideoFrame((*pos)->uiFrameWidth, (*pos)->uiFrameHeigh, (*pos)->uiFrameWidth * 4, bmdFormat8BitBGRA, bmdFrameFlagDefault, &pDLVideoFrame) != S_OK)
							goto bail;
					}
					else if (nColorFormat == ColorFormat_UYVY)
					{
						if ((*pos)->pDLOutput->CreateVideoFrame((*pos)->uiFrameWidth, (*pos)->uiFrameHeigh, (*pos)->uiFrameWidth * 2, bmdFormat8BitYUV, bmdFrameFlagDefault, &pDLVideoFrame) != S_OK)
							goto bail;
					}
					else
					{
						if ((*pos)->pDLOutput->CreateVideoFrame((*pos)->uiFrameWidth, (*pos)->uiFrameHeigh, (*pos)->uiFrameWidth * 2, bmdFormat8BitYUV, bmdFrameFlagFlipVertical, &pDLVideoFrame) != S_OK)
							goto bail;
					}

					void*	pFrame;
					pDLVideoFrame->GetBytes((void**)&pFrame);
					memset(pFrame, 0x00, pDLVideoFrame->GetRowBytes() * (*pos)->uiFrameHeigh);

// 					Log::writeMessage(LOG_SDI, 1, "SetPreroll%d ScheduleVideoFrame!%d", nDeviceID, (*pos)->ref);

					int n = ((((*pos)->uiTotalFrames) - FRAMECOUNT) * (*pos)->frameDuration);
					if ((*pos)->pDLOutput->ScheduleVideoFrame(pDLVideoFrame, n, (*pos)->frameDuration, (*pos)->frameTimescale) != S_OK)
						goto bail;

// 					Log::writeMessage(LOG_SDI, 1, "ScheduleVideoFrame 输出起点=%lld, Duration=%lld, Timescale=%lld!", ((*pos)->uiTotalFrames * (*pos)->frameDuration), (*pos)->frameDuration, (*pos)->frameTimescale);

					pDLVideoFrame->Release();
					pDLVideoFrame = NULL;

					(*pos)->uiTotalFrames++;
				}
				if ((*pos)->pDLOutput->BeginAudioPreroll() != S_OK)
					return;

				break;
			}
		}
	}
	return;
bail:
	if (pDLVideoFrame)
	{
		pDLVideoFrame->Release();
		pDLVideoFrame = NULL;
	}
}

int SDIOutput::SDI_GetDeviceList(DeviceInfo** pDevicelist, int* nDeviceCnt)
{
	Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList Begin!");
	if (m_pDevicelist)
	{
		Log::writeError(LOG_SDI, 1, "SDI_GetDeviceList Not First Begin!");
		*pDevicelist = m_pDevicelist;
	}
	else
	{
		int id = 1;
		int nDeviceCount = 0;
		IDeckLink*	pDL = NULL;
		IDeckLinkIterator*				pDLIterator = NULL;
		HRESULT result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDLIterator);
		if (FAILED(result))
		{
			Log::writeError(LOG_SDI, 1, "SDI_GetDeviceList CoCreateInstance IID_IDeckLinkIterator 1 Error! result = %d", result);
			return errDeckLinkDrivers;
		}

		while (pDLIterator->Next(&pDL) == S_OK)
		{
			id++;
			nDeviceCount++;
			SAFE_RELEASE(pDL);
		}
		SAFE_RELEASE(pDLIterator);
		m_nDeviceCount = nDeviceCount;

		result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDLIterator);
		if (FAILED(result))
		{
			Log::writeError(LOG_SDI, 1, "SDI_GetDeviceList CoCreateInstance IID_IDeckLinkIterator 2 Error! result = %d", result);
			return errDeckLinkDrivers;
		}

		int nDeviceIndex = 0;
		DeviceInfo *pTemp = new DeviceInfo[nDeviceCount];
		*pDevicelist = pTemp;
		m_pDevicelist = pTemp;
		BSTR str;
		while (pDLIterator->Next(&pDL) == S_OK)
		{
			Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList IDeckLinkIterator Next!");

			if (pDL->GetDisplayName(&str) == S_OK)
			{
				Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList IDeckLink GetDisplayName! result = %s", str);

				int nLen = 256;
				char strName[256];
				WCHARToAnsi((std::wstring)str, strName, nLen);
				pTemp->strID = nDeviceIndex + 1;
				pTemp->strName = new char[nLen + 1];
				memcpy(pTemp->strName, strName, nLen + 1);
				nDeviceIndex++;
				pTemp++;
				SysFreeString(str);
				SAFE_RELEASE(pDL);
			}
		}
		SAFE_RELEASE(pDLIterator);
		if (nDeviceCnt == 0)
		{
			Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList nDeviceCnt == 0!");
			return errIDeckLink;
		}
	}
	*nDeviceCnt = m_nDeviceCount;

	Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList End! nDeviceCnt == %d", m_nDeviceCount);

	return S_OK;
}

int SDIOutput::SDI_GetDisplayModeList(DisplayMode** pModeList, int* nModeCount)
{
	Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList Begin!");

	if (m_pDisplayModeList)
	{
		Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList Not First Begin!");
		*pModeList = m_pDisplayModeList;
	}
	else
	{
		IDeckLinkDisplayModeIterator*   displayModeIterator = NULL;
		IDeckLinkDisplayMode*           SDIdisplayMode = NULL;
		IDeckLinkIterator*				pDLIterator = NULL;
		IDeckLink*						pDL = NULL;
		IDeckLinkOutput*				pDLOutput = NULL;
		HRESULT result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDLIterator);
		if (FAILED(result))
		{
			Log::writeError(LOG_SDI, 1, "SDI_GetDisplayModeList CoCreateInstance IID_IDeckLinkIterator 1 Error! result = %d", result);
			return errDeckLinkDrivers;
		}

		if (result = pDLIterator->Next(&pDL) == S_FALSE)
		{
			Log::writeError(LOG_SDI, 1, "SDI_GetDisplayModeList CoCreateInstance pDLIterator->Next Error! result = %d", GetLastError());
			SAFE_RELEASE(pDLIterator);
			return errDeckLinkDrivers;
		}

		if (pDL)
		{
			if (pDL->QueryInterface(IID_IDeckLinkOutput, (void**)&pDLOutput) != S_OK)
			{
				Log::writeError(LOG_SDI, 1, "SDI_GetDisplayModeList QueryInterface IID_IDeckLinkOutput Error!");
				SAFE_RELEASE(pDL);
				SAFE_RELEASE(pDLIterator);
				return errDeckLinkDrivers;
			}

			if (pDLOutput->GetDisplayModeIterator(&displayModeIterator) == S_OK)
			{
				Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList IDeckLinkOutput GetDisplayModeIterator!");
				int nModeIndex = 0;
				DisplayMode *pTemp = new DisplayMode[32];
				*pModeList = pTemp;
				m_pDisplayModeList = pTemp;
				BSTR str;
				while (displayModeIterator->Next(&SDIdisplayMode) == S_OK)
				{
					Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList IDeckLinkDisplayModeIterator Next!");

					if (SDIdisplayMode->GetName(&str) == S_OK)
					{
						Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList IDeckLinkDisplayMode GetName! result = %s", str);

						int nLen = 256;
						char strName[256];
						WCHARToAnsi((std::wstring)str, strName, nLen);
						pTemp->strName = new char[nLen + 1];
						pTemp->id = (SDIOUT_DISPLAYMODE)nModeIndex;
						memcpy(pTemp->strName, strName, nLen + 1);
						nModeIndex++;
						pTemp++;
						SysFreeString(str);
						SAFE_RELEASE(SDIdisplayMode);
					}
				}
				m_nModeCount = nModeIndex;
				SAFE_RELEASE(displayModeIterator);
			}
			SAFE_RELEASE(pDLOutput);
			SAFE_RELEASE(pDL);
			SAFE_RELEASE(pDLIterator);
		}
		else
		{
			SAFE_RELEASE(pDLIterator);
			return errDeckLinkDrivers;
		}
	}
	*nModeCount = m_nModeCount;

	Log::writeMessage(LOG_SDI, 1, "SDI_GetDisplayModeList End! nModeCount == %d", m_nModeCount);

	return S_OK;
}

int SDIOutput::SDI_SetDeviceProperty(int deviceID, bool bInput)
{
	mapProperty::iterator it = m_mapProperty.find(deviceID);
	if (it == m_mapProperty.end())
		m_mapProperty.insert(mapProperty::value_type(deviceID, bInput));
	else
		m_mapProperty[deviceID] = bInput;
	return S_OK;
}

int SDIOutput::SDI_SetOutDevicePara(int deviceID, SDIOUT_DISPLAYMODE mode)
{
	mapOutDevicePara::iterator it = m_mapOutDevicePara.find(deviceID);
	if (it == m_mapOutDevicePara.end())
	{
		m_mapOutDevicePara.insert(mapOutDevicePara::value_type(deviceID, mode));
	}
	return S_OK;
}

int SDIOutput::SDI_ReleaseDeviceList()
{
	if (m_pDevicelist)
	{
		for (size_t i = 0; i < m_nDeviceCount; i++)
		{
			DeviceInfo temp = m_pDevicelist[i];
			if (temp.strName)
			{
				delete[] temp.strName;
				temp.strName = NULL;
			}
		}
		delete[] m_pDevicelist;
		m_pDevicelist = NULL;
	}
	return S_OK;
}

int SDIOutput::SDI_ReleaseDisplayModeList()
{
	if (m_pDisplayModeList)
	{
		for (size_t i = 0; i < m_nModeCount; i++)
		{
			DisplayMode temp = m_pDisplayModeList[i];
			if (temp.strName)
			{
				delete[] temp.strName;
				temp.strName = NULL;
			}
		}
		delete m_pDisplayModeList;
		m_pDisplayModeList = NULL;
	}
	return S_OK;
}

mapProperty& SDIOutput::SDI_GetPropertyMap()
{
	return m_mapProperty;
}

mapOutDevicePara& SDIOutput::SDI_GetOutDeviceParaMap()
{
	return m_mapOutDevicePara;
}

bool SDIOutput::Restart(bool bAudio, ChannelInfo* pChannelInfo, int nChannel, int nSample, int nBytePerSample, int nAudioLength, bool bParaChange, SDIOUT_COLORFORMAT colorFormat, int nWidth, int nHeight)
{
	if (bAudio)
	{
		if (pChannelInfo->nPreSrcChannelCount != nChannel
			|| pChannelInfo->nPreSrcSampleCount != nSample
			|| pChannelInfo->nPreSrcBytePerSample != nBytePerSample
			|| pChannelInfo->nPreSrcAudioLength != nAudioLength)
		{
			if (pChannelInfo->resampler)
			{
				src_delete(pChannelInfo->resampler);
				pChannelInfo->resampler = NULL;
			}
			if (pChannelInfo->pResampleBuffer)
			{
				delete[]pChannelInfo->pResampleBuffer;
				pChannelInfo->pResampleBuffer = NULL;
			}
			if (pChannelInfo->convertBufferFloat)
			{
				delete[]pChannelInfo->convertBufferFloat;
				pChannelInfo->convertBufferFloat = NULL;
			}
			if (pChannelInfo->convertBufferShort)
			{
				delete[]pChannelInfo->convertBufferShort;
				pChannelInfo->convertBufferShort = NULL;
			}
			if (pChannelInfo->doubleChannelBuffer)
			{
				delete[]pChannelInfo->doubleChannelBuffer;
				pChannelInfo->doubleChannelBuffer = NULL;
			}

			bParaChange = true;
			pChannelInfo->nPreSrcChannelCount = nChannel;
			pChannelInfo->nPreSrcSampleCount = nSample;
			pChannelInfo->nPreSrcBytePerSample = nBytePerSample;
			pChannelInfo->nPreSrcAudioLength = nAudioLength;
		}
	}
	else
	{
		if (pChannelInfo->colorFormat != colorFormat || pChannelInfo->nPreSrcWidth != nWidth || pChannelInfo->nPreSrcHeight != nHeight)
		{
			bParaChange = true;
			pChannelInfo->colorFormat = colorFormat;
			pChannelInfo->nPreSrcWidth = nWidth;
			pChannelInfo->nPreSrcHeight = nHeight;
		}
	}
	return bParaChange;
}

int SDIOutput::SDI_Configuration()
{
	int id = 1;
	IDeckLink*	pDL = NULL;
	IDeckLinkIterator*	pDLIterator = NULL;
	HRESULT result = CoCreateInstance(CLSID_CDeckLinkIterator, NULL, CLSCTX_ALL, IID_IDeckLinkIterator, (void**)&pDLIterator);
	if (FAILED(result))
	{
		Log::writeError(LOG_SDI, 1, "SDI_GetDeviceList CoCreateInstance IID_IDeckLinkIterator 1 Error! result = %d", result);
		return errDeckLinkDrivers;
	}

	while (pDLIterator->Next(&pDL) == S_OK)
	{
		HRESULT result;
		IDeckLinkConfiguration*				deckLinkConfiguration = NULL;
		result = pDL->QueryInterface(IID_IDeckLinkConfiguration, (void**)&deckLinkConfiguration);
		Log::writeMessage(LOG_SDI, 1, "SDI_GetDeviceList QueryInterface IID_IDeckLinkConfiguration! result = %d", result);

		if (result == S_OK && deckLinkConfiguration)
		{
			if (id == 1 || id == 2)
				result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigDuplexMode, bmdDuplexModeHalf);

			if (m_mapProperty[id])
				result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoInputConnection, bmdVideoConnectionSDI);
			else
				result = deckLinkConfiguration->SetInt(bmdDeckLinkConfigVideoOutputConnection, bmdVideoConnectionSDI);

			deckLinkConfiguration->SetFlag(bmdDeckLinkConfigBlackVideoOutputDuringCapture, TRUE);
			deckLinkConfiguration->SetFlag(bmdDeckLinkConfigFieldFlickerRemoval, TRUE);

			deckLinkConfiguration->WriteConfigurationToPreferences();
			SAFE_RELEASE(deckLinkConfiguration);

			id++;
		}
		SAFE_RELEASE(pDL);
	}
	SAFE_RELEASE(pDLIterator);
}
