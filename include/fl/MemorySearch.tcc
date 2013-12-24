/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_memory_search_tcc
#define fl_memory_search_tcc


#include "fl/search.h"
#include "fl/lapack.h"

#include <limits>


namespace fl
{
  // class MemorySearch -------------------------------------------------------

  template<class T>
  MemorySearch<T>::MemorySearch (T toleranceF, T toleranceX)
  {
	if (toleranceF < (T) 0) toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceF = toleranceF;
	this->toleranceX = toleranceX;

	maxIterations = 200;
  }

  template<class T>
  void
  MemorySearch<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	const int parameters = point.rows ();
	const int last = 2 * parameters;

	std::vector<Sample *> samples;

	searchable.dimension (point);
	Vector<T> value = searchable.value (point);
	Sample * best = new Sample (point, value.norm (2));
	samples.push_back (best);

	// Scan for bounds in each dimension
	for (int d = 0; d < point.rows (); d++)
	{
	  int samplesAdded = 0;

	  // Scan in positive direction
	  T offset = (T) 1;
	  T start = point[d];
	  while (true)
	  {
		point[d] = start + offset;
		value = searchable.value (point);
		Sample * s = new Sample (point, value.norm (2));
		samples.push_back (s);
		samplesAdded++;
		if (std::isinf (s->value)) offset /= (T) 2.1;  // divide by slightly different number than the multiply in next case; otherwise, could have infinite loop
		else if (s->value <= best->value)
		{
		  if (s->value < best->value) best = s;
		  offset *= (T) 2;
		}
		else break;
	  }

	  // Scan in negative direction
	  // Only necessary if we added just one sample above, because otherwise
	  // the sample behind us implicitly has higher cost.
	  if (samplesAdded < 2)
	  {
		offset = (T) -1;
		while (true)
		{
		  point[d] = start + offset;
		  value = searchable.value (point);
		  Sample * s = new Sample (point, value.norm (2));
		  samples.push_back (s);
		  if (std::isinf (s->value)) offset /= (T) 2.1;  // divide by slightly different numberr than the multiply in next case; otherwise, could have infinite loop
		  else if (s->value <= best->value)
		  {
			if (s->value < best->value) best = s;
			offset *= (T) 2;
		  }
		  else break;
		}
	  }

	  point[d] = start;
	}

	// Look for minimum

	std::map<T, Sample *> sorted;
	for (int i = 0; i < samples.size (); i++)
	{
	  Sample * s = samples[i];
	  sorted.insert (std::make_pair ((s->x - best->x).norm (2), s));
	}
	std::vector<Sample *> top;
	typename std::map<T, Sample *>::iterator it = sorted.begin ();
	for (int i = 0; i <= last; i++, it++) top.push_back (it->second);

	Sample * previousBest = 0;
	int iterations = 0;
	while (iterations++ < maxIterations)
	{
	  Vector<T> target (parameters);

	  if (best != previousBest)
	  {
		previousBest = best;

		// Fit quadratic to 2d+1 points
		//std::cerr << "fitting" << std::endl;
		Matrix<T> A (last+1, last+1);
		Vector<T> B (last+1);
		for (int r = 0; r <= last; r++)
		{
		  Sample * s = top[r];
		  A(r,0) = (T) 1;
		  for (int j = 0; j < parameters; j++)
		  {
			int c = 1 + 2 * j;
			T x = s->x[j];
			A(r,c  ) = x;
			A(r,c+1) = x * x;
		  }

		  B[r] = s->value;
		}

		Matrix<T> X;
		gelss (A, X, B, (T *) 0, true, true);
		for (int j = 0; j < parameters; j++)
		{
		  int r = 1 + 2 * j;
		  target[j] = - X[r] / (2 * X[r+1]);
		}
	  }
	  else
	  {
		Vector<T> vNearest  = top[1   ]->x - best->x;
		Vector<T> vFarthest = top[last]->x - best->x;
		T ratio = vFarthest.norm (2) / vNearest.norm (2);
		if (ratio < 10) ratio = (T) 2;  // bisect, or bracket if farthest is more than 10x farther than nearest
		target = best->x + vFarthest / ratio;
	  }

	  // Sample, and update nearest neighbors
	  value = searchable.value (target);
	  Sample * s = new Sample (target, value.norm (2));
	  samples.push_back (s);
	  if (s->value < best->value)
	  {
		previousBest = best;
		best = s;
		sorted.clear ();
		for (int i = 0; i < samples.size (); i++)
		{
		  s = samples[i];
		  sorted.insert (std::make_pair ((s->x - best->x).norm (2), s));
		}
	  }
	  else
	  {
		sorted.insert (std::make_pair ((s->x - best->x).norm (2), s));
	  }
	  top.clear ();
	  it = sorted.begin ();
	  for (int i = 0; i <= last; i++, it++) top.push_back (it->second);

	  // Termination conditions
	  if ((top[last]->x - best->x).norm (2) < toleranceX) break;  // working range is sufficiently narrow that we don't want to continue probing
	  if (best->value < toleranceF) break;  // already near zero, so not much room or need for improvement
	  //   Check for sufficiently shallow local minimum
	  int i;
	  for (i = 1; i <= last; i++) if (top[i]->value - best->value < toleranceF) break;
	  if (i > last) break;
	}

	point = best->x;
	for (int i = 0; i < samples.size (); i++) delete samples[i];
  }
}


#endif
