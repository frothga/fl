/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/02/25 14:46:37  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/09 04:04:47  Fred
Put UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:38:35  Fred
Add UIUC copyright notice.

Revision 1.2  2004/07/22 15:18:12  rothgang
Probe for double-buffering first.  Workaround (?) for newer XFree86 servers.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/glx.h"


using namespace fl;
using namespace std;


// class GLXContext -----------------------------------------------------------

fl::GLXContext::GLXContext (fl::Screen * screen)
{
  if (! screen)
  {
	screen = & fl::Display::getPrimary ()->defaultScreen ();
  }
  this->screen = screen;

  // Determine a visual
  doubleBuffer = true;
  int attributesDoubleBuffer[] =
  {
	GLX_RGBA,
	GLX_DOUBLEBUFFER,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	None
  };
  XVisualInfo * vinfo = glXChooseVisual (screen->display->display, screen->number, attributesDoubleBuffer);
  if (! vinfo)
  {
	doubleBuffer = false;
	int attributesSingleBuffer[] =
	{
	  GLX_RGBA,
	  GLX_RED_SIZE, 1,
	  GLX_GREEN_SIZE, 1,
	  GLX_BLUE_SIZE, 1,
	  None
	};
	vinfo = glXChooseVisual (screen->display->display, screen->number, attributesSingleBuffer);
	if (! vinfo)
	{
	  throw "Can't find a visual";
	}
  }

  // Finish creating context
  context = glXCreateContext (screen->display->display, vinfo, NULL, true);
}

fl::GLXContext::~GLXContext ()
{
  glXDestroyContext (screen->display->display, context);
}

bool
fl::GLXContext::isDirect () const
{
  return glXIsDirect (screen->display->display, context);
}


// class GLXDrawable ----------------------------------------------------------

void
fl::GLXDrawable::makeCurrent (fl::GLXContext & context) const
{
  glXMakeCurrent (screen->display->display, id, context.context);
}

void
fl::GLXDrawable::swapBuffers () const
{
  glXSwapBuffers (screen->display->display, id);
}


// class GLXWindow ------------------------------------------------------------
