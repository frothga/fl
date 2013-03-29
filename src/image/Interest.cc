/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 210 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/interest.h"


using namespace std;
using namespace fl;


// class InterestOperator -----------------------------------------------------

uint32_t InterestOperator::serializeVersion = 0;

InterestOperator::~InterestOperator ()
{
}

void
InterestOperator::run (const Image & image, PointSet & result)
{
  ImageCache::shared.setOriginal (image);
  run (ImageCache::shared, result);
}

void
InterestOperator::serialize (Archive & archive, uint32_t version)
{
}
