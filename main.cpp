
#include "video_analysis.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	VideoAnalysis w;
	w.show();
	return a.exec();
}
