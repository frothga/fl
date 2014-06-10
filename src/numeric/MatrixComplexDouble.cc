/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include <complex>
/// A small cheat to make the basic norm() template work.
namespace std
{
  inline complex<double>
  max (const double & a, const complex<double> & b)
  {
	return complex<double> (max (a, b.real ()), 0);
  }
}

#define flNumeric_MS_EVIL
#include "fl/Matrix.tcc"


using namespace fl;
using namespace std;


template class MatrixAbstract<complex<double> >;
template class MatrixStrided<complex<double> >;
template class Matrix<complex<double> >;
template class MatrixTranspose<complex<double> >;
template class MatrixRegion<complex<double> >;

namespace fl
{
  template SHARED std::ostream & operator << (std::ostream & stream, const MatrixAbstract<complex<double> > & A);
  template SHARED std::istream & operator >> (std::istream & stream, MatrixAbstract<complex<double> > & A);
  template SHARED MatrixAbstract<complex<double> > & operator << (MatrixAbstract<complex<double> > & A, const std::string & source);
}
