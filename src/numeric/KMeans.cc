#include "cluster.h"
#include "inline.h"
#include "linear.h"

#include <mtl/utils.h>
#include <algorithm>

using namespace std;


// class ClusterGauss ---------------------------------------------------------

ClusterGauss::ClusterGauss (LaMatrix & center, float alpha)
: covariance   (center.nrows (), center.nrows ()),
  eigenvectors (center.nrows (), center.nrows ()),
  eigenvalues  (center.nrows ()),
  eigenverse   (center.nrows (), center.nrows ())
{
  this->alpha = alpha;
  this->center = LaMatrix (center.nrows (), 1);
  copy (center, this->center);
  set_diagonal (covariance, 1);
  prepareInverse ();
}

ClusterGauss::ClusterGauss (LaMatrix & center, LaMatrix & covariance, float alpha)
: eigenvectors (center.nrows (), center.nrows ()),
  eigenvalues  (center.nrows ()),
  eigenverse   (center.nrows (), center.nrows ())
{
  this->alpha = alpha;
  this->center = LaMatrix (center.nrows (), 1);
  copy (center, this->center);
  this->covariance = LaMatrix (center.nrows (), center.nrows ());
  copy (covariance, this->covariance);
  prepareInverse ();
}

ClusterGauss::ClusterGauss (istream & stream)
{
  read (stream);
}

ClusterGauss::~ClusterGauss ()
{
}

void
ClusterGauss::prepareInverse ()
{
  syev (covariance, eigenvalues, eigenvectors);
  float d = 1.0;
  int scale = 0;
  for (int i = 0; i < eigenvectors.ncols (); i++)
  {
	float s = sqrt (fabs (eigenvalues[i]));
	if (s == 0)
	{
	  mtl::set (rows (eigenverse)[i], 0);
	}
	else
	{
	  for (int j = 0; j < eigenvectors.nrows (); j++)
	  {
		eigenverse (i, j) = eigenvectors (j, i) / s;
	  }
	}
	d *= 2.0 * PI * eigenvalues[i];
	int ts;  // "temp scale"
	d = frexp (d, &ts);
	scale += ts;
  }
  if (d < 0)
  {
	cerr << "warning: there is a negative eigenvalue" << endl;
	d = fabs (d);
  }
  det = 0.5 * (log (d) + scale * log (2.0));
}

float
ClusterGauss::probability (const LaMatrix & point, float * scale, float * minScale)
{
  LaMatrix b = point - center;
  LaMatrix tm = eigenverse * b;
  float d2 = sum_squares (tm[0]);  // d2 is the true distance squared.
  d2 = min (d2, largestNormalFloat);
  float distance = d2 / 2.0 - log (alpha) + det;  // "distance" takes into account the rest of the probability formula; suitable for scaling.
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
	  return exp (*scale - distance);
	}
  }
  return max (exp (-distance), smallestNormalFloat);
}

void
ClusterGauss::read (istream & stream)
{
  // Extract alpha
  stream.read ((char *) &alpha, sizeof (alpha));

  // Extract center
  int dimension;
  if (! stream.read ((char *) &dimension, sizeof (dimension)))
  {
	throw "Stream is down.  Can't read dimension or finish constructing.";
  }
  center = LaMatrix (dimension, 1);
  stream.read ((char *) center.data (), dimension * sizeof (float));

  // Extract covariance
  covariance = LaMatrix (dimension, dimension);
  stream.read ((char *) covariance.data (), dimension * dimension * sizeof (float));

  // Initialize everything else
  eigenvectors = LaMatrix (center.nrows (), center.nrows ());
  eigenvalues  = LaVector (center.nrows ());
  eigenverse   = LaMatrix (center.nrows (), center.nrows ());
  prepareInverse ();
}

void
ClusterGauss::write (ostream & stream)
{
  stream.write ((char *) &alpha, sizeof (alpha));
  int dimension = center.nrows ();
  stream.write ((char *) &dimension, sizeof (dimension));
  stream.write ((char *) center.data (), dimension * sizeof (float));
  stream.write ((char *) covariance.data (), dimension * dimension * sizeof (float));
}

/*
void
ClusterGauss::read (FILE * stream)
{
  // Extract alpha
  fread (&alpha, sizeof (float), 1, stream);

  // Extract center
  int dimension;
  fread (&dimension, sizeof (int), 1, stream);
  center = LaMatrix (dimension, 1);
  fread (center.data (), sizeof (float), dimension, stream);

  // Extract covariance
  covariance = LaMatrix (dimension, dimension);
  fread (covariance.data (), sizeof (float), dimension * dimension, stream);

  // Initialize everything else
  eigenvectors = LaMatrix (center.nrows (), center.nrows ());
  eigenvalues  = LaVector (center.nrows ());
  eigenverse   = LaMatrix (center.nrows (), center.nrows ());
  prepareInverse ();
}

void
ClusterGauss::write (FILE * stream)
{
  fwrite (&alpha, sizeof (float), 1, stream);
  int dimension = center.nrows ();
  fwrite (&dimension, sizeof (int), 1, stream);
  fwrite (center.data (), sizeof (float), dimension, stream);
  fwrite (covariance.data (), sizeof (float), dimension * dimension, stream);
}
*/


// class KMeans ---------------------------------------------------------------

KMeans::KMeans (float maxSize, float minSize, int initialK, int maxK, const string & clusterFileName)
{
  this->maxSize  = maxSize;
  this->minSize  = minSize;
  this->initialK = initialK;
  this->maxK     = maxK;

  this->clusterFileName = clusterFileName;
}

KMeans::KMeans (istream & stream, const string & clusterFileName)
{
  read (stream);

  this->clusterFileName = clusterFileName;
}

void
KMeans::run (const std::vector<LaMatrix> & data)
{
  throw "Currently, only the parallel version of KMeans is capable of computing clusters.";
}

void
KMeans::estimate (const vector<LaMatrix> & data, LaMatrix & member, int jbegin, int jend)
{
  for (int j = jbegin; j < jend; j++)
  {
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
	// Since LaMatrix is column major, this accesses the jth column and scales it.
	LaMatrix::OneD::iterator lmi, lmiEnd;  // read lmi as "LaMatrix iterator"
	lmiEnd = member[j].end ();
	for (lmi = member[j].begin (); lmi != lmiEnd; lmi++)
	{
	  *lmi /= sum;
	}
  }
}

float
KMeans::maximize (const vector<LaMatrix> & data, const LaMatrix & member, int i)
{
  // Calculute new cluster center
  LaMatrix center (data[0].nrows (), 1);
  float sum = 0;
  for (int j = 0; j < data.size (); j++)
  {
	add (scaled (data[j], member (i, j)), center);
	sum += member (i, j);
  }
  LaMatrix::OneD::iterator lmi, lmiEnd;
  lmiEnd = center[0].end ();
  for (lmi = center[0].begin (); lmi != lmiEnd; lmi++)
  {
	*lmi /= sum;
  }

  // Update alpha
  clusters[i].alpha = sum / data.size ();
  if (clusters[i].alpha <= smallestNormalFloat)
  {
	cerr << "alpha got too small " << clusters[i].alpha << endl;
    clusters[i].alpha = smallestNormalFloat;
  }

  // Calculate new covariance matrix
  LaMatrix & covariance = clusters[i].covariance;
  mtl::set (covariance, 0);
  for (int j = 0; j < data.size (); j++)
  {
	LaMatrix delta = data[j] - center;
	add (scaled (delta * trans (delta), member (i, j)), covariance);
  }
  float covarianceTotal = 0;
  for (int j = 0; j < covariance.ncols (); j++)
  {
	lmiEnd = covariance[j].end ();
	for (lmi = covariance[j].begin (); lmi != lmiEnd; lmi++)
	{
	  *lmi /= sum;
	  covarianceTotal += *lmi;
	}
  }
  if (covarianceTotal == 0)
  {
	cerr << "covariance went to zero; setting to I * " << smallestNormalFloat << endl;
	set_diagonal (covariance, smallestNormalFloat);
  }

  // Record changes to cluster.  While we are at it, detect if we have
  // converged.
  float result = two_norm ((center - clusters[i].center)[0]);
  clusters[i].center = center;
  clusters[i].prepareInverse ();

  return result;
}

int
KMeans::classify (const LaMatrix & point)
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

LaMatrix
KMeans::representative (int group)
{
  return clusters[group].center;
}

/*
void
KMeans::read (FILE * stream)
{
  fread (&maxSize, sizeof (int), 1, stream);
  fread (&minSize, sizeof (int), 1, stream);
  fread (&initialK, sizeof (int), 1, stream);  // Actually, the current K!  :)
  fread (&maxK, sizeof (int), 1, stream);

  clusters.clear ();
  for (int i = 0; i < initialK; i++)
  {
	ClusterGauss c (stream);
	clusters.push_back (c);
  }

  int count;
  fread (&count, sizeof (int), 1, stream);
  for (int i = 0; i < count; i++)
  {
	float change;
	fread (&change, sizeof (float), 1, stream);
	changes.push_back (change);
  }

  fread (&count, sizeof (int), 1, stream);
  for (int i = 0; i < count; i++)
  {
	float velocity;
	fread (&velocity, sizeof (float), 1, stream);
	velocities.push_back (velocity);
  }
}

void
KMeans::write (FILE * stream)
{
  clusterFileTime = time (NULL);

  fprintf (stream, "%s\n", name);

  fwrite (&maxSize, sizeof (int), 1, stream);
  fwrite (&minSize, sizeof (int), 1, stream); 
  int K = clusters.size ();
  fwrite (&K, sizeof (int), 1, stream);
  fwrite (&maxK, sizeof (int), 1, stream);

  for (int i = 0; i < K; i++)
  {
	clusters[i].write (stream);
  }

  int count = changes.size ();
  fwrite (&count, sizeof (int), 1, stream);
  for (int i = 0; i < count; i++)
  {
	fwrite (&changes[i], sizeof (float), 1, stream);
  }

  count = velocities.size ();
  fwrite (&count, sizeof (int), 1, stream);
  for (int i = 0; i < count; i++)
  {
	fwrite (&velocities[i], sizeof (float), 1, stream);
  }

  clusterFileSize = ftell (stream);

  fsync (fileno (stream));
}
*/

void
KMeans::read (istream & stream, bool withName)
{
  if (withName)
  {
	string temp;
	getline (stream, temp);
  }

  stream.read ((char *) &maxSize,  sizeof (maxSize));
  stream.read ((char *) &minSize,  sizeof (minSize));
  stream.read ((char *) &initialK, sizeof (initialK));  // Actually, the current K!  :)
  stream.read ((char *) &maxK,     sizeof (maxK));

  clusters.clear ();
  for (int i = 0; i < initialK; i++)
  {
	ClusterGauss c (stream);
	clusters.push_back (c);
  }

  int count;
  stream.read ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	float change;
	stream.read ((char *) &change, sizeof (change));
	changes.push_back (change);
  }

  stream.read ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	float velocity;
	stream.read ((char *) &velocity, sizeof (velocity));
	velocities.push_back (velocity);
  }
}

void
KMeans::write (ostream & stream)
{
  clusterFileTime = time (NULL);

  stream << typeid (*this).name () << endl;

  stream.write ((char *) &maxSize, sizeof (maxSize));
  stream.write ((char *) &minSize, sizeof (minSize));
  int K = clusters.size ();
  stream.write ((char *) &K,       sizeof (K));
  stream.write ((char *) &maxK,    sizeof (maxK));

  for (int i = 0; i < K; i++)
  {
	clusters[i].write (stream);
  }

  int count = changes.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	stream.write ((char *) &changes[i], sizeof (float));
  }

  count = velocities.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
	stream.write ((char *) &velocities[i], sizeof (float));
  }

  clusterFileSize = stream.tellp ();
}
