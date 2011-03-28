/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_fourier_tcc
#define fl_fourier_tcc


#include "fl/fourier.h"


namespace fl
{
  /**
	 Count the number of bits with value 0 that are less significant than the
	 least-significant bit with value 1 in the input word.
	 This code came from http://graphics.stanford.edu/~seander/bithacks.html.
	 That page states at the top that individual code fragments such as this
	 one are in the public domain.
  **/
  static inline int
  trailingZeros (const uint32_t & a)
  {
	static const int Mod37BitPosition[] = // map a bit value mod 37 to its position
	{
	  32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
	  7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
	  20, 8, 19, 18
	};
	return Mod37BitPosition[(-a & a) % 37];
  }

  template<class T>
  Fourier<T>::Fourier (bool normalize, bool destroyInput, bool sizeFromOutput)
  : normalize (normalize),
	destroyInput (destroyInput),
	sizeFromOutput (sizeFromOutput)
  {
	cachedPlan = 0;
  }

  template<class T>
  Fourier<T>::~Fourier ()
  {
	if (cachedPlan)
	{
	  pthread_mutex_lock   (&mutexPlan);
	  destroy_plan (cachedPlan);
	  pthread_mutex_unlock (&mutexPlan);
	}
  }

  template<class T>
  void
  Fourier<T>::dft (int direction, const MatrixStrided<std::complex<T> > & I, MatrixStrided<std::complex<T> > & O)
  {
	int rows = I.rows ();
	int cols = I.columns ();
	if (sizeFromOutput  &&  O.rows ()  &&  O.columns ())
	{
	  rows = std::min (rows, O.rows ());
	  cols = std::min (cols, O.columns ());
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	typename traitsFFTW<T>::iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != direction
          || cachedKind       != -1  // complex to complex
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		pthread_mutex_lock   (&mutexPlan);
		destroy_plan (cachedPlan);
		pthread_mutex_unlock (&mutexPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  pthread_mutex_lock   (&mutexPlan);
	  cachedPlan = plan
	  (
		rank, dims,
		0, 0,
		(typename traitsFFTW<T>::complex *) Idata, (typename traitsFFTW<T>::complex *) Odata,
		direction, flags
	  );
	  pthread_mutex_unlock (&mutexPlan);
	  cachedDirection = direction;
	  cachedKind      = -1;  // complex to complex
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	execute (cachedPlan, (typename traitsFFTW<T>::complex *) Idata, (typename traitsFFTW<T>::complex *) Odata);
	if (normalize) O /= std::sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (const MatrixStrided<T> & I, MatrixStrided<std::complex<T> > & O)
  {
	int rows = I.rows ();
	int cols = I.columns ();
	if (sizeFromOutput  &&  O.rows ()  &&  O.columns ())
	{
	  rows = std::min (rows, (O.rows () - 1) * 2 + 1);
	  cols = std::min (cols,  O.columns ());
	}
	const int Orows = rows / 2 + 1;
	if (O.rows () < Orows  ||  O.columns () < cols) O.resize (Orows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	typename traitsFFTW<T>::iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != -1   // forward
          || cachedKind       != -2   // mixed complex-real
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		pthread_mutex_lock   (&mutexPlan);
		destroy_plan (cachedPlan);
		pthread_mutex_unlock (&mutexPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  pthread_mutex_lock   (&mutexPlan);
	  cachedPlan = plan
	  (
		rank, dims,
		0, 0,
		(T *) Idata, (typename traitsFFTW<T>::complex *) Odata,
		flags
	  );
	  pthread_mutex_unlock (&mutexPlan);
	  cachedDirection = -1;  // forward
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	execute (cachedPlan, (T *) Idata, (typename traitsFFTW<T>::complex *) Odata);
	if (normalize) O /= std::sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (const MatrixStrided<std::complex<T> > & I, MatrixStrided<T> & O)
  {
	int rows = (I.rows () - 1) * 2;
	int cols =  I.columns ();
	if (sizeFromOutput  &&  O.rows ()  &&  O.columns ())
	{
	  // If O is larger than the largest odd-size, we always trim back to the
	  // largest odd-size.  However, it might be more intuitive to base this
	  // on the oddness/evenness of O.
	  rows = std::min (rows + 1, O.rows ());
	  cols = std::min (cols,     O.columns ());
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	// No input-preserving transformation is available, so copy off data if
	// destroyInput is false.
	Matrix<std::complex<T> > W;
	if (destroyInput) W = I;           // alias to I's memory
	else              W.copyFrom (I);  // duplicate I's memory

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	typename traitsFFTW<T>::iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = W.rows () == 1 ? W.strideC : W.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = W.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = W.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) W.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | FFTW_DESTROY_INPUT;  // We have no choice but to destroy input.

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != 1  // reverse
          || cachedKind       != -2  // mixed complex-real
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		pthread_mutex_lock   (&mutexPlan);
		destroy_plan (cachedPlan);
		pthread_mutex_unlock (&mutexPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  pthread_mutex_lock   (&mutexPlan);
	  cachedPlan = plan
	  (
	    rank, dims,
		0, 0,
		(typename traitsFFTW<T>::complex *) Idata, (T *) Odata,
		flags
	  );
	  pthread_mutex_unlock (&mutexPlan);
	  cachedDirection = 1;   // reverse
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	execute (cachedPlan, (typename traitsFFTW<T>::complex *) Idata, (T *) Odata);
	if (normalize) O /= std::sqrt (rows * cols);
  }

  template<class T>
  void
  Fourier<T>::dft (int kind, const MatrixStrided<T> & I, MatrixStrided<T> & O)
  {
	int rows = I.rows ();
	int cols = I.columns ();
	if (sizeFromOutput  &&  O.rows ()  &&  O.columns ())
	{
	  rows = std::min (rows, O.rows ());
	  cols = std::min (cols, O.columns ());
	}
	if (O.rows () < rows  ||  O.columns () < cols) O.resize (rows, cols);

	const int rank = (rows == 1  ||  cols == 1) ? 1 : 2;
	typename traitsFFTW<T>::iodim dims[2];
	if (rank == 1)
	{
	  dims[0].n  = rows * cols;
	  dims[0].is = I.rows () == 1 ? I.strideC : I.strideR;
	  dims[0].os = O.rows () == 1 ? O.strideC : O.strideR;

	  dims[1].n  = 0;
	  dims[1].is = 0;
	  dims[1].os = 0;
	}
	else  // rank == 2
	{
	  dims[0].n  = cols;
	  dims[0].is = I.strideC;
	  dims[0].os = O.strideC;

	  dims[1].n  = rows;
	  dims[1].is = I.strideR;
	  dims[1].os = O.strideR;
	}

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != 0   // none
          || cachedKind       != kind
          || cachedFlags      != flags
          || cachedDims[0].n  != dims[0].n
          || cachedDims[0].is != dims[0].is
          || cachedDims[0].os != dims[0].os
          || cachedDims[1].n  != dims[1].n
          || cachedDims[1].is != dims[1].is
          || cachedDims[1].os != dims[1].os
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		pthread_mutex_lock   (&mutexPlan);
		destroy_plan (cachedPlan);
		pthread_mutex_unlock (&mutexPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  fftw_r2r_kind kinds[2];
	  kinds[0] = (fftw_r2r_kind) kind;
	  kinds[1] = (fftw_r2r_kind) kind;
	  pthread_mutex_lock   (&mutexPlan);
	  cachedPlan = plan
	  (
	    rank, dims,
		0, 0,
		(T *) Idata, (T *) Odata,
		kinds, flags
	  );
	  pthread_mutex_unlock (&mutexPlan);
	  cachedDirection = 0;   // none
	  cachedKind      = kind;
	  cachedFlags     = flags;
	  cachedDims[0]   = dims[0];
	  cachedDims[1]   = dims[1];
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	execute (cachedPlan, (T *) Idata, (T *) Odata);
	if (normalize)
	{
	  T N;
	  switch (kind)
	  {
		case FFTW_R2HC:
		case FFTW_HC2R:
		case FFTW_DHT:
		  N = rows * cols;
		  break;
		case FFTW_REDFT00:
		  N = 4 * (rows - 1) * (cols - 1);
		  break;
		case FFTW_RODFT00:
		  N = 4 * (rows + 1) * (cols + 1);
		  break;
		case FFTW_REDFT10:
		case FFTW_REDFT01:
		case FFTW_REDFT11:
		case FFTW_RODFT10:
		case FFTW_RODFT01:
		case FFTW_RODFT11:
		  N = 4 * rows * cols;
		  break;
	  }
	  O /= std::sqrt (N);
	}
  }
}


#endif
