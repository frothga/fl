/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/neighbor.h"


using namespace fl;
using namespace std;


// class Neighbor -------------------------------------------------------------

Neighbor::~Neighbor ()
{
}

uint32_t Neighbor::serializeVersion = 0;

void
Neighbor::serialize (Archive & archive, uint32_t version)
{
}


// class Entry ----------------------------------------------------------------

Neighbor::Entry::Entry (MatrixAbstract<float> * point, void * item)
: point (point),
  item (item)
{
}

MatrixAbstract<float> *
Neighbor::Entry::clone (bool deep) const
{
  if (deep) return new Entry (point->clone (true), item);
  return new Entry (point, item);
}

int
Neighbor::Entry::rows () const
{
  return point->rows ();
}

int
Neighbor::Entry::columns () const
{
  return point->columns ();
}

void
Neighbor::Entry::resize (const int rows, const int columns)
{
  point->resize (rows, columns);
}


// class KDTree ---------------------------------------------------------------

KDTree::KDTree ()
{
  root = 0;
  bucketSize = 5;
  k = 5;  // it doesn't make sense for k to be less than bucketSize
  epsilon = 1e-4;
}

KDTree::~KDTree ()
{
  clear ();
}

void
KDTree::clear ()
{
  delete root;
}

void
KDTree::serialize (Archive & archive, uint32_t version)
{
}

void
KDTree::set (const vector<MatrixAbstract<float> *> & data)
{
  vector<MatrixAbstract<float> *> temp = data;

  int dimensions = temp[0]->rows ();
  lo.resize (dimensions);
  hi.resize (dimensions);
  lo.clear ( INFINITY);
  hi.clear (-INFINITY);

  vector<MatrixAbstract<float> *>::iterator t = temp.begin ();
  for (; t != temp.end (); t++)
  {
	float * a = &(**t)[0];
	float * l = &lo[0];
	float * h = &hi[0];
	float * end = l + dimensions;
	while (l < end)
	{
	  l = min (l, a);
	  h = max (h, a);
	  a++;
	  l++;
	  h++;
	}
  }

  root = construct (temp);
}

void
KDTree::find (const MatrixAbstract<float> & query, vector<MatrixAbstract<float> *> & result) const
{
  // Determine distance of query from bounding rectangle for entire tree
  int dimensions = query.rows ();
  float distance = 0;
  for (int i = 0; i < dimensions; i++)
  {
	distance += min (0.0f, lo[i] - query[i]) + min (0.0f, query[i] - hi[i]);
  }

  // Recursively collect closest points
  Query q;
  q.k = k;
  q.point = &query;
  q.oneEpsilon = pow (1 + epsilon, 2);
  root->search (distance, q);
  result.reserve (q.sorted.size ());
  multimap<float, MatrixAbstract<float> *>::iterator sit = q.sorted.begin ();
  for (; sit != q.sorted.end (); sit++)
  {
	result.push_back (sit->second);
  }
}

KDTree::Node *
KDTree::construct (vector<MatrixAbstract<float> *> & points)
{
  int count = points.size ();
  if (count == 0)
  {
	return 0;
  }
  else if (count <= bucketSize)
  {
	Leaf * result = new Leaf;
	result->points = points;
	return result;
  }
  else  // count > bucketSize
  {
	// todo: pass the split method as a function pointer
	int dimensions = lo.rows ();
	int d = 0;
	float longest = 0;
	for (int i = 0; i < dimensions; i++)
	{
	  float length = hi[i] - lo[i];
	  if (length > longest)
	  {
		d = i;
		longest = length;
	  }
	}
	sort (points, d);
	int cut = count / 2;
	vector<MatrixAbstract<float> *>::iterator b = points.begin ();
	vector<MatrixAbstract<float> *>::iterator c = b + cut;
	vector<MatrixAbstract<float> *>::iterator e = points.end ();

	Branch * result = new Branch;
	result->dimension = d;
	result->lo = lo[d];
	result->hi = hi[d];
	result->mid = (*points[cut])[d];

	hi[d] = result->mid;
	vector<MatrixAbstract<float> *> tempPoints (b, c);
	result->lowNode = construct (tempPoints);
	hi[d] = result->hi;

	lo[d] = result->mid;
	tempPoints.clear ();
	tempPoints.insert (tempPoints.begin (), c, e);
	result->highNode = construct (tempPoints);
	lo[d] = result->lo;  // it is important to restore lo[d] so that when recursion unwinds the vector is still correct

	return result;
  }
}

void
KDTree::sort (vector<MatrixAbstract<float> *> & points, int dimension)
{
  multimap<float, MatrixAbstract<float> *> sorted;
  vector<MatrixAbstract<float> *>::iterator it = points.begin ();
  for (; it != points.end (); it++)
  {
	sorted.insert (make_pair ((**it)[dimension], *it));
  }

  points.clear ();
  points.reserve (sorted.size ());
  multimap<float, MatrixAbstract<float> *>::iterator sit = sorted.begin ();
  for (; sit != sorted.end (); sit++)
  {
	points.push_back (sit->second);
  }
}


// class KDTree::Node ---------------------------------------------------------

KDTree::Node::~Node ()
{
}


// class KDTree::Branch -------------------------------------------------------

KDTree::Branch::~Branch ()
{
  delete lowNode;
  delete highNode;
}

void
KDTree::Branch::search (float distance, Query & q) const
{
  float qmid = (*q.point)[dimension];
  float newOffset = qmid - mid;
  if (newOffset < 0)  // lowNode is closer
  {
	if (lowNode) lowNode->search (distance, q);
	if (highNode)
	{
	  float oldOffset = max (lo - qmid, 0.0f);
	  distance += newOffset * newOffset - oldOffset * oldOffset;
	  if (q.sorted.rbegin ()->first > distance * q.oneEpsilon) highNode->search (distance, q);
	}
  }
  else  // newOffset >= 0, so highNode is closer
  {
	if (highNode) highNode->search (distance, q);
	if (lowNode)
	{
	  float oldOffset = max (qmid - hi, 0.0f);
	  distance += newOffset * newOffset - oldOffset * oldOffset;
	  if (q.sorted.rbegin ()->first > distance * q.oneEpsilon) lowNode->search (distance, q);
	}
  }
}


// class KDTree::Leaf ---------------------------------------------------------

void
KDTree::Leaf::search (float distance, Query & q) const
{
  int count = points.size ();
  int dimensions = points[0]->rows ();
  float limit = q.sorted.size () ? q.sorted.rbegin ()->first : INFINITY;
  for (int i = 0; i < count; i++)
  {
	MatrixAbstract<float> * p = points[i];

	// Measure distance using early-out method.  May save operations in
	// high-dimensional spaces.
	// Here we make the assumption that the values are stored contiguously in
	// memory.  This is a good place to check for bugs if using more exotic
	// matrix types (not recommended).
	float * x = &(*p)[0];
	float * y = &(*q.point)[0];
	float * end = x + dimensions;
	float total = 0;
	while (x < end  &&  total < limit)
	{
	  float t = *x++ - *y++;
	  total += t * t;
	}

	q.sorted.insert (make_pair (total, p));
	if (q.sorted.size () > q.k)
	{
	  multimap<float, MatrixAbstract<float> *>::iterator it = q.sorted.end ();
	  it--;  // it is one past end of collection, so we must back up one step
	  q.sorted.erase (it);
	}
  }
}
