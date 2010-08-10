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
}

void
SlideShow::show (const Image & image, int centerX, int centerY)
{
	// Display window
	ShowWindowAsync (window, SW_SHOWNORMAL);
	UpdateWindow (window);  // force an immediate paint
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

void
SlideShow::clear ()
{
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
		case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint (window, &ps);
			// TODO: Add any drawing code here...
			EndPaint (window, &ps);
			break;
		}
		case WM_DESTROY:
			PostQuitMessage (0);
			break;
		default:
			return DefWindowProc (window, message, wParam, lParam);
	}

	return 0;
}
