/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/X.h"

#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <sys/select.h>
#include <math.h>
#include <stdio.h>
#include <iostream>


using namespace std;
using namespace fl;


// class Display --------------------------------------------------------------

fl::Display fl::Display::primary;

fl::Display::Display ()
{
  display = 0;

  XSetErrorHandler (errorHandler);
  XSetIOErrorHandler (ioErrorHandler);
}

fl::Display::Display (const string & name)
{
  initialize (name);
}

/**
   @todo May need a global mutexX to lock access to Xlib for calls to
   XOpenDisplay() here and XCloseDisplay() in the destructor.
 **/
void
fl::Display::initialize (const string & name)
{
  const char * cname = NULL;
  if (name.size () > 0)
  {
	cname = name.c_str ();
  }

  if (! (display = XOpenDisplay (cname)))
  {
	char message[256];
	sprintf (message, "Can't connect to X server: %s", XDisplayName (cname));
	throw message;
  }

  int count = ScreenCount (display);
  screens.resize (count, (fl::Screen *) NULL);

  done = false;
  threadMessagePump = thread (&fl::Display::messagePump, this);
}

fl::Display::~Display ()
{
  if (display)
  {
	done = true;
	mutexDisplay.lock ();
	XSync (display, true);
	mutexDisplay.unlock ();
	threadMessagePump.join ();

	vector<fl::Screen *>::iterator i;
	for (i = screens.begin (); i < screens.end (); i++)
	{
	  if (*i) delete *i;
	}

	XCloseDisplay (display);
  }
}

fl::Display *
fl::Display::getPrimary ()
{
  if (! primary.display)
  {
	primary.initialize ("");
  }

  return &primary;
}

void
fl::Display::addCallback (const fl::Window & window)
{
  mutexCallback.lock ();
  callbacks.insert (make_pair (window.id, (fl::Window *) &window));
  mutexCallback.unlock ();
}

void
fl::Display::removeCallback (const fl::Window & window)
{
  mutexCallback.lock ();
  callbacks.erase (window.id);
  mutexCallback.unlock ();
}

void
fl::Display::messagePump ()
{
  try
  {
	int fd = ConnectionNumber (display);
	while (! done)
	{
	  mutexDisplay.lock ();
	  if (XPending (display) == 0)
	  {
		mutexDisplay.unlock ();

		// Use select() to suspend until input is available
		fd_set fds;
		FD_ZERO (&fds);
		FD_SET (fd, &fds);
		struct timeval timeout;
		timeout.tv_sec  = 0;
		timeout.tv_usec = 100000;  // wait no more than 0.1s before checking if done flag is set
		select (fd + 1, &fds, 0, 0, &timeout);
		continue;
	  }
	  XEvent event;
	  XNextEvent (display, &event);
	  mutexDisplay.unlock ();

	  mutexCallback.lock ();
	  map<XID, fl::Window *>::iterator i = callbacks.find (event.xany.window);
	  if (i != callbacks.end ())
	  {
		// By actually processing the event inside this critical section, we
		// open the possibility of the lock being held for a long time.
		// However, this guarantees that an event will finish processing
		// before the window destructor proceeds past the removeCallback()
		// function call.
		i->second->processEvent (event);
	  }
	  mutexCallback.unlock ();
	}
  }
  catch (char * error)
  {
	cerr << error << endl;
  }
}

fl::Screen &
fl::Display::defaultScreen ()
{
  int number = DefaultScreen (display);
  if (! screens[number])
  {
	screens[number] = new fl::Screen (this, number);
  }
  return *screens[number];
}

Atom
fl::Display::internAtom (const std::string & name, bool onlyIfExists)
{
  lock_guard<mutex> lock (mutexDisplay);
  return XInternAtom (display, name.c_str (), onlyIfExists);
}

void
fl::Display::putBackEvent (XEvent & event)
{
  mutexDisplay.lock ();
  XPutBackEvent (display, &event);
  mutexDisplay.unlock ();
}

void
fl::Display::flush ()
{
  mutexDisplay.lock ();
  XFlush (display);
  mutexDisplay.unlock ();
}

void
fl::Display::lock ()
{
  mutexDisplay.lock ();
}

void
fl::Display::unlock ()
{
  mutexDisplay.unlock ();
}

int
fl::Display::errorHandler (::Display * display, XErrorEvent * event)
{
  cerr << "Ignoring X protocol error" << endl;
  return 0;
}

int
fl::Display::ioErrorHandler (::Display * display)
{
  cerr << "X i/o error (terminating)" << endl;
  return 0;
}


// class Screen ---------------------------------------------------------------

fl::Screen::Screen (fl::Display * display, int number)
{
  if (! display)
  {
	display = fl::Display::getPrimary ();
  }

  display->lock ();
  screen = ScreenOfDisplay (display->display, number);
  root.id = RootWindowOfScreen (screen);
  ::Visual * vp = DefaultVisualOfScreen (screen);
  display->unlock ();

  this->number = number;
  this->display = display;
  root.screen = this;

  visual = new fl::Visual (*this, vp);
  visuals.insert (make_pair (visual->id, visual));
}

fl::Screen::~Screen ()
{
  map<VisualID, fl::Visual *>::iterator i;
  for (i = visuals.begin (); i != visuals.end (); i++)
  {
	delete i->second;
  }
}

fl::Window &
fl::Screen::rootWindow () const
{
  return (fl::Window &) root;
}

unsigned long
fl::Screen::blackPixel () const
{
  display->lock ();
  unsigned long result = BlackPixelOfScreen (screen);
  display->unlock ();
  return result;
}

int
fl::Screen::defaultDepth () const
{
  display->lock ();
  int result = DefaultDepth (display->display, number);
  display->unlock ();
  return result;
}

fl::Visual &
fl::Screen::defaultVisual () const
{
  return *visual;
}


// class Visual ---------------------------------------------------------------

fl::Visual::Visual ()
{
  screen = NULL;
  visual = NULL;
}

fl::Visual::Visual (fl::Screen & screen, ::Visual * visual)
{
  screen.display->lock ();
  XVisualInfo vinfo;
  vinfo.visualid = XVisualIDFromVisual (visual);
  int count = 0;
  XVisualInfo * vinfos = XGetVisualInfo (screen.display->display, VisualIDMask, &vinfo, &count);
  initialize (screen, vinfos);
  XFree (vinfos);
  screen.display->unlock ();
}

void
fl::Visual::initialize (fl::Screen & screen, XVisualInfo * vinfo)
{
  this->screen     = &screen;

  visual           = vinfo->visual;
  id               = vinfo->visualid;
  depth            = vinfo->depth;
  //colorClass       = vinfo->class;
  colormapSize     = vinfo->colormap_size;
  bitsPerChannel   = vinfo->bits_per_rgb;

  int d = (int) ceil (depth / 8.0);
  if (d == 3)
  {
	d = 4;
  }
  format = new PixelFormatRGBABits (d, vinfo->red_mask, vinfo->green_mask, vinfo->blue_mask, 0x0);
}

XImage *
fl::Visual::createImage (const Image & image, Image & formatted) const
{
  formatted = image * *format;
  PixelBufferPacked * pbp = (PixelBufferPacked *) formatted.buffer;
  char * buffer = pbp ? (char *) pbp->memory : 0;
  screen->display->lock ();
  XImage * result = XCreateImage
  (
    screen->display->display,
	visual,
	depth,
	ZPixmap,
	0,
	buffer,
	formatted.width, formatted.height,
	(int) (format->depth * 8),
	0
  );
  screen->display->unlock ();

  // Should hook in a replacement delete routine in the image structure to
  // prevent the buffer attached to "formatted" from being destroyed.

  return result;
}


// class Resource -------------------------------------------------------------

fl::Resource::~Resource ()
{
  // In various subclasses, make appropriate X call to destroy resource
  // associated with id.
}


// class Drawable -------------------------------------------------------------

void
fl::Drawable::getGeometry (int & x, int & y, int & width, int & height, int & border, int & depth) const
{
  ::Window root;
  screen->display->lock ();
  XGetGeometry (screen->display->display, id, &root, &x, &y, (unsigned int *) &width, (unsigned int *) &height, (unsigned int *) &border, (unsigned int *) &depth);
  screen->display->unlock ();
}

void
fl::Drawable::getSize (int & width, int & height) const
{
  int barf;
  ::Window root;
  screen->display->lock ();
  XGetGeometry (screen->display->display, id, &root, &barf, &barf, (unsigned int *) &width, (unsigned int *) &height, (unsigned int *) &barf, (unsigned int *) &barf);
  screen->display->unlock ();
}

void
fl::Drawable::putImage (const fl::GC & gc, const XImage * image, int toX, int toY, int fromX, int fromY, int width, int height)
{
  if (width < 1)
  {
	width = image->width;
  }
  if (height < 1)
  {
	height = image->height;
  }
  width = min (width, image->width - fromX);
  height = min (height, image->height - fromY);
  if (width <= 0  ||  height <= 0) return;  // no point in putting an image with no extent
  screen->display->lock ();
  XPutImage
  (
    screen->display->display,
	id,
	gc.gc,
	(XImage *) image,
	fromX, fromY,
	toX, toY,
	width, height
  );
  screen->display->unlock ();
}

Image
fl::Drawable::getImage (int x, int y, int width, int height) const
{
  if (width <= 0  ||  height <= 0)
  {
	getSize (width, height);
  }

  screen->display->lock ();
  XImage * image = XGetImage (screen->display->display, id, x, y, width, height, ~ ((unsigned long) 0), ZPixmap);
  screen->display->unlock ();

  int d = (int) ceil (image->depth / 8.0);
  if (d == 3)
  {
	d = 4;
  }
  PixelFormatRGBABits format (d, image->red_mask, image->green_mask, image->blue_mask, 0x0);

  Image temp ((unsigned char *) image->data, image->width, image->height, format);
  Image result = temp * RGBAChar;  // since alpha channel == 0, should be that RGBAChar != format, so buffer will be duplicated
  screen->display->lock ();
  XDestroyImage (image);
  screen->display->unlock ();

  return result;
}

void
fl::Drawable::copyArea (const fl::GC & gc, const fl::Drawable & source, int toX, int toY, int fromX, int fromY, int width, int height)
{
  int sourceWidth;
  int sourceHeight;
  int barf;
  source.getGeometry (barf, barf, sourceWidth, sourceHeight, barf, barf);
  if (width < 1)
  {
	width = sourceWidth;
  }
  if (height < 1)
  {
	height = sourceHeight;
  }
  width  = min (width,  sourceWidth  - fromX);
  height = min (height, sourceHeight - fromY);
  screen->display->lock ();
  XCopyArea
  (
    screen->display->display,
	source.id, id,
	gc.gc,
	fromX, fromY,
	width, height,
	toX, toY
  );
  screen->display->unlock ();
}


// class EventPredicate -------------------------------------------------------

Bool
EventPredicate::predicate (::Display * display, XEvent * event, XPointer arg)
{
  EventPredicate * me = (EventPredicate *) arg;
  return me->value (*event) ? True : False;
}


// class Window ---------------------------------------------------------------

fl::Window::Window (fl::Screen * screen, ::Window id)
{
  this->screen = screen;
  this->id     = id;
}

fl::Window::Window (fl::Window & parent, int width, int height, int x, int y)
{
  screen = parent.screen;

  unsigned long black = screen->blackPixel ();
  screen->display->lock ();
  id = XCreateSimpleWindow
  (
    screen->display->display,
	parent.id,
	x, y,
	width, height,
	0,	
	black,
	black
  );
  screen->display->unlock ();
}

fl::Window::Window (fl::Screen & screen, int width, int height, int x, int y)
{
  this->screen = &screen;

  unsigned long black = screen.blackPixel ();
  screen.display->lock ();
  id = XCreateSimpleWindow
  (
    screen.display->display,
	screen.root.id,
	x, y,
	width, height,
	0,	
	black,
	black
  );
  screen.display->unlock ();
}

fl::Window::~Window ()
{
  screen->display->removeCallback (*this);

  // Because we have already unregistered the callback, we will not receive
  // the DestroyNotify message.
  screen->display->lock ();
  XDestroyWindow (screen->display->display, id);
  screen->display->unlock ();
}

void
fl::Window::selectInput (long eventMask)
{
  fl::Display * display = screen->display;
  screen->display->lock ();
  XSelectInput (display->display, id, eventMask);
  screen->display->unlock ();
  display->addCallback (*this);
}

void
fl::Window::map ()
{
  screen->display->lock ();
  XMapWindow (screen->display->display, id);
  screen->display->unlock ();
}

void
fl::Window::unmap ()
{
  screen->display->lock ();
  XUnmapWindow (screen->display->display, id);
  screen->display->unlock ();
}

void
fl::Window::resize (int width, int height)
{
  screen->display->lock ();
  XResizeWindow (screen->display->display, id, width, height);
  screen->display->unlock ();
}

void
fl::Window::setColormap (fl::Colormap & colormap)
{
  screen->display->lock ();
  XSetWindowColormap
  (
    screen->display->display,
	id,
	colormap.id
  );
  screen->display->unlock ();
}

void
fl::Window::setWMProtocols (const vector<Atom> & protocols)
{
  screen->display->lock ();
  XSetWMProtocols (screen->display->display, id, (Atom *) & protocols[0], protocols.size ());
  screen->display->unlock ();
}

void
fl::Window::setWMName (const std::string name)
{
  screen->display->lock ();
  XStoreName (screen->display->display, id, name.c_str ());
  screen->display->unlock ();
  // Should use XSetWMName, but this is simpler.  Only needed for non-ASCII
  // encodings, so may be appropriate for the function prototype...
  // setWMName (const std::wstring name)
}

void
fl::Window::clear (int x, int y, int width, int height, bool exposures)
{
  screen->display->lock ();
  XClearArea (screen->display->display, id, x, y, width, height, exposures);
  screen->display->unlock ();
}

void
fl::Window::changeProperty (Atom property, Atom type, int mode, const std::string value)
{
  screen->display->lock ();
  XChangeProperty (screen->display->display, id, property, type, 8, mode, (const unsigned char *) value.c_str (), value.size ());
  screen->display->unlock ();
}

bool
fl::Window::checkTypedEvent (XEvent & event, int eventType)
{
  screen->display->lock ();
  bool result = XCheckTypedWindowEvent (screen->display->display, id, eventType, &event);
  screen->display->unlock ();
  return result;
}

bool
fl::Window::checkIfEvent (XEvent & event, EventPredicate & predicate)
{
  screen->display->lock ();
  bool result = XCheckIfEvent (screen->display->display, &event, &predicate.predicate, (XPointer) &predicate);
  screen->display->unlock ();
  return result;
}

bool
fl::Window::sendEvent (XEvent & event, unsigned long eventMask, bool propogate)
{
  event.xany.window = id;
  screen->display->lock ();
  bool result = XSendEvent (screen->display->display, id, propogate, eventMask, &event);
  screen->display->unlock ();
  return result;
}

bool
fl::Window::processEvent (XEvent & event)
{
  // Default action is to pass message off to parent, if any.
  return false;

  // Parenting should be implemented in a container window class, which would
  // be a subclass of Window.
}


// class Colormap -------------------------------------------------------------

fl::Colormap::Colormap (fl::Visual & visual, int alloc)
{
  screen = visual.screen;
  screen->display->lock ();
  id = XCreateColormap
  (
	screen->display->display,
	screen->root.id,
	visual.visual,
	alloc
  );
  screen->display->unlock ();
}

fl::Colormap::~Colormap ()
{
  screen->display->lock ();
  XFreeColormap (screen->display->display, id);
  screen->display->unlock ();
}


// class GC -------------------------------------------------------------------

fl::GC::GC (fl::Screen * screen, ::GC gc, bool shouldFree)
{
  this->screen     = screen;
  this->gc         = gc;
  this->shouldFree = shouldFree;
}

fl::GC::GC (fl::Screen & screen, unsigned long valuemask, XGCValues * values)
{
  this->screen = &screen;
  screen.display->lock ();
  gc = XCreateGC
  (
    screen.display->display,
	screen.root.id,
	valuemask,
	values
  );
  screen.display->unlock ();
  shouldFree = true;
}

fl::GC::~GC ()
{
  if (shouldFree)
  {
	screen->display->lock ();
	XFreeGC (screen->display->display, gc);
	screen->display->unlock ();
  }
}
