/*
Author: Fred Rothganger
Created 5/31/2005
*/


#include "fl/interest.h"

#include <math.h>


// For testing only
#include "fl/slideshow.h"
#include <assert.h>


using namespace std;
using namespace fl;


// class InterestMSER ---------------------------------------------------------

InterestMSER::InterestMSER (int delta)
{
  this->delta = delta;
  minScale = 1.0;
}

bool trace = false;

/**
   Find parent, with path compression.  In CLR ch. 22, this is presented as
   a recursive function.  However, recursion is not necessary, and managing
   a private stack is more efficient.  This is also safe, since it can be
   shown that the stack depth won't exceed the number of gray-levels, due to
   the fact that pixels are added in image sequence.
 **/
static inline InterestMSER::Node *
findSet (InterestMSER::Node * n)
{
  if (n->parent == n) return n;  // early out

  InterestMSER::Node * path[256];
  register int sp = 0;
  do
  {
	path[sp++] = n;
	n = n->parent;
  }
  while (n->parent != n);
  // n should now point to the root of roots
  while (sp > 0)
  {
	path[--sp]->parent = n;
  }
  return n;
}

/*
static inline bool
integrity (InterestMSER::Root * root, unsigned char level)
{
  InterestMSER::Node * n = root->head;
  int size = root->size;
  // coded only for MSER+
  for (int l = level - 1; l >= root->level; l--)
  {
	InterestMSER::Node * end = root->heads[l];
	int count = 0;
	while (n != end)
	{
	  count++;
	  n = n->next;
	  if (! n)
	  {
		cerr << root << " hit unexpected null pointer" << endl;
		return false;
	  }
	}
	int expectedCount = size - root->sizes[l];
	size = root->sizes[l];
	if (count != expectedCount)
	{
	  cerr << root << " " << l+1 << " = " << count << " " << expectedCount << endl;
	  return false;
	}
  }

  int count = 0;
  while (n)
  {
	count++;
	n = n->next;
  }
  if (count != size)
  {
	cerr << root << " " << (int) root->level << " = " << count << " " << size << endl;
	return false;
  }
  return true;
}
*/

static inline void
merge (InterestMSER::Node * grow, InterestMSER::Node * destroy)
{
  grow->root->size += destroy->root->size;
  destroy->parent = grow;

  assert (destroy->next == 0);
  assert (grow->root->head);
  destroy->next = grow->root->head;
  grow->root->head = destroy->root->head;

  destroy->root->previous->next = destroy->root->next;
  destroy->root->next->previous = destroy->root->previous;
  delete destroy->root;
  destroy->root = 0;
}

/**
   Combine a given pixel i with a given neighbor n.  If i is already in
   a set, then join the sets.
 **/
static inline void
join (InterestMSER::Node * i, InterestMSER::Node * n)
{
  if (n->parent)
  {
	InterestMSER::Node * r = findSet (n);
	if (i->parent)
	{
	  assert (i->parent->parent == i->parent);
	  assert (i->parent->root);
	  assert (r->root);

	  if (i->parent == r) return;  // already a member of r, so don't join again!
	  if (i->parent->root->size > r->root->size)
	  {
		merge (i->parent, r);
	  }
	  else
	  {
		merge (r, i->parent);
		i->parent = r;
	  }
	}
	else
	{
	  r->root->size++;
	  i->parent = r;
	  i->next = r->root->head;
	  r->root->head = i;
	}
  }
}

inline void
InterestMSER::addGrayLevel (unsigned char level, bool sign, int * lists[], Node * nodes, const int width, const int height, Root * roots, vector<PointMSER *> & regions)
{
  cerr << "addGrayLevel: " << (int) level << " " << sign << " " << regions.size () << endl;

  int lastX = width - 1;
  int lastY = height - 1;

  int * l = lists[level];
  int * e = lists[level+1];
  while (l < e)
  {
	int index = *l;
	Node * i = nodes + index;

	int x = index % width;
	int y = index / width;

	if (y > 0)     join (i, i - width);
	if (x > 0)     join (i, i - 1);
	if (y < lastY) join (i, i + width);
	if (x < lastX) join (i, i + 1);

	if (! i->parent)
	{
	  Root * r = new Root;
	  r->next = roots->next;
	  r->next->previous = r;
	  r->previous = roots;
	  roots->next = r;

	  r->size = 1;
	  r->head = i;
	  r->level = level;
	  r->oldLevel = 0;

	  i->root = r;
	  i->parent = i;
	  // i->next == 0 by construction ("nodes" is zeroed)
	}

	l++;
  }

  // Update all histories
  Root * r = roots->next;
  while (r != roots)
  {
	r->sizes[level] = r->size;
	r->heads[level] = r->head;
	r = r->next;
  }

  // Update all rate histories
  int c = sign ? level - delta : level + delta;
  int a = c - delta;
  int b = c + delta;
  if (a >= 0  &&  b <= 255)
  {
	r = roots->next;
	while (r != roots)
	{
	  if (sign ? r->level <= a : r->level >= b)
	  {
		r->rates[c] = fabs ((float) (r->sizes[b] - r->sizes[a])) / r->sizes[c];
	  }
	  r = r->next;
	}

	// Check for local minima in rates
	c += sign ? -1 : 1;
	a = c - delta - 1;
	b = c + delta + 1;
	if (a >= 0  &&  b <= 255)
	{
	  r = roots->next;
	  while (r != roots)
	  {
		if (sign ? r->level <= a : r->level >= b)
		{
		  if (r->rates[c] < r->rates[c+1]  &&  r->rates[c] < r->rates[c-1])
		  {
			// Got an MSER!  Now record it and generate shape matrix and scale.

			Node * head = r->heads[c];
			Node * representative = findSet (head);
			Node * tail;
			float oldWeight;
			if (r->oldLevel)
			{
			  tail = r->heads[r->oldLevel];
			  oldWeight = r->sizes[r->oldLevel];
			}
			else
			{
			  tail = 0;
			  oldWeight = 0;
			}

			float totalWeight = r->sizes[c];
			float newWeight = totalWeight - oldWeight;

			// Find centroid
			float x = 0;
			float y = 0;
			Node * n = head;
			while (n != tail)
			{
			  int index = n - nodes;
			  x += index % width;
			  y += index / width;
			  n = n->next;
			}
			x /= newWeight;  // "average x"
			y /= newWeight;

			// Find covariance
			double xx = 0;
			double xy = 0;
			double yy = 0;
			n = head;
			while (n != tail)
			{
			  int index = n - nodes;
			  float dx = index % width - x;
			  float dy = index / width - y;
			  xx += dx * dx;
			  xy += dx * dy;
			  yy += dy * dy;
			  n = n->next;
			}
			xx /= newWeight;
			xy /= newWeight;
			yy /= newWeight;

			// Merge covariance with previous one, if it exists
			if (oldWeight)
			{
			  // Find new average position
			  newWeight /= totalWeight;
			  oldWeight /= totalWeight;
			  float cx = x * newWeight + r->x * oldWeight;
			  float cy = y * newWeight + r->y * oldWeight;

			  // Offsets
			  float nx = x - cx;
			  float ny = y - cy;
			  float ox = r->x - cx;
			  float oy = r->y - cy;

			  // Update
			  r->x = cx;
			  r->y = cy;
			  r->xx = (xx + nx * nx) * newWeight + (r->xx + ox * ox) * oldWeight;
			  r->xy = (xy + nx * ny) * newWeight + (r->xy + ox * oy) * oldWeight;
			  r->yy = (yy + ny * ny) * newWeight + (r->yy + oy * oy) * oldWeight;
			}
			else
			{
			  r->x = x;
			  r->y = y;
			  r->xx = xx;
			  r->xy = xy;
			  r->yy = yy;
			}
			r->oldLevel = c;

			// Update affine part of point
			//   Determine scale
			float scale = sqrt (r->xx * r->yy - r->xy * r->xy);
			if (scale >= minScale)
			{
			  PointMSER * m = new PointMSER (representative - nodes, level, sign);
			  m->x        = r->x;
			  m->y        = r->y;
			  m->weight   = r->size;  // size and scale are closely correlated
			  m->scale    = scale;
			  m->detector = PointInterest::MSER;

			  // Cholesky decomposition (square root matrix) of covariance
			  double l11 = sqrt (r->xx);
			  double l12 = r->xy / l11;
			  double l22 = sqrt (r->yy - l12 * l12);
			  m->A(0,0) = l11 / scale;
			  m->A(1,0) = l12 / scale;
			  // A(0,1) == 0 due to PointAffine constructor
			  m->A(1,1) = l22 / scale;

			  regions.push_back (m);
			}
		  }
		}

		r = r->next;
	  }
	}
  }
}

static inline void
clear (InterestMSER::Root * roots)
{
  InterestMSER::Root * r = roots->next;
  while (r != roots)
  {
	r = r->next;
	delete r->previous;
  }
  roots->next = roots;
  roots->previous = roots;
}

void
InterestMSER::run (const Image & image, InterestPointSet & result)
{
  if (*image.format != GrayChar)
  {
	run (image * GrayChar, result);
	return;
  }

  // Separate image into gray levels
  //   Pass 1 -- Determine size of each gray-level list
  int listSizes[257];  // includes a 0 at the end to help set up stop points
  memset (listSizes, 0, 257 * sizeof (int));
  unsigned char * start = (unsigned char *) image.buffer;
  int imageSize = image.width * image.height;
  unsigned char * end = start + imageSize;
  unsigned char * pixel = start;
  while (pixel < end)
  {
	listSizes[*pixel++]++;
  }
  ImageOf<int> sorted (image.width, image.height, RGBAChar);  // not really RGBAChar, but it gives us the right size pixels
  int * lists[257];  // start points of list for each gray-level; includes a stop point at the end
  int * l = &sorted(0,0);
  for (int i = 0; i <= 256; i++)
  {
	lists[i] = l;
	l += listSizes[i];
  }
  //   Pass 2 -- Assign pixels to gray-level lists
  int * ls[256];  // working pointers into list
  memcpy (ls, lists, 256 * sizeof (unsigned char *));
  pixel = start;
  int i = 0;
  while (pixel < end)
  {
	*(ls[*pixel++])++ = i++;
  }

  // MSER+
  vector<PointMSER *> regions;
  Node * nodes = new Node[imageSize];
  memset (nodes, 0, imageSize * sizeof (Node));
  Root * roots = new Root;
  roots->next = roots;
  roots->previous = roots;
  for (int i = 0; i < 256; i++)
  {
	addGrayLevel ((unsigned char) i, true, lists, nodes, image.width, image.height, roots, regions);
  }

  // MSER-
  memset (nodes, 0, imageSize * sizeof (Node));
  clear (roots);
  for (int i = 255; i >= 0; i--)
  {
	addGrayLevel ((unsigned char) i, false, lists, nodes, image.width, image.height, roots, regions);
  }

  vector<PointMSER *>::iterator it = regions.begin ();
  while (it != regions.end ())
  {
	result.push_back (*it++);
  }

  clear (roots);
  delete roots;
  delete nodes;
}
