#ifndef slideshow_h
#define slideshow_h


#include "fl/X.h"
#include "fl/image.h"


namespace fl
{
  class SlideShow : public Window
  {
  public:
	SlideShow ();  // Figure out everything automatically
	~SlideShow ();

	virtual bool processEvent (XEvent & event);

	void show (const Image & image, int centerX = 0, int centerY = 0);  // Start displaying image.  Ensure that the point (centerX, centerY) is in the displayable area.
	void waitForClick ();
	void paint ();

	//int                depth;
	fl::Visual *       visual;
	//PixelFormatRGBBits format;
	fl::Colormap *     colormap;
	fl::GC *           gc;
	bool               waitingForClick;
	Image              image;
	XImage *           ximage;
	pthread_mutex_t    mutexImage;
	Atom               WM_DELETE_WINDOW;
	Atom               WM_PROTOCOLS;  // For some reason, this isn't defined in Xatom.h

	bool               modeDrag;  // Indicates that there was motion between button down and button up
	int                lastX;  // Where the last button event occurred
	int                lastY;
	int                offsetX; // Position in image where window starts
	int                offsetY;
	int                width;   // Current size of window
	int                height;
  };
}


#endif
