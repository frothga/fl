/*
Author: Fred Rothganger
Created 2/11/2006 to provide pthread emulation on Windows


Revisions 1.1 thru 1.3 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.3  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.2  2006/02/18 00:25:33  Fred
Don't use PTHREAD_PARAMETER, since the type is the same on both posix and
Windows.

Add mutexes.

Revision 1.1  2006/02/14 06:12:48  Fred
Fake pthreads on Windows.
-------------------------------------------------------------------------------
*/


#ifndef fl_thread_h
#define fl_thread_h


#ifndef WIN32

#  include <pthread.h>

#  define PTHREAD_RESULT void *

#else


#  include <windows.h>
#  undef min
#  undef max

#  define PTHREAD_RESULT DWORD WINAPI

typedef DWORD pthread_t;
#define pthread_attr_t void
typedef CRITICAL_SECTION pthread_mutex_t;
#define pthread_mutex_attr_t void

inline int
pthread_create (pthread_t * thread, pthread_attr_t * attributes, LPTHREAD_START_ROUTINE startRoutine, void * arg)
{
  HANDLE handle = CreateThread
  (
    NULL,
	0,
	startRoutine,
	arg,
	0,
	thread
  );
  if (handle == NULL)
  {
	return GetLastError ();
  }
  CloseHandle (handle);
  return 0;
}

inline int
pthread_mutex_init (pthread_mutex_t * mutex, const pthread_mutex_attr_t * mutexattr)
{
  InitializeCriticalSection (mutex);
  return 0;
}

inline int
pthread_mutex_destroy (pthread_mutex_t * mutex)
{
  DeleteCriticalSection (mutex);
  return 0;
}

inline int
pthread_mutex_lock (pthread_mutex_t * mutex)
{
  EnterCriticalSection (mutex);
  return 0;
}

inline int
pthread_mutex_unlock(pthread_mutex_t * mutex)
{
  LeaveCriticalSection (mutex);
  return 0;
}


#endif


#endif
