#include "video_analysis.h"
#include <ctime>
#include <QtGui>
#include <QtCore>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/tracking.hpp>
#include <iostream>

using std::cout;
using std::endl;
using namespace cv;

VideoAnalysis::VideoAnalysis(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	// 初始化成员变量
	timer = new QTimer(this);	// 初始化计时器
	is_detecting = false;
	timer->setInterval(1000/FPS);	// 设置计时器间隔

	init_signal();
	init_ui();
}

void VideoAnalysis::init_ui() {
	ui.tool_bar->addAction(ui.open_video_action);
	ui.tool_bar->addSeparator();
	ui.tool_bar->addAction(ui.detect_action);
}

void VideoAnalysis::init_signal() {
	connect(ui.open_video_action, SIGNAL(triggered()), this, SLOT(openVideo()));	// 将菜单动作信号与函数槽相连接
	connect(timer, SIGNAL(timeout()), this, SLOT(display()));	// 将时钟信号与函数槽相连接	
	connect(ui.detect_action, SIGNAL(triggered()), this, SLOT(start_detect()));
}

VideoAnalysis::~VideoAnalysis()
{
	delete timer;
	timer = NULL;
}

void VideoAnalysis::openVideo() {
	QString fileName = QFileDialog::getOpenFileName(this, "打开视频", ".", "Video (*.avi)");
	capture.open(fileName.toStdString());	// 打开视频文件
	assert(capture.isOpened());
	timer->start();		// 计时器开启
}

void VideoAnalysis::display() {
	capture >> frame;
	if (frame.empty()) {
		timer->stop();
		return;
	}

	if (is_detecting) {
		frame = detect();
	}
	QImage image = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
	ui.screen->setPixmap(QPixmap::fromImage(image));

}



void VideoAnalysis::start_detect() {
	if (ui.detect_action->isChecked()) {
		prev_frame = frame.clone();
		motion_history = Mat::zeros(frame.rows, frame.cols, CV_32F);
		is_detecting = true;
	} else {
		is_detecting = false;
	}
	
}


Mat VideoAnalysis::detect() {
	const double MHI_DURATION = 0.5;
	const double MAX_TIME_DELTA = 0.25;
	const double MIN_TIME_DELTA = 0.05;

	Mat frame_diff, gray_diff, motion_mask;
	absdiff(frame, prev_frame, frame_diff);		// 求两帧之差差
	cvtColor(frame_diff, gray_diff, COLOR_BGR2GRAY);	// 灰度化
	double thrs = 32;
	threshold(gray_diff, motion_mask, thrs, 1, THRESH_BINARY);	// 二值化
	double timestamp = clock() / 1000;

	updateMotionHistory(motion_mask, motion_history, timestamp, MHI_DURATION);
	Mat mg_mask, mg_orient;
	calcMotionGradient(motion_history, mg_mask, mg_orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 5);
	Mat seg_mask;
	std::vector<Rect> seg_bounds;
	segmentMotion(motion_history, seg_mask, seg_bounds, timestamp, MAX_TIME_DELTA);

	Mat vis;
	vis = frame.clone();
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

		Mat silh_roi = motion_mask(Range(y, y+rh), Range(x, x+rw));
		Mat orient_roi = mg_orient(Range(y, y+rh), Range(x, x+rw));
		Mat mask_roi = mg_mask(Range(y, y+rh), Range(x, x+rw));
		Mat mhi_roi = motion_history(Range(y, y+rh), Range(x, x+rw));
		if (norm(silh_roi, NORM_L1) < area*0.05) {
			continue;
		}
		double angle = calcGlobalOrientation(orient_roi, mask_roi, mhi_roi, timestamp, MHI_DURATION);
		Scalar color(0, 0, 255);	// BGR
		if (i == 0) {
			color[0] = 255;
			color[2] = 0;
		}
		draw_motion_comp(vis, *it, angle, color);
	}
	prev_frame = frame.clone();
	return vis;
}

void VideoAnalysis::draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color) {
	rectangle(vis, rect, color, 3);
	int r = min(rect.width/2, rect.height/2);
    int cx = rect.x + rect.width / 2;
	int cy = rect.y + rect.height / 2;
    // double angle = angle*3.14/180;
	circle(vis, Point(cx, cy), r, color, 3);
}