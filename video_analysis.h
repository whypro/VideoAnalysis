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

	
	QTimer *timer;
	Mat frame;
	VideoCapture capture;

private slots:
	void openVideo();
	void display();
};

#endif // VIDEO_ANALYSIS_H
