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
