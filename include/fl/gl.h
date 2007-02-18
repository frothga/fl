/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
