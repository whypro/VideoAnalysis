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
	Ui::VideoAnalysisClass ui;
	void init_signal();
	void init_ui();
	
	QTimer *timer;
	static const int FPS = 24;
	Mat frame;
	Mat prev_frame;
	Mat motion_history;
	VideoCapture capture;
	bool is_detecting;

	Mat detect();
	void draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color);


private slots:
	void openVideo();
	void display();
	void start_detect();
};

#endif // VIDEO_ANALYSIS_H
