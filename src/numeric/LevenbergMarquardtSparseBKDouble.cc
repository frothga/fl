/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


const double LevenbergMarquardtSparseBK<double>::epsilon = DBL_EPSILON;
const double LevenbergMarquardtSparseBK<double>::minimum = DBL_MIN;
template class LevenbergMarquardtSparseBK<double>;
