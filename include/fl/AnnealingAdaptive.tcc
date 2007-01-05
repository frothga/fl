/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/01/05 14:05:02  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/09/26 04:18:06  Fred
Add detail to revision history.

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/12 04:59:59  rothgang
Use std versions of pow, sqrt, and fabs so that choice of type specific version
will be automatic when template is instantiated.  IE: std contains type
overloaded versions of the functions rather than separately named functions.

Revision 1.1  2004/04/19 21:22:43  rothgang
Create template versions of Search classes.
-------------------------------------------------------------------------------
*/


#ifndef fl_annealing_adaptive_tcc
#define fl_annealing_adaptive_tcc


#include "fl/search.h"
#include "fl/random.h"
#include "fl/math.h"


namespace fl
{
  // class AnnealingAdaptive --------------------------------------------------

  template<class T>
  AnnealingAdaptive<T>::AnnealingAdaptive (bool minimize, int levels, int patience)
  {
	this->minimize = minimize;
	this->levels = levels;
	this->patience = patience;
  }

  template<class T>
  void
  AnnealingAdaptive<T>::search (Searchable<T> & searchable, Vector<T> & point)
  {
	int dimension = point.rows ();

	int patience = this->patience;
	if (patience < 0)
	{
	  patience = dimension;
	}
	if (patience < 1)
	{
	  patience = 1;
	}

	Vector<T> value;
	searchable.value (point, value);
	T lastDistance = value.frob (2);
	int gotBetter = 0;
	int gotWorse = 0;
	int level = 0;
	while (level < levels)
	{
	  // Generate a guess
	  Vector<T> guess (dimension);
	  for (int r = 0; r < dimension; r++)
	  {
		guess[r] = randGaussian ();
	  }
	  guess.normalize ();
	  guess *= std::pow ((T) 0.5, level);
	  guess += point;

	  // Evaluate distance from guess to value
	  searchable.value (guess, value);
	  T distance = value.frob (2);
	  bool improved;
	  if (minimize)
	  {
		improved = distance <= lastDistance;
	  }
	  else
	  {
		improved = distance >= lastDistance;
	  }

	  // If improved, keep guess
	  if (improved)
	  {
		gotBetter++;
		gotWorse = 0;
		point = guess;
		lastDistance = distance;
	  }
	  else
	  {
		gotWorse++;
		gotBetter = 0;
	  }

	  // Adjust temperature
	  if (gotWorse > patience)
	  {
		level++;
		gotWorse = 0;
	  }
	  if (gotBetter > patience)
	  {
		level--;
		gotBetter = 0;
	  }
	}
  }
}


#endif
