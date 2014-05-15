#ifndef VIDEO_ANALYSIS_H
#define VIDEO_ANALYSIS_H

#include <QtGui/QMainWindow>
#include "ui_video_analysis.h"

class VideoAnalysis : public QMainWindow
{
	Q_OBJECT

public:
	VideoAnalysis(QWidget *parent = 0, Qt::WFlags flags = 0);
	~VideoAnalysis();

private:
	Ui::VideoAnalysisClass ui;
};

#endif // VIDEO_ANALYSIS_H
