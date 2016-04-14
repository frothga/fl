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


#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


namespace fl
{
  inline int hardwareThreads ()
  {
#   ifdef _SC_NPROCESSORS_ONLN
	return sysconf (_SC_NPROCESSORS_ONLN);
#   else
	return std::thread::hardware_concurrency ();
#   endif
  }

  /**
	 Dispatches units of work to a number of threads. All work units are of
	 the same type, and can be anything from a range of integers to a range
	 of objects stored in an STL collection.
	 To use this class, you must:
	 <ul>
	 <li>Override the abstract virtual function process() to do the actual work.
	 <li>Call run() with a range of values to iterate over.
	 </ul>
	 The derived class is a good place to store data shared by all threads.
  **/
  template<typename I>
  class ParallelFor
  {
  public:
	/**
	   @param threadRequest The number of threads to use. If zero, then use
	   100% of hardware threads. If integer, use exactly the requested number.
	   If non-integer, treat the request as a percentage of hardware threads.
	**/
	ParallelFor (float threadRequest = 0)
	{
	  threadCount = (int) threadRequest;
	  if (threadRequest == 0  ||  threadRequest != threadCount)  // Need to calculate targetThreads
	  {
		if (threadRequest == 0) threadCount = hardwareThreads ();
		else                    threadCount = (int) ceil (hardwareThreads () * threadRequest);
		if (threadCount < 1) threadCount = 1;
	  }

	  done = false;
	}

	virtual ~ParallelFor ()
	{
	  // Signal all threads that we're done.
	  mutexI.lock ();  // ensures proper timing of condition variable
	  done = true;
	  mutexI.unlock ();
	  conditionI.notify_all ();
	  
	  for (int t = 0; t < threads.size (); t++) threads[t].join ();
	}

	void run (const I startAt, const I stopBefore)
	{
	  // Store new range to work on
	  std::unique_lock<std::mutex> lock (mutexI);
	  i   = startAt;
	  end = stopBefore;
	  lock.unlock ();
	  conditionI.notify_all ();

	  // Create threads if needed
	  if (threads.size () == 0)
	  {
		active = threadCount;
		threads.reserve (threadCount);
		for (int t = 0; t < threadCount; t++) threads.emplace_back ([this]
		{
		  // Outer loop handles barrier between calls to run()
		  while (true)
		  {
			// Block until a job is available, or the object is destructing.
			std::unique_lock<std::mutex> lock (mutexI);
			while (i == end  &&  ! done)
			{
			  if (--active == 0) conditionActive.notify_all ();
			  conditionI.wait (lock);
			  active++;
			}
			if (done) return;  // Terminate this thread.

			// Inner loop fetches job indices and processes them
			// This version will work correctly with iterators over linked lists.
			while (i != end)
			{
			  I current = i++;
			  lock.unlock ();
			  process (current);
			  lock.lock ();
			}
		  }
		});
	  }

	  // Block until all work is completed
	  lock.lock ();
	  conditionActive.wait (lock, [this]{return i == this->end  &&  active == 0;});
	}

	virtual void process (const I i) = 0;

	int                      threadCount;
	std::vector<std::thread> threads;
	std::mutex               mutexI;  ///< Don't use this for anything else! Create additional mutexes to guard any other shared data in derived classes.
	std::condition_variable  conditionI;
	std::condition_variable  conditionActive;
	bool                     done;    ///< Indicates that we are destructing, so threads in pool can exit.
	uint32_t                 active;  ///< Number of threads that are actively doing work. Must drop to zero before run() returns.
	I                        i;
	I                        end;
  };

  /**
	 Specialization of ParallelFor to take advantage of additional efficiency
	 afforded by int. It is necessary to repeat the entire class in order to
	 change i to atomic_int.
  **/
  template<>
  class ParallelFor<int>
  {
  public:
	ParallelFor (float threadRequest = 0)
	{
	  int threadCount = (int) threadRequest;
	  if (threadRequest == 0  ||  threadRequest != threadCount)
	  {
		if (threadRequest == 0) threadCount = hardwareThreads ();
		else                    threadCount = (int) ceil (hardwareThreads () * threadRequest);
		if (threadCount < 1) threadCount = 1;
	  }

	  i = 0;
	  end = 0;
	  done = false;
	  active = threadCount;
	  threads.reserve (threadCount);
	  for (int t = 0; t < threadCount; t++) threads.emplace_back ([this]
	  {
		// Outer loop handles barrier between calls to run()
		while (true)
		{
		  // Block until a job is available, or the object is destructing.
		  {
			std::unique_lock<std::mutex> lock (mutexI);
			while (i >= end  &&  ! done)
			{
			  if (--active == 0) conditionActive.notify_all ();
			  conditionI.wait (lock);
			  active++;
			}
			if (done) return;  // Terminate this thread.
		  }

		  // Inner loop fetches job indices and processes them
		  while (true)
		  {
			int current = i++;
			if (current >= end) break;
			process (current);
		  }
		}
	  });
	}

	virtual ~ParallelFor ()
	{
	  mutexI.lock ();
	  done = true;
	  mutexI.unlock ();
	  conditionI.notify_all ();
	  
	  for (int t = 0; t < threads.size (); t++) threads[t].join ();
	}

	void run (const int startAt, const int stopBefore)
	{
	  // Store new range to work on
	  std::unique_lock<std::mutex> lock (mutexI);
	  i   = startAt;
	  end = stopBefore;
	  lock.unlock ();
	  conditionI.notify_all ();

	  // Block until all work is completed
	  lock.lock ();
	  conditionActive.wait (lock, [this]{return i >= this->end  &&  active == 0;});
	}

	virtual void process (const int i) = 0;

	std::vector<std::thread> threads;
	std::mutex               mutexI;  ///< Don't use this for anything else! Create additional mutexes to guard any other shared data in derived classes.
	std::condition_variable  conditionI;
	std::condition_variable  conditionActive;
	bool                     done;    ///< Indicates that we are destructing, so threads in pool can exit.
	uint32_t                 active;  ///< Number of threads that are actively doing work. Must drop to zero before run() returns.
	std::atomic_int          i;
	int                      end;
  };
}


#endif
