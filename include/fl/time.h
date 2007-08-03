/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3, 1.5 and 1.6 Copyright 2005 Sandia Corporation.
Revisions 1.8 thru 1.13    Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.13  2007/08/03 04:52:58  Fred
Update Sandia copyright notice.

Revision 1.12  2007/08/03 04:51:06  Fred
Enable hires counter under Cygwin.

Revision 1.11  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.10  2006/02/14 06:08:36  Fred
Add low-resolution sleep function to mimic the posix version.

Revision 1.9  2006/01/22 05:22:38  Fred
Add comment to revision history.

Revision 1.8  2006/01/22 05:21:34  Fred
Change semantics of reset() slightly, so that it will only start the watch if
it was alreay running.

Revision 1.7  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 03:34:25  Fred
Update revision history and add Sandia copyright notice.

Suppress min and max macros dragged in from somewhere by windows.h

Rearrange calls in Windows version of getTimestamp() so it reads the current
counter before the frequency.  This is in keeping with the policy that we read
the current time as close as possible to the start of the function.

Revision 1.5  2005/05/29 21:49:39  Fred
Use hi-res timers for Windows implementation of getTimestamp().

Fix comments on Stopwatch to correctly describe current behavior.

Revision 1.4  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 20:57:18  Fred
MSVC compilability fix:  Use available time function for getting (not quite so)
fine grained time under Win32.  Look into using QueryPerformanceCounter() and
QueryPerformanceFrequency() to get finer timing.

Revision 1.2  2004/06/22 21:25:21  rothgang
Change interface slightly so that one doesn't have to stop the timer to read
total time to present.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_time_h
#define fl_time_h


#include <ostream>

#if defined(WIN32) || defined(__CYGWIN__)
#  include <windows.h>
#  undef min
#  undef max
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif


namespace fl
{
# ifdef WIN32
  inline int
  sleep (unsigned int seconds)
  {
	Sleep (seconds * 1000);
	return 0;
  }
# endif

  inline double
  getTimestamp ()
  {
#   if defined(WIN32) || defined(__CYGWIN__)

	LARGE_INTEGER count;
	LARGE_INTEGER frequency;
	QueryPerformanceCounter (&count);
	QueryPerformanceFrequency (&frequency);
	return (double) count.QuadPart / frequency.QuadPart;

#   else

	struct timeval t;
	gettimeofday (&t, NULL);
	return t.tv_sec + (double) t.tv_usec / 1e6;

#   endif
  }

  /**
	 Like a stopwatch, this class accumulates time as long as it is
	 "running", and it can be paused.  By default, it starts running the
	 moment it is created.  In addition to stopping and starting, it can
	 also clear its accumulated time and start from zero again.
  **/
  class Stopwatch
  {
  public:
	/**
	   /param run Indicates that we should start accumulating time immediately.
	 **/
	Stopwatch (bool run = true)
	{
	  accumulator = 0;
	  if (run)
	  {
		timestamp = getTimestamp ();
	  }
	  else
	  {
		timestamp = 0;
	  }
	}

	/**
	   Clears accumulated time.
	 **/
	void reset ()
	{
	  accumulator = 0;
	  if (timestamp) timestamp = getTimestamp ();  // Only start if already running before call to this function.
	}

	/**
	   Sets the beginning point for measuring a period of time.  If this
	   stopwatch is already running, then this method throws away all time
	   since that most recent start, but retains any time accumulated before
	   that start.
	 **/
	void start ()
	{
	  timestamp = getTimestamp ();
	}

	/**
	   Adds the current time period to total time, and then pauses the timer.
	   You must call start() to begin measuring an additional time period.
	**/
	void stop ()
	{
	  double current = getTimestamp ();
	  if (timestamp)
	  {
		accumulator += current - timestamp;
	  }
	  timestamp = 0;
	}

	double total () const
	{
	  double current = getTimestamp ();
	  if (timestamp)
	  {
		return accumulator + current - timestamp;
	  }
	  else
	  {
		return accumulator;
	  }
	}

	double accumulator;
	double timestamp;
  };

  inline std::ostream &
  operator << (std::ostream & stream, const Stopwatch & stopwatch)
  {
	stream << stopwatch.total ();
	return stream;
  }
}

#endif
