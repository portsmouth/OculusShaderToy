
#include "Mac.h"
#include <QMainWindow>
#import <Cocoa/Cocoa.h>

void Mac::fullscreen(QWidget* window)
{
	// Unclear why this is necessary, but without it,
	// fullscreen mode on Mac doesn't work.
	NSView *nsview = (NSView *) window->winId();
}
