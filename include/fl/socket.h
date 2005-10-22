/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


10/2005 Fred Rothganger -- Use generic int types for x64 compatibility.
*/


#ifndef socket_h
#define socket_h


#ifdef WIN32
  #include <winsock2.h>
#else
  #include <sys/socket.h>
#endif

#include <istream>


namespace fl
{
  class SocketStreambuf : public std::streambuf
  {
  public:
	SocketStreambuf (int socket = -1, int timeout = 0);
	void init (int socket);

	int_type        underflow ();
	int_type        overflow (int_type c);
	int             sync ();
	std::streamsize showmanyc ();

	bool waitForInput ();  // Waits for timeout seconds for input from socket.
	bool waitForOutput ();  // Ditto for output.  Returns true if ready for output.  Returns false if timeout expires.

	int socket;
	char getBuffer[4096];
	char putBuffer[4096];
	int timeout;
  };

  class SocketStream : public std::iostream
  {
  public:
	SocketStream (int socket = -1, int timeout = 0) : buffer (socket, timeout), std::iostream (&buffer) {}
	void init (int socket) {buffer.init (socket);}

	int in_avail () {return buffer.in_avail ();}

	SocketStreambuf buffer;
  };
}


#endif
