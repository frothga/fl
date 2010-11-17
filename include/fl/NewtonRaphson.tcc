/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_newton_raphson_tcc
#define fl_newton_raphson_tcc


#include "fl/search.h"
#include "fl/lapack.h"

#include <limits>


namespace fl
{
  // class NewtonRaphson ------------------------------------------------------

  /**
	 @param direction Indicates whether we should seek a minimum (-1),
	 maximum (1), or simply the closest extremum (0).  Normally, the
	 Newton-Raphson is described as seeking the nearest extremum.  However,
	 we set the default to seek a minimum, since this is the most common use.
  **/
  template<class T>
  NewtonRaphson<T>::NewtonRaphson (int direction, T toleranceX, T updateRate, int maxIterations)
  : direction (direction),
	updateRate (updateRate),
	maxIterations (maxIterations)
  {
	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;
  }

  template<class T>
  void
  NewtonRaphson<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	for (int i = 0; i < maxIterations; i++)
	{
	  searchable.dimension (point);

	  Vector<T> g;
	  searchable.gradient (point, g);

	  Matrix<T> H;
	  searchable.hessian (point, H);

	  // delta = !H * g; save eigenvalues for sign test
	  Matrix<T> Z;
	  Vector<T> L;
	  syev (H, L, Z);
	  Vector<T> delta = Z * (~Z * g / L);

	  if (direction == 0)
	  {
		delta *= -updateRate;
	  }
	  else
	  {
		// Determine if H is positive or negative definite, that is, if we are
		// headed towards a local minimum or maximum, respectively.  For any
		// other case (saddle point) we do nothing.
		int sign = 0;
		int positive = 0;
		int negative = 0;
		for (int j = 0; j < L.rows (); j++)
		{
		  if      (L[j] > 0) positive++;
		  else if (L[j] < 0) negative++;
		}
		if (positive >  0  &&  negative == 0) sign =  1;  // positive semi-definite
		if (positive == 0  &&  negative >  0) sign = -1;  // negative semi-definite

		// Manipulate sign so we go in the right direction
		if (sign) delta *= sign * direction * updateRate;
		else      delta *= -updateRate;
	  }

	  point += delta;
	  if (delta.norm (2) < toleranceX) break;
	}
  }
}


#endif
