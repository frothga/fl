/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8  thru 1.10 Copyright 2005 Sandia Corporation.
Revisions 1.12 thru 1.14 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.14  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.13  2006/02/18 00:38:23  Fred
Update revision history.

Revision 1.12  2006/02/18 00:34:07  Fred
Make KMP a Listener, and use client connection code in SocketStream, rather
than handling these TCP rituals directly.

Revision 1.11  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.10  2005/10/13 03:32:11  Fred
Add Sandia distribution terms.

Revision 1.9  2005/09/12 03:43:23  Fred
Change lapacks.h to lapack.h

Add Sandia copyright notice.  Need to add license info before release.

Revision 1.8  2005/08/06 16:06:01  Fred
MSVC compatibility fix: use binary mode for files.

Revision 1.7  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.6  2004/04/19 17:22:21  rothgang
Move substantial pieces of code from KMeansParallel::run() into separate
functions to allow implementation of non-parallel KMeans::run() with minimal
redundancy.  Change interface of read() and write() to match other parts of
library.

Revision 1.5  2004/04/06 21:09:39  rothgang
Guard against merging cluster membership info when removing a newly created
cluster center.

Revision 1.4  2003/12/31 16:36:15  rothgang
Convert to fl namespace and add to library.

Revision 1.3  2003/12/24 15:21:35  rothgang
Use frob() rather than norm().

Revision 1.2  2003/12/23 19:12:02  rothgang
Convert to use fl library.

Revision 1.1  2003/12/23 18:57:25  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/12/23 18:57:25  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/cluster.h"
#include "fl/time.h"
#include "fl/random.h"
#include "fl/socket.h"
#include "fl/lapack.h"

#include <algorithm>
#include <sys/stat.h>
#include <fstream>
#include <errno.h>
#include <time.h>


using namespace fl;
using namespace std;


// class KMeansParallel -------------------------------------------------------

KMeansParallel::KMeansParallel (float maxSize, float minSize, int initialK, int maxK, const string & clusterFileName)
: KMeans (maxSize, minSize, initialK, maxK, clusterFileName),
  Listener (4000)
{
}

KMeansParallel::KMeansParallel (istream & stream, const string & clusterFileName)
: KMeans (stream, clusterFileName),
  Listener (4000)
{
}

void
KMeansParallel::run (const std::vector<Vector<float> > & data)
{
  this->data = &data;

  // Fire up parallel processing
  ClusterMethod::stop = false;
  pthread_mutex_init (&stateLock, NULL);
  state = initializing;
  pthread_t pid;
  pthread_create (&pid, NULL, listenThread, this);
  initialize (data);

  // Iterate to convergence.  Convergence condition is that cluster
  // centers are stable and that all features fall within maxSize of
  // nearest cluster center.
  iteration = 0;
  bool converged = false;
  while (! converged  &&  ! ClusterMethod::stop)
  {
	cerr << "========================================================" << iteration++ << endl;
	double timestamp = getTimestamp ();

    // We assume that one iteration takes a very long time, so the cost of
    // dumping our state every time is relatively small (esp. compared to the
    // cost of losing everything in a crash).
	if (clusterFileName.size ())
	{
	  ofstream target (clusterFileName.c_str (), ios::binary);
	  write (target);
	}
	else
	{
	  cout.flush ();
	  rewind (stdout);
	  write (cout);
	}

	// Estimation: Generate probability of membership for each datum in each cluster.
	Matrix<float> member (clusters.size (), data.size ());
	pthread_mutex_lock (&stateLock);
	this->member = member;
	unitsPending = (int) ceil ((float) data.size () / workUnitSize);
	if (workUnits.size ())
	{
	  cerr << "Non-empty work queue in estimate!" << endl;
	  exit (1);
	}
	for (int i = 0; i < unitsPending; i++)
	{
	  workUnits.push_back (i);
	}
	state = estimating;
	pthread_mutex_unlock (&stateLock);
	while (unitsPending  &&  ! ClusterMethod::stop)
	{
	  sleep (1);
	}
	cerr << endl;
	if (ClusterMethod::stop)
	{
	  Listener::stop = true;
	  break;
	}

	// Maximization: Update clusters based on member data.
	cerr << clusters.size () << endl;
	pthread_mutex_lock (&stateLock);
	largestChange = 0;
	if (workUnits.size ())
	{
	  cerr << "Non-empty work queue in maximize!" << endl;
	  exit (1);
	}
	unitsPending = clusters.size ();
	for (int i = 0; i < clusters.size (); i++)
	{
	  workUnits.push_back (i);
	}
	state = maximizing;
	pthread_mutex_unlock (&stateLock);
	while (unitsPending  &&  ! ClusterMethod::stop)
	{
	  sleep (1);
	}
	if (ClusterMethod::stop)
	{
	  Listener::stop = true;
	  break;
	}

	// Check for convergence, and update number of clusters.
	pthread_mutex_lock (&stateLock);
	state = checking;
	pthread_mutex_unlock (&stateLock);
	converged = convergence (data, member, largestChange);

	cerr << "time = " << getTimestamp () - timestamp << endl;
  }
}

PTHREAD_RESULT
KMeansParallel::listenThread (void * arg)
{
cerr << "starting listenThread" << endl;
  KMeansParallel * me = (KMeansParallel *) arg;
  me->listen (portNumber);
  return NULL;
}

void
KMeansParallel::processConnection (SocketStream & ss, struct sockaddr_in & clientAddress)
{
  struct in_addr address  = clientAddress.sin_addr;
  struct hostent * hp = gethostbyaddr ((char *) &address, sizeof (address), AF_INET);
  char peer[128];
  if (hp)
  {
	strcpy (peer, hp->h_name);
  }
  else
  {
	strcpy (peer, "unknown");
  }
  strtok (peer, ".");
  cerr << peer << " starting proxyThread" << endl;

  const vector<Vector<float> > & data = *this->data;

  int lastIteration = -1;
  while (ss.good ()  &&  ! ClusterMethod::stop)
  {
	pthread_mutex_lock (&stateLock);
	EMstate state = this->state;
	int unit = -1;
	if (workUnits.size ())
	{
	  unit = workUnits.back ();
	  workUnits.pop_back ();
	}
	pthread_mutex_unlock (&stateLock);

	if (ClusterMethod::stop)
	{
	  break;
	}

	if (unit < 0)
	{
	  sleep (1);
	  continue;
	}

	switch (state)
	{
	  case estimating:
	  {
		int command;
		if (lastIteration != iteration)
		{
		  lastIteration = iteration;
		  command = 1;  // Re-read cluster file
		  ss.write ((char *) &command, sizeof (command));
		  // The following trivia will help the client determine when NFS is
		  // presenting a current copy of the file.
		  ss.write ((char *) &clusterFileSize, sizeof (clusterFileSize));
		  ss.write ((char *) &clusterFileTime, sizeof (clusterFileTime));
		  ss.flush ();
		}
		command = 2;  // Perform estimation
		ss.write ((char *) &command, sizeof (command));
		ss.write ((char *) &unit, sizeof (unit));
		ss.flush ();
		int jbegin = workUnitSize * unit;
		int jend = jbegin + workUnitSize;
		jend = min (jend, (int) data.size ());
		ss.read ((char *) & member (0, jbegin), clusters.size () * (jend - jbegin) * sizeof (float));
//cerr << ss.gcount () << " " << clusters.size () * (jend - jbegin) * sizeof (float) << endl;
		if (ss.good ())
		{
		  pthread_mutex_lock (&stateLock);
		  unitsPending--;
		  pthread_mutex_unlock (&stateLock);
          cerr << ".";
		}
		else
		{
		  pthread_mutex_lock (&stateLock);
		  workUnits.push_back (unit);
		  pthread_mutex_unlock (&stateLock);
cerr << peer << " put back " << unit << endl;
		}
		break;
	  }
	  case maximizing:
	  {
		int command = 3;
		ss.write ((char *) &command, sizeof (command));
		ss.write ((char *) &unit, sizeof (unit));
		for (int j = 0; j < data.size (); j++)
		{
		  ss.write ((char *) & member (unit, j), sizeof (float));
		}
		ss.flush ();
		float change;
		ss.read ((char *) &change, sizeof (change));
		try
		{
		  clusters[unit].read (ss);
		}
		catch (...)
		{
		  // The stream is down.  It should already be marked bad, which will
		  // be handled correctly below.
		}
		if (ss.good ())
		{
		  pthread_mutex_lock (&stateLock);
		  largestChange = max (largestChange, change);
		  unitsPending--;
		  pthread_mutex_unlock (&stateLock);
		  float minev = largestNormalFloat;
		  float maxev = 0;
		  for (int j = 0; j < clusters[unit].eigenvalues.rows (); j++)
		  {
			minev = min (minev, fabsf (clusters[unit].eigenvalues[j]));
			maxev = max (maxev, fabsf (clusters[unit].eigenvalues[j]));
		  }
		  cerr << unit << " = " << clusters[unit].alpha << " " << change << " " << sqrt (minev) << " " << sqrt (maxev) << endl;
		}
		else
		{
		  pthread_mutex_lock (&stateLock);
		  workUnits.push_back (unit);
		  pthread_mutex_unlock (&stateLock);
cerr << peer << " put back " << unit << endl;
		}
		break;
	  }
	  default:
		sleep (1);
	}
  }
cerr << peer << " exiting proxyThread" << endl;
}

void
KMeansParallel::client (std::string serverName)
{
  // Open connection to server
  char portName[256];
  sprintf (portName, "%i", portNumber);
  SocketStream ss (serverName, portName);
  char hostname[256];
  gethostname (hostname, sizeof (hostname));
  strtok (hostname, ".");
  cerr << "Connection complete: " << serverName << " " << hostname << endl;

  // Handle requests
  while (ss.good ())
  {
	int command;
	ss.read ((char *) &command, sizeof (command));
	if (! ss.good ())
	{
	  break;
	}

	switch (command)
	{
	  case 1: // Re-read clusters file
	  {
cerr << "re-read clusters" << endl;
        off_t size;
		time_t mtime;
		ss.read ((char *) &size, sizeof (size));
		ss.read ((char *) &mtime, sizeof (mtime));
cerr << "  expecting: " << size << " " << ctime (&mtime);
		struct stat stats;
		double starttime = getTimestamp ();
		do
		{
		  sleep (1);
		  stat (clusterFileName.c_str (), &stats);
		  if (getTimestamp () - starttime > 120)
		  {
			// 30 seconds is normal NFS expiration time
			cerr << "NFS took too long to synchronize" << endl;
			exit (1);
		  }
cerr << "  checking NFS: " << stats.st_size << " " << ctime (&stats.st_mtime);
		}
		while (stats.st_size != size  ||  stats.st_mtime < mtime);
		ifstream target (clusterFileName.c_str (), ios::binary);
        if (! target.good ())
		{
		  cerr << "Unable to reopen cluster file " << clusterFileName << endl;
		  exit (1);
		}
		read (target);
		member.resize (clusters.size (), data->size ());
		break;
	  }
	  case 2:  // Perform estimation for one block
	  {
cerr << "perform estimation" << endl;
		int unit;
		ss.read ((char *) &unit, sizeof (unit));
		if (! ss.good ())
		{
		  break;
		}
cerr << "  unit = " << unit << endl;
		int jbegin = workUnitSize * unit;
		int jend = jbegin + workUnitSize;
		jend = min (jend, (int) data->size ());
cerr << "  j = " << jbegin << " " << jend << endl;
		estimate (*data, member, jbegin, jend);
cerr << "  about to write member block" << endl;
		ss.write ((char *) & member (0, jbegin), clusters.size () * (jend - jbegin) * sizeof (float));
		ss.flush ();
cerr << "  wrote member block" << endl;
		break;
	  }
	  case 3:  // Perform maximization for one cluster
	  {
cerr << "perform maximization" << endl;
        if (! clusters.size ())  // Catch the very special case that we jumped in during maximization
		{
		  cerr << "  need to read cluster file" << endl;
		  ifstream target (clusterFileName.c_str (), ios::binary);
		  if (! target.good ())
		  {
			cerr << "Unable to reopen cluster file " << clusterFileName << endl;
			exit (1);
		  }
		  read (target);
		  member.resize (clusters.size (), data->size ());
		}
		int unit;
		ss.read ((char *) &unit, sizeof (unit));
cerr << "  unit = " << unit << endl;
		for (int j = 0; j < data->size (); j++)
		{
		  ss.read ((char *) & member (unit, j), sizeof (float));
		}
		if (! ss.good ())
		{
cerr << ss.bad () << " " << ss.eof () << " " << ss.fail () << endl;
		  break;
		}
cerr << "  done receiving member" << endl;
		float change = maximize (*data, member, unit);
		ss.write ((char *) &change, sizeof (change));
cerr << "  about to write cluster" << endl;
		clusters[unit].write (ss);
		ss.flush ();
cerr << "  wrote cluster" << endl;
		break;
	  }
	  default:
		cerr << "exiting due to unrecognized command" << endl;
		return;
	}
  }

  cerr << "exiting due to bad stream" << endl;
}
