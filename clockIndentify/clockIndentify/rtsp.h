#pragma once
#include "pch.h"
#include "opencv/cv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <memory>
#include <queue>
#include <string>
using namespace std;
using namespace cv;

namespace cap
{
	class Camera
	{
	public:
		Camera(int index=0,int readly=0);
		int OpenStreamFrom(string url);
		void operator>>(Mat &pCvMat);
		int Init();
		char* GetUrl() { return this->url; }
		~Camera();
	private:
		shared_ptr<AVPacket> ReadPacketFromSource();
		void PrintfContainerElapseTime(char *pszContainerName, char *pszOperator, long lElapsetime);
	private:
		char url[256];
		AVFormatContext *inputContext = nullptr;//保存视频流的信息
	public:
		AVCodec *pCodec; //解码器指针  
		AVCodecContext* pCodecCtx; //ffmpeg解码类的类成员  
		AVFrame* pAvFrame;
		AVFrame* pFrameBGR;
		struct SwsContext *img_convert_ctx;
		int videoindex = -1;
		uint8_t *out_buffer;
		double frame_rate;
		int keyframe;
		int index;
		int readly;
		int size;
	};
}