/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Revised by Fred Rothganger
*/


#ifndef fl_time_h
#define fl_time_h


#include <ostream>

#ifdef WIN32
#  include <windows.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif

namespace fl
{
  inline double
  getTimestamp ()
  {
#   ifdef WIN32

	LARGE_INTEGER frequency;
	LARGE_INTEGER count;
	QueryPerformanceFrequency (&frequency);
	QueryPerformanceCounter (&count);
	return (double) count.QuadPart / frequency.QuadPart;

#   else

	struct timeval t;
	gettimeofday (&t, NULL);
	return t.tv_sec + (double) t.tv_usec / 1e6;

#   endif
  }

  /**
	 Like a stopwatch, this class accumulates time as long as it is
	 "running", and it can be paused.  It starts running the moment
	 it is created.  In addition to stopping and starting, it can
	 also clear its accumulated time and start from zero again.
  **/
  class Stopwatch
  {
  public:
	Stopwatch ()
	{
	  reset ();
	}

	/**
	   Clears accumulated time and starts running.
	 **/
	void reset ()
	{
	  accumulator = 0;
	  timestamp = getTimestamp ();
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
