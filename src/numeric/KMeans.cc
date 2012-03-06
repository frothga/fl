/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/cluster.h"
#include "fl/random.h"


using namespace std;
using namespace fl;


// class KMeans ---------------------------------------------------------------

KMeans::KMeans (int K)
: clusters (K)
{
}

void
KMeans::run (const std::vector<Vector<float> > & data)
{
  stop = false;

  const int K = clusters.size ();
  const int count = data.size ();
  const int dimension = data[0].rows ();

  for (int i = 0; i < K; i++) clusters[i].resize (dimension);

  // Random initial assignments
  vector<int> assignments (count);
  for (int i = 0; i < count; i++) assignments[i] = rand () % K;

  // Iterate until clusters stop moving (for the most part)
  int smallestChange = count;
  int lastImprovement = 0;
  while (! stop)
  {
	// Update cluster centers
	vector<int> clusterSizes (K, 0);
	for (int i = 0; i < K; i++) clusters[i].clear ();
	for (int i = 0; i < count; i++)
	{
	  const int index = assignments[i];
	  clusters[index] += data[i];
	  clusterSizes[index]++;
	}
	for (int i = 0; i < K; i++) clusters[i] /= clusterSizes[i];

	// Assign data to clusters
	int changes = 0;
	for (int i = 0; i < count; i++)
	{
	  int c = classify (data[i]);
	  if (c != assignments[i])
	  {
		changes++;
		assignments[i] = c;
	  }
	}

	// Check for convergence
	if (changes == 0) break;
	if (changes < smallestChange)
	{
	  smallestChange = changes;
	  lastImprovement = 0;
	}
	else if (++lastImprovement > 3) break;
  }
}

int
KMeans::classify (const Vector<float> & point)
{
  int   bestCluster  = 0;
  float bestDistance = (clusters[0] - point).norm (2);
  for (int i = 1; i < clusters.size (); i++)
  {
	float distance = (clusters[i] - point).norm (2);
	if (distance < bestDistance)
	{
	  bestDistance = distance;
	  bestCluster  = i;
	}
  }

  return bestCluster;
}

Vector<float>
KMeans::distribution (const Vector<float> & point)
{
  Vector<float> result (clusters.size ());
  result.clear ();
  result[classify (point)] = 1;
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
  return clusters[group];
}

void
KMeans::serialize (Archive & archive, uint32_t version)
{
  archive & *((ClusterMethod *) this);
  archive & clusters;
}
