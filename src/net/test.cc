/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/socket.h"

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>


using namespace std;
using namespace fl;


#define portNumber 60000


void
handler (int s)
{
  cerr << "Got signal " << s << endl;
}

int
main (int argc, char * argv[])
{
  signal (SIGIO, handler);
  signal (SIGPIPE, handler);

  const int blockSize = 1000000;
  int data[blockSize];

  if (argc < 2)
  {
	// Be server
	cerr << "server" << endl;

	int sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
	{
	  perror ("socket");
	  exit (1);
	}

	struct sockaddr_in sin;
	bzero (&sin, sizeof (sin));
	sin.sin_family      = AF_INET;
	sin.sin_port        = htons (portNumber);
	sin.sin_addr.s_addr = htonl (INADDR_ANY);

	if (bind (sock, (struct sockaddr *) &sin, sizeof (sin)) == -1)
	{
	  perror ("bind");
	  exit (1);
	}
	if (listen (sock, 5) == -1)
	{
	  perror ("listen");
	  exit (1);
	}

	int optval = 1;
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof (optval));

	struct sockaddr_in peer;
	socklen_t size;
	int connection = accept (sock, (sockaddr *) &peer, &size);
	if (connection == -1)
	{
	  cerr << "accept failed: " << strerror (errno) << endl;
	  exit (1);
	}
	cerr << "got first connectin" << endl;
	close (connection);
	connection = accept (sock, (sockaddr *) &peer, &size);
	if (connection == -1)
	{
	  cerr << "accept failed: " << strerror (errno) << endl;
	  exit (1);
	}
	cerr << "got second connectin" << endl;

	struct hostent * hp = gethostbyaddr ((char *) &peer.sin_addr, sizeof (peer.sin_addr), AF_INET);
    string remoteName = "unknown";
	if (hp)
	{
	  remoteName = hp->h_name;
	}
	else
	{
	  herror ("gethostbyaddr failed: ");
	  remoteName = inet_ntoa (peer.sin_addr);
	}
	cerr << "connection = " << remoteName << endl;


	SocketStream ss (connection, 60);

	int i = 0;
	while (ss.good ())
	{
	  int count = rand () % blockSize + 1;
	  data[count - 1] = i;
	  cerr << i << " writing " << count * sizeof (int) << endl;
	  ss.write ((char *) &count, sizeof (count));
	  ss.write ((char *) data, count * sizeof (int));
	  ss.flush ();
	  if (! ss.good ())
	  {
		cerr << i << " write bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
	  }

	  ss.read ((char *) &count, sizeof (count));
	  cerr << i << " got count " << count << " " << ss.gcount () << endl;
	  int readCount = 0;
	  for (int j = 0; j < count; j++)
	  {
		ss.read ((char *) &data[j], sizeof (int));
		readCount += ss.gcount ();
	  }
	  cerr << i << " read " << readCount << endl;
	  if (! ss.good ())
	  {
		cerr << i << " read bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
	  }
	  if (data[count - 1] != i)
	  {
		cerr << i << " read bad value " << data[count - 1] << endl;
	  }

	  i++;
	}

	close (connection);
  }
  else
  {
	// Be client
	cerr << "client" << endl;

	string serverName = argv[1];
	struct hostent * hp;
	hp = gethostbyname (serverName.c_str ());
	if (! hp)
	{
	  cerr << "Couldn't look up server " << serverName << endl;
	  exit (1);
	}

	struct sockaddr_in server;
	bzero ((char *) &server, sizeof (server));
	bcopy (hp->h_addr, &server.sin_addr, hp->h_length);
	server.sin_family = hp->h_addrtype;
	server.sin_port   = htons (portNumber);

	int connection = socket (hp->h_addrtype, SOCK_STREAM, 0);
	if (connection < 0)
	{
	  cerr << "Unable to create stream socket" << endl;
	  exit (1);
	}
	if (connect (connection, (sockaddr *) &server, sizeof (server)) < 0)
	{
	  perror ("connect");
	  exit (1);
	}
	cerr << "got first connection" << endl;

	close (connection);
	connection = socket (hp->h_addrtype, SOCK_STREAM, 0);
	if (connection < 0)
	{
	  cerr << "Unable to create stream socket" << endl;
	  exit (1);
	}
	if (connect (connection, (sockaddr *) &server, sizeof (server)) < 0)
	{
	  perror ("connect");
	  exit (1);
	}
	cerr << "got second connection" << endl;

	char hostname[256];
	gethostname (hostname, sizeof (hostname));
	strtok (hostname, ".");
	cerr << "Connection complete: " << serverName << " " << hostname << endl;


	SocketStream ss (connection);

	int i = 0;
	while (ss.good ())
	{
	  int count;
	  ss.read ((char *) &count, sizeof (count));
	  cerr << i << " got count " << count << " " << ss.gcount () << endl;
	  int readCount = 0;
	  for (int j = 0; j < count; j++)
	  {
		ss.read ((char *) &data[j], sizeof (int));
		readCount += ss.gcount ();
	  }
	  cerr << i << " read " << readCount << endl;
	  if (! ss.good ())
	  {
		cerr << i << " read bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
	  }
	  if (data[count - 1] != i)
	  {
		cerr << i << " read bad value " << data[count - 1] << endl;
	  }

	  sleep (rand () % 2);

	  count = rand () % blockSize + 1;
	  data[count - 1] = i;
	  cerr << i << " writing " << count * sizeof (int) << endl;
	  ss.write ((char *) &count, sizeof (count));
	  ss.write ((char *) data, count * sizeof (int));
	  ss.flush ();
	  if (! ss.good ())
	  {
		cerr << i << " write bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
	  }

	  i++;
	}

	close (connection);
  }

  return 0;
}
