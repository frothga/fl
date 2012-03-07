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
}

KMeansTree::~KMeansTree ()
{
  clear ();
}

void
KMeansTree::clear ()
{
  for (int i = 0; i < subtrees.size (); i++) delete subtrees[i];
  subtrees.clear ();
}

void
KMeansTree::run (const std::vector<Vector<float> > & data)
{
  kmeans.run (data);
  cerr << depth;
  if (depth <= 1) return;

  const int K     = kmeans.K;
  const int count = data.size ();

  vector<vector<Vector<float> > > partition (K);
  for (int i = 0; i < count; i++)
  {
	int g = kmeans.classify (data[i]);
	partition[g].push_back (data[i]);
  }

  clear ();
  for (int i = 0; i < K; i++)
  {
	if (partition[i].size ())
	{
	  KMeansTree * tree = new KMeansTree (K, depth - 1);
	  tree->run (partition[i]);
	  subtrees.push_back (tree);
	}
	else subtrees.push_back (0);
  }
}

int
KMeansTree::classify (const Vector<float> & point)
{
  int g = kmeans.classify (point);
  if (subtrees.size () == 0) return g;

  int result = g * pow (kmeans.K, depth - 1);
  if (subtrees[g]) result += subtrees[g]->classify (point);
  return result;
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
  return pow (kmeans.K, depth);
}

Vector<float>
KMeansTree::representative (int group)
{
  if (subtrees.size () == 0) return kmeans.representative (group);

  int count = pow (kmeans.K, depth - 1);
  int s = group / count;
  int g = group % count;
  if (subtrees[s]) return subtrees[s]->representative (g);
  else             return kmeans.representative (s);
}

void
KMeansTree::serialize (Archive & archive, uint32_t version)
{
  archive & *((ClusterMethod *) this);
  archive & kmeans;
  archive & depth;
  archive & subtrees;
}
