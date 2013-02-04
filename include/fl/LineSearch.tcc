/*
Author: Fred Rothganger

Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_line_search_tcc
#define fl_line_search_tcc


#include "fl/search.h"
#include "fl/lapack.h"

#include <limits>


namespace fl
{
  // class LineSearch --------------------------------------------------

  template<class T>
  LineSearch<T>::LineSearch (T toleranceF, T toleranceX)
  {
	if (toleranceF < (T) 0) toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceF = toleranceF;
	this->toleranceX = toleranceX;
	//std::cerr << "toleranceF,X = " << toleranceF << " " << toleranceX << std::endl;

	maxIterations = 200;
  }

  /**
	 @todo Keeping a vector of points visited is inefficient, since we
	 really only need the current triple of points.  However, it makes
	 the code easier to write.
   **/
  template<class T>
  void
  LineSearch<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	if (point.rows () == 0) throw "Line search requires a point with at least one element.";

	std::vector<T> xs (3);
	std::vector<T> ys (3);

	xs[0] = point[0] - (T) 1.0;
	xs[1] = point[0];
	xs[2] = point[0] + (T) 1.0;

	Vector<T> value;

	int i = -1;
	T bestY = INFINITY;
	for (int j = 0; j < 3; j++)
	{
	  point[0] = xs[j];
	  searchable.dimension (point);
	  value = searchable.value (point);
	  T y = value.norm (2);
	  ys[j] = y;
	  if (y < bestY)
	  {
		bestY = y;
		i = j;
	  }
	}
	if (i < 0) throw "Function appears flat";

	// These flags allow us to apply special tricks to speed up search,
	// but only once until the configuration of points changes.
	bool newCenter = true;
	bool newLeft   = true;
	bool newRight  = true;

	int iterations = 0;
	while (iterations++ < maxIterations)
	{
	  T x;
	  int it;  // position at which to insert next (x,y) pair
	  if (i == 0)
	  {
		x = xs[i] - (xs[i+1] - xs[i]) * 2;
		it = 0;
	  }
	  else if (i == xs.size () - 1)
	  {
		x = xs[i] + (xs[i] - xs[i-1]) * 2;
		it = i + 1;
	  }
	  else
	  {
		bool needBisect = true;

		T dLeft = xs[i] - xs[i-1];
		T dRight = xs[i+1] - xs[i];
		if (newLeft  &&  dLeft / dRight > 10)
		{
		  //std::cerr << "bracketing on left" << std::endl;
		  x = xs[i] - dRight;
		  it = i;
		  newLeft = false;
		  needBisect = false;
		}
		else if (newRight  &&  dRight / dLeft > 10)
		{
		  //std::cerr << "bracketing on right" << std::endl;
		  x = xs[i] + dLeft;
		  it = i + 1;
		  newRight = false;
		  needBisect = false;
		}
		else if (newCenter)
		{
		  // Fit quadratic to three points
		  //std::cerr << "fitting" << std::endl;
		  Matrix<T> A (3, 3);
		  Vector<T> B (3);
		  for (int j = 0; j < 3; j++)
		  {
			x = xs[i - 1 + j];
			A(j,0) = (T) 1;
			A(j,1) = x;
			A(j,2) = x * x;

			B[j] = ys[i - 1 + j];
		  }

		  Matrix<T> X;
		  gelss (A, X, B, (T *) 0, true, true);
		  x = - X[1] / (2 * X[2]);

		  // Sometimes x can be out of range due to low slope, so guard
		  // against this.
		  needBisect = false;
		  if      (x >= xs[i+1]) needBisect = true;
		  else if (x >  xs[i])   it = i + 1;
		  else if (x >  xs[i-1]) it = i;
		  else                   needBisect = true;

		  newCenter = false;
		}

		if (needBisect)
		{
		  // Standard bisection
		  //std::cerr << "bisecting" << std::endl;
		  T diff01 = xs[i]   - xs[i-1];
		  T diff12 = xs[i+1] - xs[i];
		  if (diff01 > diff12)
		  {
			x = (xs[i-1] + xs[i]) / 2;
			it = i;
		  }
		  else
		  {
			x = (xs[i] + xs[i+1]) / 2;
			it = i + 1;
		  }
		}
	  }

	  point[0] = x;
	  searchable.dimension (point);
	  value = searchable.value (point);
	  T y = value.norm (2);
	  xs.insert (xs.begin () + it, x);
	  ys.insert (ys.begin () + it, y);

	  if (it == i) i++;
	  if (ys[it] < ys[i])
	  {
		if (it < i) newRight = true;  // because the center will become the right
		else        newLeft  = true;  // because the center will become the left
		newCenter = true;
		i = it;
	  }
	  else
	  {
		if (it < i) newLeft  = true;
		else        newRight = true;
	  }

	  // Termination conditions
	  int a = std::max (0,                    i - 1);
	  int b = std::min ((int) xs.size () - 1, i + 1);
	  if (xs[b] - xs[a] < toleranceX) break;  // working range is sufficiently narrow that we don't want to continue probing
	  if (   ys[b] - ys[i] < toleranceF
		  && ys[a] - ys[i] < toleranceF) break;  // sufficiently shallow local minimum
	  if (ys[i] < toleranceF) break;  // already near zero, so not much room or need for improvement
	}

	point[0] = xs[i];
  }
}


#endif
