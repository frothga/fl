/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#ifndef socket_h
#define socket_h


#include <sys/socket.h>
#include <istream>


namespace fl
{
  class SocketStreambuf : public std::streambuf
  {
  public:
	SocketStreambuf (int socket = -1, int timeout = 0);
	void init (int socket);

	int underflow ();
	int overflow (int c);
	int sync ();
	int showmanyc ();

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
