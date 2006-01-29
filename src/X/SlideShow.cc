/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


01/2006 Fred Rothganger -- Get rid of dead code.  Add optimized expose handler.
*/


#include "fl/slideshow.h"

#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <unistd.h>
#include <math.h>
#include <vector>

// For debugging only
#include <iostream>


using namespace std;
using namespace fl;


// class SlideShow ------------------------------------------------------------

SlideShow::SlideShow ()
: fl::Window (fl::Display::getPrimary ()->defaultScreen (), 640, 480)
{
  pthread_mutex_init (&mutexImage, NULL);
  pthread_mutex_init (&waitingMutex, NULL);
  pthread_cond_init (&waitingCondition, NULL);

  visual = & screen->defaultVisual ();

  colormap = new fl::Colormap (*visual, AllocNone);
  setColormap (*colormap);

  selectInput
  (
     ExposureMask |
	 KeyPressMask |
	 StructureNotifyMask |
	 ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
   );

  WM_PROTOCOLS = screen->display->internAtom ("WM_PROTOCOLS");
  WM_DELETE_WINDOW = screen->display->internAtom ("WM_DELETE_WINDOW");
  vector<Atom> protocols;
  protocols.push_back (WM_DELETE_WINDOW);
  setWMProtocols (protocols);

  gc = new fl::GC (*screen, 0, NULL);

  ximage = NULL;

  offsetX = 0;
  offsetY = 0;
  width = 640;  // These values will be corrected by a configure event when window is mapped.
  height = 480;
}

SlideShow::~SlideShow ()
{
  screen->display->removeCallback (*this);  // Need to stop event handling before destroying ximage.
  unmap ();
  stopWaiting ();
  if (ximage)
  {
	ximage->data = (char *) malloc (1);  // Will be freed immediately by XDestroyImage.
	XDestroyImage (ximage);
  }
  pthread_mutex_destroy (&mutexImage);
  pthread_cond_destroy (&waitingCondition);
  pthread_mutex_destroy (&waitingMutex);
  delete colormap;
  delete gc;
}

bool
SlideShow::processEvent (XEvent & event)
{
  switch (event.type)
  {
    case Expose:
	{
	  int destX   = event.xexpose.x;
	  int destY   = event.xexpose.y;
	  int sourceX = destX + offsetX;
	  int sourceY = destY + offsetY;
	  int w       = event.xexpose.width;
	  int h       = event.xexpose.height;
	  putImage (*gc, ximage, destX, destY, sourceX, sourceY, w, h);
	  return true;
	}
    case MapNotify:
	{
	  paint ();
	  return true;
	}
    case ClientMessage:
	{
	  if (event.xclient.message_type == WM_PROTOCOLS)
	  {
		if (event.xclient.data.l[0] == WM_DELETE_WINDOW)
		{
		  stopWaiting ();
		  unmap ();
		  return true;
		}
	  }
	  break;
	}
    case ConfigureNotify:
	{
	  width = event.xconfigure.width;
	  height = event.xconfigure.height;
	  return true;
	}
    case ButtonPress:
	{
	  modeDrag = false;
	  lastX = event.xbutton.x;
	  lastY = event.xbutton.y;
	  return true;
	}
    case ButtonRelease:
	{
	  if (! modeDrag)
	  {
		stopWaiting ();
	  }
	  return true;
	}
    case MotionNotify:
	{
	  modeDrag = true;
	  int deltaX = event.xmotion.x - lastX;
	  int deltaY = event.xmotion.y - lastY;
	  lastX = event.xmotion.x;
	  lastY = event.xmotion.y;

	  deltaX = deltaX <? image.width - width - offsetX;
	  deltaX = deltaX >? -offsetX;
	  offsetX += deltaX;

	  deltaY = deltaY <? image.height - height - offsetY;
	  deltaY = deltaY >? -offsetY;
	  offsetY += deltaY;

	  if (deltaX  ||  deltaY)
	  {
		int fromX, toX;
		if (deltaX > 0)
		{
		  fromX = deltaX;
		  toX = 0;
		}
		else
		{
		  fromX = 0;
		  toX = -deltaX;
		}

		int fromY, toY;
		if (deltaY > 0)
		{
		  fromY = deltaY;
		  toY = 0;
		}
		else
		{
		  fromY = 0;
		  toY = -deltaY;
		}

		pthread_mutex_lock (&mutexImage);

		copyArea (*gc, *this, toX, toY, fromX, fromY);

		if (deltaX > 0)
		{
		  putImage (*gc, ximage, width - deltaX, 0, offsetX + width - deltaX, offsetY, deltaX, height);
		}
		if (deltaX < 0)
		{
		  putImage (*gc, ximage, 0, 0, offsetX, offsetY, -deltaX, height);
		}
		if (deltaY > 0)
		{
		  putImage (*gc, ximage, 0, height - deltaY, offsetX, offsetY + height - deltaY, width, deltaY);
		}
		if (deltaY < 0)
		{
		  putImage (*gc, ximage, 0, 0, offsetX, offsetY, width, -deltaY);
		}

		pthread_mutex_unlock (&mutexImage);
	  }

	  return true;
	}
    case KeyPress:
	{
	  stopWaiting ();
	  return true;
	}
  }

  return fl::Window::processEvent (event);
}

void
SlideShow::waitForClick ()
{
  // Put thread to sleep.
  pthread_mutex_lock (&waitingMutex);
  pthread_cond_wait (&waitingCondition, &waitingMutex);
  pthread_mutex_unlock (&waitingMutex);
}

void
SlideShow::stopWaiting ()
{
  // Release waiting threads.
  pthread_mutex_lock (&waitingMutex);
  pthread_cond_broadcast (&waitingCondition);
  pthread_mutex_unlock (&waitingMutex);
}

void
SlideShow::show (const Image & image, int centerX, int centerY)
{
  pthread_mutex_lock (&mutexImage);
  if (ximage)
  {
	ximage->data = (char *) malloc (1);  // Will be freed immediately by XDestroyImage.
	XDestroyImage (ximage);
	ximage = NULL;
  }
  ximage = visual->createImage (image, this->image);
  pthread_mutex_unlock (&mutexImage);

  if (! ximage)
  {
	throw "XCreateImage failed";
  }

  if (centerX < offsetX  ||  centerX > offsetX + width  ||  centerY < offsetY   ||  centerY > offsetY + height)
  {
	offsetX = centerX - width / 2;
	offsetX = offsetX <? image.width - width;
	offsetX = offsetX >? 0;

	offsetY = centerY - height / 2;
	offsetY = offsetY <? image.height - height;
	offsetY = offsetY >? 0;
  }

  map ();  // In case we aren't already visible
  paint ();
  XFlush (screen->display->display);
}

void
SlideShow::paint ()
{
  pthread_mutex_lock (&mutexImage);
  if (ximage)
  {
	int w = width <? image.width - offsetX;
	int h = height <? image.height - offsetY;
	putImage (*gc, ximage, 0, 0, offsetX, offsetY, w, h);
  }
  pthread_mutex_unlock (&mutexImage);
}
