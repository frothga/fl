#include "cluster.h"

#include <fl/lapacks.h>
#include <fl/pi.h>

#include <algorithm>

using namespace std;
using namespace fl;


// class ClusterGauss ---------------------------------------------------------

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
	d *= 2.0 * PI * eigenvalues[i];
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
ClusterGauss::read (istream & stream)
{
  stream.read ((char *) &alpha, sizeof (alpha));
  center.read (stream);
  covariance.read (stream);
  prepareInverse ();
}

void
ClusterGauss::write (ostream & stream)
{
  stream.write ((char *) &alpha, sizeof (alpha));
  center.write (stream, false);
  covariance.write (stream, false);
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
KMeans::run (const std::vector<Vector<float> > & data)
{
  throw "Currently, only the parallel version of KMeans is implemented.";
}

void
KMeans::estimate (const vector<Vector<float> > & data, Matrix<float> & member, int jbegin, int jend)
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
	for (int i = 0; i < clusters.size (); i++)
	{
	  member (i, j) /= sum;
	}
  }
}

float
KMeans::maximize (const vector<Vector<float> > & data, const Matrix<float> & member, int i)
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
  if (covariance.frob (1) == 0)
  {
	cerr << "covariance went to zero; setting to I * " << smallestNormalFloat << endl;
	covariance.identity (smallestNormalFloat);
  }

  // Record changes to cluster.  While we are at it, detect if we have
  // converged.
  float result = (center - clusters[i].center).frob (2);
  clusters[i].center = center;
  clusters[i].prepareInverse ();

  return result;
}

int
KMeans::classify (const Vector<float> & point)
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

int
KMeans::classCount ()
{
  return clusters.size ();
}

Vector<float>
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
