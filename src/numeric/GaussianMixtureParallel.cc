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
#include <pthread.h>


using namespace fl;
using namespace std;


// class GaussianMixtureParallel ----------------------------------------------

GaussianMixtureParallel::GaussianMixtureParallel (float maxSize, float minSize, int initialK, int maxK, const string & clusterFileName)
: GaussianMixture (maxSize, minSize, initialK, maxK, clusterFileName),
  Listener (4000)
{
}

GaussianMixtureParallel::GaussianMixtureParallel (const string & clusterFileName)
: GaussianMixture (clusterFileName),
  Listener (4000)
{
}

void
GaussianMixtureParallel::run (const vector<Vector<float> > & data, const vector<int> & classes)
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
	  Archive (clusterFileName, "w") & *this;
	}
	else
	{
	  cout.flush ();
	  rewind (stdout);
	  Archive (cout) & *this;
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

void *
GaussianMixtureParallel::listenThread (void * arg)
{
cerr << "starting listenThread" << endl;
  GaussianMixtureParallel * me = (GaussianMixtureParallel *) arg;
  me->listen (portNumber);
  return NULL;
}

void
GaussianMixtureParallel::processConnection (SocketStream & ss, struct sockaddr_in & clientAddress)
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
		  Archive ((istream &) ss) & clusters[unit];
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
GaussianMixtureParallel::client (std::string serverName)
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
		Archive target (clusterFileName, "r");
        if (! target.in->good ())
		{
		  cerr << "Unable to reopen cluster file " << clusterFileName << endl;
		  exit (1);
		}
		target & *this;
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
		  Archive target (clusterFileName, "r");
		  if (! target.in->good ())
		  {
			cerr << "Unable to reopen cluster file " << clusterFileName << endl;
			exit (1);
		  }
		  target & *this;
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
		Archive ((ostream &) ss) & clusters[unit];
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
