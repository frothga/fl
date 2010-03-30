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
	 @param updateRate Proportion of gradient vector to add to position at each
	 iteration.  If negative, then we will head towards a minimum.  If
	 positive, we will head towards a maximum.  Default is -0.01.
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
	while (true)
	{
	  Vector<T> gradient;
	  searchable.gradient (point, gradient);
	  point += gradient * updateRate;
	  if (gradient.norm (2) < toleranceX) break;
	}
  }
}


#endif
