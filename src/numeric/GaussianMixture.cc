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
#include "fl/lapack.h"
#include "fl/random.h"
#include "fl/time.h"

#include <algorithm>
#include <fstream>
#include <time.h>


using namespace std;
using namespace fl;


// class ClusterGauss ---------------------------------------------------------

uint32_t ClusterGauss::serializeVersion = 0;

ClusterGauss::ClusterGauss ()
{
}

ClusterGauss::ClusterGauss (Vector<float> & center, float alpha)
{
  this->alpha = alpha;
  this->center.copyFrom (center);
  covariance.resize (center.rows (), center.rows ());
  covariance.identity ();
  prepareInverse ();
}

ClusterGauss::ClusterGauss (Vector<float> & center, Matrix<float> & covariance, float alpha)
{
  this->alpha = alpha;
  this->center.copyFrom (center);
  this->covariance.copyFrom (covariance);
  prepareInverse ();
}

ClusterGauss::~ClusterGauss ()
{
}

void
ClusterGauss::prepareInverse ()
{
  syev (covariance, eigenvalues, eigenvectors);
  eigenverse.resize (eigenvectors.columns (), eigenvectors.rows ());
  float d = 1.0;
  int scale = 0;
  for (int i = 0; i < eigenverse.rows (); i++)
  {
	float s = sqrt (fabs (eigenvalues[i]));
	if (s == 0)
	{
	  for (int j = 0; j < eigenverse.columns (); j++)
	  {
		eigenverse (i, j) = 0;
	  }
	}
	else
	{
	  for (int j = 0; j < eigenverse.columns (); j++)
	  {
		eigenverse (i, j) = eigenvectors (j, i) / s;
	  }
	}
	d *= TWOPIf * eigenvalues[i];
	int ts;  // "temp scale"
	d = frexp (d, &ts);
	scale += ts;
  }
  if (d < 0)
  {
	cerr << "warning: there is a negative eigenvalue" << endl;
	d = fabsf (d);
  }
  det = 0.5 * (logf (d) + scale * logf (2.0));
}

float
ClusterGauss::probability (const Vector<float> & point, float * scale, float * minScale)
{
  Vector<float> tm = eigenverse * (point - center);
  float d2 = tm.dot (tm);  // d2 is the true distance squared.
  d2 = min (d2, largestNormalFloat);
  float distance = d2 / 2.0 - logf (alpha) + det;  // "distance" takes into account the rest of the probability formula; suitable for scaling.
  if (scale)
  {
	if (minScale)
	{
	  float needScale = distance - largestDistanceFloat;
	  *scale = max (*scale, needScale);
	  *minScale = min (*minScale, needScale);
	}
	else
	{
	  return expf (*scale - distance);
	}
  }
  return max (expf (-distance), smallestNormalFloat);
}

void
ClusterGauss::serialize (Archive & archive, uint32_t version)
{
  archive & alpha;
  archive & center;
  archive & covariance;

  if (archive.in) prepareInverse ();
}


// class GaussianMixture ------------------------------------------------------

GaussianMixture::GaussianMixture (float maxSize, float minSize, int initialK, int maxK, const string & clusterFileName)
{
  this->maxSize  = maxSize;
  this->minSize  = minSize;
  this->initialK = initialK;
  this->maxK     = maxK;

  this->clusterFileName = clusterFileName;
}

GaussianMixture::GaussianMixture (const string & clusterFileName)
{
  this->clusterFileName = clusterFileName;
}

void
GaussianMixture::run (const vector<Vector<float> > & data, const vector<int> & classes)
{
  stop = false;
  initialize (data);

  // Iterate to convergence.  Convergence condition is that cluster
  // centers are stable and that all features fall within maxSize of
  // nearest cluster center.
  int iteration = 0;
  bool converged = false;
  while (! converged  &&  ! stop)
  {
	cerr << "========================================================" << iteration++ << endl;
	double timestamp = getTimestamp ();

    // We assume that one iteration takes a very long time, so the cost of
    // dumping our state every time is relatively small (esp. compared to the
    // cost of losing everything in a crash).
	if (clusterFileName.size ()) Archive (clusterFileName, "w") & *this;

	// Estimation: Generate probability of membership for each datum in each cluster.
	Matrix<float> member (clusters.size (), data.size ());
	estimate (data, member, 0, data.size ());
	if (stop)
	{
	  break;
	}

	// Maximization: Update clusters based on member data.
	cerr << clusters.size () << endl;
	float largestChange = 0;
	for (int i = 0; i < clusters.size (); i++)
	{
	  float change = maximize (data, member, i);
	  largestChange = max (largestChange, change);
	}
	if (stop)
	{
	  break;
	}

	converged = convergence (data, member, largestChange);

	cerr << "time = " << getTimestamp () - timestamp << endl;
  }
}

void
GaussianMixture::initialize (const vector<Vector<float> > & data)
{
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
	syev (covariance, eigenvalues, eigenvectors, true);
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
	cerr << "GaussianMixture already initialized with:" << endl;
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
}

void
GaussianMixture::estimate (const vector<Vector<float> > & data, Matrix<float> & member, int jbegin, int jend)
{
  for (int j = jbegin; j < jend; j++)
  {
    // TODO: The current approach to normalization below forces everything
	// to be calculated twice.  Instead, we should remember the "scale" value
	// for each feature between calls and adapt it as we go.
	float sum = 0;
	float scale = 0;
	float minScale = largestNormalFloat;
	for (int i = 0; i < clusters.size (); i++)
	{
	  float value = clusters[i].probability (data[j], &scale, &minScale);
	  member (i, j) = value;
	  sum += value;
	}
	// The following case compensates for lack of numerical resolution.
	if (sum <= smallestNormalFloat * (clusters.size () + 1)  ||  isinf (sum)  ||  isnan (sum))
	{
	  const float safetyMargin = 10;
	  if (scale - minScale > 2 * largestDistanceFloat - safetyMargin)
	  {
		scale = minScale + 2 * largestDistanceFloat - safetyMargin;
	  }
	  else
	  {
		scale += safetyMargin;
	  }
	  sum = 0;
	  for (int i = 0; i < clusters.size (); i++)
	  {
		float value = clusters[i].probability (data[j], &scale);
		member (i, j) = value;
		sum += value;
	  }
	}
	member.column (j) /= sum;
  }
}

float
GaussianMixture::maximize (const vector<Vector<float> > & data, const Matrix<float> & member, int i)
{
  // Calculute new cluster center
  Vector<float> center (data[0].rows ());
  center.clear ();
  float sum = 0;
  for (int j = 0; j < data.size (); j++)
  {
	center += data[j] * member (i, j);
	sum += member (i, j);
  }
  center /= sum;

  // Update alpha
  clusters[i].alpha = sum / data.size ();
  if (clusters[i].alpha <= smallestNormalFloat)
  {
	cerr << "alpha got too small " << clusters[i].alpha << endl;
    clusters[i].alpha = smallestNormalFloat;
  }

  // Calculate new covariance matrix
  Matrix<float> & covariance = clusters[i].covariance;
  covariance.clear ();
  for (int j = 0; j < data.size (); j++)
  {
	Vector<float> delta = data[j] - center;
	covariance += delta * ~delta *= member (i, j);
  }
  covariance /= sum;
  if (covariance.norm (1) == 0)
  {
	cerr << "covariance went to zero; setting to I * " << smallestNormalFloat << endl;
	covariance.identity (smallestNormalFloat);
  }

  // Record changes to cluster.  While we are at it, detect if we have
  // converged.
  float result = (center - clusters[i].center).norm (2);
  clusters[i].center = center;
  clusters[i].prepareInverse ();

  return result;
}

bool
GaussianMixture::convergence (const vector<Vector<float> > & data, const Matrix<float> & member, float largestChange)
{
  bool converged = false;

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
		float distance = (clusters[i].center - clusters[j].center).norm (2);
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
	  if (remove < member.rows ())  // Test is necessary because "remove" could be newly created cluster (see above) with no row in member yet.
	  {
		for (int j = 0; j < data.size (); j++)
		{
		  member (merge, j) += member (remove, j);
		}
		maximize (data, member, merge);
	  }
	  clusters.erase (clusters.begin () + remove);
	}
  }

  return converged;
}

int
GaussianMixture::classify (const Vector<float> & point)
{
  int result = -1;
  float highest = smallestNormalFloat;
  for (int i = 0; i < clusters.size (); i++)
  {
	float value = clusters[i].probability (point);
	if (value > highest)
	{
	  result = i;
	  highest = value;
	}
  }

  return result;
}

Vector<float>
GaussianMixture::distribution (const Vector<float> & point)
{
  Vector<float> result (clusters.size ());
  vector< Vector<float> > data;
  data.push_back (point);
  estimate (data, result, 0, 0);
  return result;
}

int
GaussianMixture::classCount ()
{
  return clusters.size ();
}

Vector<float>
GaussianMixture::representative (int group)
{
  return clusters[group].center;
}

void
GaussianMixture::serialize (Archive & archive, uint32_t version)
{
  cerr << "top of write" << endl;
  if (archive.out) clusterFileTime = time (NULL);

  archive & *((ClusterMethod *) this);

  archive & maxSize;
  archive & minSize;
  initialK = clusters.size ();
  archive & initialK;
  archive & maxK;

  archive & clusters;
  archive & changes;
  archive & velocities;

  if (archive.out) clusterFileSize = archive.out->tellp ();
  cerr << "bottom of write" << endl;
}
