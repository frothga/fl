/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_fourier_h
#define fl_fourier_h


#include "fl/matrix.h"

#include <complex>

#include <fftw3.h>


namespace fl
{
  template<class T>
  class Fourier
  {
  public:
	Fourier (bool destroyInput = true);
	~Fourier ();

	void dft (int direction, const Matrix<std::complex<T> > & input, Matrix<std::complex<T> > & output);
	void dft (               const Matrix<std::complex<T> > & input, Matrix<T>                & output);
	void dft (               const Matrix<T>                & input, Matrix<std::complex<T> > & output);
 	void dft (int kind,      const Matrix<T>                & input, Matrix<T>                & output);

 	void dht  (const MatrixAbstract<T> & input, Matrix<T> & output);

 	void dct  (const MatrixAbstract<T> & input, Matrix<T> & output);
 	void idct (const MatrixAbstract<T> & input, Matrix<T> & output);
 	void dst  (const MatrixAbstract<T> & input, Matrix<T> & output);
 	void idst (const MatrixAbstract<T> & input, Matrix<T> & output);

	bool destroyInput;  ///< Indicates that the input matrix can be overwritten by the process.

	// Cached plan (internal to implementation).
	fftw_plan     cachedPlan;
	int           cachedDirection;
	int           cachedKind;
	unsigned int  cachedFlags;
	int           cachedInRows;
	int           cachedInColumns;
	int           cachedInStrideR;
	int           cachedInStrideC;
	int           cachedAlignment;
	bool          cachedInPlace;
  };
}


#endif
