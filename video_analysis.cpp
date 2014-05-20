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

	// 初始化成员变量
	timer = new QTimer(this);	// 初始化计时器
	is_detecting = false;		// 初始化检测标志位
	timer->setInterval(1000/FPS);	// 设置计时器间隔

	init_signal();	// 初始化Qt信号槽
	init_ui();		// 初始化界面控件
}

void VideoAnalysis::init_ui() {
	// 将 Action 添加到菜单栏，初始化状态栏
	ui.tool_bar->addAction(ui.open_video_action);
	ui.tool_bar->addSeparator();
	ui.tool_bar->addAction(ui.detect_action);
	ui.status_bar->addWidget(status_label);
}

void VideoAnalysis::destroy_ui() {
	delete status_label;
}

void VideoAnalysis::init_signal() {
	connect(ui.open_video_action, SIGNAL(triggered()), this, SLOT(openVideo()));	// 将菜单动作信号与打开文件函数槽相连接
	connect(timer, SIGNAL(timeout()), this, SLOT(display()));	// 将时钟信号与函数槽相连接	
	connect(ui.detect_action, SIGNAL(triggered()), this, SLOT(start_detect()));		// 将菜单动作与检测函数槽相连接
}

VideoAnalysis::~VideoAnalysis()
{
	delete timer;
	timer = NULL;
	destroy_ui();
}

void VideoAnalysis::openVideo() {
	QString fileName = QFileDialog::getOpenFileName(this, "打开视频", ".", "Video (*.avi)");	// 弹出选择文件对话框
	capture.open(fileName.toStdString());	// 打开视频文件
	assert(capture.isOpened());
	timer->start();		// 计时器开启
}

void VideoAnalysis::display() {
	capture >> frame;	// 当视频文件的帧输入 frame 成员变量
	if (frame.empty()) {	// 如果文件结束，停止
		timer->stop();
		return;
	}
	Mat vis = frame;
	if (is_detecting) {		// 如果监控标志位开启，则执行监控函数
		vis = detect();
	}
	
	// 将帧显示到界面上
	QImage image = QImage((const unsigned char *)(vis.data), vis.cols, vis.rows, QImage::Format_RGB888).rgbSwapped();
	ui.screen->setPixmap(QPixmap::fromImage(image));

}

void VideoAnalysis::start_detect() {
	
	if (ui.detect_action->isChecked()) {
		// 点击检测按钮后，开始检测，则初始化历史帧和运动记录矩阵
		prev_frame = frame.clone();
		motion_history = Mat::zeros(frame.rows, frame.cols, CV_32F);
		is_detecting = true;
	} else {
		// 停止检测
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
	double thrs = 32;									// 设置阈值
	threshold(gray_diff, motion_mask, thrs, 1, THRESH_BINARY);	// 二值化
	double timestamp = float(clock()) / 1000;				// 得到当前时间戳
	updateMotionHistory(motion_mask, motion_history, timestamp, MHI_DURATION);	// 更新运动历史矩阵
	Mat mg_mask, mg_orient;
	calcMotionGradient(motion_history, mg_mask, mg_orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 5);	// 计算运动历史图像的梯度方向
	Mat seg_mask;
	std::vector<Rect> seg_bounds;
	segmentMotion(motion_history, seg_mask, seg_bounds, timestamp, MAX_TIME_DELTA);		// 将整个运动分割为独立的运动部分
	status_label->setText(QString::number(timestamp));
	Mat vis;
	switch(ui.output_dial->value()) {
	case 0:
		vis = frame.clone();		// 正常显示
		break;
	case 1:
		vis = frame_diff.clone();	// 帧间差分显示
		break;
//  	case 2:
//  		threshold((motion_history - (timestamp - MHI_DURATION)) / MHI_DURATION, vis, 0, 255, THRESH_BINARY);	// 二值化
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

		
		Mat silh_roi = motion_mask(Range(y, y+rh), Range(x, x+rw));		// 获取图像ROI
		Mat orient_roi = mg_orient(Range(y, y+rh), Range(x, x+rw));		// 获取图像ROI
		Mat mask_roi = mg_mask(Range(y, y+rh), Range(x, x+rw));			// 获取图像ROI
		Mat mhi_roi = motion_history(Range(y, y+rh), Range(x, x+rw));	// 获取图像ROI
		if (norm(silh_roi, NORM_L1) < area*0.05) {
			continue;
		}
		double angle = calcGlobalOrientation(orient_roi, mask_roi, mhi_roi, timestamp, MHI_DURATION);	// 计算每个运动部件的运动方向
		Scalar color(255, 0, 0);	// BGR，默认为蓝色
		if (i == 0) {	// 如果运动范围大于阈值，设置为红色
			color[0] = 0;
			color[2] = 255;
		}
		draw_motion_comp(vis, *it, angle, color);	// 在该帧上画出区域
	}
	prev_frame = frame.clone();		// 保存历史帧
	return vis;
}

void VideoAnalysis::draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color) {
	// 绘制目标区域，和运动方向
	rectangle(vis, rect, Scalar(0, 255, 0), 3);
	int r = min(rect.width/2, rect.height/2);
	Point center(rect.x + rect.width / 2, rect.y + rect.height / 2);
    angle = angle*M_PI/180;
	circle(vis, center, r, color, 3);
	line(vis, center, Point(int(center.x+cos(angle)*r), int(center.y+sin(angle)*r)), color, 3);

}