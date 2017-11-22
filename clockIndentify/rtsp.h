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
	private:
		char url[256];
		AVFormatContext *inputContext = nullptr;//������Ƶ������Ϣ
	public:
		AVCodec *pCodec; //������ָ��  
		AVCodecContext* pCodecCtx; //ffmpeg����������Ա  
		AVFrame* pAvFrame;
		AVFrame* pFrameBGR;
		struct SwsContext *img_convert_ctx;
		int videoindex = -1;
		uint8_t *out_buffer;
		double frame_rate;
		int index;
		int readly;
		int size;
	};
}
