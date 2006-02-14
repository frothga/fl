/*
Author: Fred Rothganger
Created 2/11/2006 to provide pthread emulation on Windows
*/


#ifndef fl_thread_h
#define fl_thread_h


#ifndef WIN32

#  include <pthread.h>

#  define PTHREAD_RESULT void *
#  define PTHREAD_PARAMETER void *

#else


#  include <windows.h>
#  undef min
#  undef max

#  define PTHREAD_RESULT DWORD WINAPI
#  define PTHREAD_PARAMETER LPVOID

typedef DWORD pthread_t;
#define pthread_attr_t void

inline int
pthread_create (pthread_t * thread, pthread_attr_t * attributes, LPTHREAD_START_ROUTINE startRoutine, PTHREAD_PARAMETER arg)
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


#endif


#endif
