#include "fl/search.h"
#include "fl/random.h"

// For debugging
#include <iostream>
using namespace std;


using namespace fl;


// class AnnealingAdaptive ----------------------------------------------------

AnnealingAdaptive::AnnealingAdaptive (bool minimize, int levels, int patience)
{
  this->minimize = minimize;
  this->levels = levels;
  this->patience = patience;
}

void
AnnealingAdaptive::search (Searchable & searchable, Vector<double> & point)
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

  Vector<double> value;
  searchable.value (point, value);
  double lastDistance = value.frob (2);
  int gotBetter = 0;
  int gotWorse = 0;
  int level = 0;
  while (level < levels)
  {
	// Generate a guess
	Vector<double> guess (dimension);
	for (int r = 0; r < dimension; r++)
	{
	  guess[r] = randGaussian ();
	}
	guess.normalize ();
	guess *= pow (0.5, level);
	guess += point;

	// Evaluate distance from guess to value
	searchable.value (guess, value);
	double distance = value.frob (2);
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
