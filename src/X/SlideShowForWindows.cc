/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/

#include "fl/slideshow.h"

// For debugging
#include <iostream>


using namespace std;
using namespace fl;


WNDCLASSEX SlideShow::windowClass;
ATOM       SlideShow::windowClassID = 0;

SlideShow::SlideShow ()
{
	if (windowClassID == 0)
	{
		windowClass.cbSize         = sizeof (windowClass);
		windowClass.style          = 0; //CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc    = windowProcedure;
		windowClass.cbClsExtra     = 0;
		windowClass.cbWndExtra     = sizeof (LONG_PTR);  // for a pointer to the instance of SlideShow
		windowClass.hInstance      = (HINSTANCE) GetModuleHandle (NULL);
		windowClass.hIcon          = 0;
		windowClass.hCursor        = LoadCursor (NULL, IDC_ARROW);
		windowClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
		windowClass.lpszMenuName   = 0;
		windowClass.lpszClassName  = "SlideShow";
		windowClass.hIconSm        = 0;
		windowClassID = RegisterClassEx (&windowClass);
	}

	image = 0;

	modeDrag = false;
	offsetX  = 0;
	offsetY  = 0;
	width    = 640;
	height   = 480;

	pthread_mutex_init (&mutexImage,       0);
	pthread_mutex_init (&waitingMutex,     0);
	pthread_cond_init  (&waitingCondition, 0);

	if (pthread_create (&messagePumpThread, 0, messagePump, this))
	{
		throw "Unable to start message pump";
	}

	pthread_mutex_lock   (&waitingMutex);
	pthread_cond_wait    (&waitingCondition, &waitingMutex);
	pthread_mutex_unlock (&waitingMutex);
}

SlideShow::~SlideShow ()
{
	stop = true;
	void * result;
	pthread_join (messagePumpThread, &result);

	pthread_cond_destroy  (&waitingCondition);
	pthread_mutex_destroy (&waitingMutex);
	pthread_mutex_destroy (&mutexImage);

	if (image) DeleteObject (image);
}

void
SlideShow::show (const Image & image, int centerX, int centerY)
{
	// Convert and store the image
	HDC windowDC = GetDC (window);
	if (! windowDC) throw "Failed to get DC";

	Image temp = image * BGRChar4;
	BITMAPINFO bmi;
	bmi.bmiHeader.biSize          = sizeof (bmi.bmiHeader);
	bmi.bmiHeader.biWidth         =   image.width;
	bmi.bmiHeader.biHeight        = - image.height;
	bmi.bmiHeader.biPlanes        = 1;
	bmi.bmiHeader.biBitCount      = 32;
	bmi.bmiHeader.biCompression   = BI_RGB;
	bmi.bmiHeader.biSizeImage     = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed       = 0;
	bmi.bmiHeader.biClrImportant  = 0;

	pthread_mutex_lock   (&mutexImage);
	if (this->image) DeleteObject (this->image);
	this->image = CreateDIBitmap
	(
		windowDC,
		&bmi.bmiHeader,
		CBM_INIT,
		(void *) ((PixelBufferPacked *) temp.buffer)->memory,
		&bmi,
		DIB_RGB_COLORS
	);
	pthread_mutex_unlock (&mutexImage);

	ReleaseDC (window, windowDC);

	// Determine initial offset (position where window looks on the image)
	if (centerX < offsetX  ||  centerX > offsetX + width  ||  centerY < offsetY   ||  centerY > offsetY + height)
	{
		offsetX = centerX - width / 2;
		offsetX = min (offsetX, image.width - width);
		offsetX = max (offsetX, 0);

		offsetY = centerY - height / 2;
		offsetY = min (offsetY, image.height - height);
		offsetY = max (offsetY, 0);
	}

	// Display window
	ShowWindowAsync (window, SW_SHOWNORMAL);
	UpdateWindow (window);
}

void
SlideShow::waitForClick ()
{
	pthread_mutex_lock   (&waitingMutex);
	pthread_cond_wait    (&waitingCondition, &waitingMutex);
	pthread_mutex_unlock (&waitingMutex);
}

void
SlideShow::stopWaiting ()
{
	pthread_mutex_lock     (&waitingMutex);
	pthread_cond_broadcast (&waitingCondition);
	pthread_mutex_unlock   (&waitingMutex);
}

void *
SlideShow::messagePump (void * arg)
{
	SlideShow * me = (SlideShow *) arg;

	// Create window
	me->window = CreateWindow
	(
		"SlideShow",
		"FL SlideShow",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		0,
		0,
		windowClass.hInstance,
		0
	);
	me->stopWaiting ();
	if (! me->window)
	{
		cerr << "Couldn't create window" << endl;
		return (void *) 1;
	}
	SetWindowLongPtr (me->window, 0, (LONG_PTR) me);

	// Message pump
	me->stop = false;
	MSG message;
	while (! me->stop  &&  GetMessage (&message, 0, 0, 0))
	{
		TranslateMessage (&message);
		DispatchMessage (&message);
	}

	DestroyWindow (me->window);
	me->stopWaiting ();
	return (void *) message.wParam;
}

LRESULT CALLBACK
SlideShow::windowProcedure (HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	SlideShow * me = (SlideShow *) GetWindowLongPtr (window, 0);

	switch (message)
	{
		case WM_SIZE:
			me->width  = LOWORD (lParam);
			me->height = HIWORD (lParam);
			break;
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC windowDC = BeginPaint (window, &ps);
			HDC imageDC = CreateCompatibleDC (windowDC);

			pthread_mutex_lock (& me->mutexImage);
			HGDIOBJ originalBMP = SelectObject (imageDC, me->image);
			BitBlt
			(
				windowDC,
				0, 0,
				me->width, me->height,
				imageDC,
				me->offsetX, me->offsetY,
				SRCCOPY
			);
			SelectObject (imageDC, originalBMP);
			pthread_mutex_unlock (& me->mutexImage);

			DeleteDC (imageDC);
			EndPaint (window, &ps);
			break;
		}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_XBUTTONDOWN:
			me->modeDrag = false;
			me->lastX = LOWORD (lParam);
			me->lastY = HIWORD (lParam);
			break;
		case WM_MOUSEMOVE:
		{
			if (wParam & (MK_LBUTTON | MK_MBUTTON | MK_RBUTTON | MK_XBUTTON1 | MK_XBUTTON2))
			{
				me->modeDrag = true;
				int deltaX = LOWORD (lParam) - me->lastX;
				int deltaY = HIWORD (lParam) - me->lastY;
				me->lastX  = LOWORD (lParam);
				me->lastY  = HIWORD (lParam);

				BITMAP bmp;
				GetObject (me->image, sizeof (bmp), &bmp);

				deltaX = min (deltaX, (int) bmp.bmWidth - me->width - me->offsetX);
				deltaX = max (deltaX, - me->offsetX);
				me->offsetX += deltaX;

				deltaY = min (deltaY, (int) bmp.bmHeight - me->height - me->offsetY);
				deltaY = max (deltaY, - me->offsetY);
				me->offsetY += deltaY;

				InvalidateRect (window, 0, 0);
			}
			break;
		}
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
		case WM_XBUTTONUP:
			if (! me->modeDrag) me->stopWaiting ();
			break;
		case WM_CLOSE:
		case WM_CHAR:
			me->stopWaiting ();
			break;
		case WM_DESTROY:
			PostQuitMessage (0);
			break;
		default:
			return DefWindowProc (window, message, wParam, lParam);
	}

	return 0;
}
