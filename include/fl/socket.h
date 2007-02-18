/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.6   Copyright 2005 Sandia Corporation.
Revisions 1.7 thru 1.10 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.10  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.9  2006/02/18 00:25:04  Fred
Don't use PTHREAD_PARAMETER, since the type is the same on both posix and
Windows.

Revision 1.8  2006/02/15 14:02:09  Fred
Add a couple more error constants.

Remove some platform specific timeout code, and remove setTimeout(), since it
is no longer needed.

Revision 1.7  2006/02/14 06:12:12  Fred
Finish adapting for Windows:  Handle the difference between Berkeley sockets
and Winsock.  Condition the socket timeout functions on WIN32.  Add a singleton
class to statically intialize Winsock on library load.

Enhance SocketStream to optionally manage the lifespan of the socket.  Add a
function to simplify creating the client side of a connection.

Add Listener, a simple TCP server.

Revision 1.6  2005/10/22 15:57:25  Fred
Change functions in SocketStreambuf to use the same generic int types as in the
parent class Streambuf.  This makes the code more portable, and in particular
in compiles correctly on x64.  Not tested yet, however.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/09 03:48:34  Fred
Incomplete compilability fix.  This does not yet work correctly under MSVC.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2004/05/03 19:23:07  rothgang
Remove dead code for counting number of bytes read.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_socket_h
#define fl_socket_h


#ifdef WIN32

// winsock2.h must be included before anything else that includes
// windows.h (namely fl/thread.h), otherwise windows.h will bring
// in winsock.h, which will conflict with winsock2.
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


#include "fl/thread.h"

#include <istream>


namespace fl
{
  class SocketStreambuf : public std::streambuf
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

  class SocketStream : public std::iostream
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

  class Listener
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
	static PTHREAD_RESULT processConnectionThread (void * param);
	struct ThreadDataHolder
	{
	  Listener * me;
	  SocketStream ss;
	  struct sockaddr_in clientAddress;
	};
  };

# ifdef WIN32
  class WinsockInitializer
  {
	WinsockInitializer ();
	~WinsockInitializer ();

	static WinsockInitializer singleton;
  };
# endif
}


#endif
