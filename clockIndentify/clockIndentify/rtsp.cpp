#include "rtsp.h"

cap::Camera::Camera(int index,int readly) 
{
	this->index = index;
	this->readly = readly;
}
cap::Camera::~Camera()
{
	av_free(this->out_buffer);
	av_free(this->pFrameBGR);
	av_free(this->pAvFrame);
	avcodec_close(this->pCodecCtx);
	avformat_close_input(&(this->inputContext));

	sws_freeContext(this->img_convert_ctx);
}
void cap::Camera::PrintfContainerElapseTime(char *pszContainerName, char *pszOperator, long lElapsetime)
{
	printf("%s 的 %s操作 用时 %d毫秒\n", pszContainerName, pszOperator, lElapsetime);
}

/*打开视频流*/
int cap::Camera::OpenStreamFrom(string url)
{
	clock_t  clockBegin, clockEnd;
	clockBegin = clock();

	this->inputContext = avformat_alloc_context();
	AVDictionary* options = nullptr;
	av_dict_set(&options, "rtsp_transport", "tcp", 0);
	int ret = avformat_open_input(&(this->inputContext), url.c_str(), nullptr, &options);
	if (ret < 0)
	{
		//av_strerror();
		av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
		return  ret;
	}
	ret = avformat_find_stream_info(this->inputContext, nullptr);
	if (ret < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
	}
	else
	{
		this->videoindex = -1;
		for (int i = 0; i < this->inputContext->nb_streams; ++i) //遍历各个流，找到第一个视频流,并记录该流的编码信息  
		{
			if (this->inputContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
				this->videoindex = i;
				break;
			}
		}
		if (this->videoindex == -1) {
			printf("Don't find a video stream !\n");
			return -1;
		}
		this->pCodecCtx = this->inputContext->streams[this->videoindex]->codec; //得到一个指向视频流的上下文指针  
		this->pCodec = avcodec_find_decoder(this->pCodecCtx->codec_id); //到该格式的解码器
		this->frame_rate = 1.0*this->inputContext->streams[this->videoindex]->avg_frame_rate.den/ this->inputContext->streams[this->videoindex]->avg_frame_rate.num;
		if (this->pCodec == NULL) {
			printf("Cant't find the decoder !\n"); //寻找解码器  
			return -1;
		}
		if (avcodec_open2(this->pCodecCtx, this->pCodec, NULL) < 0) { //打开解码器  
			printf("Can't open the decoder !\n");
			return -1;
		}

		//av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n", url.c_str());
	}

	clockEnd = clock();

	PrintfContainerElapseTime("某数据", "进行某操作", clockEnd - clockBegin);
	return ret;
}

int cap::Camera::Init()
{
	this->pAvFrame = av_frame_alloc(); //分配帧存储空间  
	this->pFrameBGR = av_frame_alloc(); //存储解码后转换的RGB数据    
	this->size = av_image_get_buffer_size(AV_PIX_FMT_BGR24, this->pCodecCtx->width, this->pCodecCtx->height, 1);// 保存BGR，opencv中是按BGR来保存的
	this->out_buffer = (uint8_t *)av_malloc(this->size);
	av_image_fill_arrays(this->pFrameBGR->data, this->pFrameBGR->linesize, this->out_buffer, AV_PIX_FMT_BGR24, this->pCodecCtx->width, this->pCodecCtx->height, 1);
	img_convert_ctx = sws_getContext(this->pCodecCtx->width, this->pCodecCtx->height, this->pCodecCtx->pix_fmt, this->pCodecCtx->width, this->pCodecCtx->height, AV_PIX_FMT_BGR24, SWS_BICUBIC, NULL, NULL, NULL);

	return 0;
}

shared_ptr<AVPacket> cap::Camera::ReadPacketFromSource()
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

void cap::Camera ::operator >> (Mat &pCvMat)
{
	int fps = int(1.0/this->frame_rate);
	int ret;
	int got_picture;
	auto packet = this->ReadPacketFromSource();
	this->keyframe = 0;
	if (packet)
	{
		AVPacket *p = packet.get();
		if (p->stream_index == this->videoindex)
		{
			if (p->flags &AV_PKT_FLAG_KEY && !this->readly)
			{
				this->readly = 1;
			}
			ret = avcodec_decode_video2(this->pCodecCtx, this->pAvFrame, &got_picture, p);
			if (ret < 0)
			{
				printf("Decode Error.（解码错误）\n");
				return;
			}
			if (got_picture)
			{
				//YUV to RGB  
				sws_scale(this->img_convert_ctx, (const uint8_t* const*)this->pAvFrame->data, this->pAvFrame->linesize, 0, this->pCodecCtx->height, this->pFrameBGR->data, this->pFrameBGR->linesize);
				memcpy(pCvMat.data, this->out_buffer, this->size);
			}
			if (readly)
			{
				this->index++;
				this->index = this->index%fps;
			}
		}

	}
}
