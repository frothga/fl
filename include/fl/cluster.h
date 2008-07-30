/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_cluster_h
#define fl_cluster_h


#include "fl/matrix.h"
#include "fl/socket.h"
#include "fl/metric.h"

#include <iostream>
#include <vector>


namespace fl
{
  // Generic clustering interface ---------------------------------------------

  class ClusterMethod
  {
  public:
	virtual void          run (const std::vector< Vector<float> > & data) = 0;  ///< Peform clustering on collection of points.
	virtual int           classify (const Vector<float> & point) = 0;  ///< Determine the single best class of given point.
	virtual Vector<float> distribution (const Vector<float> & point) = 0;  ///< Return a probability distribution over the classes.  Row number in the returned Vector corresponds to class number.
	virtual int           classCount () = 0;  ///< Returns the number of classes.
	virtual Vector<float> representative (int group) = 0;  ///< Return a representative member of group.  "group" has same semantics as return value of classify (); we just can't use the word "class" because it is a keyword in C++.  :)

	// The read () and write () methods should serialize enough data to either
	// resume clustering with a call to run () or to answer cluster queries via
	// classify () and representative ().
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	bool stop;  ///< If set true, signals run () to terminate at the next reasonable spot.  run () should clear this flag when it first starts, but only monitor it after that.
  };


  // Soft K-means -------------------------------------------------------------

  class ClusterGauss
  {
  public:
	ClusterGauss (Vector<float> & center, float alpha = 1.0);
	ClusterGauss (Vector<float> & center, Matrix<float> & covariance, float alpha = 1.0);
	ClusterGauss (std::istream & stream);  ///< Construct from stream
	~ClusterGauss ();

	void prepareInverse ();  ///< When covariance is changed, update cached information necessary to compute Mahalanobis distance.
	float probability (const Vector<float> & point, float * scale = NULL, float * minScale = NULL);  ///< The probability of being in the cluster, which is simply the Gaussian of the distance from the center.  Result is multiplied by exp (scale) if minScale == NULL; otherwise scale and minScale are updated, and result is unscaled.
	void read (std::istream & stream);
	void write (std::ostream & stream) const;

	float alpha;
	Vector<float> center;
	Matrix<float> covariance;
	Matrix<float> eigenvectors;
	Vector<float> eigenvalues;
	Matrix<float> eigenverse;
	float det;  ///< preprocessed multiplier that goes in front of probability expression.  Includes determinant of the covariance matrix.
  };

  class KMeans : public ClusterMethod
  {
  public:
	KMeans (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");  ///< clusterFileName refers to target file for new clustering data, which is very likely to be different from input file.
	KMeans (std::istream & stream, const std::string & clusterFileName = "");  ///< Construct from stream.

	virtual void          run (const std::vector< Vector<float> > & data);
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);
	virtual void          read (std::istream & stream);
	virtual void          write (std::ostream & stream) const;

	void initialize (const std::vector< Vector<float> > & data);
	void estimate (const std::vector< Vector<float> > & data, Matrix<float> & member, int jbegin, int jend);
	float maximize (const std::vector< Vector<float> > & data, const Matrix<float> & member, int i);
	bool convergence (const std::vector< Vector<float> > & data, const Matrix<float> & member, float largestChange);

	// State of clustering process
	float maxSize;  ///< Largest length of dominant axis of covariance matrix.  If any cluster exceeds this value, create a new cluster.
	float minSize;  ///< Closest that two clusters can be before they merge.
	int initialK;  ///< Lower bound on expected number of clusters.
	int maxK;  ///< Largest number of clusters allowed.
	std::vector<ClusterGauss> clusters;
	std::vector<float> changes;
	std::vector<float> velocities;

	// Control information
	std::string clusterFileName;
	time_t clusterFileTime;  ///< Time in seconds
	off_t clusterFileSize;
  };

  class KMeansParallel : public KMeans, public Listener
  {
  public:
	KMeansParallel (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");
	KMeansParallel (std::istream & stream, const std::string & clusterFileName = "");

	virtual void run (const std::vector< Vector<float> > & data);
	virtual void processConnection (fl::SocketStream & ss, struct sockaddr_in & clientAddress);

	static void * listenThread (void * arg);
	void client (std::string serverName);

	// Shared state for parallel processing
	int iteration;
	const std::vector<Vector<float> > * data;
	Matrix<float> member;
	float largestChange;
	enum EMstate
	{
	  initializing,
	  estimating,
	  maximizing,
	  checking
	};
	EMstate          state;
	pthread_mutex_t  stateLock;  ///< Used for all access to shared structures
	std::vector<int> workUnits;  ///< List of current tasks, identified only by ints.  Basically, these are positions in a well-defined loop.
	int              unitsPending;  ///< Number of workUnits still being worked on.  Allows for error recovery by monitoring completion as a separate concept from tasks claimed by a thread.
  };

  // Find a more elegant way to disseminate these constants!
#define workUnitSize 1000
#define portNumber   60000
  const float smallestNormalFloat = 1e-38;
  const float largestNormalFloat = 1e38;
  const float largestDistanceFloat = 87;  ///< = ln (1 / smallestNormalFloat); Actually distance squared, not distance.


  // Kohonen map --------------------------------------------------------------

  class ClusterCosine
  {
  public:
	ClusterCosine (int dimension);
	ClusterCosine (Vector<float> & center);
	ClusterCosine (std::istream & stream);

	float distance (const Vector<float> & point);
	float update (const Vector<float> & point, float weight);

	void read (std::istream & stream);
	void write (std::ostream & stream) const;

	Vector<float> center;
  };

  class Kohonen : public ClusterMethod
  {
  public:
	Kohonen (int width, float sigma = 1.0, float learningRate = 0.1, float decayRate = 0.5);
	Kohonen (std::istream & stream);

	virtual void          run (const std::vector< Vector<float> > & data);
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);
	virtual void          read (std::istream & stream);
	virtual void          write (std::ostream & stream) const;

	std::vector<ClusterCosine> map;
	int width;  ///< Number of discrete positions in one dimension.
	float sigma;  ///< Of Gaussian that determines neighborhood to be updated.
	float learningRate;  ///< How much to scale feature vector during update.
	float decayRate;  ///< How much to scale learningRate after each iteration.
  };


  // Agglomerative clustering -------------------------------------------------

  class ClusterAgglomerative
  {
  public:
	ClusterAgglomerative (const Vector<float> & center, int count = 1);
	ClusterAgglomerative (std::istream & stream);

	void operator += (const ClusterAgglomerative & that);
	void read (std::istream & stream);
	void write (std::ostream & stream) const;

	Vector<float> center;
	int count;  ///< Number of data represented by this cluster.
  };

  class Agglomerate : public ClusterMethod
  {
  public:
	Agglomerate (Metric * comparison, float distanceLimit, int minClusters = 1);
	Agglomerate (std::istream & stream);
	~Agglomerate ();

	virtual void          run (const std::vector< Vector<float> > & data);
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	Metric * metric;
	float distanceLimit;  ///< The largest distance permissible between two clusters.
	int minClusters;  ///< The target number of clusters at convergence.  Result will be no smaller than this unless there are fewer input data.
	std::vector<ClusterAgglomerative *> clusters;
  };
}


#endif
