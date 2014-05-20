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
	void init_signal();				// ��ʼ���źŲ�
	void init_ui();					// ��ʼ���ռ�
	void destroy_ui();				// ���ٿؼ�
	
	QTimer *timer;					// ��ʱ��
	static const int FPS = 24;		// ÿ���֡��
	Mat frame;						// ��ǰ֡
	Mat prev_frame;					// ��һ֡
	Mat motion_history;				// �˶���ʷ
	VideoCapture capture;			// ��Ƶ����
	bool is_detecting;				// ����ʶλ

	Mat detect();					// Ŀ���⺯��
	void draw_motion_comp(Mat &vis, Rect rect, double angle, Scalar color);		// ������ƺ���

	QLabel* status_label;			// ״̬����Label


private slots:
	void openVideo();				// ����Ƶ����
	void display();					// ��ʾ֡����
	void start_detect();			// ��ʼ��⺯��
};

#endif // VIDEO_ANALYSIS_H
