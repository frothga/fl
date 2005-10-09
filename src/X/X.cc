/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for Cygwin
*/


#include "fl/X.h"

#include <X11/Xutil.h>
#include <X11/Xos.h>

#include <math.h>
#include <iostream>


using namespace std;
using namespace fl;


// class Display --------------------------------------------------------------

fl::Display fl::Display::primary;

fl::Display::Display ()
{
  display = 0;

  XInitThreads ();
  XSetErrorHandler (errorHandler);
  XSetIOErrorHandler (ioErrorHandler);
}

fl::Display::Display (const string & name)
{
  initialize (name);
}

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

  pthread_mutexattr_t attrCallback;
  pthread_mutexattr_init (&attrCallback);
  pthread_mutexattr_settype (&attrCallback, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init (&mutexCallback, &attrCallback);
  pthread_mutexattr_destroy (&attrCallback);

  int count = ScreenCount (display);
  screens.resize (count, (fl::Screen *) NULL);

  done = false;
  if (pthread_create (&pidMessagePump, NULL, messagePump, this) == 0)
  {
	cerr << "Message pump started for " << XDisplayName (cname) << endl;
  }
  else
  {
	char message[256];
	sprintf (message, "Message pump failed for %s", XDisplayName (cname));
	throw message;
  }
}

fl::Display::~Display ()
{
  if (display)
  {
	done = true;
	XSync (display, true);
	pthread_join (pidMessagePump, NULL);

	pthread_mutex_destroy (&mutexCallback);

	vector<fl::Screen *>::iterator i;
	for (i = screens.begin (); i < screens.end (); i++)
	{
	  if (*i)
	  {
		delete *i;
	  }
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
  pthread_mutex_lock (&mutexCallback);
  callbacks.insert (make_pair (window.id, (fl::Window *) &window));
  pthread_mutex_unlock (&mutexCallback);
}

void
fl::Display::removeCallback (const fl::Window & window)
{
  pthread_mutex_lock (&mutexCallback);
  callbacks.erase (window.id);
  pthread_mutex_unlock (&mutexCallback);
}

void *
fl::Display::messagePump (void * arg)
{
  fl::Display * display = (fl::Display *) arg;

  try
  {
	XEvent event;
	while (! display->done)
    {
	  XNextEvent (display->display, &event);

	  pthread_mutex_lock (& display->mutexCallback);
	  map<XID, fl::Window *>::iterator i = display->callbacks.find (event.xany.window);
	  if (i != display->callbacks.end ())
	  {
		// By actually processing the event inside this critical section, we
		// open the possibility of the lock being held for a long time.
		// However, this guarantees that an event will finish processing
		// before the window destructor proceeds past the removeCallback()
		// function call.
		i->second->processEvent (event);
	  }
	  pthread_mutex_unlock (& display->mutexCallback);
	}
  }
  catch (char * error)
  {
	cerr << error << endl;
  }

  return 0;
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
  return XInternAtom (display, name.c_str (), onlyIfExists);
}

void
fl::Display::putBackEvent (XEvent & event)
{
  XPutBackEvent (display, &event);
}

void
fl::Display::flush ()
{
  XFlush (display);
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

  screen = ScreenOfDisplay (display->display, number);
  this->number = number;
  this->display = display;

  root.screen = this;
  root.id = RootWindowOfScreen (screen);

  // Get default visual
  ::Visual * vp = DefaultVisualOfScreen (screen);
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
  return BlackPixelOfScreen (screen);
}

int
fl::Screen::defaultDepth () const
{
  return DefaultDepth (display->display, number);
}

fl::Visual &
fl::Screen::defaultVisual () const
{
  return *visual;
}


// class Visual ---------------------------------------------------------------

fl::Visual::Visual ()
: format (0, 0, 0, 0, 0)
{
  screen = NULL;
  visual = NULL;
}

fl::Visual::Visual (fl::Screen & screen, ::Visual * visual)
: format (0, 0, 0, 0, 0)
{
  XVisualInfo vinfo;
  vinfo.visualid = XVisualIDFromVisual (visual);
  int count = 0;
  XVisualInfo * vinfos = XGetVisualInfo (screen.display->display, VisualIDMask, &vinfo, &count);
  initialize (screen, vinfos);
  XFree (vinfos);
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
  format.depth     = d;
  format.redMask   = vinfo->red_mask;
  format.greenMask = vinfo->green_mask;
  format.blueMask  = vinfo->blue_mask;
  format.alphaMask = 0x0;
}

XImage *
fl::Visual::createImage (const Image & image, Image & formatted) const
{
  formatted = image * format;
  char * buffer = (char *) formatted.buffer;
  XImage * result = XCreateImage
  (
    screen->display->display,
	visual,
	depth,
	ZPixmap,
	0,
	buffer,
	formatted.width, formatted.height,
	format.depth * 8,
	0
  );

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
  XGetGeometry (screen->display->display, id, &root, &x, &y, (unsigned int *) &width, (unsigned int *) &height, (unsigned int *) &border, (unsigned int *) &depth);
}

void
fl::Drawable::getSize (int & width, int & height) const
{
  int barf;
  ::Window root;
  XGetGeometry (screen->display->display, id, &root, &barf, &barf, (unsigned int *) &width, (unsigned int *) &height, (unsigned int *) &barf, (unsigned int *) &barf);
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
  width = width <? image->width - fromX;
  height = height <? image->height - fromY;
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
}

Image
fl::Drawable::getImage (int x, int y, int width, int height) const
{
  if (width <= 0  ||  height <= 0)
  {
	getSize (width, height);
  }

  XImage * image = XGetImage (screen->display->display, id, x, y, width, height, ~ ((unsigned long) 0), ZPixmap);

  int d = (int) ceil (image->depth / 8.0);
  if (d == 3)
  {
	d = 4;
  }
  PixelFormatRGBABits format (d, image->red_mask, image->green_mask, image->blue_mask, 0x0);

  Image temp ((unsigned char *) image->data, image->width, image->height, format);
  Image result = temp * RGBAChar;  // since alpha channel == 0, should be that RGBAChar != format, so buffer will be duplicated
  XDestroyImage (image);

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
  width = width <? sourceWidth - fromX;
  height = height <? sourceHeight - fromY;
  XCopyArea
  (
    screen->display->display,
	source.id, id,
	gc.gc,
	fromX, fromY,
	width, height,
	toX, toY
  );
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

  id = XCreateSimpleWindow
  (
    screen->display->display,
	parent.id,
	x, y,
	width, height,
	0,	
	screen->blackPixel (),
	screen->blackPixel ()
  );
}

fl::Window::Window (fl::Screen & screen, int width, int height, int x, int y)
{
  this->screen = &screen;

  id = XCreateSimpleWindow
  (
    screen.display->display,
	screen.root.id,
	x, y,
	width, height,
	0,	
	screen.blackPixel (),
	screen.blackPixel ()
  );
}

fl::Window::~Window ()
{
  screen->display->removeCallback (*this);

  // Because we have already unregistered the callback, we will not receive
  // the DestroyNotify message.
  XDestroyWindow (screen->display->display, id);
}

void
fl::Window::selectInput (long eventMask)
{
  fl::Display * display = screen->display;
  XSelectInput (display->display, id, eventMask);
  display->addCallback (*this);
}

void
fl::Window::map ()
{
  XMapWindow (screen->display->display, id);
}

void
fl::Window::unmap ()
{
  XUnmapWindow (screen->display->display, id);
}

void
fl::Window::resize (int width, int height)
{
  XResizeWindow (screen->display->display, id, width, height);
}

void
fl::Window::setColormap (fl::Colormap & colormap)
{
  XSetWindowColormap
  (
    screen->display->display,
	id,
	colormap.id
  );
}

void
fl::Window::setWMProtocols (const vector<Atom> & protocols)
{
  XSetWMProtocols (screen->display->display, id, (Atom *) & protocols[0], protocols.size ());
}

void
fl::Window::setWMName (const std::string name)
{
  XStoreName (screen->display->display, id, name.c_str ());
  // Should use XSetWMName, but this is simpler.  Only needed for non-ASCII
  // encodings, so may be appropriate for the function prototype...
  // setWMName (const std::wstring name)
}

void
fl::Window::clear (int x, int y, int width, int height, bool exposures)
{
  XClearArea (screen->display->display, id, x, y, width, height, exposures);
}

void
fl::Window::changeProperty (Atom property, Atom type, int mode, const std::string value)
{
  XChangeProperty (screen->display->display, id, property, type, 8, mode, (const unsigned char *) value.c_str (), value.size ());
}

bool
fl::Window::checkTypedEvent (XEvent & event, int eventType)
{
  return XCheckTypedWindowEvent (screen->display->display, id, eventType, &event);
}

bool
fl::Window::sendEvent (XEvent & event, unsigned long eventMask, bool propogate)
{
  event.xany.window = id;
  return XSendEvent (screen->display->display, id, propogate, eventMask, &event);
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
  id = XCreateColormap
  (
	screen->display->display,
	screen->root.id,
	visual.visual,
	alloc
  );
}

fl::Colormap::~Colormap ()
{
  XFreeColormap (screen->display->display, id);
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
  gc = XCreateGC
  (
    screen.display->display,
	screen.root.id,
	valuemask,
	values
  );
  shouldFree = true;
}

fl::GC::~GC ()
{
  if (shouldFree)
  {
	XFreeGC (screen->display->display, gc);
  }
}
