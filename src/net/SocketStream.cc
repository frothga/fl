/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.6  Copyright 2005 Sandia Corporation.
Revisions 1.7 thru 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:41:14  Fred
Use CVS Log to generate revision history.

Revision 1.8  2006/02/15 06:38:36  Fred
Use select() as the primary method for handling timeouts on read and write. 
Gets rid of both Windows and posix special cases.

Determined that checking if connection is down in showmanyc() is not critical,
so cut out the Windows portion of it.

Revision 1.7  2006/02/14 06:00:53  Fred
Finish adapting to work under Windows:  Make socket timeout code conditional on
WIN32.  Handle most of the differences between Berkeley sockets and Winsock in
the socket.h header file.  Add a singleton class to initialize Winsock when
library loads.

Enhance SocketStream class to optionally manage the lifespan of the socket. 
Added a method to simplify starting up the client side of a connection.  Moved
most of the code out of socket.h and into this file.

Revision 1.6  2005/10/22 15:58:10  Fred
Change functions in SocketStreambuf to use the same generic int types as in the
parent class streambuf.  This makes the code more portable, and in particular
in compiles correctly on x64.  Not tested yet, however.

Revision 1.5  2005/10/13 03:28:02  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/13 03:26:07  Fred
Preliminary work on making this code compile unser MSVC.  It does not work yet.

Revision 1.3  2005/04/23 19:39:45  Fred
Add UIUC copyright notice.

Revision 1.2  2004/07/22 15:41:01  rothgang
Finish throwing out SocketStream.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/socket.h"


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
