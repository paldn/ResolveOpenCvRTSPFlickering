#include "live.h"


live::Pusher::Pusher(){}

live::Pusher::~Pusher()
{
	avformat_close_input(&(this->inputContext));
	avformat_close_input(&(this->outputContext));
}

int live::Pusher::Init(string url, string url2)
{
	if (this->OpenInput(url)<0)return 0;
	if (this->OpenOutput(url2)<0)return 0;
	return 1;
}

int live::Pusher::OpenInput(string inputUrl)
{
	this->inputContext = avformat_alloc_context();
	AVDictionary* options = nullptr;
	av_dict_set(&options, "rtsp_transport", "udp", 0);
	int ret = avformat_open_input(&inputContext, inputUrl.c_str(), nullptr, &options);
	if (ret < 0)
	{
		return  ret;
	}
	ret = avformat_find_stream_info(this->inputContext, nullptr);
	for (int i = 0; i < this->inputContext->nb_streams; ++i) //遍历各个流，找到第一个视频流,并记录该流的编码信息  
	{
		if (this->inputContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			this->videoindex = i;
			break;
		}
	}
	return ret;
}

shared_ptr<AVPacket> live::Pusher::ReadPacketFromSource()
{
	shared_ptr<AVPacket> packet(static_cast<AVPacket*>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) { av_packet_free(&p); av_freep(&p); });
	av_init_packet(packet.get());
	int ret = av_read_frame(this->inputContext, packet.get());
	if (ret >= 0)
	{
		return packet;
	}
	else
	{
		return nullptr;
	}
}
void live::Pusher::av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb)
{
	if (pkt->pts != AV_NOPTS_VALUE)
		pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
	if (pkt->dts != AV_NOPTS_VALUE)
		pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
	if (pkt->duration > 0)
		pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}
int live::Pusher::WritePacket(shared_ptr<AVPacket> packet)
{
	auto inputStream = this->inputContext->streams[packet->stream_index];
	auto outputStream = this->outputContext->streams[packet->stream_index];
	av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
	return av_interleaved_write_frame(this->outputContext, packet.get());
}

int live::Pusher::OpenOutput(string outUrl)
{
	int ret = avformat_alloc_output_context2(&(this->outputContext), nullptr, "flv", outUrl.c_str());
	if (ret < 0)
	{
		goto Error;
	}
	//avformat_new_stream();
	ret = avio_open2(&(this->outputContext->pb), outUrl.c_str(), AVIO_FLAG_READ_WRITE, nullptr, nullptr);
	if (ret < 0)
	{
		goto Error;
	}
	for (int i = 0; i < inputContext->nb_streams; i++)
	{
		AVStream * stream = avformat_new_stream(this->outputContext, this->inputContext->streams[i]->codec->codec);
		ret = avcodec_copy_context(stream->codec, this->inputContext->streams[i]->codec);
		if (ret < 0)
		{
			goto Error;
		}
	}
	this->outputContext->streams[videoindex]->codec->width = this->inputContext->streams[videoindex]->codec->width;
	this->outputContext->streams[videoindex]->codec->height = this->inputContext->streams[videoindex]->codec->height;
	ret = avformat_write_header(this->outputContext, nullptr);
	if (ret < 0)
	{
		goto Error;
	}

	return ret;
Error:
	if (this->outputContext)
	{
		for (int i = 0; i < this->outputContext->nb_streams; i++)
		{
			avcodec_close(this->outputContext->streams[i]->codec);
		}
		avformat_close_input(&(this->outputContext));
	}
	return ret;
}