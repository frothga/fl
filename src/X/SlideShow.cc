/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
	screen->display->lock ();
	XDestroyImage (ximage);
	screen->display->unlock ();
  }
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
	  bool found = false;
	  while (checkTypedEvent (event, event.type)) found = true;
	  if (found)
	  {
		// We've purged the whole queue of Expose events.  Now post a new
		// one to the end, to ensure we actually draw something.
		sendEvent (event);
		return true;
	  }
	  // else fall through to MapNotify handler below...
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

	  deltaX = min (deltaX, image.width - width - offsetX);
	  deltaX = max (deltaX, -offsetX);
	  offsetX += deltaX;

	  deltaY = min (deltaY, image.height - height - offsetY);
	  deltaY = max (deltaY, -offsetY);
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

		mutexImage.lock ();

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

		mutexImage.unlock ();
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
  unique_lock<mutex> lock (waitingMutex);
  waitingCondition.wait (lock);
}

void
SlideShow::stopWaiting ()
{
  // Release waiting threads.
  waitingCondition.notify_all ();
}

void
SlideShow::show (const Image & image, int centerX, int centerY)
{
  mutexImage.lock ();
  if (ximage)
  {
	ximage->data = (char *) malloc (1);  // Will be freed immediately by XDestroyImage.
	screen->display->lock ();
	XDestroyImage (ximage);
	screen->display->unlock ();
	ximage = NULL;
  }
  ximage = visual->createImage (image, this->image);
  mutexImage.unlock ();

  if (! ximage)
  {
	throw "XCreateImage failed";
  }

  if (centerX < offsetX  ||  centerX > offsetX + width  ||  centerY < offsetY   ||  centerY > offsetY + height)
  {
	offsetX = centerX - width / 2;
	offsetX = min (offsetX, image.width - width);
	offsetX = max (offsetX, 0);

	offsetY = centerY - height / 2;
	offsetY = min (offsetY, image.height - height);
	offsetY = max (offsetY, 0);
  }

  map ();  // In case we aren't already visible
  paint ();
  screen->display->flush ();
}

void
SlideShow::paint ()
{
  mutexImage.lock ();
  if (ximage)
  {
	int w = min (width,  image.width  - offsetX);
	int h = min (height, image.height - offsetY);
	putImage (*gc, ximage, 0, 0, offsetX, offsetY, w, h);
  }
  mutexImage.unlock ();
}
