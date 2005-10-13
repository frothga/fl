/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/factory.h"


using namespace std;
using namespace fl;


// class ClusterAgglomerative -------------------------------------------------

ClusterAgglomerative::ClusterAgglomerative (const Vector<float> & center, int count)
{
  this->center = center;  // shallow copy.  Use deep copy when updating center.
  this->count = count;
}

ClusterAgglomerative::ClusterAgglomerative (istream & stream)
{
  read (stream);
}

void
ClusterAgglomerative::operator += (const ClusterAgglomerative & that)
{
  center = ((center * count) + (that.center * that.count)) / (count + that.count);
  count += that.count;
}

void
ClusterAgglomerative::read (istream & stream)
{
  center.read (stream);
  stream.read ((char *) &count, sizeof (count));
}

void
ClusterAgglomerative::write (ostream & stream)
{
  center.write (stream, false);
  stream.write ((char *) &count, sizeof (count));
}


// class Agglomerate ----------------------------------------------------------

Agglomerate::Agglomerate (Comparison * comparison, float distanceLimit, int minClusters)
{
  this->comparison = comparison;
  this->distanceLimit = distanceLimit;
  this->minClusters = minClusters;
}

Agglomerate::Agglomerate (istream & stream)
{
  read (stream);
}

Agglomerate::~Agglomerate ()
{
  delete comparison;

  for (int i = 0; i < clusters.size (); i++)
  {
	delete clusters[i];
  }
}

void
Agglomerate::run (const std::vector< Vector<float> > & data)
{
  stop = false;

  // Make clusters out of all the data
  clusters.resize (data.size ());
  for (int i = 0; i < data.size (); i++)
  {
	clusters[i] = new ClusterAgglomerative (data[i]);  // shallow copy of datum
  }

  // Prepare distance matrix
  MatrixPacked<float> distances (clusters.size ());
  for (int i = 0; i < clusters.size (); i++)
  {
	for (int j = i + 1; j < clusters.size (); j++)
	{
	  distances(i,j) = comparison->value (clusters[i]->center, clusters[j]->center);
	}
  }

  // agglomerate to convergence
  while (! stop  &&  clusters.size () > minClusters)
  {
	// Find the next closest cluster pair
	float closestDistance = -1;
	int bestI;
	int bestJ;
	for (int i = 0; i < clusters.size (); i++)
	{
	  for (int j = i + 1; j < clusters.size (); j++)
	  {
		float distance = distances(i,j);
		if (distance > closestDistance)
		{
		  closestDistance = distance;
		  bestI = i;
		  bestJ = j;
		}
	  }
	}

	cerr << clusters.size () << " " << closestDistance << endl;

	if (closestDistance >= distanceLimit)
	{
	  (*clusters[bestI]) += (*clusters[bestJ]);
	  delete clusters[bestJ];
	  clusters.erase (clusters.begin () + bestJ);

	  // Reshuffle memoized distances
	  for (int i = 0; i < bestJ; i++)
	  {
		for (int j = bestJ; j < clusters.size (); j++)
		{
		  distances(i,j) = distances(i,j+1);
		}
	  }
	  for (int i = bestJ; i < clusters.size (); i++)
	  {
		for (int j = i; j < clusters.size (); j++)
		{
		  distances(i,j) = distances(i+1,j+1);
		}
	  }
	  for (int j = 0; j < clusters.size (); j++)
	  {
		distances(bestI,j) = comparison->value (clusters[bestI]->center, clusters[j]->center);
	  }
	}
	else
	{
	  break;
	}
  }
}

int
Agglomerate::classify (const Vector<float> & point)
{
  int result = 0;
  float bestValue = 0;
  for (int i = 0; i < clusters.size (); i++)
  {
	float value = comparison->value (point, clusters[i]->center);
	if (value > bestValue)
	{
	  result = i;
	  bestValue = value;
	}
  }
  return result;
}

Vector<float>
Agglomerate::distribution (const Vector<float> & point)
{
  Vector<float> result (clusters.size ());
  for (int i = 0; i < clusters.size (); i++)
  {
	result[i] = comparison->value (point, clusters[i]->center);
  }
  result.normalize ();
  return result;
}

int
Agglomerate::classCount ()
{
  return clusters.size ();
}

Vector<float>
Agglomerate::representative (int group)
{
  return clusters[group]->center;
}

void
Agglomerate::read (istream & stream)
{
  comparison = Factory<Comparison>::read (stream);

  stream.read ((char *) &distanceLimit, sizeof (distanceLimit));
  stream.read ((char *) &minClusters, sizeof (minClusters));

  int count = 0;
  stream.read ((char *) &count, sizeof (count));
  if (! stream.good ())
  {
	throw "Can't finish reading clusters: stream bad.";
  }
  clusters.resize (count);
  for (int i = 0; i < count; i++)
  {
	clusters[i] = new ClusterAgglomerative (stream);
  }
}

void
Agglomerate::write (ostream & stream, bool withName)
{
  ClusterMethod::write (stream, withName);

  comparison->write (stream, true);

  stream.write ((char *) &distanceLimit, sizeof (distanceLimit));
  stream.write ((char *) &minClusters, sizeof (minClusters));

  int count = clusters.size ();
  stream.write ((char *) &count, sizeof (count));
  for (int i = 0; i < count; i++)
  {
    clusters[i]->write (stream);
  }
}
