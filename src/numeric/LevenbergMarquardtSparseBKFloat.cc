/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


template <> const float LevenbergMarquardtSparseBK<float>::epsilon = FLT_EPSILON;
template <> const float LevenbergMarquardtSparseBK<float>::minimum = FLT_MIN;
template class LevenbergMarquardtSparseBK<float>;
