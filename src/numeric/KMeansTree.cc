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


// class KMeansTree -----------------------------------------------------------

KMeansTree::KMeansTree (int K, int depth)
: kmeans (K),
  depth (depth)
{
  if (depth > 1)
  {
	for (int i = 0; i < K; i++) subtrees.push_back (new KMeansTree (K, depth - 1));
  }
}

KMeansTree::~KMeansTree ()
{
  for (int i = 0; i < subtrees.size (); i++) delete subtrees[i];
}

void
KMeansTree::run (const std::vector<Vector<float> > & data)
{
  const int K = kmeans.clusters.size ();
  const int count = data.size ();

  kmeans.run (data);
  if (depth <= 1) return;

  vector<vector<Vector<float> > > partition (K);
  for (int i = 0; i < count; i++)
  {
	int g = kmeans.classify (data[i]);
	partition[g].push_back (data[i]);
  }
  for (int i = 0; i < K; i++)
  {
	subtrees[i]->run (partition[i]);
  }
}

int
KMeansTree::classify (const Vector<float> & point)
{
  int g = kmeans.classify (point);
  if (depth <= 1) return g;

  int K = kmeans.clusters.size ();
  int count = pow (K, depth - 1);
  return g * count + subtrees[g]->classify (point);
}

Vector<float>
KMeansTree::distribution (const Vector<float> & point)
{
  Vector<float> result (classCount ());
  result.clear ();
  result[classify (point)] = 1;
  return result;
}

int
KMeansTree::classCount ()
{
  int K = kmeans.clusters.size ();
  return pow (K, depth);
}

Vector<float>
KMeansTree::representative (int group)
{
  if (depth <= 1) return kmeans.representative (group);

  int K = kmeans.clusters.size ();
  int count = pow (K, depth - 1);
  int s = group / count;
  int g = group % count;
  return subtrees[s]->representative (g);
}

void
KMeansTree::serialize (Archive & archive, uint32_t version)
{
  archive & *((ClusterMethod *) this);
  archive & kmeans;
  archive & depth;
  archive & subtrees;
}
