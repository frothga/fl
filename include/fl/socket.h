/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_socket_h
#define fl_socket_h


#ifdef WIN32

// winsock2.h must be included before anything else that includes
// windows.h.  Otherwise windows.h will bring in winsock.h, which
// will conflict with winsock2.
#  include <winsock2.h>
#  include <ws2tcpip.h>

#  define socklen_t int

#  define SENDFLAGS 0
#  define IOCTLSOCKET ioctlsocket
#  define CLOSESOCKET closesocket

#  define GETLASTERROR WSAGetLastError ()
#  define EADDRINUSE  WSAEADDRINUSE
#  define EINTR       WSAEINTR
#  define EINPROGRESS WSAEINPROGRESS
#  define EMFILE      WSAEMFILE
#  define ENOBUFS     WSAENOBUFS
#  define ENOTSOCK    WSAENOTSOCK
#  define EOPNOTSUPP  WSAEOPNOTSUPP
#  define EBADF       WSAEBADF
#  define EINVAL      WSAEINVAL
#  define EFAULT      WSAEFAULT

#else

#  include <sys/types.h>
#  include <sys/socket.h>
#  include <sys/ioctl.h>
#  include <sys/poll.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <errno.h>
#  include <sys/select.h>
#  include <arpa/inet.h>
#  include <unistd.h>

#  ifdef __CYGWIN__
#    define NO_IPV6
#  endif

#  define SOCKET int
#  define INVALID_SOCKET (SOCKET) -1
#  define SOCKET_ERROR -1

#  define SENDFLAGS MSG_NOSIGNAL
#  define IOCTLSOCKET ioctl
#  define CLOSESOCKET close

#  define GETLASTERROR errno

#endif


#include <istream>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNet_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  class SHARED SocketStreambuf : public std::streambuf
  {
  public:
	SocketStreambuf (SOCKET socket = INVALID_SOCKET, int timeout = 0);

	void attach (SOCKET socket);
	void closeSocket ();

	int_type        underflow ();
	int_type        overflow (int_type c);
	int             sync ();
	std::streamsize showmanyc ();

	SOCKET socket;
	char getBuffer[4096];
	char putBuffer[4096];
	int timeout;
  };

  class SHARED SocketStream : public std::iostream
  {
  public:
	SocketStream (const std::string & hostname, const std::string & port, int timeout = 0);
	SocketStream (SOCKET socket = INVALID_SOCKET, int timeout = 0);
	~SocketStream ();  ///< Destroy the socket if we own it.

	void connect (const std::string & hostname, const std::string & port);
	void attach (SOCKET socket);
	void detach ();
	void setTimeout (int timeout = 0);

	int in_avail ();

	SocketStreambuf buffer;
	bool ownSocket;  ///< Indicates that we created the socket ourselves, and must shut it down on destruction.
  };

  class SHARED Listener
  {
  public:
	Listener (int timeout = 0, bool threaded = true);
	~Listener ();

	void listen (int port, int lastPort = -1);
	virtual void processConnection (SocketStream & ss, struct sockaddr_in & clientAddress) = 0;  ///< Override this function to implement the server.

	bool threaded;  ///< If true, create a new thread per connection.  If false, each connection will be handled serially on the listen() thread.
	int timeout;  ///< Number of seconds to pass to SocketStream constructor.
	int port;  ///< TCP port that the server is actually listening on.
	bool stop;  ///< Indicates that listen() should terminate as soon as feasible.

	// Machinery for spawning threads
	static void * processConnectionThread (void * param);
	struct ThreadDataHolder
	{
	  Listener * me;
	  SocketStream ss;
	  struct sockaddr_in clientAddress;
	};
  };

# ifdef WIN32
  class SHARED WinsockInitializer
  {
	WinsockInitializer ();
	~WinsockInitializer ();

	static WinsockInitializer singleton;
  };
# endif
}


#endif
