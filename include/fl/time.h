/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_time_h
#define fl_time_h


#if __GLIBCXX__ <= 20120305
#  define _GLIBCXX_USE_NANOSLEEP
#endif

#include <ostream>
#include <thread>

#if defined(WIN32) || defined(__CYGWIN__)
#  define _WINSOCKAPI_
#  define NOMINMAX
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

#ifdef WIN32
#  include <time.h>
#  define localtime_r(A,B) localtime_s(B,A)
#endif

#undef SHARED
#ifdef _MSC_VER
#  ifdef flBase_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  inline void sleep (double seconds)
  {
	std::this_thread::sleep_for (std::chrono::duration<double> (seconds));
  }

  SHARED double ClockRealtime  ();  ///< @return Number of seconds since epoch, as defined by the OS. (Unix=1970/1/1. Windows=1601/1/1.)
  SHARED double ClockMonotonic ();  ///< @return A time value (in seconds) that never goes backward
  SHARED double ClockProcess   ();  ///< @return Amount of time (in seconds) that this process (all threads added together) has spent in the CPU. Includes both kernel and user time.
  SHARED double ClockThread    ();  ///< @return Amount of time (in seconds) that this thread has spent in the CPU. Includes both kernel and user time.

  inline double
  getTimestamp ()
  {
	return ClockMonotonic ();
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
	Stopwatch (bool run = true, double (* clock) () = ClockMonotonic)
	: clock (clock)
	{
	  accumulator = 0;
	  if (run)
	  {
		timestamp = (*clock) ();
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
	  if (timestamp) timestamp = (*clock) ();  // Only start if already running before call to this function.
	}

	/**
	   Sets the beginning point for measuring a period of time.  If this
	   stopwatch is already running, then this method throws away all time
	   since that most recent start, but retains any time accumulated before
	   that start.
	 **/
	void start ()
	{
	  timestamp = (*clock) ();
	}

	/**
	   Adds the current time period to total time, and then pauses the timer.
	   You must call start() to begin measuring an additional time period.
	**/
	void stop ()
	{
	  double current = (*clock) ();
	  if (timestamp)
	  {
		accumulator += current - timestamp;
	  }
	  timestamp = 0;
	}

	double total () const
	{
	  double current = (*clock) ();
	  if (timestamp)
	  {
		return accumulator + current - timestamp;
	  }
	  else
	  {
		return accumulator;
	  }
	}

	double (* clock) ();
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
