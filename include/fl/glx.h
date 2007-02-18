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
