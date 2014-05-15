#include "video_analysis.h"
#include <QtGui>
#include <QtCore>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

VideoAnalysis::VideoAnalysis(QWidget *parent, Qt::WFlags flags)
	: QMainWindow(parent, flags)
{
	ui.setupUi(this);
	connect(ui.open_video_action, SIGNAL(triggered()), this, SLOT(openVideo()));
	
	timer = new QTimer(this);
	timer->setInterval(1000/24);
	connect(timer, SIGNAL(timeout()), this, SLOT(display()));	

}

VideoAnalysis::~VideoAnalysis()
{
	delete timer;
}

void VideoAnalysis::openVideo() {
	QString fileName = QFileDialog::getOpenFileName(this, "´ò¿ªÊÓÆµ", ".", "Video (*.avi)");
	capture.open(fileName.toStdString());
	assert(capture.isOpened());
	timer->start();

}

void VideoAnalysis::display() {
	capture >> frame;
	if (frame.empty()) {
		timer->stop();
		return;
	}
	//ui.screen->setText("hehe");
	//QMessageBox::about(this, "hehe", "hehe");
	QImage image = QImage((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888).rgbSwapped();
	ui.screen->setPixmap(QPixmap::fromImage(image));
	//ui.screen->show();
}
