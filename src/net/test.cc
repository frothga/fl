/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 thru 1.8 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/23 02:41:14  Fred
Use CVS Log to generate revision history.

Revision 1.7  2006/02/15 06:36:08  Fred
Add timeout to client side socket.  Add code to switch between reading one
large block and a lot of small values.

Revision 1.6  2006/02/14 06:04:07  Fred
Rewrote to use update SocketStream and new Listener classes.  Removes the need
for much of the connection establishing code.  This version compiles and works
under both Windows (VS 2003) and Cygwin.

Revision 1.5  2006/02/11 14:39:41  Fred
Fixes for Cygwin: include errno.h and allocate a smaller buffer.  The buffer is
allocated on the stack, and it was causing a core dump.

Revision 1.4  2005/10/13 03:28:02  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:39:45  Fred
Add UIUC copyright notice.

Revision 1.2  2004/07/22 15:39:14  rothgang
Move data block to common location.  Update include statement for shared
include directory.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/socket.h"
#include "fl/time.h"

#include <iostream>
#include <string>


using namespace std;
using namespace fl;


#define portNumber 60000
#define blockSize 1000000
#define timeout 60


class TestListener : public Listener
{
public:
  TestListener (int * data)
  : Listener (timeout)
  {
	state = 0;
	this->data = data;
  }

  virtual void processConnection (SocketStream & ss, struct sockaddr_in & clientAddress)
  {
	switch (state++)
	{
	  case 0:
		cerr << "Got first connection" << endl;
		break;
	  case 1:
	  {
		cerr << "Got second connection" << endl;

		struct hostent * hp = gethostbyaddr ((char *) &clientAddress.sin_addr, sizeof (clientAddress.sin_addr), AF_INET);
		string remoteName = "unknown";
		if (hp)
		{
		  remoteName = hp->h_name;
		}
		else
		{
		  cerr << "gethostbyaddr failed" << endl;
		  remoteName = inet_ntoa (clientAddress.sin_addr);
		}
		cerr << "connection = " << remoteName << endl;

		int i = 0;
		while (ss.good ())
		{
		  int count = rand () % blockSize + 1;
		  data[count - 1] = i;
		  cerr << i << " writing " << count * sizeof (int) << endl;
		  ss.write ((char *) &count, sizeof (count));
		  ss.write ((char *) data, count * sizeof (int));
		  ss.flush ();
		  data[count - 1] = count - 1;
		  if (! ss.good ())
		  {
			cerr << i << " write bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
		  }

		  ss.read ((char *) &count, sizeof (count));
		  cerr << i << " got count " << count << " " << ss.gcount () << endl;
		  int readCount = 0;
		  if (rand () % 2)
		  {
			cerr << i << " reading single block" << endl;
			ss.read ((char *) data, count * sizeof (data[0]));
			readCount = ss.gcount ();
		  }
		  else
		  {
			cerr << i << " reading individual entries" << endl;
			for (int j = 0; j < count; j++)
			{
			  ss.read ((char *) &data[j], sizeof (int));
			  readCount += ss.gcount ();
			  if (j < count - 1  &&  data[j] != j) cerr << "unexpected value: " << data[j] << " rather than " << j << endl;
			}
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
		  data[count - 1] = count - 1;

		  i++;
		}
		// Fall through to case 2 below..
	  }
	  case 2:
	  default:
		stop = true;
	}
  }

  int state;
  int * data;
};


int
main (int argc, char * argv[])
{
  try
  {
	int * data = new int[blockSize];
	for (int i = 0; i < blockSize; i++) data[i] = i;

	if (argc < 2)
	{
	  // Be server
	  cerr << "server" << endl;

	  TestListener tl (data);
	  tl.listen (portNumber);
	}
	else
	{
	  // Be client
	  cerr << "client" << endl;

	  string serverName = argv[1];
	  char portName[256];
	  sprintf (portName, "%i", portNumber);

	  SocketStream ss (serverName, portName, timeout);
	  cerr << "got first connection" << endl;

	  ss.connect (serverName, portName);
	  cerr << "got second connection" << endl;

	  char hostname[256];
	  gethostname (hostname, sizeof (hostname));
	  strtok (hostname, ".");
	  cerr << "Connection complete: " << serverName << " " << hostname << endl;

	  int i = 0;
	  while (ss.good ())
	  {
		int count;
		ss.read ((char *) &count, sizeof (count));
		cerr << i << " got count " << count << " " << ss.gcount () << endl;
		int readCount = 0;
		if (rand () % 2)
		{
		  cerr << i << " reading single block" << endl;
		  ss.read ((char *) data, count * sizeof (data[0]));
		  readCount = ss.gcount ();
		}
		else
		{
		  cerr << i << " reading individual entries" << endl;
		  for (int j = 0; j < count; j++)
		  {
			ss.read ((char *) &data[j], sizeof (int));
			readCount += ss.gcount ();
			if (j < count - 1  &&  data[j] != j) cerr << "unexpected value: " << data[j] << " rather than " << j << endl;
		  }
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
		data[count - 1] = count - 1;

		sleep (rand () % 2);

		count = rand () % blockSize + 1;
		data[count - 1] = i;
		cerr << i << " writing " << count * sizeof (int) << endl;
		ss.write ((char *) &count, sizeof (count));
		ss.write ((char *) data, count * sizeof (int));
		ss.flush ();
		data[count - 1] = count - 1;
		if (! ss.good ())
		{
		  cerr << i << " write bad " << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
		}

		i++;
	  }
	}
  }
  catch (const char * message)
  {
	cerr << "Exception: " << message << endl;
  }

  return 0;
}
