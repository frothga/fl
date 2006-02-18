/*
Author: Fred Rothganger
Created 2/11/2006 to provide pthread emulation on Windows
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
