
#include <QApplication>
#include <QMainWindow>
#include <QLabel>

#include <iostream>
#include <fstream>
#include <sstream>

#include "Renderer.h"

#include <QGraphicsScene>
#include <QGraphicsView>

class GraphicsScene : public QGraphicsScene
{
public:

	GraphicsScene(Renderer* renderer) : m_renderer(renderer) {}

	virtual void drawBackground(QPainter *painter, const QRectF &rect)
	{
		m_renderer->render();
	}

	Renderer* getRenderer() { return m_renderer; }

private:

	Renderer* m_renderer;
};

class GraphicsView : public QGraphicsView
{
public:
	GraphicsView(GraphicsScene* graphicsScene) : QGraphicsView(graphicsScene), m_graphicsScene(graphicsScene) {}

	virtual void mousePressEvent(QMouseEvent *event) { m_graphicsScene->getRenderer()->mousePressEvent(event); }
	virtual void mouseMoveEvent(QMouseEvent *event)  { m_graphicsScene->getRenderer()->mouseMoveEvent(event);  }
	virtual void keyPressEvent(QKeyEvent *event)     { m_graphicsScene->getRenderer()->keyPressEvent(event);   }
	virtual void keyReleaseEvent(QKeyEvent *event)   { m_graphicsScene->getRenderer()->keyReleaseEvent(event); }
	//virtual void wheelEvent(QWheelEvent *)           { m_graphicsScene->getRenderer()->wheelEvent(event);      }

private:
	GraphicsScene* m_graphicsScene;
};



int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	QGLFormat format;
	format.setDoubleBuffer(true);
	format.setDepth(true);
	format.setAlpha(false);
	format.setSampleBuffers(false);
	format.setSamples(16);

	Renderer* renderer = new Renderer(1920, 1080, format, NULL);

	GraphicsScene* graphicsScene = new GraphicsScene(renderer);
	graphicsScene->setSceneRect(QRectF(0,0,1920,1080));

	GraphicsView* view = new GraphicsView(graphicsScene);
	view->showFullScreen();
	view->setViewport(renderer);

	return app.exec();
}
