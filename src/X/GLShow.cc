/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/02/25 14:46:37  Fred
Use CVS Log to generate revision history.

Revision 1.3  2005/10/09 04:04:47  Fred
Put UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/04/23 19:38:35  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/gl.h"


using namespace fl;
using namespace std;


// class GLShow ---------------------------------------------------------------

GLShow::GLShow (int width, int height)
: GLXWindow (fl::Display::getPrimary ()->defaultScreen (), width, height)
{
  contextInitialized = false;

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

  pthread_mutex_init (&waitingMutex, NULL);
  pthread_cond_init (&waitingCondition, NULL);
}

GLShow::~GLShow ()
{
  unmap ();

  // When this destructor is called, either we have already released waiting
  // threads due to WM_DESTROY_WINDOW or this object is being destroyed
  // directly.  If we are being destroyed directly by some thread, we should
  // release any waiting threads first.
  stopWaiting ();

  pthread_cond_destroy (&waitingCondition);
  pthread_mutex_destroy (&waitingMutex);
}

bool
GLShow::processEvent (XEvent & event)
{
  switch (event.type)
  {
    case Expose:
	{
	  // Ignore this event if there is another expose event in the queue.
	  // It is possible to achieve a similar effect (ie: ignoring multiple
	  // exposes) by checking the count field of the event structure.
	  // However, this approach is more general.
	  XEvent peek;
	  if (checkTypedEvent (peek, Expose))
	  {
		screen->display->putBackEvent (peek);
	  }
	  else
	  {
		display ();
	  }
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
	  if (! contextInitialized)
	  {
		makeCurrent (context);  // We only have to bind the context once on this event thread.
		initContext ();
		contextInitialized = true;
	  }
	  reshape (event.xconfigure.width, event.xconfigure.height);
	  return true;
	}
    case ButtonPress:
	{
	  dragMode = false;
	  lastX = event.xbutton.x;
	  lastY = event.xbutton.y;
	  return true;
	}
    case ButtonRelease:
	{
	  if (! dragMode)
	  {
		click (event.xbutton.x, event.xbutton.y, event.xbutton.state);
	  }
	  return true;
	}
    case MotionNotify:
	{
	  dragMode = true;
	  drag (event.xbutton.x, event.xbutton.y, event.xmotion.state);
	  lastX = event.xbutton.x;
	  lastY = event.xbutton.y;
	  return true;
	}
    case KeyPress:
	{
	  KeySym keysym = XLookupKeysym ((XKeyEvent *) &event, event.xkey.state);
	  keyboard (keysym);
	  return true;
	}
  }

  return fl::Window::processEvent (event);
}

void
GLShow::waitForClose ()
{
  // Put thread to sleep.  Wake up on WM_DELETE_WINDOW, which casues call to
  // stopWaiting ().
  pthread_mutex_lock (&waitingMutex);
  pthread_cond_wait (&waitingCondition, &waitingMutex);
  pthread_mutex_unlock (&waitingMutex);
}

void
GLShow::stopWaiting ()
{
  // Release waiting threads
  pthread_mutex_lock (&waitingMutex);
  pthread_cond_broadcast (&waitingCondition);
  pthread_mutex_unlock (&waitingMutex);
}

void
GLShow::initContext ()
{
  // Do nothing
}

void
GLShow::reshape (int width, int height)
{
  // Do nothing
}

void
GLShow::display ()
{
  // Do nothing
}

void
GLShow::drag (int toX, int toY, unsigned int state)
{
  // Do nothing
}

void
GLShow::click (int x, int y, unsigned int state)
{
  // Do nothing
}

void
GLShow::keyboard (KeySym keysym)
{
  // Do nothing
}

