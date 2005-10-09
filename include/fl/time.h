/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC
05/2005 Fred Rothganger -- Use hires timer under MS Windows
08/2005 Fred Rothganger -- Reorder hires functions to read time ASAP
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_time_h
#define fl_time_h


#include <ostream>

#ifdef WIN32
#  include <windows.h>
#  undef min
#  undef max
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
