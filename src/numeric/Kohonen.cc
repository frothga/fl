#include "cluster.h"

#include <fl/random.h>
#include <fl/convolve.h>
#include <fl/time.h>


using namespace std;
using namespace fl;


// class ClusterCosine --------------------------------------------------------

ClusterCosine::ClusterCosine (int dimension)
{
  center.resize (dimension);
  for (int i = 0; i < dimension; i++)
  {
	center[i] = randGaussian ();
  }
  center.normalize ();
}

ClusterCosine::ClusterCosine (Vector<float> & center)
{
  this->center.copyFrom (center);
  this->center.normalize ();
}

ClusterCosine::ClusterCosine (istream & stream)
{
  read (stream);
}

float
ClusterCosine::distance (const Vector<float> & point)
{
  return center.dot (point);
}

float
ClusterCosine::update (const Vector<float> & point, float weight)
{
  int dimension = point.rows ();
  Vector<float> oldCenter;
  oldCenter.copyFrom (center);
  center += point * weight;
  center.normalize ();
  return (center - oldCenter).frob (2);
}

void
ClusterCosine::read (istream & stream)
{
  center.read (stream);
}

void
ClusterCosine::write (ostream & stream)
{
  center.write (stream, false);
}


// class Kohonen -------------------------------------------------------------

Kohonen::Kohonen (int width, float sigma, float learningRate, float decayRate)
{
  this->width        = width;
  this->sigma        = sigma;
  this->learningRate = learningRate;
  this->decayRate    = decayRate;
}

Kohonen::Kohonen (istream & stream)
{
  read (stream);
}

void
Kohonen::run (const std::vector<Vector<float> > & data)
{
  stop = false;

  int rows = data[0].rows ();

  // Prepare set of random clusters
  int count = width * width;
  for (int i = 0; i < count; i++)
  {
	ClusterCosine c (rows);
	map.push_back (c);
  }

  // Prepare a Gaussian kernel to use as our neighborhood function
  ImageOf<float> lambda = Gaussian2D (sigma);
  int hx = lambda.width / 2;
  int hy = lambda.height / 2;
  lambda *= 1.0 / lambda (hx, hy);  // Normalize so peak value is 1
  int pad = width * (int) ceilf ((float) lambda.width / width);

  vector<float> changes;
  while (! stop  &&  learningRate > 1e-6)
  {
	float largestChange = 0;
	double startTime = getTimestamp ();

	for (int i = 0; i < data.size ()  &&  ! stop; i++)
	{
	  // Find closest cluster
	  int cluster = classify (data[i]);
	  int cx = cluster / width;  // This implies column major organization
	  int cy = cluster % width;

	  // Update neighborhood
	  for (int x = 0; x < lambda.width; x++)
	  {
		for (int y = 0; y < lambda.height; y++)
		{
		  int dx = (cx + (x - hx) + pad) % width;
		  int dy = (cy + (y - hy) + pad) % width;
		  float change = map[dx * width + dy].update (data[i], learningRate * lambda (x, y));
		  largestChange = max (change, largestChange);
		}
	  }
	  if (i % 1000 == 0)
	  {
		cerr << ".";
	  }
	}
	cerr << endl;

	// Analyze movement data to detect convergence
	cerr << "change = " << largestChange;
	largestChange /= learningRate;
	cerr << "\t" << largestChange;
	changes.push_back (largestChange);
	if (changes.size () > 4)
	{
	  changes.erase (changes.begin ()); // Limit size of history
	  float average = 0;
	  for (int i = 0; i < changes.size (); i++)
	  {
		average += changes[i];
	  }
	  average /= changes.size ();
	  float variance = 0;
	  for (int i = 0; i < changes.size (); i++)
	  {
		float difference = changes[i] - average;
		variance += difference * difference;
	  }
	  variance /= changes.size ();
	  float z = sqrt (variance);
	  cerr << "\t" << z;
	  if (z < 0.02)
	  {
		learningRate *= decayRate;
	  }
 	}
	cerr << endl;

	cerr << "learningRate = " << learningRate << endl;
	cerr << "time = " << getTimestamp () - startTime << endl;
  }
}

int
Kohonen::classify (const Vector<float> & point)
{
  int result = -1;
  float highest = 0;
  for (int i = 0; i < map.size (); i++)
  {
	float value = map[i].distance (point);
	if (value > highest)
	{
	  result = i;
	  highest = value;
	}
  }

  return result;
}

int
Kohonen::classCount ()
{
  return map.size ();
}

Vector<float>
Kohonen::representative (int group)
{
  return map[group].center;
}

void
Kohonen::read (istream & stream, bool withName)
{
  if (withName)
  {
	string temp;
	getline (stream, temp);
  }

  stream.read ((char *) &width,        sizeof (width));
  stream.read ((char *) &sigma,        sizeof (sigma));
  stream.read ((char *) &learningRate, sizeof (learningRate));
  stream.read ((char *) &decayRate,    sizeof (decayRate));

  int count = width * width;
  for (int i = 0; i < count; i++)
  {
	ClusterCosine c (stream);
	map.push_back (c);
  }
}

void
Kohonen::write (ostream & stream)
{
  stream << typeid (*this).name () << endl;

  stream.write ((char *) &width,        sizeof (width));
  stream.write ((char *) &sigma,        sizeof (sigma));
  stream.write ((char *) &learningRate, sizeof (learningRate));
  stream.write ((char *) &decayRate,    sizeof (decayRate));

  for (int i = 0; i < map.size (); i++)
  {
	map[i].write (stream);
  }
}
