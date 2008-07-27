/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_x_h
#define fl_x_h


#include "fl/image.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <pthread.h>
#include <string>
#include <map>
#include <vector>


namespace fl
{
  // This is a set of classes that wrap the main structures of Xlib.  Functions
  // closely associated with these structures are included in the respective
  // classes.

  // Classes are named directly after their Xlib structures, and
  // are only distinguished by the namespace (ie: Xlib structures are in the
  // global namespace and wrapper classes are in namespace "fl").  Therefore,
  // the namespace is an important part of the name of every structure
  // mentioned in the code.

  // Thread safety is handled at this and higher levels, rather than by
  // turning on Xlib's own thread safety mechanism.


  // Forward declarations -----------------------------------------------------

  class Screen;
  class Window;


  // Core classes -------------------------------------------------------------

  /**
	 The life of a connection is exactly commensurate with the life of a
	 Display object.
	 When a Display object is constructed, it automatically starts a new
	 message pump thread.
	 One global static Display object is constructed which connects to the
	 default display automatically.  This may be a pain or a boon, depending
	 on your goals.
  **/
  class Display
  {
  public:
	Display (const std::string & name);
	void initialize (const std::string & name);  // Open named display.  If name == "", then open default display.
	~Display ();  // Close display

	void addCallback (const fl::Window & window);
	void removeCallback (const fl::Window & window);
	static void * messagePump (void * arg);

	fl::Screen & defaultScreen ();
	Atom internAtom (const std::string & name, bool onlyIfExists = false);
	void putBackEvent (XEvent & event);
	void flush ();

	// Data
	::Display * display;

	bool                        done;
	pthread_t                   pidMessagePump;
	pthread_mutex_t             mutexCallback;
	std::map<XID, fl::Window *> callbacks;

	std::vector<fl::Screen *> screens;

	static int errorHandler (::Display * display, XErrorEvent * event);
	static int ioErrorHandler (::Display * display);

	// Singleton pattern for primary (default) display.
	static fl::Display * getPrimary ();
  protected:
	Display ();  ///< Allow construction of singleton w/o actually connecting.  Only the singleton "Display::primary" should ever be constructed with this function, since it also contains one-time initializations of Xlib.
	static fl::Display primary;
  };

  class Visual
  {
  public:
	Visual ();
	Visual (fl::Screen & screen, ::Visual * visual);
	void initialize (fl::Screen & screen, XVisualInfo * vinfo);

	XImage * createImage (const Image & image, Image & formatted) const;  // Returns pointer to an XImage containing "image".  "formatted" is bound to the same memory buffer as result.

	// Data
	fl::Screen *             screen;
	::Visual *               visual;
	VisualID                 id;
	unsigned int             depth;
	int                      colorClass;
	PointerPoly<PixelFormat> format;
	int                      colormapSize;
	int                      bitsPerChannel;
  };

  /**
	 Abstract parent of wraps for all server-side resources
	 The current model for resources is that:
	 <ol>
	 <li>The lifespan of the client side object (ie: instance of this class)
	 is exactly commensurate with the lifespan of the server side object.
	 <li>There is exactly one instance of the client side object for each
	 server side object.
	 </ol>
	 A more flexible model would be to allow multiple client side objects
	 per server side object, and to loosely hold the id.  There would be
	 a reference count for each resource type on each Display.  Only when
	 the reference count drops to zero is the server side object freed.
	 There would be functions to attach or detach an id to/from a particular
	 Resource object, which of course would also update the reference count.
  **/
  class Resource
  {
  public:
	virtual ~Resource ();

	fl::Screen * screen;
	XID id;
  };

  class Colormap : public Resource
  {
  public:
	Colormap (fl::Visual & visual, int alloc);
	~Colormap ();
  };

  /**
	 A GC is not really a resource, but it wraps one, so we go ahead and
	 subclasss Resource for this.
  **/
  class GC : public Resource
  {
  public:
	GC (fl::Screen * screen = NULL, ::GC gc = NULL, bool shouldFree = false);
	GC (fl::Screen & screen, unsigned long valuemask, XGCValues * values);

	~GC ();

	::GC gc;
	bool shouldFree;
  };

  /**
	 Abstract parent of all drawable resources (ie: Window and Pixmap).
	 This class exists so some functions can specify that they take Drawables
	 rather than just Windows or Pixmaps.
  **/
  class Drawable : public Resource
  {
  public:
	void getGeometry (int & x, int & y, int & width, int & height, int & border, int & depth) const;
	void getSize (int & width, int & height) const;  // Convenience function associated with getGeometry().
	void putImage (const fl::GC & gc, const XImage * image, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = 0, int height = 0);  // width == 0 or height == 0 means use appropriate value from image structure.
	Image getImage (int x = 0, int y = 0, int width = 0, int height = 0) const;  // Duplicates indicatd region into resulting Image object.  width == 0 or height == 0 means use max area possible.
	void copyArea (const fl::GC & gc, const fl::Drawable & source, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = 0, int height = 0);  // width == 0 or height == 0 means use appropriate value from source Drawable
  };

  /**
	 Helper class for filtering events.
   **/
  class EventPredicate
  {
  public:
	virtual bool value (XEvent & event) = 0;  ///< Do the actual test here.
	static Bool predicate (::Display * display, XEvent * event, XPointer arg);  ///< Bridge function which is passed into X event functions.
  };

  /// An actual X window!  This is meant to be subclassed by "widgets".
  class Window : public virtual Drawable
  {
  public:
	Window (fl::Screen * screen = NULL, ::Window id = None);
	Window (fl::Window & parent, int width = 100, int height = 100, int x = 0, int y = 0);
	Window (fl::Screen & screen, int width = 100, int height = 100, int x = 0, int y = 0);
	virtual ~Window ();

	void selectInput (long eventMask);
	void map ();
	void unmap ();
	void resize (int width, int height);
	void setColormap (fl::Colormap & colormap);
	void setWMProtocols (const std::vector<Atom> & protocols);
	void setWMName (const std::string name);
	void clear (int x = 0, int y = 0, int width = 0, int height = 0, bool exposures = false);
	void changeProperty (Atom property, Atom type, int mode, const std::string value);
	bool checkTypedEvent (XEvent & event, int eventType);
	bool checkIfEvent (XEvent & event, EventPredicate & predicate);
	bool sendEvent (XEvent & event, unsigned long eventMask = 0, bool propogate = true);  // could add optional argument for target window id

	virtual bool processEvent (XEvent & event);  ///< Return value == true indicates that message was fully processed and may now be discarded.  Return value == false indicates message should be passed up parent hierarchy.
  };

  class Screen
  {
  public:
	Screen (fl::Display * display = NULL, int number = 0);  ///< display == NULL means grab primary display
	~Screen ();

	fl::Window & rootWindow () const;
	unsigned long blackPixel () const;
	int defaultDepth () const;
	fl::Visual & defaultVisual () const;

	// Data
	::Screen *    screen;
	int           number;
	fl::Display * display;
	fl::Window    root;
	fl::Visual *  visual;
	std::map<VisualID, fl::Visual *> visuals;
  };
}


#endif
