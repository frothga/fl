#include "fl/cluster.h"
#include "fl/time.h"
#include "fl/random.h"
#include "fl/socket.h"
#include "fl/lapacks.h"

#include <algorithm>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fstream>


using namespace fl;
using namespace std;


// class KMeansParallel -------------------------------------------------------

KMeansParallel::KMeansParallel (float maxSize, float minSize, int initialK, int maxK, const string & clusterFileName)
: KMeans (maxSize, minSize, initialK, maxK, clusterFileName)
{
}

KMeansParallel::KMeansParallel (istream & stream, const string & clusterFileName)
: KMeans (stream, clusterFileName)
{
}

void
KMeansParallel::run (const std::vector<Vector<float> > & data)
{
  this->data = &data;

  // Fire up parallel processing
  stop = false;
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
  while (! converged  &&  ! stop)
  {
	cerr << "========================================================" << iteration++ << endl;
	double timestamp = getTimestamp ();

    // We assume that one iteration takes a very long time, so the cost of
    // dumping our state every time is relatively small (esp. compared to the
    // cost of losing everything in a crash).
	if (clusterFileName.size ())
	{
	  ofstream target (clusterFileName.c_str ());
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
	while (unitsPending  &&  ! stop)
	{
	  sleep (1);
	}
	cerr << endl;
	if (stop)
	{
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
	while (unitsPending  &&  ! stop)
	{
	  sleep (1);
	}
	if (stop)
	{
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
KMeansParallel::listenThread (void * arg)
{
cerr << "starting listenThread" << endl;
  KMeansParallel * kmeans = (KMeansParallel *) arg;

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

  while (! kmeans->stop)
  {
	fd_set readfds;
	FD_ZERO (&readfds);
	FD_SET (sock, &readfds);
	int result = select (sock + 1, &readfds, NULL, NULL, NULL);  // No timeout, because select is interruptable by signals, which is all we need.
	if (result == 0)
	{
	  continue;
	}
	if (result < 0)
	{
	  cerr << "listenThread shutting down due to error: " << strerror (errno) << endl;
	  break;
	}

	ThreadDataHolder * td = new ThreadDataHolder;
	td->kmeans = kmeans;

	socklen_t size;
	if ((td->connection = accept (sock, (sockaddr *) &td->peer, &size)) == -1)
	{
	  cerr << "accept failed: " << strerror (errno) << endl;
	  continue;
	}

	pthread_t pid;
	pthread_create (&pid, NULL, proxyThread, td);
  }

cerr << "exiting listenThread" << endl;
  return NULL;
}

void *
KMeansParallel::proxyThread (void * arg)
{
  ThreadDataHolder * td   = (ThreadDataHolder *) arg;
  KMeansParallel * kmeans = td->kmeans;
  struct in_addr address  = td->peer.sin_addr;
  int connection          = td->connection;
  delete td;

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

  const vector<Vector<float> > & data = *kmeans->data;
  SocketStream ss (connection, 4000);

  int lastIteration = -1;
  while (ss.good ()  &&  ! kmeans->stop)
  {
	pthread_mutex_lock (&kmeans->stateLock);
	EMstate state = kmeans->state;
	int unit = -1;
	if (kmeans->workUnits.size ())
	{
	  unit = kmeans->workUnits.back ();
	  kmeans->workUnits.pop_back ();
	}
	pthread_mutex_unlock (&kmeans->stateLock);

	if (kmeans->stop)
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
		if (lastIteration != kmeans->iteration)
		{
		  lastIteration = kmeans->iteration;
		  command = 1;  // Re-read cluster file
		  ss.write ((char *) &command, sizeof (command));
		  // The following trivia will help the client determine when NFS is
		  // presenting a current copy of the file.
		  ss.write ((char *) &kmeans->clusterFileSize, sizeof (kmeans->clusterFileSize));
		  ss.write ((char *) &kmeans->clusterFileTime, sizeof (kmeans->clusterFileTime));
		  ss.flush ();
		}
		command = 2;  // Perform estimation
		ss.write ((char *) &command, sizeof (command));
		ss.write ((char *) &unit, sizeof (unit));
		ss.flush ();
		int jbegin = workUnitSize * unit;
		int jend = jbegin + workUnitSize;
		jend = min (jend, (int) data.size ());
		ss.read ((char *) & kmeans->member (0, jbegin), kmeans->clusters.size () * (jend - jbegin) * sizeof (float));
//cerr << ss.gcount () << " " << kmeans->clusters.size () * (jend - jbegin) * sizeof (float) << endl;
		if (ss.good ())
		{
		  pthread_mutex_lock (&kmeans->stateLock);
		  kmeans->unitsPending--;
		  pthread_mutex_unlock (&kmeans->stateLock);
          cerr << ".";
		}
		else
		{
		  pthread_mutex_lock (&kmeans->stateLock);
		  kmeans->workUnits.push_back (unit);
		  pthread_mutex_unlock (&kmeans->stateLock);
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
		  ss.write ((char *) & kmeans->member (unit, j), sizeof (float));
		}
		ss.flush ();
		float change;
		ss.read ((char *) &change, sizeof (change));
		try
		{
		  kmeans->clusters[unit].read (ss);
		}
		catch (...)
		{
		  // The stream is down.  It should already be marked bad, which will
		  // be handled correctly below.
		}
		if (ss.good ())
		{
		  pthread_mutex_lock (&kmeans->stateLock);
		  kmeans->largestChange = max (kmeans->largestChange, change);
		  kmeans->unitsPending--;
		  pthread_mutex_unlock (&kmeans->stateLock);
		  float minev = largestNormalFloat;
		  float maxev = 0;
		  for (int j = 0; j < kmeans->clusters[unit].eigenvalues.rows (); j++)
		  {
			minev = min (minev, fabsf (kmeans->clusters[unit].eigenvalues[j]));
			maxev = max (maxev, fabsf (kmeans->clusters[unit].eigenvalues[j]));
		  }
		  cerr << unit << " = " << kmeans->clusters[unit].alpha << " " << change << " " << sqrt (minev) << " " << sqrt (maxev) << endl;
		}
		else
		{
		  pthread_mutex_lock (&kmeans->stateLock);
		  kmeans->workUnits.push_back (unit);
		  pthread_mutex_unlock (&kmeans->stateLock);
cerr << peer << " put back " << unit << endl;
		}
		break;
	  }
	  default:
		sleep (1);
	}
  }

  close (connection);
cerr << peer << " exiting proxyThread" << endl;
  return NULL;
}

void
KMeansParallel::client (std::string serverName)
{
  // Open connection to server
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

  char hostname[256];
  gethostname (hostname, sizeof (hostname));
  strtok (hostname, ".");
  cerr << "Connection complete: " << serverName << " " << hostname << endl;

  SocketStream ss (connection);

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
		ifstream target (clusterFileName.c_str ());
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
		  ifstream target (clusterFileName.c_str ());
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
  close (connection);
}
