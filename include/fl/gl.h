/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
*/


#ifndef fl_gl_h
#define fl_gl_h


#include "fl/glx.h"

// This header does not directly require keysym.h, but it makes using
// GLShow easier for the developer.
#include <X11/keysym.h>


namespace fl
{
  class GLShow : public GLXWindow
  {
  public:
	GLShow (int width = 400, int height = 400);  // Figure out everything automatically
	~GLShow ();

	virtual bool processEvent (XEvent & event);

	void waitForClose ();  // Current thread goes to sleep, and returns once this winodw is destroyed.
	void stopWaiting ();  // Releases all threads waiting on this window.  Presumably one of these threads will then destroy this object.

	virtual void initContext ();  // One-time initialization of GL context.
	virtual void reshape (int width, int height);  // Called when window is first constructed and each time its shape changes.
	virtual void display ();  // Called when part of the window needs to be repainted.
	virtual void drag (int toX, int toY, unsigned int state);  // Called during mouse drags.
	virtual void click (int x, int y, unsigned int state);  // Called when mouse button releases w/o intervening drag.
	virtual void keyboard (KeySym keysym);  // Called when a keystroke occurs.


	fl::GLXContext     context;
	bool               contextInitialized;

	Atom               WM_DELETE_WINDOW;
	Atom               WM_PROTOCOLS;  // For some reason, this isn't defined in Xatom.h

	bool               dragMode;
	int                lastX;  // Where the last button event occurred
	int                lastY;

	pthread_mutex_t    waitingMutex;
	pthread_cond_t     waitingCondition;
  };
}


#endif
