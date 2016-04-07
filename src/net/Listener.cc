/*
Author: Fred Rothganger
Created 2/11/2006 to provide convenient setup of TCP connections.  Adapted
from numeric/KMeansParallel.cc network code.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/socket.h"
#include "fl/time.h"

#include <string.h>
#ifdef HAVE_PTHREAD
#  include <pthread.h>
#endif


using namespace fl;
using namespace std;


// class Listener -------------------------------------------------------------

Listener::Listener (int timeout, bool threaded)
: timeout  (timeout),
  threaded (threaded)
{
}

/**
   @param port First port to try.
   @param lastPort If port can't be opened, try subsequent ones until this
   one is tried.
   @param scanTimeout How many seconds to keep scanning for an open port. 0 means
   forever.
**/
void
Listener::listen (int port, int lastPort, int scanTimeout)
{
  SOCKET sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) throw "Unable to create socket";

  struct sockaddr_in address;
  memset (&address, 0, sizeof (address));
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = htonl (INADDR_ANY);

  int optval = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof (optval));

  // Scan for open port
  int firstPort = port;
  if (lastPort < 0) lastPort = port;
  Stopwatch timer;
  while (port <= lastPort  &&  ! stop)
  {
	address.sin_port = htons (port);
	if (::bind (sock, (struct sockaddr *) &address, sizeof (address)))
	{
	  int error = GETLASTERROR;
	  if (error == EADDRINUSE  &&  port < lastPort)
	  {
		port++;
		continue;
	  }
	  if (scanTimeout <= 0  ||  timer.total () < scanTimeout)
	  {
		port = firstPort;
		sleep (1);
		continue;
	  }
	  throw "bind failed (probably due to a lingering socket)";
	}
	break;
  }
  if (stop)
  {
	CLOSESOCKET (sock);
	return;
  }
  this->port = port;

  if (::listen (sock, SOMAXCONN))
  {
	throw "listen failed";
  }

# ifdef HAVE_PTHREAD
  pthread_attr_t attributes;
  pthread_attr_init (&attributes);
  pthread_attr_setdetachstate (&attributes, PTHREAD_CREATE_DETACHED);
# endif

  fd_set readfds;
  timeval selectTimeout;

  stop = false;
  while (! stop)
  {
	// Monitor socket for requests, but come up for air once in a while.
	FD_ZERO (&readfds);
	FD_SET (sock, &readfds);
	selectTimeout.tv_sec  = 10;
	selectTimeout.tv_usec = 0;
	int ready = select (sock + 1, &readfds, NULL, NULL, &selectTimeout);
	if (stop) break;
	if (! ready) continue;
	if (ready == SOCKET_ERROR)
	{
	  int error = GETLASTERROR;
	  if (error == EINTR  ||  error == EINPROGRESS)
	  {
		continue;  // Not fatal, so keep listening.
	  }
	  break;  // Fatal, so exit.
	}

	// At this point we know a request is ready, so grab it.
	struct sockaddr_in clientAddress;
	socklen_t size = sizeof (clientAddress);
	SOCKET connection = accept (sock, (struct sockaddr *) &clientAddress, &size);
	if (connection == SOCKET_ERROR)
	{
	  // Here we are checking if the error is fatal to listening in general,
	  // rather than to the current connection.
	  int error = GETLASTERROR;
	  if (   error == EBADF
		  || error == ENOTSOCK
		  || error == EOPNOTSUPP
		  || error == EINVAL
		  || error == EFAULT)
	  {
		break;  // Fatal error
	  }
	  continue;  // Not fatal, so resume litening
	}

#   ifdef HAVE_PTHREAD
	if (threaded)
	{
	  ThreadDataHolder * data = new ThreadDataHolder;
	  data->me = this;
	  data->ss.attach (connection);
	  data->ss.setTimeout (timeout);
	  data->ss.ownSocket = true;
	  data->clientAddress = clientAddress;

	  pthread_t pid;
	  pthread_create (&pid, &attributes, processConnectionThread, data);

	  continue;  // Don't fall through to the non-threaded processor below.
	}
#   endif

	SocketStream ss (connection, timeout);
	ss.ownSocket = true;
	processConnection (ss, clientAddress);
  }

# ifdef HAVE_PTHREAD
  pthread_attr_destroy (&attributes);
# endif

  CLOSESOCKET (sock);
}

void *
Listener::processConnectionThread (void * param)
{
  ThreadDataHolder * holder = (ThreadDataHolder *) param;
  holder->me->processConnection (holder->ss, holder->clientAddress);
  delete holder;
  return 0;
}
