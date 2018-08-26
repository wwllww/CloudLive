#include"MultimediaRender.h"
#include "OperatNew.h"

#ifdef OPERATOR_NEW
#define new OPERATOR_NEW
#pragma message("new(__FILE__,__LINE__)")
#endif


void ImgResizeYV12(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *VSrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW / 2;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *USrcImg = pSrcImg + srcW*srcH + srcW*srcH / 4;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW / 2;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;


	bool bUseCrop = false;
	int UseCropH = 0;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}

		if (((float)(srcW / srcH) != (float)(dstW / dstH)) && srcW == dstW)
		{
			if (srcH > dstH)
			{
				UseCropH = (srcH - dstH) / 2;
				bUseCrop = true;
			}
		}

	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int tSrcH, tSrcW;
	if (!bUseCrop)
	{
		for (int i = 0; i < dstH; i++)
		{
			tSrcH = (int)(rateH*double(i) + CropH + 0.5);
			for (int j = 0; j < dstW; j++)
			{
				tSrcW = (int)(rateW * double(j) + CropW + 0.5);
				pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
			}
		}

		for (int i = 0; i < UdstH; i++)
		{
			tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
			for (int j = 0; j < UdstW; j++)
			{
				tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
				UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW + tSrcW];
			}
		}

		for (int i = 0; i < VdstH; i++)
		{
			tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
			for (int j = 0; j < VdstW; j++)
			{
				tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
				VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW + tSrcW];
			}
		}

	}
	else
	{
		memcpy(pDstImg, pDstImg + UseCropH * srcW, dstW * dstH);
		memcpy(UDstImg, USrcImg + UseCropH * srcW / 4, dstW * dstH / 4);
		memcpy(VDstImg, VSrcImg + UseCropH * srcW / 4, dstW * dstH / 4);
	}


}

void ImgResizeRGB32(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	// 	if (3 * srcW > 4 * srcH)
	// 	{
	// 		CorrectSrcW = srcH * 4 / 3;
	// 		CorrectSrcH = srcH;
	// 		CropW = (srcW - CorrectSrcW) / 2;
	// 	}
	// 	else
	// 	{
	// 		CorrectSrcW = srcW;
	// 		CorrectSrcH = srcW * 3 / 4;
	// 		CropH = (srcH - CorrectSrcH) / 2;
	// 	}
	// 
	// 	double rateH = (double)CorrectSrcH / (double)dstH;
	// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH - 1; i >= 0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4];
			G = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 1];
			R = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 2];

			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;

			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void ImgResizeRGB32R(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	// 	if (3 * srcW > 4 * srcH)
	// 	{
	// 		CorrectSrcW = srcH * 4 / 3;
	// 		CorrectSrcH = srcH;
	// 		CropW = (srcW - CorrectSrcW) / 2;
	// 	}
	// 	else
	// 	{
	// 		CorrectSrcW = srcW;
	// 		CorrectSrcH = srcW * 3 / 4;
	// 		CropH = (srcH - CorrectSrcH) / 2;
	// 	}
	// 
	// 	double rateH = (double)CorrectSrcH / (double)dstH;
	// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH - 1; i >= 0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 1];
			G = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 2];
			R = pSrcImg[tSrcH*srcW * 4 + tSrcW * 4 + 3];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			/*Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);*/
			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;


			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void ImgResizeRGB24(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	// 	if (3 * srcW > 4 * srcH)
	// 	{
	// 		CorrectSrcW = srcH * 4 / 3;
	// 		CorrectSrcH = srcH;
	// 		CropW = (srcW - CorrectSrcW) / 2;
	// 	}
	// 	else
	// 	{
	// 		CorrectSrcW = srcW;
	// 		CorrectSrcH = srcW * 3 / 4;
	// 		CropH = (srcH - CorrectSrcH) / 2;
	// 	}
	// 
	// 	double rateH = (double)CorrectSrcH / (double)dstH;
	// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;
	int32_t Y, U, V, R, G, B;

	unsigned char*bufY = pDstImg;
	unsigned char*bufU = bufY + dstW * dstH;
	unsigned char*bufV = bufU + (dstW* dstH / 4);

	int tSrcH, tSrcW;
	for (int i = dstH - 1; i >= 0; i--)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			B = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 0];
			G = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 1];
			R = pSrcImg[tSrcH*srcW * 3 + tSrcW * 3 + 2];
			//pDstImg[i*dstW + j + 3] = pSrcImg[tSrcH*srcW + tSrcW + 3];
			/*Y = (int32_t)(0.299f * R + 0.587f * G + 0.114f * B);
			U = (int32_t)((B - Y) * 0.565f + 128);
			V = (int32_t)((R - Y) * 0.713f + 128);*/
			Y = (int)(19595 * R + 38467 * G + 7471 * B) >> 16;
			U = ((int)((B - Y) * 37028) >> 16) + 128;
			V = ((int)((R - Y) * 56727) >> 16) + 128;


			Y = min(255, max(0, Y));
			*(bufY++) = Y;

			if (j % 2 == 0 && i % 2 == 0)
			{
				if (U > 255)
				{
					U = 255;
				}
				if (U < 0)
				{
					U = 0;
				}
				*(bufU++) = U;
				if (V > 255)
				{
					V = 255;
				}
				if (V < 0)
				{
					V = 0;
				}
				*(bufV++) = V;
			}
		}
	}
}

void ImgResizeYUV422(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *USrcImg = pSrcImg + srcW*srcH;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *VSrcImg = pSrcImg + srcW*srcH + srcW*srcH / 2;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	// 	if (3 * srcW > 4 * srcH)
	// 	{
	// 		CorrectSrcW = srcH * 4 / 3;
	// 		CorrectSrcH = srcH;
	// 		CropW = (srcW - CorrectSrcW) / 2;
	// 	}
	// 	else
	// 	{
	// 		CorrectSrcW = srcW;
	// 		CorrectSrcH = srcW * 3 / 4;
	// 		CropH = (srcH - CorrectSrcH) / 2;
	// 	}
	// 
	// 	double rateH = (double)CorrectSrcH / (double)dstH;
	// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	int tSrcH, tSrcW;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[tSrcH*UsrcW * 2 + tSrcW];
		}
	}

	for (int i = 0; i < VdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[tSrcH*VsrcW * 2 + tSrcW];
		}
	}
}


void ImgResizeUYVY(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *YSrcImg = pSrcImg + 1;
	unsigned char *USrcImg = pSrcImg;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;


	unsigned char *VSrcImg = pSrcImg + 2;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;


	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;


	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	// 	if (3 * srcW > 4 * srcH)
	// 	{
	// 		CorrectSrcW = srcH * 4 / 3;
	// 		CorrectSrcH = srcH;
	// 		CropW = (srcW - CorrectSrcW) / 2;
	// 	}
	// 	else
	// 	{
	// 		CorrectSrcW = srcW;
	// 		CorrectSrcH = srcW * 3 / 4;
	// 		CropH = (srcH - CorrectSrcH) / 2;
	// 	}
	// 
	// 	double rateH = (double)CorrectSrcH / (double)dstH;
	// 	double rateW = (double)CorrectSrcW / (double)dstW;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;


	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[(tSrcH*VsrcW + tSrcW) * 4];
		}
	}

	return;
}

void ImgResizeYUY2(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH)
{
	unsigned char *YSrcImg = pSrcImg;
	unsigned char *USrcImg = pSrcImg + 1;
	unsigned char *UDstImg = pDstImg + dstW*dstH;

	int UsrcW = srcW;
	int UdstW = dstW / 2;
	int UsrcH = srcH / 2;
	int UdstH = dstH / 2;

	unsigned char *VSrcImg = pSrcImg + 3;
	unsigned char *VDstImg = pDstImg + dstW*dstH + dstW*dstH / 4;

	int VsrcW = srcW;
	int VdstW = dstW / 2;
	int VsrcH = srcH / 2;
	int VdstH = dstH / 2;

	//double rateH = (double)srcH / (double)dstH;
	//double rateW = (double)srcW / (double)dstW;

	int CorrectSrcW, CorrectSrcH;
	int CropW = 0;
	int CropH = 0;

	if (srcW > srcH)
	{
		CorrectSrcW = srcW;
		CorrectSrcH = srcW * 9 / 16;
		CropH = (srcH - CorrectSrcH) / 2;
		if (CropH < 0)
		{
			CropH = 0;
			CorrectSrcW = srcH * 16 / 9;
			CorrectSrcH = srcH;
			CropW = (srcW - CorrectSrcW) / 2;
		}
	}
	else
	{
		CorrectSrcW = srcH * 16 / 9;
		CorrectSrcH = srcH;
		CropW = (srcW - CorrectSrcW) / 2;
		if (CropW < 0)
		{
			CropW = 0;
			CorrectSrcW = srcW;
			CorrectSrcH = srcW * 9 / 16;
			CropH = (srcH - CorrectSrcH) / 2;
		}
	}

	double rateH = (double)CorrectSrcH / (double)dstH;
	double rateW = (double)CorrectSrcW / (double)dstW;

	for (int i = 0; i < dstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}


	for (int i = 0; i < VdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + CropH / 2 + 0.5);
		for (int j = 0; j < VdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + CropW / 2 + 0.5);
			VDstImg[i*VdstW + j] = VSrcImg[(tSrcH*VsrcW + tSrcW) * 4];
		}
	}

	return;
}

void ResizeRGB(unsigned char* pSrcImg, unsigned char* pDstImg, int srcW, int srcH, int dstW, int dstH, int pixformat)
{
	double rateH = (double)srcH / (double)dstH;
	double rateW = (double)srcW / (double)dstW;

	unsigned  int * tSrcW = new  unsigned  int[dstW];
	for (unsigned int j = 0; j < dstW; j++)
	{
		tSrcW[j] = (int)(rateW * double(j) + 0.5);
	}

	int tSrcH;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH * double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			if ( pixformat == ColorType_RGB24 )
			{
				pDstImg[(dstH - i - 1) * dstW * 3 + j * 3] = pSrcImg[tSrcH * srcW * 3 + tSrcW[j] * 3];
				pDstImg[(dstH - i - 1) * dstW * 3 + j * 3 + 1] = pSrcImg[tSrcH * srcW * 3 + tSrcW[j] * 3 + 1];
				pDstImg[(dstH - i - 1) * dstW * 3 + j * 3 + 2] = pSrcImg[tSrcH * srcW * 3 + tSrcW[j] * 3 + 2];
			}
			else if ( pixformat == ColorType_RGBA32REVERSE )
			{
				pDstImg[i * dstW * 4 + j * 4] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4];
				pDstImg[i * dstW * 4 + j * 4 + 1] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 1];
				pDstImg[i * dstW * 4 + j * 4 + 2] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 2];
				pDstImg[i * dstW * 4 + j * 4 + 3] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 3];
			}
			else if ( pixformat == ColorType_RGB )
			{
				pDstImg[(dstH - i - 1) * dstW * 4 + j * 4] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4];
				pDstImg[(dstH - i - 1) * dstW * 4 + j * 4 + 1] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 1];
				pDstImg[(dstH - i - 1) * dstW * 4 + j * 4 + 2] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 2];
				pDstImg[(dstH - i - 1) * dstW * 4 + j * 4 + 3] = pSrcImg[tSrcH * srcW * 4 + tSrcW[j] * 4 + 3];
			}
		}
	}
	delete[] tSrcW;
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
		tSrcW[j] = (int)(rateW * double(j) + 0.5);
	}

	int tSrcH;
	for (int i = 0; i < dstH; i++)
	{
		tSrcH = (int)(rateH * double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			pDstImg[i*dstW + j] = pSrcImg[tSrcH * srcW + tSrcW[j]];
		}
	}

	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH * double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			UDstImg[i*UdstW + j] = USrcImg[tSrcH * UsrcW + tSrcW[j]];
			VDstImg[i*UdstW + j] = VSrcImg[tSrcH * UsrcW + tSrcW[j]];
		}
	}
	delete[] tSrcW;
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
		tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + 0.5);
			pDstImg[i*dstW + j] = pSrcImg[tSrcH*srcW + tSrcW];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			tSrcW = (int)(rateW * double(j) + 0.5);
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
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
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
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < dstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			pDstImg[i*dstW + j] = YSrcImg[(tSrcH*srcW + tSrcW) * 2];
		}
	}
	for (int i = 0; i < UdstH; i++)
	{
		int tSrcH = (int)(rateH*double(i) + 0.5);
		for (int j = 0; j < UdstW; j++)
		{
			int tSrcW = (int)(rateW * double(j) + 0.5);
			UDstImg[i*UdstW + j] = USrcImg[(tSrcH*UsrcW + tSrcW) * 4];
			VDstImg[i*UdstW + j] = VSrcImg[(tSrcH*UsrcW + tSrcW) * 4];
		}
	}
}

void CopyDataI420(unsigned char *DstImg, unsigned char *SrcImg, int srcW, int srcH, int dstW, int dstH, int NewW, unsigned char* pTemImg, TColorType ColorType)
{
	//int NewW = srcW * dstH / srcH;


	if (ColorType == ColorType_I420)
		ResizeYUV420(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	else if (ColorType == ColorType_YUY2)
	{
		ImgResizeYUY2(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_YV12)
	{
		ImgResizeYV12(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_RGB)
	{
		ImgResizeRGB32(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_RGB24)
	{
		ImgResizeRGB24(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_RGBA32REVERSE)
	{
		ImgResizeRGB32R(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_UYVY)
	{
		ImgResizeUYVY(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}
	else if (ColorType == ColorType_YVYU)
	{
		ImgResizeYUV422(SrcImg, pTemImg, srcW, srcH, NewW, dstH);
	}

	unsigned char *USrcImg = pTemImg + NewW*dstH;
	unsigned char *UDstImg = DstImg + dstW*dstH;
	unsigned char *VSrcImg = pTemImg + NewW*dstH + NewW*dstH / 4;
	unsigned char *VDstImg = DstImg + dstW*dstH + dstW*dstH / 4;

	//y
	for (int h = 0; h < dstH; ++h)
	{
		for (int w = (dstW - NewW) / 2, i = 0; w < (dstW - NewW) / 2 + NewW,i < NewW; ++w,++i)
		{
			DstImg[h * dstW + w] = pTemImg[h *NewW + i];
		}
	}

	//uv
	dstH >>= 1;
	dstW >>= 1;
	NewW >>= 1;
	for (int h = 0; h < dstH; ++h)
	{
		for (int w = (dstW  - NewW ) / 2, i = 0; w < (dstW - NewW) / 2 + NewW, i < NewW; ++w, ++i)
		{
			UDstImg[h * dstW + w] = USrcImg[h *NewW + i];
			VDstImg[h * dstW + w] = VSrcImg[h *NewW + i];
		}
	}
}

CMultimediaRender::CMultimediaRender() :CanAudio(false), Width(0), Height(0), Pixformat(0), AudioOut(NULL)
{
	m_pos.x = m_pos.y = 0;
	m_size.x = m_size.y = 0;
	Pitch = 0; 
	texture = NULL;
	Oldtexture = NULL;
	m_pDes = NULL;
	colorConvertShader = NULL;
	colorHandle = NULL;
	LpConverData = NULL;
	LpTemData = NULL;
	SwapRender = NULL;
	m_bInteraction = false;
	bReadyToDraw = false;
	SDID3DResize = NULL;// new CD3DReszie(CSLiveManager::GetInstance()->BSParam.DeviceSetting.AdpterID);//CSLiveManager::GetInstance()->GetD3DLittleRender();
	D3DRender = GetD3DRender();
	String strShader = ChooseShader(DeviceOutputType_I420);
	colorConvertShader = NULL;

	bUseYV12 = false;
	bUseI420 = false;
	Pixformat = ColorType_HDYC;
	memset(&WaveFormat, 0, sizeof(WAVEFORMATEX));
}

CMultimediaRender::~CMultimediaRender()
{
	m_lock.Lock();
	AudioDestroy();
	VideoDestroy();
	if (colorConvertShader)
		delete colorConvertShader;
	colorConvertShader = NULL;

	if (LpConverData)
		delete [] LpConverData;

	LpConverData = NULL;

	if (LpTemData)
		delete[] LpTemData;
	LpTemData = NULL;

	if (SwapRender)
		delete SwapRender;

	if (SDID3DResize)
		delete SDID3DResize;


	m_lock.UnLock();
}

bool CMultimediaRender::SetVideoRender(const Vect2 &pos, const Vect2 &size)
{
	m_lock.Lock();
	m_pos = pos;
	m_size = size;
	m_lock.UnLock();
	//m_hwnd = Hwnd;
	bReadyToDraw = true;
	return true;
}

bool CMultimediaRender::SetAudioRender(int channels, int samplerate, int sampleperbits)
{
	String &MonitorDevice = GetDirectorMonitorDevices();
	m_lock.Lock();
	AudioDestroy();

	AudioOut = new AudioWaveOut();
	if (!MonitorDevice.Compare(TEXT("停用")) && !MonitorDevice.Compare(TEXT("Disable")))
	{
		AudioOut->Initialize(MonitorDevice.Array(), channels, samplerate, sampleperbits);
	}
	m_lock.UnLock();
	return true;
}

void CMultimediaRender::AudioDestroy()
{
	if (AudioOut)
	{
		delete AudioOut;
		AudioOut = NULL;
	}
}

void CMultimediaRender::VideoDestroy()
{
	m_previewlock.Lock();
	if (m_pDes)
	{
		delete m_pDes;
		m_pDes = nullptr;
	}
	if (texture) 
	{
		delete texture;
		texture = nullptr;
	}
	m_previewlock.UnLock();
}

void CMultimediaRender::Render(CSampleData* data, bool bAudioDisabled)
{

	m_lock.Lock();
	data->AddRef();

	bool bNeedConvert = false;
	if (/*m_bInteraction && */data->cx < data->cy && !data->bAudio)
	{
		bNeedConvert = true;
		if (Width != data->cx || Height != data->cy)
		{
			if (LpConverData)
				delete[] LpConverData;

			LpConverData = new unsigned char[data->cx * data->cy * 3 / 2];

			//先置成黑色
			memset(LpConverData, 0x00, data->cx * data->cy);
			memset(LpConverData + data->cx * data->cy, 0x80, data->cx * data->cy / 2);

			if (LpTemData)
				delete[] LpTemData;
		
			LpTemData = new unsigned char[data->cx * data->cx  * data->cx / data->cy * 3 / 2];//以data->cy : data->cx = data->cx : NewWidth -> NewWidth = data->cx * data->cx / data->cy

			Width = data->cx;
			Height = data->cy;
		}

		//各种格式的统一转成I420
		CopyDataI420(LpConverData, data->lpData, data->cx, data->cy, data->cy, data->cx, data->cx * data->cx / data->cy, LpTemData, (TColorType)data->colorType);

		data->colorType = ColorType_I420;
	}

	int DesWidth, DesHeight;

	SDIMutex.Lock();
	BlackMagic* blackMagic = BlackMagic::Instance();
	for (auto& id : SIDIDs)
	{
		if (id.enable && blackMagic->AllEnable())
		{
//			GetDisPlayMode((SDIOUT_DISPLAYMODE)id.displayMode, DesWidth, DesHeight);

// 			bool bConvert = false;
// 
// 			if (LpConverData && bNeedConvert)
// 			{
// 				if (DesWidth != data->cy || DesHeight != data->cx)
// 				{
// 					bConvert = true;
// 				}
// 			}
// 			else if (!data->bAudio)
// 			{
// 				if (DesWidth != data->cx || DesHeight != data->cy)
// 				{
// 					bConvert = true;
// 				}
// 			}
// 
// 			if (!bConvert)
			{
				if (id.colorFormat != (SDIOUT_COLORFORMAT)data->colorType && (!data->bAudio))
				{
					blackMagic->SDI_StopOut(id.id);
					blackMagic->SDI_StartOut(id.id, (SDIOUT_DISPLAYMODE)id.displayMode, (SDIOUT_COLORFORMAT)data->colorType, 0);
					id.colorFormat = (SDIOUT_COLORFORMAT)data->colorType;
				}
			}
// 			else if (SDID3DResize)
// 			{
// 
// 				if (id.colorFormat != ColorFormat_RGBA32REVERSE && (!data->bAudio))
// 				{
// 					blackMagic->SDI_StopOut(id.id);
// 					blackMagic->SDI_StartOut(id.id, (SDIOUT_DISPLAYMODE)id.displayMode, ColorFormat_RGBA32REVERSE, 0);
// 					id.colorFormat = ColorFormat_RGBA32REVERSE;
// 				}
// 			}
			

			if (LpConverData && bNeedConvert)
			{
// 				if (bConvert && SDID3DResize) //进来一定不是音频
// 				{
// 					m_previewlock.Lock();
// 					unsigned char *ResizeBuf = SDID3DResize->Resize(LpConverData, data->cy, data->cx, (TColorType)data->colorType, DesWidth, DesHeight, data->bFieldSignal);
// 					if (ResizeBuf)
// 					{
// 						blackMagic->SDI_RenderDevice(id, ResizeBuf, DesWidth, DesHeight, ColorFormat_RGBA32REVERSE, data->bAudio, data->pAudioFormat, DesWidth * DesHeight * 4, false);
// 						SDID3DResize->UnMap();
// 					}
// 					m_previewlock.UnLock();
// 
// 				}
// 				else
				{
					blackMagic->SDI_RenderDevice(id, LpConverData, data->cy, data->cx, (SDIOUT_COLORFORMAT)data->colorType, data->bAudio, data->pAudioFormat, data->dataLength, false);
				}

			}
			else
			{
// 				if (bConvert && SDID3DResize)
// 				{
// 					//进来一定不是音频
// 					m_previewlock.Lock();
// 					unsigned char *ResizeBuf = SDID3DResize->Resize(data->lpData, data->cx, data->cy, (TColorType)data->colorType, DesWidth, DesHeight, data->bFieldSignal);
// 					//QWORD Start = GetQPCMS();
// 					if (ResizeBuf)
// 					{
// 						blackMagic->SDI_RenderDevice(id, ResizeBuf, DesWidth, DesHeight, ColorFormat_RGBA32REVERSE, data->bAudio, data->pAudioFormat, DesWidth * DesHeight * 4, false);
// 						SDID3DResize->UnMap();
// 					}
// 					m_previewlock.UnLock();
// 				}
// 				else
				{
					if (data->bAudio && !data->UserData || !data->bAudio) //视频捕捉设备的声音不投SDI,目前只有视频捕捉设备UserData有值
					{
						blackMagic->SDI_RenderDevice(id, data->lpData, data->cx, data->cy, (SDIOUT_COLORFORMAT)data->colorType, data->bAudio, data->pAudioFormat, data->dataLength, false);
					}
				}
				
			}
		}
	}
	SDIMutex.UnLock();

	if (data->bAudio)
	{
		WAVEFORMATEX* pAudioFormat = (WAVEFORMATEX*)(data->pAudioFormat);
		if (AudioOut == NULL || WaveFormat.nChannels != pAudioFormat->nChannels || 
			WaveFormat.nSamplesPerSec != pAudioFormat->nSamplesPerSec  ||
			WaveFormat.wBitsPerSample != pAudioFormat->wBitsPerSample)
		{
			SetAudioRender(pAudioFormat->nChannels, pAudioFormat->nSamplesPerSec, pAudioFormat->wBitsPerSample);
			WaveFormat = *pAudioFormat;
			//Log::writeMessage(LOG_RTSPSERV, 1, "WaveFormat 成功！");
		}	

		AudioRender(data->lpData, data->dataLength);
	}
	else
	{
		if (bAudioDisabled)
			AudioDestroy();
		if (LpConverData && bNeedConvert)
		{
			VideoRender(LpConverData, data->cy, data->cx, data->colorType);
		}
		else
		{
			VideoRender(data->lpData, data->cx, data->cy, data->colorType);
		}
		
	}
	data->Release();
	m_lock.UnLock();
}

void CMultimediaRender::VideoRender(unsigned char* Buffer, int width, int height, int pixformat)
{
	m_previewlock.Lock();

	if (Pixformat != pixformat)
	{
		if (m_pDes)
		{
			delete m_pDes;
			m_pDes = NULL;
		}
		if (texture)
		{
			Oldtexture = texture;
			delete texture;
			texture = NULL;
		}
		Pixformat = pixformat;
	}

	float nByte = 0;
	if ( pixformat == ColorType_I420 || pixformat == ColorType_YV12 )
		nByte = 1.5;
	else if ( pixformat == ColorType_YUY2 || pixformat == ColorType_UYVY )
		nByte = 2;
	else if ( pixformat == ColorType_RGB24 )
		nByte = 3;
	else if ( pixformat == ColorType_RGB || pixformat == ColorType_RGBA32REVERSE)
		nByte = 4;

	int len = m_size.x * m_size.y * nByte;
	if (m_pDes == NULL)
		m_pDes = new unsigned char[len];
	//memset(m_pDes, 0, len);

	if (pixformat == ColorType_RGB24)
	{
		if (texture == NULL)
		{
			LPBYTE textureData = (LPBYTE)Allocate_Bak(m_size.x * m_size.y * 4);
			msetd(textureData, 0xFF0000FF, m_size.x * m_size.y * 4);
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGR, textureData, FALSE, FALSE);
			Free(textureData);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGR, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}

		ResizeRGB(Buffer, m_pDes, width, height, m_size.x, m_size.y, pixformat);
	}
	else if ( pixformat == ColorType_RGBA32REVERSE )
	{
		if (texture == NULL)
		{
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGRA, nullptr, FALSE, FALSE);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGRA, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}

		ResizeRGB(Buffer, m_pDes, width, height, m_size.x, m_size.y, pixformat);
	}
	else if (pixformat == ColorType_RGB )
	{
		if (texture == NULL)
		{
			LPBYTE textureData = (LPBYTE)Allocate_Bak(m_size.x * m_size.y * 4);
			msetd(textureData, 0xFFFF0000, m_size.x * m_size.y * 4);
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGR, textureData, FALSE, FALSE);
			Free(textureData);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_BGR, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}

		ResizeRGB(Buffer, m_pDes, width, height, m_size.x, m_size.y, pixformat);
	}
	else if (pixformat == ColorType_I420 || pixformat == ColorType_YV12)
	{
		if (texture == NULL)
		{
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGBA, nullptr, FALSE, FALSE);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGBA, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}
			
		ResizeYUV420(Buffer, m_pDes, width, height, m_size.x, m_size.y);
	}
	else if (pixformat == ColorType_YUY2)
	{
		if (texture == NULL)
		{
			BYTE *textureData = (BYTE*)Allocate_Bak(m_size.x * m_size.y * 4);
			msetd(textureData, 0xFF0000FF, m_size.x * m_size.y * 4);
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGB, textureData, FALSE, FALSE);
			Free(textureData);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGB, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}


		ResizeYUY2ToYUV420(Buffer, m_pDes, width, height, m_size.x, m_size.y);
	}
	else if (pixformat == ColorType_UYVY)
	{
		if (texture == NULL)
		{
			BYTE *textureData = (BYTE*)Allocate_Bak(m_size.x * m_size.y * 4);
			msetd(textureData, 0xFF0000FF, m_size.x * m_size.y * 4);
			texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGB, textureData, FALSE, FALSE);
			Free(textureData);

			if (Oldtexture == texture)
			{
				texture = D3DRender->CreateTexture(m_size.x, m_size.y, GS_RGB, nullptr, FALSE, FALSE);
				delete Oldtexture;
				Oldtexture = NULL;
			}
		}

		ResizeUYVYToYUV420(Buffer, m_pDes, width, height, m_size.x, m_size.y);
	}
	m_previewlock.UnLock();
}

void CMultimediaRender::UpLoad()
{
	if (Pixformat == ColorType_RGB24)
	{
		D3DRender->SetImage(texture, m_pDes, GS_IMAGEFORMAT_BGR, m_size.x * 3);
	}
	else if (Pixformat == ColorType_RGBA32REVERSE)
	{
		D3DRender->SetImage(texture, m_pDes, GS_IMAGEFORMAT_BGRA, m_size.x * 4);
	}
	else if (Pixformat == ColorType_RGB)
	{
		D3DRender->SetImage(texture, m_pDes, GS_IMAGEFORMAT_BGRX, m_size.x * 4);
	}
	else if (Pixformat == ColorType_I420 || Pixformat == ColorType_YV12 || Pixformat == ColorType_YUY2 || Pixformat == ColorType_UYVY)
	{
		LPBYTE lpData;
		UINT pitch;
		if (S_OK == D3DRender->Map(texture, lpData, pitch, D3D11_MAP_WRITE_DISCARD))
		{
			PackPlanar(lpData, m_pDes, m_size.x, m_size.y, pitch, 0, m_size.y, m_size.x, 0);
			D3DRender->Unmap(texture);
		}
	}
}

void CMultimediaRender::AudioRender(unsigned char* Buffer, unsigned int ilen)
{
	if (AudioOut && CanAudio)
		AudioOut->push_pcm_data((char*)Buffer, ilen);
}	

bool CMultimediaRender::SetAudioRender()
{

	if ((AudioOut != NULL))
	{
		CanAudio = !CanAudio;

		return CanAudio;
	}
	else
	{
		return false;
	}
}

bool CMultimediaRender::CanPlayAudio()
{
	bool result;
	m_lock.Lock();
	result = (AudioOut != NULL);
	m_lock.UnLock();
	return result;
}

String CMultimediaRender::ChooseShader(DeviceColorType colorType)
{
	String strShader;
	strShader << SHADER_PATH;
	if (colorType == DeviceOutputType_I420)
		strShader << TEXT("YUVToRGB.pShader");
	else if (colorType == DeviceOutputType_YV12)
		strShader << TEXT("YVUToRGB.pShader");
	else if (colorType == DeviceOutputType_YVYU)
		strShader << TEXT("YVXUToRGB.pShader");
	else if (colorType == DeviceOutputType_YUY2)
		strShader << TEXT("YUXVToRGB.pShader");
	else if (colorType == DeviceOutputType_UYVY)
		strShader << TEXT("UYVToRGB.pShader");
	else if (colorType == DeviceOutputType_HDYC)
		strShader << TEXT("HDYCToRGB.pShader");
	else
		strShader.Clear();

	return strShader;
}

void CMultimediaRender::ResetShaderYV12()
{
	if (!bUseYV12 && Pixformat == ColorType_YV12)
	{
		if (colorConvertShader)
			delete colorConvertShader;

		colorConvertShader = NULL;

		String &strShader = ChooseShader(DeviceOutputType_YV12);
		colorConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);
		if (colorConvertShader)
			colorHandle = colorConvertShader->GetParameterByName(TEXT("gamma"));

		colorConvertShader->SetFloat(colorHandle, 1.0f);

		D3DRender->LoadPixelShader(colorConvertShader);
		bUseYV12 = true;
		bUseI420 = false;
	}
	else if (!bUseI420 && (Pixformat == ColorType_I420 || Pixformat == ColorType_YUY2 || Pixformat == ColorType_UYVY))
	{
		if (colorConvertShader)
			delete colorConvertShader;

		colorConvertShader = NULL;

		String &strShader = ChooseShader(DeviceOutputType_I420);
		colorConvertShader = D3DRender->CreatePixelShaderFromFile(strShader);
		if (colorConvertShader)
			colorHandle = colorConvertShader->GetParameterByName(TEXT("gamma"));

		colorConvertShader->SetFloat(colorHandle, 1.0f);

		D3DRender->LoadPixelShader(colorConvertShader);

		bUseI420 = true;
		bUseYV12 = false;
	}
}


bool CMultimediaRender::HaveSDIOut()
{
	BlackMagic* blackMagic = BlackMagic::Instance();
	return (FirstSDIRender != -1000) && blackMagic->AllEnable();
}

void CMultimediaRender::PushOrUpdateSIDId(const SDIID& id)
{
	if (FirstSDIRender == -1000 && id.enable)
	{
		FirstSDIRender = id.id;
	}
	else if (FirstSDIRender == id.id && (!id.enable))
	{
		FirstSDIRender = -1000;
	}

	SDIMutex.Lock();

	bool find = false;
	for (auto& sid : SIDIDs)
	{
		if (sid.id == id.id)
		{
			sid.enable = id.enable;
			sid.colorFormat = id.colorFormat;
			sid.displayMode = id.displayMode;
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

	SDIMutex.UnLock();
}

bool CMultimediaRender::FindAndRemoveId(int id)
{
	SDIMutex.Lock();
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
	SDIMutex.UnLock();

	return 0;
}

void CMultimediaRender::SetIsInteraction(bool bInteraction)
{
	m_bInteraction = bInteraction;
}

void CMultimediaRender::GetDisPlayMode(SDIOUT_DISPLAYMODE mode, int &Width, int& Height)
{
	switch (mode)
	{
	case NTSC:
		Width = 720;
		Height = 576;
		break;
	case PAL:
		Width = 720;
		Height = 480;
		break;
	case HD1080p2398:
	case HD1080p24:
	case HD1080p25:
	case HD1080p2997:
	case HD1080p30:
	case HD1080i50:
	case HD1080i5994:
	case HD1080i6000:
	case HD1080p50:
	case HD1080p5994:
	case HD1080p60:
		Width = 1920;
		Height = 1080;
		break;
	case HD720p50:
	case HD720p5994:
	case HD720p60:
		Width = 1280;
		Height = 720;
		break;
	default:
		break;
	}
}

void CMultimediaRender::InitD3DReSize()
{
	if (!SDID3DResize)
		return;

	if (!SDID3DResize->Init())
	{
		Log::writeError(LOG_RTSPSERV, 1, "SDID3DResize 初始化失败!");
	}
}

void CMultimediaRender::RenderTexture(Shader *VertShader, Shader *PixShader)
{
	if (!bReadyToDraw)
		return;

// 	if (!SwapRender)
// 		SwapRender = D3DRender->CreateRenderTargetSwapChain((HWND)m_hwnd, m_size.x, m_size.y);

// 	Shader *MainVertShader = SDID3DResize->GetMainVertexShader();
// 	Shader* MainPixShader = SDID3DResize->GetMainPixelShader();
// 	Texture *SDITexture = SDID3DResize->GetSDITexture();

	m_previewlock.Lock(); //保护D3DRender因为在SDIOut中也用到D3DRender

	//D3DRender->SetRenderTarget(SwapRender);

	if (texture)
	{
// 		D3DRender->Ortho(0.0f, m_size.x, m_size.y, 0.0f, -100.0f, 100.0f);
// 		D3DRender->SetViewport(0.0f, 0.0f, m_size.x, m_size.y);
// 		D3DRender->ClearRenderTarget(0xFF000000);

		D3DRender->LoadVertexShader(VertShader);
		if (Pixformat == ColorType_I420 || Pixformat == ColorType_YV12
			|| Pixformat == ColorType_YUY2 || Pixformat == ColorType_UYVY)
		{
			ResetShaderYV12();

			if (colorConvertShader && colorHandle)
			{
				colorConvertShader->SetFloat(colorHandle, 1.0f);
				D3DRender->LoadPixelShader(colorConvertShader);
			}
		}
		else
		{
			bUseYV12 = false;
			bUseI420 = false;
			D3DRender->LoadPixelShader(PixShader);
		}

		UpLoad();
		D3DRender->DrawSprite(texture, 0xFFFFFFFF, m_pos.x, m_pos.y, m_pos.x + m_size.x, m_pos.y + m_size.y);

// 		if (HaveSDIOut() && SDITexture)
// 		{
// 			D3DRender->LoadPixelShader(MainPixShader);
// 			DWORD Width, Height;
// 			D3DRender->GetTextureWH(SDITexture, Width, Height);
// 			D3DRender->EnableBlending(TRUE);
// 			D3DRender->BlendFunction(GS_BLEND_SRCALPHA, GS_BLEND_INVSRCALPHA, 1.0f);
// 			D3DRender->DrawSprite(SDITexture, 0xFFFFFFFF, m_size.x - Width, 0, m_size.x, Height);
// 			D3DRender->EnableBlending(FALSE);
// 		}

	}
	//D3DRender->Present(SwapRender);
	//D3DRender->Flush();

	m_previewlock.UnLock();
}
