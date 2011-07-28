/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef slideshow_h
#define slideshow_h


#include "fl/image.h"

#ifdef _MSC_VER

#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  undef min
#  undef max
#  include <pthread.h>

#  undef SHARED
#  ifdef flX_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif

namespace fl
{
  class SHARED SlideShow
  {
  public:
	SlideShow ();
	~SlideShow ();

	void show (const Image & image, int centerX = 0, int centerY = 0);
	void waitForClick ();
	void stopWaiting ();

	//virtual bool processMessage (HWND window, UINT message, WPARAM wParam, LPARAM lParam);

	HBITMAP            image;
	pthread_mutex_t    mutexImage;

	bool               modeDrag;  ///< Indicates that there was motion between button down and button up
	int                lastX;  ///< Where the last button event occurred
	int                lastY;
	int                offsetX; ///< Position in image where window starts
	int                offsetY;
	int                width;   ///< Current size of window
	int                height;

	pthread_t          messagePumpThread;
	pthread_mutex_t    waitingMutex;
	pthread_cond_t     waitingCondition;
	HWND               window;

	// Static functions that implement window class
	static void *           messagePump (void * arg);
	static LRESULT CALLBACK windowProcedure (HWND window, UINT message, WPARAM wParam, LPARAM lParam);
	static WNDCLASSEX windowClass;
	static ATOM       windowClassID;
  };
}

#else

#  include "fl/X.h"

namespace fl
{
  class SlideShow : public Window
  {
  public:
	SlideShow ();
	~SlideShow ();

	virtual bool processEvent (XEvent & event);

	void show (const Image & image, int centerX = 0, int centerY = 0);  ///< Start displaying image.  Ensure that the point (centerX, centerY) is in the displayable area.
	void waitForClick ();
	void stopWaiting ();  ///< Releases all threads waiting on this window.
	void paint ();

	fl::Visual *       visual;
	fl::Colormap *     colormap;
	fl::GC *           gc;
	Image              image;
	XImage *           ximage;
	pthread_mutex_t    mutexImage;
	Atom               WM_DELETE_WINDOW;
	Atom               WM_PROTOCOLS;  ///< For some reason, this isn't defined in Xatom.h

	bool               modeDrag;  ///< Indicates that there was motion between button down and button up
	int                lastX;  ///< Where the last button event occurred
	int                lastY;
	int                offsetX; ///< Position in image where window starts
	int                offsetY;
	int                width;   ///< Current size of window
	int                height;

	pthread_mutex_t    waitingMutex;
	pthread_cond_t     waitingCondition;
  };
}

#endif


#endif
