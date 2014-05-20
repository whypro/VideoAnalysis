#ifndef VIDEO_ANALYSIS_H
#define VIDEO_ANALYSIS_H

#include <QtGui/QMainWindow>
#include <QtCore/QTimer>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

#include "ui_video_analysis.h"

class VideoAnalysis : public QMainWindow
{
	Q_OBJECT

public:
	VideoAnalysis(QWidget *parent = 0, Qt::WFlags flags = 0);
	~VideoAnalysis();

private:
	Ui::VideoAnalysisClass ui;		// Qt UI
	void init_signal();				// 初始化信号槽
	void init_ui();					// 初始化空间
	void destroy_ui();				// 销毁控件
	
	QTimer *timer;					// 计时器
	static const int FPS = 24;		// 每秒的帧数
	Mat frame;						// 当前帧
	Mat prev_frame;					// 上一帧
	Mat motion_history;				// 运动历史
	VideoCapture capture;			// 视频对象
	bool is_detecting;				// 检测标识位

	Mat detect();					// 目标检测函数
	void draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color);		// 区域绘制函数

	QLabel* status_label;			// 状态栏的Label


private slots:
	void openVideo();				// 打开视频函数
	void display();					// 显示帧函数
	void start_detect();			// 开始检测函数
};

#endif // VIDEO_ANALYSIS_H
