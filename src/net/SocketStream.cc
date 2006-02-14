/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fixes for MSVC (incomplete).
10/2005 Fred Rothganger -- Use generic int types for x64 compatibility.
02/2006 Fred Rothganger -- Finish Win32 version.  Add SocketStream::connect()
*/


#include "fl/socket.h"


// For debugging
#include <iostream>


using namespace fl;
using namespace std;


// class SocketStreambuf ------------------------------------------------------

SocketStreambuf::SocketStreambuf (SOCKET socket, int timeout)
{
  attach (socket);
  setTimeout (timeout);
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

void
SocketStreambuf::setTimeout (int timeout)
{
  this->timeout = timeout;

# ifdef WIN32
  if (timeout  &&  socket != INVALID_SOCKET)
  {
	int t = timeout * 1000;
	setsockopt
	(
	  socket,
	  SOL_SOCKET,
	  SO_SNDTIMEO,
	  (char *) &t,
	  sizeof (timeout)
	);
	setsockopt
	(
	  socket,
	  SOL_SOCKET,
	  SO_RCVTIMEO,
	  (char *) &t,
	  sizeof (timeout)
	);
  }
# endif
}

SocketStreambuf::int_type
SocketStreambuf::underflow ()
{
# ifndef WIN32
  if (! waitForInput ())
  {
	return EOF;
  }
# endif
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
#   ifndef WIN32
	if (! waitForOutput ())
	{
	  return -1;
	}
#   endif
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

# ifdef WIN32

#   pragma message (__FILE__ "(152): warning: Need WinSock code to detect whether the connection is still up")

# else

  if (! count)
  {
	// Verify that connection is still up
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

#ifndef WIN32

/**
  Waits for timeout seconds for input from socket.
  This is a hack to work around the inability to set socket timeouts in Linux.
  The alternative to polling is to use SIGIO, but that creates more depency on
  properly configured client code.
  \return true if input is available, or false if timeout is reached.
**/
bool
SocketStreambuf::waitForInput ()
{
  struct pollfd p;
  p.fd = socket;
  p.events = POLLIN | POLLPRI;

  if (poll (&p, 1, timeout ? timeout * 1000 : -1) <= 0)
  {
	return false;  // On timeout or error.
  }

  if (p.revents & (POLLERR | POLLHUP | POLLNVAL))
  {
	return false;
  }

  return p.revents & (POLLIN | POLLPRI);
}

/**
  Waits for timeout seconds for socket to become ready for output.
  See comments on waitForInput().
  \return true if ready for output, or false if timeout expires.
**/
bool
SocketStreambuf::waitForOutput ()
{
  struct pollfd p;
  p.fd = socket;
  p.events = POLLOUT;

  if (poll (&p, 1, timeout ? timeout * 1000 : -1) <= 0)
  {
	return false;
  }

  if (p.revents & (POLLERR | POLLHUP | POLLNVAL))
  {
	return false;
  }

  return p.revents & POLLOUT;
}

#endif


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
  buffer.setTimeout (timeout);
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
