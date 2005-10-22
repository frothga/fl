/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


12/2004 Fred Rothganger -- Compilability fixes for MSVC (incomplete).
10/2005 Fred Rothganger -- Use generic int types for x64 compatibility.
*/


#include "fl/socket.h"

#ifdef WIN32
#else
  #include <sys/ioctl.h>
  #include <sys/poll.h>
  #include <fcntl.h>
#endif

// To enable cerr for tracing...
//#include <iostream>
//using namespace std;


using namespace fl;


// class SocketStreambuf ------------------------------------------------------

SocketStreambuf::SocketStreambuf (int socket, int timeout)
{
  this->socket = socket;
  this->timeout = timeout;

  setg (getBuffer, getBuffer, getBuffer);
  setp (putBuffer, putBuffer + sizeof (putBuffer));
}

SocketStreambuf::int_type
SocketStreambuf::underflow ()
{
  if (! waitForInput ())
  {
	return EOF;
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
	if (! waitForOutput ())
	{
	  return -1;
	}
	int sent = send (socket, start, count, MSG_NOSIGNAL);
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
  int count = 0;
  if (ioctl (socket, FIONREAD, &count))
  {
	return -1;
  }
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
  return count;
}

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
