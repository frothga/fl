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


#ifndef fl_matrix_complex_tcc
#define fl_matrix_complex_tcc


#include "fl/matrix.h"

/// A small cheat to make the basic norm() template work.
namespace std
{
  inline complex<FL_BASIC_TYPE>
  max (const FL_BASIC_TYPE & a, const complex<FL_BASIC_TYPE> & b)
  {
	return complex<FL_BASIC_TYPE> (max (a, b.real ()), 0);
  }
}

#include "fl/Matrix.tcc"

namespace fl
{
  template<>
  MatrixResult<std::complex<FL_BASIC_TYPE> >
  MatrixAbstract<std::complex<FL_BASIC_TYPE> >::conj () const
  {
	int h = rows ();
	int w = columns ();
	for (int c = 0; c < w; c++)
	{
	  for (int r = 0; r < h; r++)
	  {
		std::complex<FL_BASIC_TYPE> & e = (*this)(r,c);
		e = std::conj (e);
	  }
	}
  }

  template<>
  MatrixResult<std::complex<FL_BASIC_TYPE> >
  MatrixStrided<std::complex<FL_BASIC_TYPE> >::conj () const
  {
	Matrix<std::complex<FL_BASIC_TYPE> > * result = new Matrix<std::complex<FL_BASIC_TYPE> > (rows_, columns_);
	std::complex<FL_BASIC_TYPE> * i = (std::complex<FL_BASIC_TYPE> *) result->data;
	std::complex<FL_BASIC_TYPE> * j = (std::complex<FL_BASIC_TYPE> *) data + offset;
	std::complex<FL_BASIC_TYPE> * end = i + rows_ * columns_;
	const int stepC = strideC - rows_ * strideR;
	while (i < end)
	{
	  std::complex<FL_BASIC_TYPE> * columnEnd = i + rows_;
	  while (i < columnEnd)
	  {
		*i++ = std::conj (*j);
		j += strideR;
	  }
	  j += stepC;
	}
	return result;
  }
}


#endif
