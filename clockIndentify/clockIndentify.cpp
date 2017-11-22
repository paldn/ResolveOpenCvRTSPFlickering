// clockIndentify.cpp : 定义控制台应用程序的入口点。
//
#include "clock.hpp"
#include "rtsp.h"
#include "live.h"
#include "pch.h"
#include <unistd.h>
#include <string>
#include <memory>
#include <thread>
#include <time.h>
#include <MQTTClient.h>

using namespace myClock;
using namespace cap;
using namespace live;


#define ADDRESS		"tcp://192.168.100.43:1883"
#define ALARM_TOPIC	"cloud_alarm_topic"
#define EVENT_TOPIC	"cloud_event_topic"
#define TEST_TOPIC	"TEST_TOPIC"
#define QOS		1
#define TIMEOUT		10000L
	
string clientId;

MQTTClient client;
MQTTClient_message pubmsg;
MQTTClient_deliveryToken token;
//pthread_mutex_t mutex;

char mainresource[128];
char subresource[128];
char rtmpurl[128];
char hlsurl[128];
char deviceName[128];
char monitorName[128];
char unit[128];
char expandData[4096];
char thresholds[128];
char checkInterval[128];
int cls;


/*MQTT pulisher*/
volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = (char*)message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
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
	AlgoIdentifyMeter MeterIdentify(deviceName,monitorName,unit,expandData,thresholds,checkInterval);
	MeterIdentify.Init();
	Mat pCvMat;
	pCvMat.create(Size(((cap::Camera*)cam)->pCodecCtx->width, ((cap::Camera*)cam)->pCodecCtx->height), CV_8UC3);
	while (1)
	{
		*((Camera*)cam) >> pCvMat;
		if (((Camera*)cam)->index == 0 && ((Camera*)cam)->readly == 1)
		{
			double scale = 480.0 / pCvMat.cols;
			Size size(480, scale*pCvMat.rows);
			Mat mat(size, CV_32S);
			resize(pCvMat, mat, size);

			vector<Vec4i> dstLine;
			float degree = -1;
			MeterIdentify.run(mat, degree, dstLine);
			MeterIdentify.show(mat, degree, dstLine);
			imshow("Source", mat);
                	waitKey(1);
			
			if(MeterIdentify.count)
			{
				continue;
			}			


			string data;
			MeterIdentify.format(data);
			pubmsg.payload = (char*)data.c_str();
        		pubmsg.payloadlen = (int)strlen(data.c_str());
        		pubmsg.qos = QOS;
        		pubmsg.retained = 0;
        		deliveredtoken = 0;
        		MQTTClient_publishMessage(client, TEST_TOPIC, &pubmsg, &token);
		}
		else
		{
			timespec dealy;
			dealy.tv_sec = 0;
			dealy.tv_nsec = 38000000;
			//nanosleep(&dealy);
			usleep(19);
			continue;
		}
		//imshow("Source", mat);
		//waitKey(1);
		//sleep(1);
		//timespec dealy;
		//dealy.tv_sec = 0;
		//dealy.tv_nsec = 1000;
		//pthread_delay_np(&dealy);
	}
	pCvMat.release();
}

int main(int argc, char** argv)
{
	
	if(argc<23)
	{
		printf("invalid parameter!");
		return 0;
	}
	
	for(int i=1;i<argc;i+=2)
	{
		if(!strcmp(argv[i],"-m"))
		{
			strcpy(mainresource,argv[i+1]);
		}
		if(!strcmp(argv[i],"-s"))
                {
                        strcpy(subresource,argv[i+1]);
                }
		if(!strcmp(argv[i],"-r"))
                {
                        strcpy(rtmpurl,argv[i+1]);
                }
		if(!strcmp(argv[i],"-h"))
                {
                        strcpy(hlsurl,argv[i+1]);
                }
		if(!strcmp(argv[i],"-c"))
                {
                    	cls = atoi(argv[i+1]);
                }
		if(!strcmp(argv[i],"-dn"))
                {
                        strcpy(deviceName,argv[i+1]);
                }
		if(!strcmp(argv[i],"-mn"))
                {
                        strcpy(monitorName,argv[i+1]);
                }
		if(!strcmp(argv[i],"-u"))
                {
                        strcpy(unit,argv[i+1]);
                }
		if(!strcmp(argv[i],"-ed"))
                {
                        strcpy(expandData,argv[i+1]);
                }
		if(!strcmp(argv[i],"-th"))
                {
                        strcpy(thresholds,argv[i+1]);
                }
		if(!strcmp(argv[i],"-ci"))
                {
                        strcpy(checkInterval,argv[i+1]);
                }
	}
	/*initzing mqtt client*/
	
	clientId = deviceName;

	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
        pubmsg = MQTTClient_message_initializer;
        int rc;

        MQTTClient_create(&client, ADDRESS, clientId.c_str(),MQTTCLIENT_PERSISTENCE_NONE, NULL);
        conn_opts.keepAliveInterval = 20;
        conn_opts.cleansession = 1;

        MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
	
        if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
        {
            printf("Failed to connect, return code %d\n", rc);
	    exit(EXIT_FAILURE);
        }

	Init();

	//pthread_mutex_init(&mutex, NULL);

	Camera cam;
	cam.OpenStreamFrom(subresource);
	cam.Init();
	pthread_t indentifyID;
	if(cls == 1)
	{
		pthread_create(&indentifyID,NULL, Indentify,&cam);
	}


	Pusher rtmpPusher, hlsPusher;

	if (!rtmpPusher.Init(mainresource, rtmpurl))goto Error;
	pthread_t rtmpPusherID;
	pthread_create(&rtmpPusherID, NULL, Transpush, &rtmpPusher);

	if(!hlsPusher.Init(mainresource, hlsurl))goto Error;
	pthread_t hlsPusherID;
	pthread_create(&hlsPusherID, NULL, Transpush, &hlsPusher);


Error:
	while (true)
	{
		this_thread::sleep_for(chrono::seconds(100));
	}
	
	MQTTClient_disconnect(client, 10000);
    	MQTTClient_destroy(&client);
	return 0;
}

