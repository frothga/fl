/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
# if defined(WIN32)  &&  ! defined(__CYGWIN__)
  inline int
  sleep (unsigned int seconds)
  {
	Sleep (seconds * 1000);
	return 0;
  }
# endif

  /**
	 Read time at highest available resolution.  In most cases this is
	 wall-clock time, but under Linux it can be switched to other timing
	 sources by defining CLOCK_SOURCE before loading this header file.
	 A key side-effect of this is to modify the behavior of Stopwatch.
	 Some common clock sources under Linux:
	 <dl>
	 <dt>CLOCK_REALTIME
	     <dd>System-wide realtime clock.
	 <dt>CLOCK_MONOTONIC
	     <dd>Represents monotonic time since some unspecified starting point.
		 (Presumably not affected by changin system time.)
	 <dt>CLOCK_PROCESS_CPUTIME_ID
	     <dd>High-resolution per-process timer from the CPU.
	 <dt>CLOCK_THREAD_CPUTIME_ID
	     <dd>Thread-specific CPU-time clock.
	 </dl>
   **/
  inline double
  getTimestamp ()
  {
#   if defined(WIN32) || defined(__CYGWIN__)

	LARGE_INTEGER count;
	LARGE_INTEGER frequency;
	QueryPerformanceCounter (&count);
	QueryPerformanceFrequency (&frequency);
	return (double) count.QuadPart / frequency.QuadPart;

#   elif defined(linux)

#     ifndef CLOCK_SOURCE
#     define CLOCK_SOURCE CLOCK_MONOTONIC
#     endif

	struct timespec t;
	clock_gettime (CLOCK_SOURCE, &t);
	return t.tv_sec + (double) t.tv_nsec / 1e9;

#   else   // other posix systems

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
