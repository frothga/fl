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

  // Generate set of random clusters
  int K = min (initialK, (int) data.size () / 2);
  if (clusters.size () < K)
  {
	cerr << "Creating " << K - clusters.size () << " clusters" << endl;

	// Locate center and covariance of entire data set
	Vector<float> center (data[0].rows ());
	center.clear ();
	for (int i = 0; i < data.size (); i++)
    {
	  center += data[i];
	  if (i % 1000 == 0)
	  {
		cerr << ".";
	  }
	}
	cerr << endl;
	center /= data.size ();

	Matrix<float> covariance (data[0].rows (), data[0].rows ());
	covariance.clear ();
	for (int i = 0; i < data.size (); i++)
	{
	  Vector<float> delta = data[i] - center;
	  covariance += delta * ~delta;
	  if (i % 1000 == 0)
	  {
		cerr << ".";
	  }
	}
	cerr << endl;
	covariance /= data.size ();
cerr << "center: " << center << endl;

	// Prepare matrix of basis vectors on which to project the cluster centers
	Matrix<float> eigenvectors;
	Vector<float> eigenvalues;
	syev (covariance, eigenvalues, eigenvectors);
	float minev = largestNormalFloat;
	float maxev = 0;
	for (int i = 0; i < eigenvalues.rows (); i++)
	{
	  minev = min (minev, fabsf (eigenvalues[i]));
	  maxev = max (maxev, fabsf (eigenvalues[i]));
	}
	cerr << "eigenvalue range = " << sqrtf (minev) << " " << sqrtf (maxev) << endl;
	for (int c = 0; c < eigenvectors.columns (); c++)
	{
	  float scale = sqrt (fabs (eigenvalues[c]));
	  eigenvectors.column (c) *= scale;
	}

	// Throw points into the space and create clusters around them
	for (int i = clusters.size (); i < K; i++)
	{
	  Vector<float> point (center.rows ());
	  for (int row = 0; row < point.rows (); row++)
	  {
		point[row] = randGaussian ();
	  }

	  point = center + eigenvectors * point;

	  ClusterGauss c (point, 1.0 / K);
	  clusters.push_back (c);
	}
  }
  else
  {
	cerr << "KMeans already initialized with:" << endl;
	cerr << "  clusters = " << clusters.size () << endl;
	cerr << "  maxSize  = " << maxSize << endl;
	cerr << "  minSize  = " << minSize << endl;
	cerr << "  maxK     = " << maxK << endl;
	cerr << "  changes: ";
	for (int i = 0; i < changes.size (); i++)
	{
	  cerr << changes[i] << " ";
	}
	cerr << endl;
	cerr << "  velocities: ";
	for (int i = 0; i < velocities.size (); i++)
	{
	  cerr << velocities[i] << " ";
	}
	cerr << endl;
  }

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

	pthread_mutex_lock (&stateLock);
	state = checking;
	pthread_mutex_unlock (&stateLock);

	// Analyze movement data to detect convergence
	cerr << "change = " << largestChange << "\t";
	largestChange /= maxSize * sqrtf ((float) data[0].rows ());
	cerr << largestChange << "\t";
	// Calculate velocity
	changes.push_back (largestChange);
	if (changes.size () > 4)
	{
	  changes.erase (changes.begin ()); // Limit size of history
	  float xbar  = (changes.size () - 1) / 2.0;
	  float sxx   = 0;
	  float nybar = 0;
	  float sxy   = 0;
	  for (int x = 0; x < changes.size (); x++)
	  {
		sxx   += powf (x - xbar, 2);
		nybar += changes[x];
		sxy   += x * changes[x];
	  }
	  sxy -= xbar * nybar;
	  float velocity = sxy / sxx;
	  cerr << velocity << "\t";

	  // Calculate acceleration
	  velocities.push_back (velocity);
	  if (velocities.size () > 4)
	  {
		velocities.erase (velocities.begin ());
		xbar  = (velocities.size () - 1) / 2.0;
		sxx   = 0;
		nybar = 0;
		sxy   = 0;
		for (int x = 0; x < velocities.size (); x++)
		{
		  sxx   += powf (x - xbar, 2);
		  nybar += velocities[x];
		  sxy   += x * velocities[x];
		}
		sxy -= xbar * nybar;
		float acceleration = sxy / sxx;
		cerr << acceleration;
		if (fabs (acceleration) < 1e-4  &&  velocity > -1e-2)
		{
		  converged = true;
		}
	  }
 	}
	cerr << endl;
    if (largestChange < 1e-4)
    {
	  converged = true;
	}

	// Change number of clusters, if necessary
    if (stop)
    {
	  break;
	}
	if (converged)
	{
	  cerr << "checking K" << endl;

	  // The basic approach is to check the eigenvalues of each cluster
	  // to see if the shape of the cluster exceeds the arbitrary limit
	  // maxSize.  If so, pick the worst offending cluster and  split
      // it into two along the dominant axis.
	  float largestEigenvalue = 0;
	  Vector<float> largestEigenvector (data[0].rows ());
	  int largestCluster = 0;
	  for (int i = 0; i < clusters.size (); i++)
	  {
		int last = clusters[i].eigenvalues.rows () - 1;
		float evf = fabsf (clusters[i].eigenvalues[0]);
		float evl = fabsf (clusters[i].eigenvalues[last]);
		if (evf > largestEigenvalue)
		{
		  largestEigenvalue = evf;
		  largestEigenvector = clusters[i].eigenvectors.column (0);
		  largestCluster = i;
		}
		if (evl > largestEigenvalue)
		{
		  largestEigenvalue = evl;
		  largestEigenvector = clusters[i].eigenvectors.column (last);
		  largestCluster = i;
		}
	  }
	  largestEigenvalue = sqrtf (largestEigenvalue);
	  if (largestEigenvalue > maxSize  &&  clusters.size () < maxK)
	  {
		Vector<float> le;
		le.copyFrom (largestEigenvector);
		largestEigenvector *= largestEigenvalue / 2;  // largestEigenvector is already unit length
		le                 *= -largestEigenvalue / 2;
		largestEigenvector += clusters[largestCluster].center;
		clusters[largestCluster].center += le;
		clusters[largestCluster].alpha /= 2;
		ClusterGauss c (largestEigenvector, clusters[largestCluster].covariance, clusters[largestCluster].alpha);
		clusters.push_back (c);
		cerr << "  splitting: " << largestCluster << " " << largestEigenvalue << endl; // " "; print_vector (c.center);
		converged = false;
	  }

	  // Also check if we need to merge clusters.
	  // Merge when clusters are closer then minSize to each other by Euclidean distance.
	  int remove = -1;
	  int merge;
	  float closestDistance = largestNormalFloat;
	  for (int i = 0; i < clusters.size (); i++)
	  {
		for (int j = i + 1; j < clusters.size (); j++)
		{
		  float distance = (clusters[i].center - clusters[j].center).frob (2);
		  if (distance < minSize  &&  distance < closestDistance)
		  {
			merge = i;
			remove = j;
			closestDistance = distance;
		  }
		}
	  }
	  if (remove > -1)
	  {
		cerr << "  merging: " << merge << " " << remove << " " << closestDistance << endl;
		converged = false;
		// Cluster "merge" claims all of cluster "remove"s points.
		for (int j = 0; j < data.size (); j++)
		{
		  member (merge, j) += member (remove, j);
		}
		maximize (data, member, merge);
		clusters.erase (clusters.begin () + remove);
	  }
	}

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
		read (target, true);
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
		  read (target, true);
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
