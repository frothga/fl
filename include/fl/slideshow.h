/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.4 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 03:47:49  Fred
Add stub for Windows version, and with revision history and Sandia copyright
notice.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2004/08/30 00:03:26  rothgang
Update comments to Doxygen format.

Use pthreads to handle waiting, rather than spinning.

Remove dead member variables.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef slideshow_h
#define slideshow_h


#ifndef _MSC_VER
#include "fl/X.h"
#endif

#include "fl/image.h"


namespace fl
{
#ifdef _MSC_VER

  class SlideShow
  {
  public:
	SlideShow () {}

	void show (const Image & image, int centerX = 0, int centerY = 0) {}
	void waitForClick () {}
	void clear () {}
  };

#else

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

#endif
}


#endif
