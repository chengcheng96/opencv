// test.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "GxIAPI.h"
#include "DxImageProc.h"
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

using namespace std;
using namespace cv;

GX_DEV_HANDLE       m_hDevice;              ///< 设备句柄
BYTE                *m_pBufferRaw;          ///< 原始图像数据
BYTE                *m_pBufferRGB;	        ///< RGB图像数据，用于显示和保存bmp图像
int64_t             m_nImageHeight;         ///< 原始图像高
int64_t             m_nImageWidth;          ///< 原始图像宽
int64_t             m_nPayLoadSize;
int64_t             m_nPixelColorFilter;    ///< Bayer格式
Mat test;

//图像回调处理函数
static void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame)
{
	//PrepareForShowImg();
	if (pFrame->status == 0)
	{
		//对图像进行某些操作
		/*memcpy(m_pBufferRaw, pFrame->pImgBuf, pFrame->nImgSize);
		// 黑白相机需要翻转数据后显示
		for (int i = 0; i <m_nImageHeight; i++)		{
		memcpy(m_pImageBuffer + i*m_nImageWidth, m_pBufferRaw + (m_nImageHeight - i - 1)*m_nImageWidth, (size_t)m_nImageWidth);
		}
		IplImage* src;
		src = cvCreateImage(cvSize(m_nImageWidth, m_nImageHeight), 8, 1);
		src->imageData = (char*)m_pImageBuffer;
		cvSaveImage("src.jpg", src);*/
		memcpy(m_pBufferRaw, pFrame->pImgBuf, pFrame->nImgSize);

		// RGB转换
		DxRaw8toRGB24(m_pBufferRaw
			, m_pBufferRGB
			, (VxUint32)(m_nImageWidth)
			, (VxUint32)(m_nImageHeight)
			, RAW2RGB_NEIGHBOUR
			, DX_PIXEL_COLOR_FILTER(m_nPixelColorFilter)
			, false);
		//	cv_Image->imageData = (char*)m_pBufferRGB;

		//	cvSaveImage("./test.bmp", cv_Image);

		//test.data = m_pBufferRaw;
		memcpy(test.data, m_pBufferRGB, m_nImageWidth*m_nImageHeight * 3);
		imwrite("./test1.bmp", test);
		namedWindow("test");
		imshow("test", test);
		waitKey(15);

	}
	return;
}

int main(int argc, char* argv[])
{
	GX_STATUS emStatus = GX_STATUS_SUCCESS;
	GX_OPEN_PARAM openParam;
	uint32_t      nDeviceNum = 0;
	openParam.accessMode = GX_ACCESS_EXCLUSIVE;
	openParam.openMode = GX_OPEN_INDEX;
	openParam.pszContent = "1";
	// 初始化库 
	emStatus = GXInitLib();
	if (emStatus != GX_STATUS_SUCCESS)
	{
		return 0;
	}
	// 枚举设备列表
	emStatus = GXUpdateDeviceList(&nDeviceNum, 1000);
	if ((emStatus != GX_STATUS_SUCCESS) || (nDeviceNum <= 0))
	{
		return 0;
	}
	//打开设备
	emStatus = GXOpenDevice(&openParam, &m_hDevice);


	//设置采集模式连续采集
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_ACQUISITION_MODE, GX_ACQ_MODE_CONTINUOUS);
	emStatus = GXSetInt(m_hDevice, GX_INT_ACQUISITION_SPEED_LEVEL, 1);
	emStatus = GXSetEnum(m_hDevice, GX_ENUM_BALANCE_WHITE_AUTO, GX_BALANCE_WHITE_AUTO_CONTINUOUS);

	bool      bColorFliter = false;
	// 获取图像大小
	emStatus = GXGetInt(m_hDevice, GX_INT_PAYLOAD_SIZE, &m_nPayLoadSize);
	// 获取宽度
	emStatus = GXGetInt(m_hDevice, GX_INT_WIDTH, &m_nImageWidth);
	// 获取高度
	emStatus = GXGetInt(m_hDevice, GX_INT_HEIGHT, &m_nImageHeight);
	test.create(m_nImageHeight, m_nImageWidth, CV_8UC3);
	//判断相机是否支持bayer格式
	bool m_bColorFilter;
	emStatus = GXIsImplemented(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_bColorFilter);
	if (m_bColorFilter)
	{
		emStatus = GXGetEnum(m_hDevice, GX_ENUM_PIXEL_COLOR_FILTER, &m_nPixelColorFilter);
	}

	m_pBufferRGB = new BYTE[(size_t)(m_nImageWidth * m_nImageHeight * 3)];
	if (m_pBufferRGB == NULL)
	{
		return false;
	}

	//为存储原始图像数据申请空间
	m_pBufferRaw = new BYTE[(size_t)m_nPayLoadSize];
	if (m_pBufferRaw == NULL)
	{
		delete[]m_pBufferRGB;
		m_pBufferRGB = NULL;

		return false;
	}

	//注册图像处理回调函数
	emStatus = GXRegisterCaptureCallback(m_hDevice, NULL, OnFrameCallbackFun);
	//发送开采命令
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_START);
	//---------------------
	//
	//在这个区间图像会通过OnFrameCallbackFun接口返给用户
	Sleep(100000);
	//
	//---------------------
	//发送停采命令
	emStatus = GXSendCommand(m_hDevice, GX_COMMAND_ACQUISITION_STOP);
	//注销采集回调
	emStatus = GXUnregisterCaptureCallback(m_hDevice);

	if (m_pBufferRGB != NULL)
	{
		delete[]m_pBufferRGB;
		m_pBufferRGB = NULL;
	}
	if (m_pBufferRaw != NULL)
	{
		delete[]m_pBufferRaw;
		m_pBufferRaw = NULL;
	}
	emStatus = GXCloseDevice(m_hDevice);
	emStatus = GXCloseLib();
	return 0;
}



