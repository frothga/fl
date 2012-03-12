/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_thread_h
#define fl_thread_h


#include <pthread.h>
#include <vector>

#ifdef WIN32
#  define _WINSOCKAPI_
#  define NOMINMAX
#  include <windows.h>
#else
#  include <unistd.h>
#endif


namespace fl
{
  /**
	 Dispatches units of work to a number of threads.  All work units are of
	 the same type, and can be anything from a range of integers to a range
	 of objects stored in an STL collection.
	 To use this class, you must:
	 <ul>
	 <li>Override the abstract virtual function process() to do the actual work.
	 <li>Call run() with a range of values to iterate over.
	 </ul>
	 The derived class is also a good place to store data shared by all
	 threads.
   **/
  template<class I>
  class ParallelFor
  {
  public:
	ParallelFor ()
	{
	  pthread_mutex_init (&mutexI, 0);
	}

	~ParallelFor ()
	{
	  pthread_mutex_destroy (&mutexI);
	}

	void run (I start, I end, int threadCount = 0)
	{
	  if (threadCount < 1)
	  {
#       ifdef WIN32
		SYSTEM_INFO info;
		GetSystemInfo (&info);
		threadCount = info.dwNumberOfProcessors;
#       elif defined(linux)
		threadCount = sysconf (_SC_NPROCESSORS_ONLN);
#       else
#       error Need a processor-count function specific to this platform.
#       endif
	  }

	  i = start;
	  this->end = end;
	  std::vector<pthread_t> threads (threadCount);
	  for (int t = 0; t < threadCount; t++) pthread_create (&threads[t], 0, threadMain, this);
	  for (int t = 0; t < threadCount; t++) pthread_join   ( threads[t], 0);
	}

	static void * threadMain (void * arg)
	{
	  ParallelFor * me = (ParallelFor *) arg;
	  while (true)
	  {
		pthread_mutex_lock   (& me->mutexI);
		I i = me->i;
		if (me->i != me->end) me->i++;
		pthread_mutex_unlock (& me->mutexI);
		if (i == me->end) return 0;
		me->process (i);
	  }
	}

	virtual void process (I & i) = 0;

	I i;
	I end;
	pthread_mutex_t mutexI;
  };
}


#endif
