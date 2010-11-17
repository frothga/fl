/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_conjugate_gradient_tcc
#define fl_conjugate_gradient_tcc


#include "fl/search.h"
#include "fl/lapack.h"

#include <limits>


namespace fl
{
  // class GradientDescent ----------------------------------------------------

  /**
	 @param toleranceX If less than 0, then use sqrt (machine precision).
	 @param updateRate Proportion of gradient vector to add to position on first
	 iteration.  If negative, then we will head towards a minimum.  If
	 positive, we will head towards a maximum.  After first iteration, the
	 step size automatically rescales, but the sign remains the same.
  **/
  template<class T>
  GradientDescent<T>::GradientDescent (T toleranceX, T updateRate)
  : updateRate (updateRate)
  {
	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;
  }

  template<class T>
  void
  GradientDescent<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	SearchableGreedy<T> * greedy = dynamic_cast<SearchableGreedy<T> *> (&searchable);
	T bestResidual = INFINITY;

	T stepSize = updateRate;
	int gotBetter = 0;
	while (true)
	{
	  searchable.dimension (point);

	  // Find gradient
	  Vector<T> gradient;
	  searchable.gradient (point, gradient);
	  if (greedy  &&  greedy->bestResidual < bestResidual)
	  {
		bestResidual = greedy->bestResidual;
		point        = greedy->bestPoint;
	  }

	  // Search for a better value
	  while (true)
	  {
		Vector<T> newPoint = point + gradient * stepSize;
		Vector<T> result;
		searchable.value (newPoint, result);
		T residual = result.norm (2);

		if (residual < bestResidual)
		{
		  point = newPoint;
		  bestResidual = residual;
		  gotBetter++;
		  break;
		}
		else
		{
		  stepSize /= 2;
		  if (std::abs (stepSize) < toleranceX) return;
		  gotBetter = 0;
		}
	  }
	  if (gradient.norm (2) < toleranceX) return;
	  if (gotBetter > 3)
	  {
		gotBetter = 0;
		stepSize *= 2;
	  }
	}
  }
}


#endif
