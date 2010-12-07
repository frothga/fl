/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/descriptor.h"


using namespace std;
using namespace fl;


// class DescriptorFiltersTexton ----------------------------------------------

DescriptorFiltersTexton::DescriptorFiltersTexton (int angles, int scales, float firstScale, float scaleStep)
{
  if (firstScale < 0)
  {
	firstScale = 1.0 / sqrt (2.0);
  }
  if (scaleStep < 0)
  {
	scaleStep = sqrt (2.0);
  }

  for (int i = 0; i < scales; i++)
  {
	float sigma = firstScale * pow (scaleStep, i);
	DifferenceOfGaussians d (sigma * scaleStep, sigma / scaleStep);
	d *= Normalize ();
	filters.push_back (d);
	for (int j = 0; j < angles; j++)
	{
	  float angle = (M_PI / angles) * j;
	  GaussianDerivativeSecond e (1, 1, 3 * sigma, sigma, angle);
	  e *= Normalize ();
	  filters.push_back (e);
	  GaussianDerivativeFirst o (1, 3 * sigma, sigma, angle);
	  o *= Normalize ();
	  filters.push_back (o);
	}
  }

  prepareFilterMatrix ();
}

