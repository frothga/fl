/*
Author: Fred Rothganger
Created 2/11/2006 to provide convenient setup of TCP connections.  Adapted
from numeric/KMeansParallel.cc network code.


Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/socket.h"


using namespace fl;
using namespace std;


// class Listener -------------------------------------------------------------

Listener::Listener (int timeout, bool threaded)
{
  this->timeout  = timeout;
  this->threaded = threaded;
}

Listener::~Listener ()
{
}

void
Listener::listen (int port, int lastPort)
{
  SOCKET sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) throw "Unable to create socket";

  struct sockaddr_in address;
  memset (&address, 0, sizeof (address));
  address.sin_family      = AF_INET;
  address.sin_addr.s_addr = htonl (INADDR_ANY);

  // Scan for open port
  if (lastPort < 0) lastPort = port;
  while (port <= lastPort)
  {
	address.sin_port = htons (port);
	if (bind (sock, (struct sockaddr *) &address, sizeof (address)))
	{
	  int error = GETLASTERROR;
	  if (error == EADDRINUSE)
	  {
		port++;
		continue;
	  }
	  throw "bind failed";
	}
	break;
  }
  this->port = port;

  if (::listen (sock, SOMAXCONN))
  {
	throw "listen failed";
  }

  int optval = 1;
  setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (const char *) &optval, sizeof (optval));

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

	if (threaded)
	{
	  ThreadDataHolder * data = new ThreadDataHolder;
	  data->me = this;
	  data->ss.attach (connection);
	  data->ss.setTimeout (timeout);
	  data->ss.ownSocket = true;
	  data->clientAddress = clientAddress;

	  pthread_t pid;
	  pthread_create (&pid, NULL, processConnectionThread, data);
	}
	else
	{
	  SocketStream ss (connection, timeout);
	  ss.ownSocket = true;
	  processConnection (ss, clientAddress);
	}
  }
}

void *
Listener::processConnectionThread (void * param)
{
  ThreadDataHolder * holder = (ThreadDataHolder *) param;
  holder->me->processConnection (holder->ss, holder->clientAddress);
  delete holder;
  return 0;
}
