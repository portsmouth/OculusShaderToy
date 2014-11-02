
#include <QApplication>
#include <QMainWindow>
#include <QLabel>

#include <iostream>
#include <fstream>
#include <sstream>

#include "Renderer.h"


int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QMainWindow w;
	w.resize(1920, 1080);
	w.show();

	QGLFormat format;
	format.setDoubleBuffer(true);
	format.setDepth(true);
	format.setAlpha(true);
	format.setSampleBuffers(true);
	format.setSamples(16);

	Renderer* renderer = new Renderer(1920, 1080, format, &w);
	w.setCentralWidget(renderer);
	renderer->show();

	return app.exec();
}
