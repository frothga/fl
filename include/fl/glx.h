/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#ifndef fl_glx_h
#define fl_glx_h


#include "fl/X.h"

#include <GL/glx.h>


namespace fl
{
  class GLXContext  // Is there an XID associated with this?  If so, it should subclass Resource.
  {
  public:
	GLXContext (fl::Screen * screen = NULL);  // Chooses a default visual.  If screen is not specified, uses default screen.
	virtual ~GLXContext ();

	bool isDirect () const;

	fl::Screen * screen;
	::GLXContext context;
	bool         doubleBuffer;
  };

  class GLXDrawable : public virtual Drawable
  {
  public:
	void makeCurrent (fl::GLXContext & context) const;
	void swapBuffers () const;
  };

  class GLXWindow : public Window, public GLXDrawable
  {
  public:
	GLXWindow () : fl::Window (fl::Display::getPrimary ()->defaultScreen ()) {};
	GLXWindow (fl::Window & parent, int width = 100, int height = 100, int x = 0, int y = 0) : fl::Window (parent, width, height, x, y) {};
	GLXWindow (fl::Screen & screen, int width = 100, int height = 100, int x = 0, int y = 0) : fl::Window (screen, width, height, x, y) {};
  };
}


#endif
