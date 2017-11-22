// clockIndentify.cpp : 定义控制台应用程序的入口点。
//
#include "clock.hpp"
#include "rtsp.h"
#include "live.h"
#include "pch.h"
#include <string>
#include <memory>
#include <thread>
#include <crtdbg.h>  
#include <Windows.h>
using namespace myClock;
using namespace cap;
using namespace live;

pthread_mutex_t mutex;

void PrintfContainerElapseTime(char *pszContainerName, char *pszOperator, long lElapsetime)
{
	printf("%s 的 %s操作 用时 %d毫秒\n", pszContainerName, pszOperator, lElapsetime);
}

void Init()
{
	av_register_all();
	avfilter_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_ERROR);
}



void* Transpush(void* pusher)
{
	while (1)
	{
		auto packet = ((Pusher*)pusher)->ReadPacketFromSource();
		if (packet)
		{
			((Pusher*)pusher)->WritePacket(packet);
		}
	}
}

void* Indentify(void* cam)
{
	//clock_t  clockBegin, clockEnd;

	AlgoIdentifyMeter MeterIdentify;
	MeterIdentify.Init();
	Mat pCvMat;
	pCvMat.create(Size(((cap::Camera*)cam)->pCodecCtx->width, ((cap::Camera*)cam)->pCodecCtx->height), CV_8UC3);
	while (1)
	{
		*((Camera*)cam) >> pCvMat;
		if (((Camera*)cam)->index == 0 && ((Camera*)cam)->readly == 1)
		{
			double scale = 512.0 / pCvMat.cols;
			Size size(512, scale*pCvMat.rows);
			Mat mat(size, CV_32S);
			resize(pCvMat, mat, size);
			vector<Vec4i> dstLine;
			float degree = -1;
			MeterIdentify.run(mat, degree, dstLine);
			MeterIdentify.show(mat, degree, dstLine);
			imshow("Source", mat);
			waitKey(1);
		}
		else
		{
			Sleep(19);
			continue;
		}
		
		//Sleep(int(rate*1000));
		//timespec dealy;
		//dealy.tv_sec = 0;
		//dealy.tv_nsec = 1000;
		//pthread_delay_np(&dealy);
	}
	pCvMat.release();
}

int main(int argc, char** argv)
{	

	_CrtSetBreakAlloc(68);
	Init();

	pthread_mutex_init(&mutex, NULL);



	Camera cam;
	cam.OpenStreamFrom("rtsp://admin:admin12345@192.168.10.80/ISAPI/streaming/channels/102");
	cam.Init();
	pthread_t indentifyID;
	pthread_create(&indentifyID,NULL, Indentify,&cam);



	Pusher rtmpPusher, hlsPusher;

	if (!rtmpPusher.Init("rtsp://admin:admin12345@192.168.10.80/ISAPI/streaming/channels/101", "rtmp://192.168.10.106:1935/live/demo"))goto Error;
	pthread_t rtmpPusherID;
	pthread_create(&rtmpPusherID, NULL, Transpush, &rtmpPusher);

	if(!hlsPusher.Init("rtsp://admin:admin12345@192.168.10.80/ISAPI/streaming/channels/101", "rtmp://192.168.10.106:1935/hls/demo2"))goto Error;
	pthread_t hlsPusherID;
	pthread_create(&hlsPusherID, NULL, Transpush, &hlsPusher);



	_CrtDumpMemoryLeaks();
Error:
	while (true)
	{
		this_thread::sleep_for(chrono::seconds(100));
	}
	return 0;
}

