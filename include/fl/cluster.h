#ifndef cluster_h
#define cluster_h


#include "linear.h"
#include "socket.h"

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


// Generic clustering interface -----------------------------------------------

class ClusterMethod
{
 public:
  static ClusterMethod * factory (std::istream & stream);

  virtual void     run (const std::vector<LaMatrix> & data) = 0;  // Peform clustering on collection of points.
  virtual int      classify (const LaMatrix & point) = 0;  // Determine class of given point.
  virtual LaMatrix representative (int group) = 0;  // Return a representative member of group.  "group" has same semantics as return value of classify (); we just can't use the word "class" because it is a keyword in C++.  :)

  // The read () and write () methods should serialize enough data to either
  // resume clustering with a call to run () or to answer cluster queries via
  // classify () and representative ().
  virtual void read (std::istream & stream, bool withName = false);
  virtual void write (std::ostream & stream);

  bool stop;  // If set true, signals run () to terminate at the next reasonable spot.  run () should clear this flag when it first starts, but only monitor it after that.
};


// Soft K-means ---------------------------------------------------------------

class ClusterGauss
{
 public:
  ClusterGauss (LaMatrix & center, float alpha = 1.0);
  ClusterGauss (LaMatrix & center, LaMatrix & covariance, float alpha = 1.0);
  ClusterGauss (std::istream & stream);  // Construct from stream
  ~ClusterGauss ();

  void prepareInverse ();  // When covariance is changed, update cached information necessary to compute Mahalanobis distance.
  float probability (const LaMatrix & point, float * scale = NULL, float * minScale = NULL);  // The probability of being in the cluster, which is simply the Gaussian of the distance from the center.  Result is multiplied by exp (scale) if minScale == NULL; otherwise scale and minScale are updated, and result is unscaled.
  //void read (FILE * stream);
  //void write (FILE * stream);  // Dump cluster data to stream (not human readable).
  void read (std::istream & stream);
  void write (std::ostream & stream);

  float alpha;
  LaMatrix center;
  LaMatrix covariance;
  LaMatrix eigenvectors;
  LaVector eigenvalues;
  LaMatrix eigenverse;
  float det;  // preprocessed multiplier that goes in front of probability expression.  Includes determinant of the covariance matrix.
};

class KMeans : public ClusterMethod
{
 public:
  KMeans (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");  // clusterFileName refers to target file for new clustering data, which is very likely to be different from input file.
  KMeans (std::istream & stream, const std::string & clusterFileName = "");  // Construct from stream.

  void run (const std::vector<LaMatrix> & data);
  int classify (const LaMatrix & point);
  LaMatrix representative (int group);
  void read (std::istream & stream, bool withName = false);
  void write (std::ostream & stream);

  void estimate (const std::vector<LaMatrix> & data, LaMatrix & member, int jbegin, int jend);
  float maximize (const std::vector<LaMatrix> & data, const LaMatrix & member, int i);
  //void read (FILE * stream);
  //void write (FILE * stream);

  // State of clustering process
  float maxSize;  // Largest length of dominant axis of covariance matrxi.  If any cluster exceeds this value, create a new cluster.
  float minSize;  // Closest that two clusters can be before they merge.
  int initialK;  // Lower bound on expected number of clusters.
  int maxK;  // Largest number of clusters allowed.
  std::vector<ClusterGauss> clusters;
  std::vector<float> changes;
  std::vector<float> velocities;

  // Control information
  std::string clusterFileName;
  time_t clusterFileTime;  // Time in seconds
  off_t clusterFileSize;
};

class KMeansParallel : public KMeans
{
 public:
  KMeansParallel (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");
  KMeansParallel (std::istream & stream, const std::string & clusterFileName = "");

  void run (const std::vector<LaMatrix> & data);

  static void * listenThread (void * arg);
  static void * proxyThread (void * arg);
  void client (std::string serverName);

  // Shared state for parallel processing
  int iteration;
  const std::vector<LaMatrix> * data;
  LaMatrix member;
  float largestChange;
  enum EMstate
  {
	initializing,
	estimating,
	maximizing,
	checking
  };
  EMstate          state;
  pthread_mutex_t  stateLock;  // Used for all access to shared structures
  std::vector<int> workUnits;  // List of current tasks, identified only by ints.  Basically, these are positions in a well-defined loop.
  int              unitsPending;  // Number of workUnits still being worked on.  Allows for error recovery by monitoring completion as a separate concept from tasks claimed by a thread.

  struct ThreadDataHolder
  {
	KMeansParallel * kmeans;
	int connection;
	struct sockaddr_in peer;
  };
};

#define workUnitSize 1000
#define portNumber   60000
const float smallestNormalFloat = 1e-38;
const float largestNormalFloat = 1e38;
const float largestDistanceFloat = 87;  // = ln (1 / smallestNormalFloat); Actually distance squared, not distance.


// Kohonen map ----------------------------------------------------------------

class ClusterCosine
{
 public:
  ClusterCosine (int dimension);
  ClusterCosine (LaMatrix & center);
  ClusterCosine (std::istream & stream);

  float distance (const LaMatrix & point);
  float update (const LaMatrix & point, float weight);

  void read (std::istream & stream);
  void write (std::ostream & stream);

  LaMatrix center;
};

class Kohonen : public ClusterMethod
{
 public:
  Kohonen (int width, float sigma = 1.0, float learningRate = 0.1, float decayRate = 0.5);
  Kohonen (std::istream & stream);

  void     run (const std::vector<LaMatrix> & data);
  int      classify (const LaMatrix & point);
  LaMatrix representative (int group);

  void read (std::istream & stream, bool withName = false);
  void write (std::ostream & stream);

  std::vector<ClusterCosine> map;
  int width;  // Number of discrete positions in one dimension.
  float sigma;  // Of Gaussian that determines neighborhood to be updated.
  float learningRate;  // How much to scale feature vector during update.
  float decayRate;  // How much to scale learningRate after each iteration.
};


#endif
