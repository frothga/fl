/*
Author: Fred Rothganger
Created 5/31/2005
Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Change Image structure.
*/


#include "fl/interest.h"

#include <math.h>


using namespace std;
using namespace fl;


// class InterestMSER ---------------------------------------------------------

InterestMSER::InterestMSER (int delta, float sizeRatio)
{
  this->delta     = delta;
  this->sizeRatio = sizeRatio;
  minScale        = 1.0;
  minSize         = 30;
  maxSizeRatio    = 0.01f;
  minLevels       = 10;
  maxRate         = 1.0f;
}

inline void
InterestMSER::clear (Root * head)
{
  Root * r = head->next;
  while (r != head)
  {
	r = r->next;
	delete r->previous;
  }
  head->next = head;
  head->previous = head;
}

inline void
InterestMSER::move (Root * root, Root & head)
{
  root->next->previous = root->previous;
  root->previous->next = root->next;
  root->next = head.next;
  root->previous = &head;
  head.next->previous = root;
  head.next = root;
}

inline void
InterestMSER::releaseAll (Root * head)
{
  if (head->next == head) return;  // guard against empty list
  head->next->previous = &deleted;
  head->previous->next = deleted.next;
  deleted.next->previous = head->previous;
  deleted.next = head->next;
  head->next = head;
  head->previous = head;
}

inline void
InterestMSER::merge (Node * grow, Node * destroy)
{
  grow->root->size += destroy->root->size;
  destroy->parent = grow;

  if (destroy->root->tail)
  {
	destroy->root->tail->next = grow->root->head;
	destroy->root->tail->root = destroy->root;
	grow->root->head = destroy->root->head;
	move (destroy->root, subsumed);
  }
  else
  {
	destroy->next = grow->root->head;
	grow->root->head = destroy->root->head;
	move (destroy->root, deleted);
  }
  destroy->root = 0;
}

/**
   Find parent, with path compression.  In CLR ch. 22, this is presented as
   a recursive function.  However, recursion is not necessary, and managing
   a private stack is more efficient.  This is also safe, since it can be
   shown that the stack depth won't exceed the number of gray-levels, due to
   the fact that pixels are added in image sequence.
**/
inline InterestMSER::Node *
InterestMSER::findSet (Node * n)
{
  if (n->parent == n) return n;  // early out

  Node * path[256];
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

/**
   Combine a given pixel i with a given neighbor n.  If i is already in
   a set, then join the sets.
**/
inline void
InterestMSER::join (Node * i, Node * n)
{
  if (n->parent)
  {
	Node * r = findSet (n);
	if (i->parent)
	{
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
InterestMSER::addGrayLevel (unsigned char level, bool sign, vector<PointMSER *> & regions)
{
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
	  Root * r;
	  if (deleted.next == &deleted)
	  {
		r = new Root;
	  }
	  else
	  {
		r = deleted.next;
		r->next->previous = &deleted;
		deleted.next = r->next;
	  }
	  r->next = roots.next;
	  r->next->previous = r;
	  r->previous = &roots;
	  roots.next = r;

	  r->size = 1;
	  r->level = level;
	  r->center = sign ? level + delta - 1 : level - delta + 1;
	  r->lower = level;
	  r->head = i;
	  r->tail = 0;
	  r->tailSize = 0;

	  i->root = r;
	  i->parent = i;
	  // i->next == 0 by construction ("nodes" is zeroed)
	}

	l++;
  }

  // Update all histories
  Root * r = roots.next;
  while (r != &roots)
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
	r = roots.next;
	while (r != &roots)
	{
	  if (sign ? r->level > a : r->level < b)
	  {
		r = r->next;
		continue;
	  }
	  r->rates[c] = fabs ((float) (r->sizes[b] - r->sizes[a])) / r->sizes[c];

	  unsigned char & center = r->center;
	  unsigned char & lower  = r->lower;
	  unsigned char firstRate = sign ? r->level + delta : r->level - delta;
	  while (true)
	  {
		bool localMinimum = true;
		if (sign)
		{
		  if (c - center <= 1) break;
		  if ((float) r->sizes[center+1] / r->sizes[c+1] >= sizeRatio) break;
		  center++;
		  if (r->rates[center] > maxRate) continue;
		  if (center - r->level < minLevels) continue;
		  if (r->sizes[center] < minSize  ||  r->sizes[center] > maxSize) continue;
		  while ((float) r->sizes[lower] / r->sizes[center] < sizeRatio  &&  center - lower > 1) lower++;
		  if (lower < firstRate) continue;

		  float localRate = r->rates[center];
		  int i = lower;
		  localMinimum = r->rates[i++] > localRate;
		  while (localMinimum  &&  i < center)
		  {
			localMinimum &= r->rates[i++] >= localRate;
		  }
		  i++;
		  while (localMinimum  &&  i <= c)
		  {
			localMinimum &= r->rates[i++] > localRate;
		  }
		}
		else
		{
		  if (center - c <= 1) break;
		  if ((float) r->sizes[center-1] / r->sizes[c-1] >= sizeRatio) break;
		  center--;
		  if (r->rates[center] > maxRate) continue;
		  if (r->level - center < minLevels) continue;
		  if (r->sizes[center] < minSize  ||  r->sizes[center] > maxSize) continue;
		  while ((float) r->sizes[lower] / r->sizes[center] < sizeRatio  &&  lower - center > 1) lower--;
		  if (lower > firstRate) continue;

		  float localRate = r->rates[center];
		  int i = lower;
		  localMinimum = r->rates[i--] > localRate;
		  while (localMinimum  &&  i > center)
		  {
			localMinimum &= r->rates[i--] >= localRate;
		  }
		  i--;
		  while (localMinimum  &&  i >= c)
		  {
			localMinimum &= r->rates[i--] > localRate;
		  }
		}
		if (! localMinimum) continue;

		// Got an MSER!  Now record it and generate shape matrix and scale.

		  Node * head = r->heads[center];
		  Node * representative = findSet (head);
		  int newWeight = r->sizes[center];
		  float totalWeight = newWeight;

		  Root * otherGaussians = 0;  // a singly-linked list
		  Root * next     = r->next;  // for reconnecting r to roots
		  Root * previous = r->previous;

		  // Find mean
		  float cx = 0;
		  float cy = 0;
		  Node * n = head;
		  while (n)
		  {
			if (n->root  &&  n->root->tail)  // n->root->tail ensures we don't suppress the very first pixel in a region
			{
			  Root * og = n->root;
			  newWeight -= og->tailSize;
			  og->next->previous = og->previous;
			  og->previous->next = og->next;
			  og->next = otherGaussians;
			  otherGaussians = og;
			}
			else
			{
			  int index = n - nodes;
			  cx += index % width;
			  cy += index / width;
			}
			n = n->next;
		  }
		  float x = cx / newWeight;
		  float y = cy / newWeight;

		  // Find covariance
		  float xx = 0;
		  float xy = 0;
		  float yy = 0;
		  n = head;
		  while (n)
		  {
			if (! n->root  ||  ! n->root->tail)
			{
			  int index = n - nodes;
			  float dx = index % width - x;
			  float dy = index / width - y;
			  xx += dx * dx;
			  xy += dx * dy;
			  yy += dy * dy;
			}
			n = n->next;
		  }
		  xx /= newWeight;
		  xy /= newWeight;
		  yy /= newWeight;

		  // Merge Gaussian with previous ones, if they exist
		  if (otherGaussians)
		  {
			// Find weighted mean of all Gaussians
			Root * og = otherGaussians;
			while (og)
			{
			  cx += og->x * og->tailSize;
			  cy += og->y * og->tailSize;
			  og = og->next;
			}
			cx /= totalWeight;
			cy /= totalWeight;

			// Find net covariance
			float dx = x - cx;
			float dy = y - cy;
			x = cx;
			y = cy;
			xx = (xx + dx * dx) * newWeight;
			xy = (xy + dx * dy) * newWeight;
			yy = (yy + dy * dy) * newWeight;
			og = otherGaussians;
			while (og)
			{
			  dx = og->x - cx;
			  dy = og->y - cy;
			  xx += (og->xx + dx * dx) * og->tailSize;
			  xy += (og->xy + dx * dy) * og->tailSize;
			  yy += (og->yy + dy * dy) * og->tailSize;
			  og = og->next;
			}
			xx /= totalWeight;
			xy /= totalWeight;
			yy /= totalWeight;

			// Destroy old Root objects
			og = otherGaussians;
			if (og == r)  // r will always be the last added to otherGaussians
			{
			  og = og->next;
			  r->next     = next;
			  r->previous = previous;
			  next->previous = r;
			  previous->next = r;
			}
			while (og)
			{
			  Root * nextOG = og->next;
			  og->next = deleted.next;
			  og->previous = &deleted;
			  deleted.next->previous = og;
			  deleted.next = og;
			  og = nextOG;
			}
		  }
		  r->x  = x;
		  r->y  = y;
		  r->xx = xx;
		  r->xy = xy;
		  r->yy = yy;
		  r->tail     = head;
		  r->tailSize = r->sizes[center];  // same as totalWeight, but avoid round trip int->float->int
		  head->root = r;  // representative->root also points to r, but has a different job
		  head->next = 0;

		  // Update affine part of point
		  //   Determine scale
		  float scale = sqrt (sqrt (r->xx * r->yy - r->xy * r->xy));  // two sqrt() calls are probably more efficient than 1 pow(0.25, x) call
		  if (scale >= minScale)
		  {
			PointMSER * m = new PointMSER (representative - nodes, center, sign);
			m->x        = r->x;
			m->y        = r->y;
			m->weight   = totalWeight;  // size and scale are closely correlated
			m->scale    = scale;
			m->detector = PointInterest::MSER;

			// Cholesky decomposition (square root matrix) of covariance
			double l11 = sqrt (r->xx);
			double l12 = r->xy / l11;
			double l22 = sqrt (r->yy - l12 * l12);
			double temp = sqrt (l11 * l22);
			m->A(0,0) = l11 / scale;
			m->A(1,0) = l12 / scale;
			// A(0,1) == 0 due to PointAffine constructor
			m->A(1,1) = l22 / scale;

			regions.push_back (m);
		  }
		}

	  r = r->next;
	}
  }
}

void
InterestMSER::run (const Image & image, InterestPointSet & result)
{
  if (*image.format != GrayChar)
  {
	run (image * GrayChar, result);
	return;
  }

  PixelBufferPacked * imageBuffer = (PixelBufferPacked *) image.buffer;
  if (! imageBuffer) throw "InterestMSER only handles packed buffers for now";

  width  = image.width;
  height = image.height;
  int imageSize = width * height;
  maxSize = (int) ceil (imageSize * maxSizeRatio);

  // Separate image into gray levels
  //   Pass 1 -- Determine size of each gray-level list
  int listSizes[257];  // includes a 0 at the end to help set up stop points
  memset (listSizes, 0, 257 * sizeof (int));
  unsigned char * start = (unsigned char *) imageBuffer->memory;
  unsigned char * end = start + imageSize;
  unsigned char * pixel = start;
  while (pixel < end)
  {
	listSizes[*pixel++]++;
  }
  ImageOf<int> sorted (width, height, RGBAChar);  // not really RGBAChar, but it gives us the right size pixels
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

  // Prepare structures for MSER passes
  nodes = new Node[imageSize];
  roots.next        = &roots;
  roots.previous    = &roots;
  subsumed.next     = &subsumed;
  subsumed.previous = &subsumed;
  deleted.next      = &deleted;
  deleted.previous  = &deleted;

  // MSER+
  vector<PointMSER *> regions;
  memset (nodes, 0, imageSize * sizeof (Node));
  for (int i = 0; i < 256; i++)
  {
	addGrayLevel ((unsigned char) i, true, regions);
  }

  // MSER-
  memset (nodes, 0, imageSize * sizeof (Node));
  releaseAll (&roots);
  releaseAll (&subsumed);
  for (int i = 255; i >= 0; i--)
  {
	addGrayLevel ((unsigned char) i, false, regions);
  }

  // Destroy structures
  delete nodes;
  clear (&roots);
  clear (&subsumed);
  clear (&deleted);

  // Store final result
  result.insert (result.end (), regions.begin (), regions.end ());
}
