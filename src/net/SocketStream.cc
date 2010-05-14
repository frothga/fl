/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/socket.h"

#include <string.h>
#include <stdlib.h>


// For debugging
#include <iostream>


using namespace fl;
using namespace std;


// class SocketStreambuf ------------------------------------------------------

SocketStreambuf::SocketStreambuf (SOCKET socket, int timeout)
{
  this->timeout = timeout;
  attach (socket);
}

void
SocketStreambuf::attach (SOCKET socket)
{
  this->socket = socket;

  setg (getBuffer, getBuffer, getBuffer);
  setp (putBuffer, putBuffer + sizeof (putBuffer));
}

void
SocketStreambuf::closeSocket ()
{
  if (socket != INVALID_SOCKET)
  {
	CLOSESOCKET (socket);
	socket = INVALID_SOCKET;
  }
}

SocketStreambuf::int_type
SocketStreambuf::underflow ()
{
  // Wait until data is available
  fd_set readfds;
  timeval timeLimit = {timeout, 0};
  while (true)
  {
	FD_ZERO (&readfds);
	FD_SET (socket, &readfds);
	int ready = select (socket + 1, &readfds, NULL, NULL, timeout ? &timeLimit : NULL);
	if (ready == 1) break;  // Success: data is ready
	if (ready == SOCKET_ERROR)
	{
	  int error = GETLASTERROR;
	  if (error == EINTR  ||  error == EINPROGRESS)
	  {
		continue;  // Not fatal, so keep listening.
	  }
	}
	return EOF;  // timeout, or fatal error
  }

  int count = recv (socket, getBuffer, sizeof (getBuffer), 0);
  if (count <= 0)
  {
	return EOF;
  }
  setg (getBuffer, getBuffer, getBuffer + count);
  return traits_type::to_int_type (getBuffer[0]);
}

SocketStreambuf::int_type
SocketStreambuf::overflow (int_type c)
{
  if (sync ())
  {
	return EOF;
  }
  if (c != EOF)
  {
	putBuffer[0] = traits_type::to_char_type (c);
	pbump (1);
  }
  return c;
}

int
SocketStreambuf::sync ()
{
  int count = pptr () - putBuffer;
  char * start = putBuffer;
  while (count > 0)
  {
	// Wait until system is ready to transmit
	fd_set writefds;
	timeval timeLimit = {timeout, 0};
	while (true)
	{
	  FD_ZERO (&writefds);
	  FD_SET (socket, &writefds);
	  int ready = select (socket + 1, NULL, &writefds, NULL, timeout ? &timeLimit : NULL);
	  if (ready == 1) break;  // Success: system is ready
	  if (ready == SOCKET_ERROR)
	  {
		int error = GETLASTERROR;
		if (error == EINTR  ||  error == EINPROGRESS)
		{
		  continue;  // Not fatal, so keep waiting.
		}
	  }
	  return -1;  // timeout, or fatal error
	}

	int sent = send (socket, start, count, SENDFLAGS);
	if (sent <= 0)
	{
	  return -1;
	}
	count -= sent;
	start += sent;
  }
  setp (putBuffer, putBuffer + sizeof (putBuffer));
  return 0;
}

std::streamsize
SocketStreambuf::showmanyc ()
{
  unsigned long count = 0;
  int error = IOCTLSOCKET (socket, FIONREAD, &count);
  if (error)
  {
	return -1;
  }

  // Verify that connection is still up.  This feature is not critical to
  // the correct operation of this streambuf.
# ifndef WIN32
  if (! count)
  {
	struct pollfd p;
	p.fd = socket;
	p.events = 0;
	poll (&p, 1, 0);
	if (p.revents & POLLHUP)
	{
	  return -1;
	}
  }
# endif

  return count;
}


// class SocketStream ---------------------------------------------------------

SocketStream::SocketStream (const string & hostname, const string & port, int timeout)
: buffer (INVALID_SOCKET, timeout), iostream (&buffer)
{
  ownSocket = false;  // to block detach(); will be set to true by connect()
  connect (hostname, port);
}

SocketStream::SocketStream (SOCKET socket, int timeout)
: buffer (socket, timeout), iostream (&buffer)
{
  ownSocket = false;
}

SocketStream::~SocketStream ()
{
  detach ();
}

void
SocketStream::connect (const string & hostname, const string & port)
{
  detach ();

# ifdef NO_IPV6

  struct hostent * hp = gethostbyname (hostname.c_str ());
  if (! hp) throw "Couldn't find host";

  struct sockaddr_in server;
  memset (&server, 0, sizeof (server));
  memcpy (&server.sin_addr, hp->h_addr, hp->h_length);
  server.sin_family = hp->h_addrtype;
  server.sin_port   = htons (atoi (port.c_str ()));

  SOCKET connection = socket (hp->h_addrtype, SOCK_STREAM, 0);
  if (connection == INVALID_SOCKET) throw "Unable to create stream socket";

  if (::connect (connection, (sockaddr *) &server, sizeof (server)))
  {
	throw "Failed to connect";
  }

# else

  struct addrinfo hint;
  memset (&hint, 0, sizeof (hint));
  hint.ai_socktype = SOCK_STREAM;

  struct addrinfo * addresses = 0;
  int error = getaddrinfo
  (
    hostname.c_str (),
	port.c_str (),
	&hint,
	&addresses
  );
  if (error  ||  ! addresses)
  {
	cerr << "error = " << error << endl;
	throw "Can't resolve address";
  }

  SOCKET connection = socket (addresses->ai_family, addresses->ai_socktype, addresses->ai_protocol);
  if (connection == INVALID_SOCKET)
  {
	freeaddrinfo (addresses);
	throw "Unable to create stream socket";
  }
  error = ::connect (connection, addresses->ai_addr, addresses->ai_addrlen);
  freeaddrinfo (addresses);
  if (error)
  {
	throw "Failed to connect";
  }

# endif

  ownSocket = true;
  buffer.attach (connection);
}

void
SocketStream::attach (SOCKET socket)
{
  detach ();
  buffer.attach (socket);
}

void
SocketStream::detach ()
{
  if (ownSocket) buffer.closeSocket ();
  ownSocket = false;
}

void
SocketStream::setTimeout (int timeout)
{
  buffer.timeout = timeout;
}

int
SocketStream::in_avail ()
{
  return buffer.in_avail ();
}


// class WinsockInitializer ---------------------------------------------------

#ifdef WIN32

WinsockInitializer WinsockInitializer::singleton;

WinsockInitializer::WinsockInitializer ()
{
  WSADATA wsaData;
  if (WSAStartup (MAKEWORD (2, 2), &wsaData))
  {
	throw "WSAStartup failed";
  }
  if (   LOBYTE (wsaData.wVersion) != 2
	  || HIBYTE (wsaData.wVersion) != 2 )
  {
	throw "WinSock 2.2 not available";
  }
}

WinsockInitializer::~WinsockInitializer ()
{
  WSACleanup ();
}

#endif
