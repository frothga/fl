/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/time.h"

#include <chrono>


using namespace fl;
using namespace std;


// Clock functions ------------------------------------------------------------

// Why keep native implementations of these, rather than relying entirely on
// std::chrono? Because this way we are assured of the best imlementation.

double fl::ClockRealtime ()
{
# if defined(WIN32) || defined(__CYGWIN__)

  FILETIME time;
  GetSystemTimeAsFileTime (&time);
  return (double) ((ULONGLONG) time) / 1e7;  // could convert this to linux epoch by subtracting 116444736000000000

# elif defined(_POSIX_VERSION)  // May need to be more specific here. clock_gettime() is defined at least in POSIX1.2008

  struct timespec t;
  clock_gettime (CLOCK_REALTIME, &t);
  return t.tv_sec + (double) t.tv_nsec / 1e9;

# else

  return std::chrono::time_point_cast<std::chrono::duration<double>> (std::chrono::system_clock::now ()).time_since_epoch ().count ();

# endif
}

double fl::ClockMonotonic ()
{
# if defined(WIN32) || defined(__CYGWIN__)

  LARGE_INTEGER count;
  LARGE_INTEGER frequency;
  QueryPerformanceCounter (&count);
  QueryPerformanceFrequency (&frequency);
  return (double) count.QuadPart / frequency.QuadPart;

# elif defined(_POSIX_VERSION)

  struct timespec t;
  clock_gettime (CLOCK_MONOTONIC, &t);
  return t.tv_sec + (double) t.tv_nsec / 1e9;

# elif __GLIBCXX__ <= 20120305

  return std::chrono::time_point_cast<std::chrono::duration<double>> (std::chrono::monotonic_clock::now ()).time_since_epoch ().count ();

# else

  return std::chrono::time_point_cast<std::chrono::duration<double>> (std::chrono::steady_clock::now ()).time_since_epoch ().count ();

# endif
}

double fl::ClockProcess ()
{
# if defined(WIN32) || defined(__CYGWIN__)

  FILETIME Dummy;
  FILETIME Kernel;
  FILETIME User;

  GetProcessTimes (GetCurrentProcess (), &Dummy, &Dummy, &Kernel, &User);
  return (double) ((ULONGLONG) Kernel + (ULONGLONG) User) / 1e7;

# elif defined(_POSIX_VERSION)

  struct timespec t;
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &t);
  return t.tv_sec + (double) t.tv_nsec / 1e9;

# else

  throw "ClockProcess not supported on this platform";

# endif
}

double fl::ClockThread ()
{
# if defined(WIN32) || defined(__CYGWIN__)

  FILETIME Dummy;
  FILETIME Kernel;
  FILETIME User;

  GetThreadTimes (GetCurrentProcess (), &Dummy, &Dummy, &Kernel, &User);
  return (double) ((ULONGLONG) Kernel + (ULONGLONG) User) / 1e7;

# elif defined(_POSIX_VERSION)

  struct timespec t;
  clock_gettime (CLOCK_THREAD_CPUTIME_ID, &t);
  return t.tv_sec + (double) t.tv_nsec / 1e9;

# else

  throw "ClockThread not supported on this platform";

# endif
}
