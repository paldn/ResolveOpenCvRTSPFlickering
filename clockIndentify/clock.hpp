#pragma once
#include "opencv/cv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/legacy/compat.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <iostream>
#include <list>
#include <json/json.h>
using namespace cv;
using namespace std;

#define LEN 3//ͼƬ����ĸ���
#define PI acos(-1.0);
//#define numPt 13//��Ŀ̶ȸ���
//#define angelgap 10//�̶ȵĵȲ�ֵ
//#define anglefr 1

#define fr 0.4

namespace myClock
{

	//��Բ����Ϣ�ṹ�壬������������(x,y)��������a,b
	typedef struct _Eclipse
	{
		int x, y, a, b;
		float angle;
	} eclipse;
	class Flipse
	{
	private:
		int threshold;//��ֵ����ֵ
		IplImage* image[LEN];
		eclipse* ec;
	public:
		friend class AlgoIdentifyMeter;
		eclipse* getEc();
		void process_image(int h = 0);
		Flipse() {}
		Flipse(IplImage* img, int threshold = 40);
		Flipse(const Mat &mat, int threshold = 40);
		Flipse(const char* filename, int threshold = 40);
		~Flipse();
	};


	typedef struct object
	{
		Point med;
		Rect srcRect;
	}Obj;
	class AlgoIdentifyMeter
	{
	public:
		AlgoIdentifyMeter();
		AlgoIdentifyMeter(char* deviceName,char* monitorName,char* unit,char* expandData,char* thresholds,char* checkInterval);
		~AlgoIdentifyMeter();

	public:

		void Init();
		void run(Mat src, float &degree, vector<Vec4i> &dstLine);
		float show(Mat src, float degree, vector<Vec4i> dstLine);
		void format(string &data);
	private:

		float GetK(Point p0, Point p1);
		void contourExtraction(Mat src, vector<Vec4i> &lines);
		void getMaxQ(float &Qmax0, float &Qmax1);
		void getCircle();
		void getDegree(float Qline, float &degree);
		void preDefinedAngle();
		float getAngle(float k1, float k2);
		void getLine(Mat src, vector<Vec4i>srcLine, vector<Vec4i> &dstLine, float &Qline);
		void getLikelyPointer(vector<float> &lineK, vector<float> &lineB, vector<Vec4i> &dstLine);
		void CalcQLine(Point start, Point end, float &Qline);
		void filterLine(Mat src, vector<Vec4i>srcLine, vector<Vec4i> &dstLine, vector<float> &lineK, vector<float> &lineB);

		/*2017-08-23*/
		void Calibration(const Mat &mat);
	private:
		Flipse* flipse;
		int m_frameCount;/*����֡��*/
		vector<Point>m_cirPoint;/*Բ����������͸��̶ȵ�����*/
		vector<float>m_angle;/*Բ���̶ȵĽǶ�*/
		Point m_center;/*Բ������*/
		float m_radius;/*Բ�İ뾶*/
		float m_kstart;/*��ʼֱ��б��*/
		float m_bstart;/*��ʼֱ�߽ؾ�*/
		float m_Qmax0;/*��ʼ�Ƕ�*/
		float m_Qmax1;/*�յ�Ƕ�*/
		float predegree;/*��һ��ָ��Ŀ̶�*/
		vector<int> pDgreee;
		int numPt;
		vector<float> ths;
		int angelgap;
		float anglefr;		
				
		int currentState;
		char currentTime[64];
		int lastState;
		int lastHardState;
		char perfData[128];
		char pluginOutput[128];
		long timestamp;
		
		char deviceName[128];
		char monitorName[128];
		char unit[128];
		char expandData[4096];
		char thresholds[128];
		char checkInterval[128];
	public:
		int interval;
		int count;
	};
}
