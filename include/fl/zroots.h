#ifndef fl_zroots_h
#define fl_zroots_h


#include "fl/complex.h"
#include "fl/matrix.h"


namespace fl
{
  // The venerable "Numerical Recipes in C" method.
  int laguer (const fl::Vector<complex double> & a, complex double & x);
  void zroots (const fl::Vector<complex double> & a, fl::Vector<complex double> & roots, bool polish = true, bool sortroots = true);
}


#endif
