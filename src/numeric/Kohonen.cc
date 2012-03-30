/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/random.h"
#include "fl/time.h"
#include <iostream>

using namespace std;
using namespace fl;


// class Kohonen -------------------------------------------------------------

Kohonen::Kohonen (int width, float sigma, float learningRate, float decayRate)
{
  this->width        = width;
  this->sigma        = sigma;
  this->learningRate = learningRate;
  this->decayRate    = decayRate;
}

void
Kohonen::run (const vector<Vector<float> > & data, const vector<int> & classes)
{
  stop = false;

  // Prepare set of random clusters
  int dimension = data[0].rows ();
  int count = width * width;
  map.resize (count, dimension);
  for (int r = 0; r < count; r++)
  {
	for (int c = 0; c < dimension; c++)
	{
	  map(r,c) = randGaussian ();
	}
	map.row (r).normalize ();
  }

  // Prepare a Gaussian kernel to use as our neighborhood function
  float sigma2 = sigma * sigma;
  int h = (int) roundp (4 * sigma);  // "half" = distance from middle until cell values become insignificant
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
  float * oldCenter = new float[dimension];
  float * end       = oldCenter + dimension;
  while (! stop  &&  learningRate > 1e-6)
  {
	float largestChange = 0;
	double startTime = getTimestamp ();

	for (int i = 0; i < data.size ()  &&  ! stop; i++)
	{
	  const Vector<float> & point = data[i];

	  // Find closest cluster
	  int cluster = classify (point);
	  int cx = cluster / width;  // This implies column major organization
	  int cy = cluster % width;

	  // Update neighborhood
	  for (int x = 0; x < s; x++)
	  {
		for (int y = 0; y < s; y++)
		{
		  int dx = (cx + (x - h) + pad) % width;
		  int dy = (cy + (y - h) + pad) % width;
		  int index = dx * width + dy;
		  float rate = learningRate * lambda(x,y);

		  float * center = & map(index,0);
		  float * c = center;
		  float * o = oldCenter;
		  float * p = & point[0];
		  float total = 0;
		  while (o < end)
		  {
			*o++ = *c;
			*c += *p++ * rate;
			total += *c * *c;
			c += count;
		  }
		  total = sqrt (total);
		  c = center;
		  o = oldCenter;
		  float change = 0;
		  while (o < end)
		  {
			*c /= total;
			float t = *c - *o++;
			change += t * t;
			c += count;
		  }
		  change = sqrt (change);
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
  delete [] oldCenter;
}

int
Kohonen::classify (const Vector<float> & point)
{
  Vector<float> distances = map * point;
  float * start = &distances[0];
  float * end   = start + distances.rows ();
  float * best  = start;
  float * i     = start + 1;
  while (i < end)
  {
	if (*i > *best) best = i;
	i++;
  }

  return best - start;
}

Vector<float>
Kohonen::distribution (const Vector<float> & point)
{
  Vector<float> result = map * point;
  result /= result.norm (1);
  return result;
}

int
Kohonen::classCount ()
{
  return map.rows ();
}

Vector<float>
Kohonen::representative (int group)
{
  return map.row (group);
}

void
Kohonen::serialize (Archive & archive, uint32_t version)
{
  archive & *((ClusterMethod *) this);
  archive & width;
  archive & sigma;
  archive & learningRate;
  archive & decayRate;
  archive & map;
}
