/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
*/


#include "fl/LevenbergMarquardt.tcc"


using namespace fl;


template <> const double LevenbergMarquardt<double>::epsilon = DBL_EPSILON;
template <> const double LevenbergMarquardt<double>::minimum = DBL_MIN;
template class LevenbergMarquardt<double>;
