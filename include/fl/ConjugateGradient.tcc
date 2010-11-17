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
  // class ConjugateGradient --------------------------------------------------

  template<class T>
  ConjugateGradient<T>::ConjugateGradient (T toleranceX, int restartIterations, int maxIterations)
  : restartIterations (restartIterations),
	maxIterations (maxIterations)
  {
	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;

	toleranceA = 0.001;  // by default, stop at 1/1000th of the direction vector length
  }

  /**
	 This implementation is based on Appendix B4 of "An Introduction to the
	 Conjugate Gradient Method Without the Agonizing Pain" by J. R. Shewchuk.
  **/
  template<class T>
  void
  ConjugateGradient<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	int iterations = maxIterations > 0 ? maxIterations : point.rows ();
	SearchableGreedy<T> * greedy = dynamic_cast<SearchableGreedy<T> *> (&searchable);
	T bestResidual = INFINITY;

	Vector<T> d;
	Vector<T> r;
	Vector<T> s;
	searchable.dimension (point);
	searchable.gradient (point, r);
	if (greedy  &&  greedy->bestResidual < bestResidual)
	{
	  bestResidual = greedy->bestResidual;
	  point        = greedy->bestPoint;
	}
	r *= -1;
	bool doScaling = scales.rows () == r.rows ();
	if (doScaling) s = r & scales;
	else           s = r;
	d = s;

	T beta = 0;
	T delta = r.dot (d);
	T thresholdX = delta * toleranceX * toleranceX;

	//LineSearch<T> lineSearch;  // slightly fewer iterations
	NewtonRaphson<T> lineSearch;  // slightly lower residual

	for (int i = 0; i < iterations  &&  delta > thresholdX; i++)
	{
	  // The line search below will issue at least one call to dimension(),
	  // so we don't need an explicit one at the top of this loop.

	  // Line search for optimal position along current direction
	  SearchableConstriction<T> line (searchable, point, d);
	  Vector<T> alpha (1);
	  alpha[0] = 0;
	  lineSearch.toleranceX = std::sqrt (toleranceA * toleranceA / d.sumSquares ());
	  lineSearch.search (line, alpha);
	  if (greedy  &&  greedy->bestResidual < bestResidual)
	  {
		bestResidual = greedy->bestResidual;
		point        = greedy->bestPoint;
	  }
	  else
	  {
		point += d * alpha[0];
	  }

	  // Update direction
	  Vector<T> r;  // Construct a new r to avoid aliasing with s.  s must remain distinct from r until after deltaMid is calculated.
	  searchable.gradient (point, r);
	  if (greedy  &&  greedy->bestResidual < bestResidual)
	  {
		bestResidual = greedy->bestResidual;
		point        = greedy->bestPoint;
	  }
	  r *= -1;
	  T deltaOld = delta;
	  T deltaMid = r.dot (s);
	  if (doScaling) s = r & scales;
	  else           s = r;
	  delta = r.dot (s);
	  beta = (delta - deltaMid) / deltaOld;
	  if ((i  &&  i % restartIterations == 0)  ||  beta <= 0)
	  {
		d = s;
	  }
	  else
	  {
		d = s + d * beta;
	  }
	}
  }
}


#endif
