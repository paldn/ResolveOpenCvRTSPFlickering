#pragma once
#include "pch.h"
#include <iostream>
#include <memory>
#include <string>
using namespace std;

namespace live
{
	class Pusher
	{
	public:
		Pusher();
		~Pusher();
		int Init(string url,string url2);
		int WritePacket(shared_ptr<AVPacket> packet);
		shared_ptr<AVPacket> ReadPacketFromSource();
	private:
		int OpenInput(string url);
		int OpenOutput(string url);
		void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb);
	private:
		char url[256];
		AVFormatContext *inputContext = nullptr;
		AVFormatContext *outputContext = nullptr;
	};
}
