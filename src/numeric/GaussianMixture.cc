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


const float smallestNormalFloat = 1e-38;
const float largestNormalFloat = 1e38;
const float largestDistanceFloat = 87;  ///< = ln (1 / smallestNormalFloat); Actually distance squared, not distance.


// class ClusterGauss ---------------------------------------------------------

uint32_t ClusterGauss::serializeVersion = 0;

ClusterGauss::ClusterGauss ()
{
}

ClusterGauss::ClusterGauss (const Vector<float> & center, float alpha)
{
  this->alpha = alpha;
  this->center.copyFrom (center);
  covariance.resize (center.rows (), center.rows ());
  covariance.identity ();
  prepareInverse ();
}

ClusterGauss::ClusterGauss (const Vector<float> & center, const Matrix<float> & covariance, float alpha)
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
  const int rows = eigenvectors.columns ();  // rows of eigenverse, which is transpose of eigenvectors
  const int cols = eigenvectors.rows ();
  eigenverse.resize (rows, cols);
  float mantissa = 1.0;
  int exponent = 0;  // determinant=mantissa*2^exponent
  int dimension = 0;
  for (int i = 0; i < rows; i++)
  {
	float s = sqrt (fabs (eigenvalues[i]));
	if (s == 0)
	{
	  for (int j = 0; j < cols; j++)
	  {
		eigenverse (i, j) = 0;
	  }
	}
	else
	{
	  for (int j = 0; j < cols; j++)
	  {
		eigenverse (i, j) = eigenvectors (j, i) / s;
	  }
	}

	// If an eigenvalue is zero, then we are effectively flat in some dimension.
	// Simply act as if we are a lower-dimensional cluster, so still compute
	// normalization factor for non-zero values.
	if (eigenvalues[i] != 0)
	{
	  dimension++;
	  mantissa *= TWOPIf * eigenvalues[i];
	  int te;  // "temp exponent"
	  mantissa = frexp (mantissa, &te);
	  exponent += te;
	}
  }
  if (mantissa < 0)
  {
	cerr << "warning: negative determinant" << endl;
	mantissa = -mantissa;
  }
  if (dimension == 0) det = 0;  // this cluster is bad
  else                det = 0.5 * (logf (mantissa) + exponent * logf (2.0));  // when applied below, turns into sqrt(determinant) in denominator of probability expression
}

float
ClusterGauss::probability (const Vector<float> & point, float * scale, float * minScale)
{
  if (det == 0) return 0;  // can no longer function as a cluster, because our covariance has collapsed

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
: maxSize         (maxSize),
  minSize         (minSize),
  initialK        (initialK),
  maxK            (maxK),
  clusterFileName (clusterFileName)
{
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
  Matrix<float> member (clusters.size (), data.size ());
  member.clear ();
  bestChange = data.size ();
  bestRadius = INFINITY;
  lastChange = 0;
  lastRadius = 0;

  // Iterate to convergence.  Convergence condition is that cluster
  // centers are stable and that all features fall within maxSize of
  // nearest cluster center.
  int iteration = 0;
  while (! stop)
  {
	cerr << "========================================================" << iteration++ << endl;
	double timestamp = getTimestamp ();

    // We assume that one iteration takes a very long time, so the cost of
    // dumping our state every time is relatively small (esp. compared to the
    // cost of losing everything in a crash).
	if (clusterFileName.size ()) Archive (clusterFileName, "w") & *this;

	// Estimation: Generate probability of membership for each datum in each cluster.
	float changes = estimate (data, member, 0, data.size ());
	if (stop) break;

	// Maximization: Update clusters based on member data.
	cerr << clusters.size () << endl;
	for (int i = 0; i < clusters.size (); i++) maximize (data, member, i);
	if (stop) break;

	if (convergence (data, member, changes)) stop = true;

	cerr << "time = " << getTimestamp () - timestamp << endl;
  }
}

void
GaussianMixture::initialize (const vector<Vector<float> > & data)
{
  // Generate set of random clusters
  int K = min (initialK, (int) data.size ());
  if (clusters.size () < K)
  {
	cerr << "Creating " << K - clusters.size () << " clusters" << endl;

	// Locate center and covariance of entire data set
	Vector<float> center (data[0].rows ());
	center.clear ();
	for (int i = 0; i < data.size (); i++)
    {
	  center += data[i];
	  if (i % 1000 == 0) cerr << ".";
	}
	cerr << endl;
	center /= data.size ();

	Matrix<float> covariance (data[0].rows (), data[0].rows ());
	covariance.clear ();
	for (int i = 0; i < data.size (); i++)
	{
	  Vector<float> delta = data[i] - center;
	  covariance += delta * ~delta;
	  if (i % 1000 == 0) cerr << ".";
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
	if (K == 1)
	{
	  ClusterGauss c (center, covariance, 1);
	  clusters.push_back (c);
	}
	else
	{
	  for (int i = clusters.size (); i < K; i++)
	  {
		Vector<float> point (center.rows ());
		for (int row = 0; row < point.rows (); row++)
		{
		  point[row] = randGaussian ();
		}

		point = center + eigenvectors * point;

		ClusterGauss c (point, covariance, 1.0 / K);
		clusters.push_back (c);
	  }
	}
  }
  else if (clusters.size () > 0)
  {
	cerr << "GaussianMixture already initialized with:" << endl;
	cerr << "  clusters = " << clusters.size () << endl;
	cerr << "  maxSize  = " << maxSize << endl;
	cerr << "  minSize  = " << minSize << endl;
	cerr << "  maxK     = " << maxK << endl;
  }
}

float
GaussianMixture::estimate (const vector<Vector<float> > & data, Matrix<float> & member, int jbegin, int jend)
{
  float changes = 0;

  for (int j = jbegin; j < jend; j++)
  {
    // TODO: The current approach to normalization below forces everything
	// to be calculated twice.  Instead, we should remember the "scale" value
	// for each feature between calls and adapt it as we go.
	float scale = 0;
	float minScale = largestNormalFloat;
	Vector<float> newMembership (clusters.size ());
	MatrixResult<float> oldMembership = member.column (j);
	for (int i = 0; i < clusters.size (); i++)
	{
	  float value = clusters[i].probability (data[j], &scale, &minScale);
	  newMembership[i] = value;
	}

	// The following case compensates for lack of numerical resolution.
	float sum = newMembership.norm (1);
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
	  for (int i = 0; i < clusters.size (); i++)
	  {
		float value = clusters[i].probability (data[j], &scale);
		newMembership[i] = value;
	  }
	}

	newMembership /= newMembership.norm (1); // Divide by sum (1-norm) rather than 2-norm, because we want a probability distribution (sum to 1) rather than a unit vector.
	float oldNorm = oldMembership.norm (2);
	if (oldNorm == 0) changes += 1;
	else              changes += 1 - oldMembership.dot (newMembership) / (oldNorm * newMembership.norm (2));
	oldMembership = newMembership;
  }

  return changes;
}

void
GaussianMixture::maximize (const vector<Vector<float> > & data, const Matrix<float> & member, int i)
{
  if (clusters[i].det == 0) return;  // no point in maintaining a cluster that has collapsed

  // Calculute new cluster center
  Vector<float> & center = clusters[i].center;
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
	delta *= member(i,j);
	covariance += delta * ~delta;
  }
  covariance /= sum;
  if (covariance.norm (1) == 0)
  {
	cerr << "warning: covariance went to zero; computing fallback value" << endl;
	// Most likely cause is not enough data to compute covariance.
	// This can happen, for example, if the cluster has effectively only 1 member.
	// Solution is to create a sphere that reaches half way to nearest cluster,
	// so this cluster can claim a reasonable share of surrounding points.
	float smallestDistance = INFINITY;
	for (int j = 0; j < clusters.size (); j++)
	{
	  if (j == i) continue;  // don't compare to ourselves!
	  smallestDistance = min (smallestDistance, (center - clusters[j].center).sumSquares ());
	}
	covariance.identity (smallestDistance / 4);  // (half of Euclidean distance)^2
  }

  clusters[i].prepareInverse ();
}

bool
GaussianMixture::convergence (const vector<Vector<float> > & data, Matrix<float> & member, float changes)
{
  // Evaluate if we have converged under the current cluster arrangement
  cerr << "changes = " << changes << " " << bestChange << " " << lastChange << endl;
  bool converged = false;
  if (changes < 1e-4)  // A "change" value of 1 is equivalent to one data item moving entirely from one cluster to another in KMeans.
  {
	converged = true;
  }
  else if (changes < bestChange)
  {
	bestChange = changes;
	lastChange = 0;
  }
  else if (++lastChange > 3) converged = true;

  // Purge collapsed clusters
  for (int i = clusters.size () - 1; i >= 0; i--)
  {
	if (clusters[i].det == 0)
	{
	  clusters.erase (clusters.begin () + i);
	  if (i < member.rows () - 1) member.region (i, 0) = member.region (i+1, 0);
	  member.rows_--;
	}
  }

  // Change number of clusters, if necessary
  if (converged)
  {
	cerr << "checking K" << endl;

	// The basic approach is to check the eigenvalues of each cluster
	// to see if the shape of the cluster exceeds the arbitrary limit
	// maxSize.  If so, pick the worst offending cluster and split
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
	  if (largestEigenvalue < bestRadius)
	  {
		bestRadius = largestEigenvalue;
		lastRadius = 0;
	  }
	  else lastRadius++;

	  if (lastRadius < 3)
	  {
		converged = false;  // change in # of clusters, so keep going
		cerr << "  splitting: " << largestCluster << " " << largestEigenvalue << " " << bestRadius << " " << lastRadius << endl;

		Vector<float> half;
		half.copyFrom (largestEigenvector);
		half *= largestEigenvalue / 2;  // largestEigenvector is already unit length
		clusters[largestCluster].alpha /= 2;
		clusters.push_back (ClusterGauss (clusters[largestCluster].center - half, clusters[largestCluster].covariance, clusters[largestCluster].alpha));
		clusters[largestCluster].center += half;

		int newRows = clusters.size ();
		Matrix<float> newMember (newRows, member.columns ());
		newMember.region (0, 0) = member;
		newMember.row (newRows - 1).clear ();
		member = newMember;
	  }
	}

	// Also check if we need to merge clusters.
	// Merge when clusters are closer then minSize to each other by Euclidean distance.
	int remove = -1;
	int merge;
	float closestGap = largestNormalFloat;
	for (int i = 0; i < clusters.size (); i++)
	{
	  for (int j = i + 1; j < clusters.size (); j++)
	  {
		float gap = (clusters[i].center - clusters[j].center).norm (2);
		if (gap < minSize  &&  gap < closestGap)
		{
		  merge = i;
		  remove = j;
		  closestGap = gap;
		}
	  }
	}
	if (remove > -1)
	{
	  converged = false;
	  cerr << "  merging: " << merge << " " << remove << " " << closestGap << endl;

	  // Cluster "merge" claims all of cluster "remove"s points.
	  member.row (merge) += member.row (remove);
	  if (remove < member.rows () - 1) member.region (remove, 0) = member.region (remove+1, 0);  // in-place move
	  member.rows_--;  // hack into matrix structure to claim smaller size; TODO: create functions in Matrix interface for adding/removing rows/columns
	  maximize (data, member, merge);
	  clusters.erase (clusters.begin () + remove);
	}

	if (! converged)
	{
	  bestChange = data.size ();
	  lastChange = 0;
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
  if (archive.out) clusterFileTime = time (NULL);

  archive & *((ClusterMethod *) this);

  archive & maxSize;
  archive & minSize;
  archive & initialK;
  archive & maxK;

  archive & clusters;

  archive & bestChange;
  archive & bestRadius;
  archive & lastChange;
  archive & lastRadius;

  if (archive.out) clusterFileSize = archive.out->tellp ();
}
