/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


const float LevenbergMarquardtSparseBK<float>::epsilon = FLT_EPSILON;
const float LevenbergMarquardtSparseBK<float>::minimum = FLT_MIN;
template class LevenbergMarquardtSparseBK<float>;
