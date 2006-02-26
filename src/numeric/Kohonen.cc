/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- MSVC compilability fix: use rint() not rintf().
        Don't include convolve.h.
*/


#include "fl/cluster.h"
#include "fl/random.h"
#include "fl/time.h"


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
  float sigma2 = sigma * sigma;
  int h = (int) rint (4 * sigma);  // "half" = distance from middle until cell values become insignificant
  int s = 2 * h + 1;  // "size" of kernel
  Matrix<float> lambda (s, s);
  for (int column = 0; column < s; column++)
  {
	for (int row = 0; row < s; row++)
	{
	  float x = column - h;
	  float y = row - h;
	  lambda(row,column) = expf (- (x * x + y * y) / (2 * sigma2));
	}
  }
  lambda *= 1.0f / lambda(h,h);  // Normalize so peak value is 1
  int pad = width * (int) ceilf ((float) s / width);

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
	  for (int x = 0; x < s; x++)
	  {
		for (int y = 0; y < s; y++)
		{
		  int dx = (cx + (x - h) + pad) % width;
		  int dy = (cy + (y - h) + pad) % width;
		  float change = map[dx * width + dy].update (data[i], learningRate * lambda(x,y));
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

Vector<float>
Kohonen::distribution (const Vector<float> & point)
{
  Vector<float> result (map.size ());
  for (int i = 0; i < map.size (); i++)
  {
	result[i] = map[i].distance (point);
  }
  result /= result.frob (1);
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
Kohonen::read (istream & stream)
{
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
Kohonen::write (ostream & stream, bool withName)
{
  ClusterMethod::write (stream, withName);

  stream.write ((char *) &width,        sizeof (width));
  stream.write ((char *) &sigma,        sizeof (sigma));
  stream.write ((char *) &learningRate, sizeof (learningRate));
  stream.write ((char *) &decayRate,    sizeof (decayRate));

  for (int i = 0; i < map.size (); i++)
  {
	map[i].write (stream);
  }
}
