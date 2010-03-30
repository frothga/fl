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
  // class SearchableConstriction ---------------------------------------------

  template<class T>
  SearchableConstriction<T>::SearchableConstriction (Searchable<T> & searchable, const Vector<T> & a, const Vector<T> & b)
  : searchable (searchable),
	a (a),
	b (b)
  {
  }

  template<class T>
  int
  SearchableConstriction<T>::dimension ()
  {
	return searchable.dimension ();
  }

  template<class T>
  void
  SearchableConstriction<T>::value (const Vector<T> & point, Vector<T> & value)
  {
	searchable.value (a + b * point[0], value);
  }


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
	int lastIteration = maxIterations > 0 ? maxIterations : point.rows ();

	Vector<T> d;
	Vector<T> r;
	searchable.gradient (point, r);
	r *= -1;

	T delta = r.sumSquares ();
	T thresholdX = delta * toleranceX * toleranceX;

	//LineSearch<T> lineSearch;  // slightly fewer iterations
	NewtonRaphson<T> lineSearch;  // slightly lower residual

	for (int i = 0; i < lastIteration  &&  delta > thresholdX; i++)
	{
	  if (i % restartIterations == 0  ||  r.dot (d) <= 0) d = r;

	  // Line search for optimal position along current direction
	  SearchableConstriction<T> line (searchable, point, d);
	  Vector<T> alpha (1);
	  alpha[0] = 0;
	  lineSearch.toleranceX = std::sqrt (toleranceA * toleranceA / d.sumSquares ());
	  lineSearch.search (line, alpha);
	  point += d * alpha[0];

	  // Update direction
	  searchable.gradient (point, r);
	  r *= -1;
	  T deltaOld = delta;
	  delta = r.sumSquares ();
	  d = r + d * (delta / deltaOld);
	}
  }
}


#endif
