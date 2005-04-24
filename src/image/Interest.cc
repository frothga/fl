/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/interest.h"


using namespace std;
using namespace fl;


// class InterestOperator -----------------------------------------------------

void
InterestOperator::run (const Image & image, vector<PointInterest> & result)
{
  multiset<PointInterest> temp;
  run (image, temp);
  result.clear ();
  result.insert (result.end (), temp.begin (), temp.end ());
}

