#ifndef time_h
#define time_h


#include <ostream>

#include <sys/time.h>
#include <unistd.h>


namespace fl
{
  inline double
  getTimestamp ()
  {
	struct timeval t;
	gettimeofday (&t, NULL);
	return t.tv_sec + (double) t.tv_usec / 1e6;
  }

  // Like a stopwatch, this class can accumulate time from several separate
  // events.
  // Note that each method, regardless of its purpose, moves the internal
  // timestamp forward to the current point in time.
  class Stopwatch
  {
  public:
	Stopwatch ()
	{
	  reset ();
	}

	void reset ()
	{
	  total = 0;
	  timestamp = getTimestamp ();
	}

	void start ()
	{
	  timestamp = getTimestamp ();
	}

	// stop() updates total time and then effectively calls start()
	void stop ()
	{
	  double current = getTimestamp ();
	  total += current - timestamp;
	  timestamp = current;
	}

	double total;
	double timestamp;
  };

  inline std::ostream &
  operator << (std::ostream & stream, const Stopwatch & stopwatch)
  {
	stream << stopwatch.total;
	return stream;
  }
}

#endif
