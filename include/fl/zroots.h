/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#ifndef fl_zroots_h
#define fl_zroots_h


#include "fl/matrix.h"


namespace fl
{
  /**
	 The venerable "Numerical Recipes in C" method.
  **/
  void zroots (const Vector<std::complex<double> > & a, Vector<std::complex<double> > & roots, bool polish = true, bool sortroots = true);
  int laguer (const Vector<std::complex<double> > & a, std::complex<double> & x);  ///< Subroutine of zroots()
}


#endif
