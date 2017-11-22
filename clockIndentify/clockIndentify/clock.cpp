#include "clock.hpp"


//指针度数
static int pDgreee[numPt] = { 60,50,40,30,20,10,0,-10,-20,-30 };

myClock::AlgoIdentifyMeter::AlgoIdentifyMeter(){}

myClock::AlgoIdentifyMeter::~AlgoIdentifyMeter(){}

void myClock::AlgoIdentifyMeter::Init()
{
	predegree = -1;
	m_frameCount = 0;
	m_cirPoint.resize(numPt + 1);

#if 0

	m_cirPoint[0].x = 204;   m_cirPoint[0].y = 94;
	m_cirPoint[1].x = 243;   m_cirPoint[1].y = 139;
	m_cirPoint[2].x = 256;   m_cirPoint[2].y = 115;
	m_cirPoint[3].x = 259;   m_cirPoint[3].y = 87;
	m_cirPoint[4].x = 250;   m_cirPoint[4].y = 62;
	m_cirPoint[5].x = 229;   m_cirPoint[5].y = 43;
	m_cirPoint[6].x = 205;   m_cirPoint[6].y = 36;
	m_cirPoint[7].x = 179;   m_cirPoint[7].y = 42;
	m_cirPoint[8].x = 160;   m_cirPoint[8].y = 60;
	m_cirPoint[9].x = 149;   m_cirPoint[9].y = 84;
	m_cirPoint[10].x = 151;  m_cirPoint[10].y = 113;
	m_cirPoint[11].x = 164;  m_cirPoint[11].y = 138;

#else

	
	m_cirPoint[0].x = 273;   m_cirPoint[0].y = 196;
	m_cirPoint[1].x = 375;   m_cirPoint[1].y = 245;
	m_cirPoint[2].x = 387;   m_cirPoint[2].y = 194;
	m_cirPoint[3].x = 375;   m_cirPoint[3].y = 148;
	m_cirPoint[4].x = 341;   m_cirPoint[4].y = 110;
	m_cirPoint[5].x = 296;   m_cirPoint[5].y = 92;
	m_cirPoint[6].x = 247;   m_cirPoint[6].y = 92;
	m_cirPoint[7].x = 202;   m_cirPoint[7].y = 114;
	m_cirPoint[8].x = 173;   m_cirPoint[8].y = 154;
	m_cirPoint[9].x = 163;   m_cirPoint[9].y = 202;
	m_cirPoint[10].x = 176;  m_cirPoint[10].y = 252;

#endif
	//getCircle();
	//preDefinedAngle();
}

void myClock::AlgoIdentifyMeter::preDefinedAngle()
{
	float k0 = GetK(m_cirPoint[0], m_cirPoint[1]);
	for (int i = 2; i <= numPt; i++)
	{
		float k = GetK(m_cirPoint[0], m_cirPoint[i]);
		float singleangle = getAngle(k0, k);
		CalcQLine(m_center, m_cirPoint[i], singleangle);
		m_angle.push_back(singleangle);
	}
}


void myClock::AlgoIdentifyMeter::run(Mat src, float &degree, vector<Vec4i> &dstLine)
{
	Calibration(src);
	getCircle();
	preDefinedAngle();

	m_frameCount++;
	vector<Vec4i> lines;
	contourExtraction(src, lines);

	//**************************2017-08-23****************************
	for (int i = 0; i < lines.size(); ++i)
	{
		Point p1,p2;
		p1.x = lines[i][0];
		p1.y = lines[i][1];
		p2.x = lines[i][2];
		p2.y = lines[i][3];

		//line(src, p1, p2, CV_RGB(0, 0, 255), 1, CV_AA, 0);
		//circle(src, p2, 5, CV_RGB(0, 0, 255), 1, CV_AA, 0);
	}

	if (m_frameCount > 2)
	{
		float Qline = -1;
		getLine(src, lines, dstLine, Qline);
		getDegree(Qline, degree);
	}
}


void myClock::AlgoIdentifyMeter::getLine(Mat src, vector<Vec4i>srcLine, vector<Vec4i> &dstLine, float &Qline)
{
	vector<float> lineK, lineB;
	filterLine(src, srcLine, dstLine, lineK, lineB);

	//for (int i = 0; i < dstLine.size(); i++)
	//{
	//line(src, Point(dstLine[i][0], dstLine[i][1]), Point(dstLine[i][2], dstLine[i][3]), Scalar(0, 255, 0));
	//}

	getLikelyPointer(lineK, lineB, dstLine);
	/*求出夹角*/
	if (dstLine.size() > 0)
	{
		line(src, m_center, m_cirPoint[1], Scalar(0, 255, 0));
		Point start(dstLine[0][0], dstLine[0][1]);
		Point end(dstLine[0][2], dstLine[0][3]);
		Qline = getAngle(m_kstart, lineK[0]);
		CalcQLine(start, end, Qline);
		if (Qline > m_angle[numPt - 2])
		{
			Qline = m_angle[numPt - 2];
		}
	}
}

void myClock::AlgoIdentifyMeter::getLikelyPointer(vector<float> &lineK, vector<float> &lineB, vector<Vec4i> &dstLine)
{
	int nline = lineK.size();
	vector<float>flag(nline, -1);
	vector<int> num(nline, 0);
	int likelyflag = 0;
	for (int i = 0; i < nline; i++)
	{
		if (flag[i] == -1)
		{
			float limitK = abs(lineK[i] * fr);
			for (int j = 0; j < nline; j++)
			{
				if ((abs(lineK[i] - lineK[j]) < limitK) && (i != j))
				{
					flag[i] = flag[j] = i;
					num[i]++;
					likelyflag = 1;
				}
			}
		}
	}

	vector<Vec4i> tempdstLine;
	vector<float> templineK, templineB;
	if (likelyflag == 1)
	{
		int num = 0;
		int maxid = 0;
		int disid = pow((dstLine[0][0] - dstLine[0][2]), 2) + pow((dstLine[0][1] - dstLine[0][3]), 2);
		for (int j = 1; j < nline; j++)
		{
			if (flag[j] != -1)
			{
				Point start(dstLine[j][0], dstLine[j][1]);
				Point end(dstLine[j][2], dstLine[j][3]);
				int dis = (start.x - end.x)*(start.x - end.x) + (start.y - end.y)*(start.y - end.y);
				if (dis - disid >= 0)
				{
					disid = dis;
					maxid = j;
				}
			}
		}
		tempdstLine.push_back(dstLine[maxid]);
		templineK.push_back(lineK[maxid]);
		templineB.push_back(lineB[maxid]);
		dstLine = tempdstLine;
		lineK = templineK;
		lineB = templineB;
	}
}

void myClock::AlgoIdentifyMeter::CalcQLine(Point start, Point end, float &Qline)
{
	float disend = (end.x - m_center.x) * (end.x - m_center.x) + (end.y - m_center.y) * (end.y - m_center.y);
	float disstart = (start.x - m_center.x) * (start.x - m_center.x) + (start.y - m_center.y) * (start.y - m_center.y);
	Point tempend = disend > disstart ? end : start;
	float dt = tempend.y - m_kstart * tempend.x - m_bstart;
	if (dt <= 0)
	{
		if (Qline > 0)
		{
			Qline = 180 - Qline;
		}
		else
		{
			Qline = abs(Qline);
		}
	}
	else
	{
		if (Qline > 0)
		{
			Qline = 360 - Qline;
		}
		else
		{
			Qline = 180 + abs(Qline);
		}
	}
}



float myClock::AlgoIdentifyMeter::show(Mat src, float degree, vector<Vec4i> dstLine)
{
	for (int i = 0; i < dstLine.size(); i++)
	{
		Vec4i I = dstLine[i];
		line(src, Point(I[0], I[1]), Point(I[2], I[3]), Scalar(0, 0, 255), 1);
	}
	line(src, m_center, m_cirPoint[1], Scalar(0, 0, 255), 1);
	circle(src, m_center, 3, Scalar(255, 0, 0), 3);
	circle(src, m_center, m_radius, Scalar(255, 255, 0), 1);
	char str_lx[20];
	sprintf(str_lx, "degree %0.1f", degree);
	cv::putText(src, str_lx, cv::Point(150, 150), CV_FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 0, 0), 2);
	return degree;
}


void myClock::AlgoIdentifyMeter::getDegree(float Qline, float &degree)
{
	for (int j = 0; j < numPt - 2; j++)
	{
		if ((Qline >= m_angle[j]) && (Qline <= m_angle[j + 1]))
		{
			float angleRange = (m_angle[j + 1] - m_angle[j]) / angelgap;
			float anglegap = ((m_angle[j + 1] - Qline) / angleRange);
			degree = pDgreee[j + 2] + anglegap * anglefr;
			break;
		}
	}

	if (degree == -1)
	{
		degree = predegree;
	}
	else
	{
		predegree = degree;
	}
}

void myClock::AlgoIdentifyMeter::filterLine(Mat src, vector<Vec4i>srcLine, vector<Vec4i> &dstLine, vector<float> &lineK, vector<float> &lineB)
{
	for (int i = 0; i < srcLine.size(); i++)
	{
		Point start(srcLine[i][0], srcLine[i][1]);
		Point end(srcLine[i][2], srcLine[i][3]);
		float k = GetK(end, start);
		float b = end.y - float(k)*float(end.x);
		float Qline = getAngle(m_kstart, k);
		CalcQLine(start, end, Qline);

		float dt = sqrt((k*k + 1)*(k*k + 1));
		float dis = float(abs(k * float(m_center.x) - m_center.y + b)) / float(dt);
		float dstart = sqrt((start.x - m_center.x) * (start.x - m_center.x) + (start.y - m_center.y) * (start.y - m_center.y));
		float dend = sqrt((end.x - m_center.x) * (end.x - m_center.x) + (end.y - m_center.y) * (end.y - m_center.y));
		float dpoint = dstart > dend ? dend : dstart;
		if (dis < 12 && dpoint < (0.8 * m_radius) && (Qline <  m_Qmax1))
		{
			dstLine.push_back(srcLine[i]);
			lineK.push_back(k);
			lineB.push_back(b);
		}
	}
}


void myClock::AlgoIdentifyMeter::contourExtraction(Mat src, vector<Vec4i> &lines)
{
	Mat temp;
	Canny(src, temp, 50, 255, 3);
	HoughLinesP(temp, lines, 1, CV_PI / 180, 80, 30, 10);
}


float myClock::AlgoIdentifyMeter::GetK(Point p0, Point p1)
{
	float k = (float)(p0.y - p1.y) / (float)(p0.x - p1.x);
	return k;
}

float myClock::AlgoIdentifyMeter::getAngle(float k1, float k2)
{
	float tt = atan(((k2 - k1) / (1 + (k1 * k2))));
	float Qline = tt * 180 / PI;
	return Qline;
}

void myClock::AlgoIdentifyMeter::getMaxQ(float &m_Qmax0, float &m_Qmax1)
{
	float mink = GetK(m_cirPoint[1], m_center);
	float maxk = GetK(m_cirPoint[numPt], m_center);
	float minQline = getAngle(m_kstart, mink);
	float maxQline = getAngle(m_kstart, maxk);

	CalcQLine(m_cirPoint[1], m_center, minQline);
	CalcQLine(m_center, m_cirPoint[numPt], maxQline);
	m_Qmax0 = minQline;
	m_Qmax1 = maxQline + 4;
}

void myClock::AlgoIdentifyMeter::getCircle()
{
	float disx = (m_cirPoint[0].x - m_cirPoint[1].x) * (m_cirPoint[0].x - m_cirPoint[1].x);
	float disy = (m_cirPoint[0].y - m_cirPoint[1].y) * (m_cirPoint[0].y - m_cirPoint[1].y);
	m_radius = sqrt(disx + disy);
	m_center.x = m_cirPoint[0].x;
	m_center.y = m_cirPoint[0].y;
	m_kstart = float(m_center.y - m_cirPoint[1].y) / float(m_center.x - m_cirPoint[1].x);
	m_bstart = m_center.y - m_kstart * m_center.x;
	getMaxQ(m_Qmax0, m_Qmax1);
}

void myClock::AlgoIdentifyMeter::Calibration(const Mat &mat)
{
	Flipse* flipse = new Flipse(mat);
	flipse->process_image();

	for (int i = 1; i < this->m_cirPoint.size(); ++i)
	{
		this->m_cirPoint[i].x += flipse->ec->x - this->m_cirPoint[0].x;
		this->m_cirPoint[i].y += flipse->ec->y - this->m_cirPoint[0].y;
	}
	this->m_cirPoint[0].x = flipse->ec->x;
	this->m_cirPoint[0].y = flipse->ec->y;

	/*
	cout << "圆心坐标为：(" << flipse->ec->x << "," << flipse->ec->y << ")，";
	cout << "长轴为：" << flipse->ec->a << "，";
	cout << "短轴为：" << flipse->ec->b << "，";
	cout << "倾斜角度为：" << flipse->ec->angle << "，";
	*/
	delete flipse;
}


//构造函数初始化对象
myClock::Flipse::Flipse(const char* filename, int threshold)
{
	this->image[0] = cvLoadImage(filename,0);
	this->image[1] = cvCloneImage(this->image[0]);
	this->image[2] = cvCloneImage(this->image[0]);
	this->threshold = threshold;
	this->ec = NULL;
}

myClock::Flipse::Flipse(IplImage* img, int threshold)
{
	this->image[0] = cvCloneImage(img);
	this->image[1] = cvCloneImage(this->image[0]);
	this->image[2] = cvCloneImage(this->image[0]);
	this->threshold = threshold;
	this->ec = NULL;
}

myClock::Flipse::Flipse(const Mat &mat, int threshold)
{
	Mat nmat;
	cvtColor(mat, nmat, CV_BGR2GRAY);
	IplImage temp = (IplImage)nmat;
	IplImage *qImg = &temp;
	this->image[0] = cvCloneImage(qImg);
	this->image[1] = cvCloneImage(this->image[0]);
	this->image[2] = cvCloneImage(this->image[0]);
	this->threshold = threshold;
	this->ec = NULL;
}

//析构函数释放资源
myClock::Flipse::~Flipse()
{
	cvReleaseImage(&this->image[0]);
	cvReleaseImage(&this->image[1]);
	cvReleaseImage(&this->image[2]);
	if (ec != NULL)
	{
		free(this->ec);
	}
}
//查找椭圆及椭圆中心坐标长轴与短轴
void myClock::Flipse::process_image(int h)
{
	this->ec = (eclipse*)malloc(sizeof(eclipse));

	//cvShowImage("Source", this->image[1]);

	CvMemStorage* stor;
	CvSeq* cont;
	CvBox2D32f* box;
	CvPoint* PointArray;
	CvPoint2D32f* PointArray2D32f;

	stor = cvCreateMemStorage(0);
	cont = cvCreateSeq(CV_SEQ_ELTYPE_POINT, sizeof(CvSeq), sizeof(CvPoint), stor);
	// 全局二值化图像
	//cvThreshold(this->image[1], this->image[0], this->threshold, 255, CV_THRESH_BINARY);
	// 局部二值化图像
	cvAdaptiveThreshold(this->image[1], this->image[0], 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 7, 3);
	//adaptiveThreshold(mat, nmat, 255, CV_ADAPTIVE_THRESH_MEAN_C, CV_THRESH_BINARY_INV, 25, 10);
	// 查找所有轮廓
	cvFindContours(this->image[0], stor, &cont, sizeof(CvContour), CV_RETR_LIST, CV_CHAIN_APPROX_NONE, cvPoint(0, 0));

	cvZero(this->image[0]);
	cvZero(this->image[2]);


	/*记录最大椭圆信息*/
	double max_area = 0;
	CvBox2D32f* tag_box;
	tag_box = (CvBox2D32f*)malloc(sizeof(CvBox2D32f));
	for (; cont; cont = cont->h_next) {

		int i;
		int count = cont->total;
		CvPoint center;
		CvSize size;

		if (count < 6)
			continue;

		// 为轮廓点分配内存
		PointArray = (CvPoint*)malloc(count * sizeof(CvPoint));
		PointArray2D32f = (CvPoint2D32f*)malloc(count * sizeof(CvPoint2D32f));

		// 为椭圆分配内存
		box = (CvBox2D32f*)malloc(sizeof(CvBox2D32f));

		// 获取轮廓点集合
		cvCvtSeqToArray(cont, PointArray, CV_WHOLE_SEQ);

		// 转换CvPoint点集合为CvBox2D32f集合.
		for (i = 0; i<count; i++) {

			PointArray2D32f[i].x = (float)PointArray[i].x;
			PointArray2D32f[i].y = (float)PointArray[i].y;
		}
		// 椭圆拟合当前轮廓
		cvFitEllipse(PointArray2D32f, count, box);

		// 绘制当前轮廓
		//cvDrawContours(image04, cont, CV_RGB(255, 255, 255),
		//CV_RGB(255, 255, 255), 0, 1, 8, cvPoint(0, 0));

		// 将椭圆数据从float转换为int类型
		center.x = cvRound(box->center.x);
		center.y = cvRound(box->center.y);
		size.width = cvRound(box->size.width*0.5);
		size.height = cvRound(box->size.height*0.5);
		box->angle = -box->angle;

		/*查找最大的椭圆*/
		double area = cvContourArea(cont, CV_WHOLE_SEQ, 0);
		if (area > max_area)
		{
			max_area = area;
			tag_box->center.x = box->center.x;
			tag_box->center.y = box->center.y;
			tag_box->size.width = box->size.width;
			tag_box->size.height = box->size.height;
			tag_box->angle = box->angle;
		}

		//cout << center.x << "," << center.y << endl;
		// 绘制椭圆
		//cvEllipse(this->image[2], center, size, box->angle, 0, 360, CV_RGB(0, 0, 255), 1, CV_AA, 0);
		//cvCircle(image04, center, 10, CV_RGB(0, 0, 255), 1, CV_AA, 0);
		// 释放内存
		free(PointArray);
		free(PointArray2D32f);
		free(box);
	}

	//this->ec->x = cvRound(tag_box->center.x + (tag_box->size.width - tag_box->size.height)*cos(tag_box->angle) / 2);
	//this->ec->y = cvRound(tag_box->center.y + (tag_box->size.width - tag_box->size.height)*sin(tag_box->angle) / 2);

	this->ec->x = cvRound(tag_box->center.x);
	this->ec->y = cvRound(tag_box->center.y);
	this->ec->a = cvRound(tag_box->size.width > tag_box->size.height ? tag_box->size.width : tag_box->size.height);
	this->ec->b = cvRound(tag_box->size.width < tag_box->size.height ? tag_box->size.width : tag_box->size.height);
	this->ec->angle = tag_box->angle;



	CvPoint point;
	point.x = cvRound(tag_box->center.x);
	point.y = cvRound(tag_box->center.y);

	CvSize size;
	size.width = cvRound(tag_box->size.width*0.5);
	size.height = cvRound(tag_box->size.height*0.5);

	cvEllipse(this->image[2], point, size, tag_box->angle, 0, 360, CV_RGB(0, 0, 255), 1, CV_AA, 0);
	cvCircle(this->image[2], point, 5, CV_RGB(0, 0, 255), 1, CV_AA, 0);
	cvCircle(this->image[2], point,this->ec->a/2, CV_RGB(0, 0, 255), 1, CV_AA, 0);
	
	free(tag_box);

	cvReleaseMemStorage(&stor);
	//cvShowImage("Result", this->image[2]);
	
}

myClock::eclipse* myClock::Flipse::getEc()
{
	return this->ec;
}