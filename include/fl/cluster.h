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


#ifndef fl_cluster_h
#define fl_cluster_h


#include "fl/matrix.h"
#include "fl/socket.h"
#include "fl/metric.h"
#include "fl/archive.h"

#include <iostream>
#include <vector>

#ifdef HAVE_PTHREAD
#  include <pthread.h>
#endif

#ifdef HAVE_LIBSVM
#  include <libsvm/svm.h>
#endif

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  // Generic clustering interface ---------------------------------------------

  class SHARED ClusterMethod
  {
  public:
	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes) = 0;  ///< Peform either supervised learning or clustering on collection of points.  @param classes may be arbitrary size, but association with data points starts at index 0.
	virtual void          run (const std::vector< Vector<float> > & data);  ///< Convenience method for calling run() in unsupervised case.
	virtual int           classify (const Vector<float> & point) = 0;  ///< Determine the single best class of given point.  Returns -1 if no class is suitable.
	virtual Vector<float> distribution (const Vector<float> & point) = 0;  ///< Return a probability distribution over the classes.  Row number in the returned Vector corresponds to class number.
	virtual int           classCount () = 0;  ///< Returns the number of classes.
	virtual Vector<float> representative (int group) = 0;  ///< Return a representative member of group.  "group" has same semantics as return value of classify (); we just can't use the word "class" because it is a keyword in C++.  :)

	/**
	   Serialize enough data to either resume clustering with a call to run ()
	   or to answer cluster queries via classify () and representative ().
	**/
	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	bool stop;  ///< If set true, signals run () to terminate at the next reasonable spot.  run () should clear this flag when it first starts, but only monitor it after that.
  };


  // Gaussian Mixture ---------------------------------------------------------

  class SHARED ClusterGauss
  {
  public:
	ClusterGauss ();
	ClusterGauss (Vector<float> & center, float alpha = 1.0);
	ClusterGauss (Vector<float> & center, Matrix<float> & covariance, float alpha = 1.0);
	~ClusterGauss ();

	void prepareInverse ();  ///< When covariance is changed, update cached information necessary to compute Mahalanobis distance.
	float probability (const Vector<float> & point, float * scale = NULL, float * minScale = NULL);  ///< The probability of being in the cluster, which is simply the Gaussian of the distance from the center.  Result is multiplied by exp (scale) if minScale == NULL; otherwise scale and minScale are updated, and result is unscaled.

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	float alpha;
	Vector<float> center;
	Matrix<float> covariance;
	Matrix<float> eigenvectors;
	Vector<float> eigenvalues;
	Matrix<float> eigenverse;
	float det;  ///< preprocessed multiplier that goes in front of probability expression.  Includes determinant of the covariance matrix.
  };

  class SHARED GaussianMixture : public ClusterMethod
  {
  public:
	GaussianMixture (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");  ///< clusterFileName refers to target file for new clustering data, which is very likely to be different from input file.
	GaussianMixture (const std::string & clusterFileName = "");

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	void serialize (Archive & archive, uint32_t version);

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

# ifdef HAVE_PTHREAD
  class SHARED GaussianMixtureParallel : public GaussianMixture, public Listener
  {
  public:
	GaussianMixtureParallel (float maxSize, float minSize, int initialK, int maxK, const std::string & clusterFileName = "");
	GaussianMixtureParallel (const std::string & clusterFileName = "");

	virtual void run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
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
# endif

  // Find a more elegant way to disseminate these constants!
#define workUnitSize 1000
#define portNumber   60000
  const float smallestNormalFloat = 1e-38;
  const float largestNormalFloat = 1e38;
  const float largestDistanceFloat = 87;  ///< = ln (1 / smallestNormalFloat); Actually distance squared, not distance.


  // KMeans -------------------------------------------------------------------

  class SHARED KMeans : public ClusterMethod
  {
  public:
	KMeans (int K = 0);

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	void serialize (Archive & archive, uint32_t version);

	int K;  ///< Desired number of clusters.  Can be changed any time.  Takes effect when run() is called.
	std::vector<Vector<float> > clusters;
  };


  // KMeansTree ---------------------------------------------------------------

  class SHARED KMeansTree : public ClusterMethod
  {
  public:
	KMeansTree (int K = 0, int depth = 1);
	~KMeansTree ();
	void clear ();

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	void serialize (Archive & archive, uint32_t version);

	KMeans kmeans;
	int depth; ///< Distance between this node and the its leaf clusters.  Total number of clusters is K^depth, so depth is always greater than zero.
	std::vector<KMeansTree *> subtrees;
  };
  

  // Kohonen map --------------------------------------------------------------

  class SHARED Kohonen : public ClusterMethod
  {
  public:
	Kohonen (int width = 10, float sigma = 1.0, float learningRate = 0.1, float decayRate = 0.5);

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	void serialize (Archive & archive, uint32_t version);

	Matrix<float> map;
	int width;  ///< Number of discrete positions in one dimension.
	float sigma;  ///< Of Gaussian that determines neighborhood to be updated.
	float learningRate;  ///< How much to scale feature vector during update.
	float decayRate;  ///< How much to scale learningRate after each iteration.
  };


  // Agglomerative clustering -------------------------------------------------

  class SHARED ClusterAgglomerative
  {
  public:
	ClusterAgglomerative ();
	ClusterAgglomerative (const Vector<float> & center, int count = 1);

	void operator += (const ClusterAgglomerative & that);

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	Vector<float> center;
	int count;  ///< Number of data represented by this cluster.
  };

  class SHARED Agglomerate : public ClusterMethod
  {
  public:
	Agglomerate ();
	Agglomerate (Metric * comparison, float distanceLimit, int minClusters = 1);
	~Agglomerate ();

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	using ClusterMethod::run;
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	void serialize (Archive & archive, uint32_t version);

	Metric * metric;
	float distanceLimit;  ///< The largest distance permissible between two clusters.
	int minClusters;  ///< The target number of clusters at convergence.  Result will be no smaller than this unless there are fewer input data.
	std::vector<ClusterAgglomerative *> clusters;
  };


  // SVM ----------------------------------------------------------------------

#ifdef HAVE_LIBSVM
  /**
	 Wrapper for libsvm.
   **/
  class SHARED SVM : public ClusterMethod
  {
  public:
	SVM ();
	~SVM ();
	void clear ();

	virtual void          run (const std::vector< Vector<float> > & data, const std::vector<int> & classes);
	virtual int           classify (const Vector<float> & point);
	virtual Vector<float> distribution (const Vector<float> & point);
	virtual int           classCount ();
	virtual Vector<float> representative (int group);

	svm_node *           vector2kernel (const Vector<float> & datum);
	static svm_node *    vector2node   (const Vector<float> & datum);
	static Vector<float> node2vector   (const svm_node * node);

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	svm_parameter parameters;
	svm_model *   model;

	Metric * metric;  ///< If nonzero, then use precumputed kernel
	std::vector<Vector<float> > data;  ///< Copy of training data.  Used to form vectors against precomputed kernel, when it is in use.
  };
#endif
}


#endif
