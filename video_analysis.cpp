#include "video_analysis.h"
#include <ctime>
#include <QtGui>
#include <QtCore>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <iostream>
#include <cmath>

using std::cout;
using std::endl;
using namespace cv;

VideoAnalysis::VideoAnalysis(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	status_label = new QLabel();

	// ��ʼ����Ա����
	timer = new QTimer(this);	// ��ʼ����ʱ��
	is_detecting = false;		// ��ʼ������־λ
	timer->setInterval(1000/FPS);	// ���ü�ʱ�����

	init_signal();	// ��ʼ��Qt�źŲ�
	init_ui();		// ��ʼ������ؼ�
}

void VideoAnalysis::init_ui() {
	// �� Action ��ӵ��˵�������ʼ��״̬��
	ui.tool_bar->addAction(ui.open_video_action);
	ui.tool_bar->addSeparator();
	ui.tool_bar->addAction(ui.detect_action);
	ui.status_bar->addWidget(status_label);
}

void VideoAnalysis::destroy_ui() {
	delete status_label;
}

void VideoAnalysis::init_signal() {
	connect(ui.open_video_action, SIGNAL(triggered()), this, SLOT(openVideo()));	// ���˵������ź�����ļ�������������
	connect(timer, SIGNAL(timeout()), this, SLOT(display()));	// ��ʱ���ź��뺯����������	
	connect(ui.detect_action, SIGNAL(triggered()), this, SLOT(start_detect()));		// ���˵��������⺯����������
}

VideoAnalysis::~VideoAnalysis()
{
	delete timer;
	timer = NULL;
	destroy_ui();
}

void VideoAnalysis::openVideo() {
	QString fileName = QFileDialog::getOpenFileName(this, "����Ƶ", ".", "Video (*.avi)");	// ����ѡ���ļ��Ի���
	capture.open(fileName.toStdString());	// ����Ƶ�ļ�
	assert(capture.isOpened());
	timer->start();		// ��ʱ������
}

void VideoAnalysis::display() {
	capture >> frame;	// ����Ƶ�ļ���֡���� frame ��Ա����
	if (frame.empty()) {	// ����ļ�������ֹͣ
		timer->stop();
		return;
	}
	Mat vis = frame;
	if (is_detecting) {		// �����ر�־λ��������ִ�м�غ���
		vis = detect();
	}
	
	// ��֡��ʾ��������
	QImage image = QImage((const unsigned char *)(vis.data), vis.cols, vis.rows, QImage::Format_RGB888).rgbSwapped();
	ui.screen->setPixmap(QPixmap::fromImage(image));

}

void VideoAnalysis::start_detect() {
	
	if (ui.detect_action->isChecked()) {
		// �����ⰴť�󣬿�ʼ��⣬���ʼ����ʷ֡���˶���¼����
		prev_frame = frame.clone();
		motion_history = Mat::zeros(frame.rows, frame.cols, CV_32F);
		is_detecting = true;
	} else {
		// ֹͣ���
		is_detecting = false;
	}
	
}


Mat VideoAnalysis::detect() {
	const double MHI_DURATION = 0.5;
	const double MAX_TIME_DELTA = 0.25;
	const double MIN_TIME_DELTA = 0.05;

	Mat frame_diff, gray_diff, motion_mask;
	absdiff(frame, prev_frame, frame_diff);		// ����֮֡���
	cvtColor(frame_diff, gray_diff, COLOR_BGR2GRAY);	// �ҶȻ�
	double thrs = 32;									// ������ֵ
	threshold(gray_diff, motion_mask, thrs, 1, THRESH_BINARY);	// ��ֵ��
	double timestamp = float(clock()) / 1000;				// �õ���ǰʱ���
	updateMotionHistory(motion_mask, motion_history, timestamp, MHI_DURATION);	// �����˶���ʷ����
	Mat mg_mask, mg_orient;
	calcMotionGradient(motion_history, mg_mask, mg_orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 5);	// �����˶���ʷͼ����ݶȷ���
	Mat seg_mask;
	std::vector<Rect> seg_bounds;
	segmentMotion(motion_history, seg_mask, seg_bounds, timestamp, MAX_TIME_DELTA);		// �������˶��ָ�Ϊ�������˶�����
	status_label->setText(QString::number(timestamp));
	Mat vis;
	switch(ui.output_dial->value()) {
	case 0:
		vis = frame.clone();		// ������ʾ
		break;
	case 1:
		vis = frame_diff.clone();	// ֡������ʾ
		break;
//  	case 2:
//  		threshold((motion_history - (timestamp - MHI_DURATION)) / MHI_DURATION, vis, 0, 255, THRESH_BINARY);	// ��ֵ��
//  		cvtColor(vis, vis, COLOR_GRAY2BGR);
//  		break;

	default:
		vis = frame.clone();
		break;
	}
	int height = frame.rows;
	int width = frame.cols;
	vector<Rect>::iterator it = seg_bounds.begin();
	seg_bounds.insert(it, Rect(0, 0, width, height));
	int i;
	for (i = 0, it = seg_bounds.begin(); it != seg_bounds.end(); it++, i++) {
		int x = it->x;
		int y = it->y;
		int rw = it->width;
		int rh = it->height;
		int area = rw * rh;
		if (area < 64*64) {
			continue;
		}

		
		Mat silh_roi = motion_mask(Range(y, y+rh), Range(x, x+rw));		// ��ȡͼ��ROI
		Mat orient_roi = mg_orient(Range(y, y+rh), Range(x, x+rw));		// ��ȡͼ��ROI
		Mat mask_roi = mg_mask(Range(y, y+rh), Range(x, x+rw));			// ��ȡͼ��ROI
		Mat mhi_roi = motion_history(Range(y, y+rh), Range(x, x+rw));	// ��ȡͼ��ROI
		if (norm(silh_roi, NORM_L1) < area*0.05) {
			continue;
		}
		double angle = calcGlobalOrientation(orient_roi, mask_roi, mhi_roi, timestamp, MHI_DURATION);	// ����ÿ���˶��������˶�����
		Scalar color(255, 0, 0);	// BGR��Ĭ��Ϊ��ɫ
		if (i == 0) {	// ����˶���Χ������ֵ������Ϊ��ɫ
			color[0] = 0;
			color[2] = 255;
		}
		draw_motion_comp(vis, *it, angle, color);	// �ڸ�֡�ϻ�������
	}
	prev_frame = frame.clone();		// ������ʷ֡
	return vis;
}

void VideoAnalysis::draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color) {
	// ����Ŀ�����򣬺��˶�����
	rectangle(vis, rect, Scalar(0, 255, 0), 3);
	int r = min(rect.width/2, rect.height/2);
	Point center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    angle = angle*M_PI/180;
	circle(vis, center, r, color, 3);
	line(vis, center, Point(int(center.x+cos(angle)*r), int(center.y+sin(angle)*r)), color, 3);

}