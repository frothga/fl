/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
	T lastDistance = value.norm (2);
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
	  T distance = value.norm (2);
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
