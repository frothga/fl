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
  doubleBuffer = false;
  int attributesSingleBuffer[] =
  {
	GLX_RGBA,
	GLX_RED_SIZE, 1,
	GLX_GREEN_SIZE, 1,
	GLX_BLUE_SIZE, 1,
	None
  };
  XVisualInfo * vinfo = glXChooseVisual (screen->display->display, screen->number, attributesSingleBuffer);
cerr << "vinfo = " << vinfo << endl;
  if (! vinfo)
  {
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
	vinfo = glXChooseVisual (screen->display->display, screen->number, attributesDoubleBuffer);
	if (! vinfo)
	{
	  throw "Can't find a visual";
	}
  }
cerr << "vinfo = " << vinfo << endl;
cerr << "  visual = " << vinfo->visual << endl;
cerr << "      id = " << hex << vinfo->visualid << dec << endl;
cerr << "   depth = " << vinfo->depth << endl;
cerr << "  colormap_size = " << vinfo->colormap_size << endl;
cerr << "  bits_per_rgb = " << vinfo->bits_per_rgb << endl;

  // Finish creating context
  context = glXCreateContext (screen->display->display, vinfo, NULL, true);
cerr << "context = " << context << endl;
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
  cerr << "make current: " << screen->display->display << " " << hex << id << dec << " " << context.context << endl;
  glXMakeCurrent (screen->display->display, id, context.context);
}

void
fl::GLXDrawable::swapBuffers () const
{
  glXSwapBuffers (screen->display->display, id);
}


// class GLXWindow ------------------------------------------------------------
