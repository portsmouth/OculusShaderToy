
#include "Mac.h"
#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include <QBackingStore>

#include <QRect>
#include <QApplication>
#include <QDesktopWidget>

void Mac::fullscreen(QWidget* window)
{
	NSView *nsview = (NSView *) window->winId();
	NSWindow *nswindow = [nsview window];

	//window->showFullScreen();
	//[nswindow toggleFullScreen:nil];
	//window->setWindowFlags(Qt::FramelessWindowHint);
}
