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
  inline int
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
  Fourier<T>::Fourier (bool destroyInput)
  : destroyInput (destroyInput)
  {
	cachedPlan = 0;
  }

  template<class T>
  Fourier<T>::~Fourier ()
  {
	if (cachedPlan) fftw_destroy_plan (cachedPlan);
  }

  template<class T>
  void
  Fourier<T>::dft (int direction, const Matrix<std::complex<T> > & I, Matrix<std::complex<T> > & O)
  {
	const int Irows = I.rows ();
	const int Icols = I.columns ();

	O.resize (Irows, Icols);  // always forces dense structure

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
          || cachedInRows     != Irows
          || cachedInColumns  != Icols
          || cachedInStrideR  != I.strideR
          || cachedInStrideC  != I.strideC
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  const int rank = (Irows == 1  ||  Icols == 1) ? 1 : 2;
	  fftw_iodim dims[2];
	  if (rank == 1)
	  {
		dims[0].n = std::max (Irows, Icols);
		dims[0].is = 1;
		dims[0].os = 1;
	  }
	  else
	  {
		dims[0].n  = Icols;
		dims[0].is = I.strideC;
		dims[0].os = O.strideC;

		dims[1].n  = Irows;
		dims[1].is = I.strideR;
		dims[1].os = O.strideR;
	  }
	  cachedPlan = fftw_plan_guru_dft
	  (
	    rank, dims,
		0, 0,
		(fftw_complex *) Idata, (fftw_complex *) Odata,
		direction, flags
	  );
	  cachedDirection = direction;
	  cachedKind      = -1;  // complex to complex
	  cachedFlags     = flags;
	  cachedInRows    = Irows;
	  cachedInColumns = Icols;
	  cachedInStrideR = I.strideR;
	  cachedInStrideC = I.strideC;
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft (cachedPlan, (fftw_complex *) Idata, (fftw_complex *) Odata);
  }

  template<class T>
  void
  Fourier<T>::dft (const Matrix<T> & I, Matrix<std::complex<T> > & O)
  {
	const int Irows = I.rows ();
	const int Icols = I.columns ();
	const int Orows = Irows / 2 + 1;
	const int Ocols = Icols;

	O.resize (Orows, Ocols);  // always forces dense structure

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
          || cachedInRows     != Irows
          || cachedInColumns  != Icols
          || cachedInStrideR  != I.strideR
          || cachedInStrideC  != I.strideC
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  const int rank = (Irows == 1  ||  Icols == 1) ? 1 : 2;
	  fftw_iodim dims[2];
	  if (rank == 1)
	  {
		dims[0].n = std::max (Irows, Icols);
		dims[0].is = 1;
		dims[0].os = 1;
	  }
	  else
	  {
		dims[0].n  = Icols;
		dims[0].is = I.strideC;
		dims[0].os = O.strideC;

		dims[1].n  = Irows;
		dims[1].is = I.strideR;
		dims[1].os = O.strideR;
	  }
	  cachedPlan = fftw_plan_guru_dft_r2c
	  (
	    rank, dims,
		0, 0,
		(double *) Idata, (fftw_complex *) Odata,
		flags
	  );
	  cachedDirection = -1;  // forward
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedInRows    = Irows;
	  cachedInColumns = Icols;
	  cachedInStrideR = I.strideR;
	  cachedInStrideC = I.strideC;
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft_r2c (cachedPlan, (double *) Idata, (fftw_complex *) Odata);
  }

  template<class T>
  void
  Fourier<T>::dft (const Matrix<std::complex<T> > & I, Matrix<T> & O)
  {
	const int Irows = I.rows ();
	const int Icols = I.columns ();
	int       Orows = (Irows - 1) * 2;  // If the original input size was odd, we lose the last row.  As a hint, O can be passed in with the original size.
	const int Ocols = Icols;

	if (O.rows () > Orows) Orows++;  // Assume that size was odd, and add back extra row that was lost in above calculation.
	O.resize (Orows, Ocols);  // always forces dense structure

	const char * Idata = (char *) I.data;
	const char * Odata = (char *) O.data;
	const bool inplace = Idata == Odata;
	const int alignment = std::min (trailingZeros ((uint32_t) (ptrdiff_t) Idata), trailingZeros ((uint32_t) (ptrdiff_t) Odata));  // OK even under 64-bit, as we only care about the first few bit positions.

	const unsigned int flags = FFTW_ESTIMATE | (destroyInput ? FFTW_DESTROY_INPUT : FFTW_PRESERVE_INPUT);

	if (cachedPlan)
	{
	  // Check if plan matches current problem
	  if (   cachedDirection  != 1  // reverse
          || cachedKind       != -2  // mixed complex-real
          || cachedFlags      != flags
          || cachedInRows     != Irows
          || cachedInColumns  != Icols
          || cachedInStrideR  != I.strideR
          || cachedInStrideC  != I.strideC
          || cachedInPlace    != inplace
          || cachedAlignment  >  alignment)
	  {
		fftw_destroy_plan (cachedPlan);
		cachedPlan = 0;
	  }
	}
	if (! cachedPlan)
	{
	  // Create new plan
	  const int rank = (Irows == 1  ||  Icols == 1) ? 1 : 2;
	  fftw_iodim dims[2];
	  if (rank == 1)
	  {
		dims[0].n = std::max (Irows, Icols);
		dims[0].is = 1;
		dims[0].os = 1;
	  }
	  else
	  {
		dims[0].n  = Ocols;
		dims[0].is = I.strideC;
		dims[0].os = O.strideC;

		dims[1].n  = Orows;
		dims[1].is = I.strideR;
		dims[1].os = O.strideR;
	  }
	  cachedPlan = fftw_plan_guru_dft_c2r
	  (
	    rank, dims,
		0, 0,
		(fftw_complex *) Idata, (double *) Odata,
		flags
	  );
	  cachedDirection = 1;   // reverse
	  cachedKind      = -2;  // mixed complex-real
	  cachedFlags     = flags;
	  cachedInRows    = Irows;
	  cachedInColumns = Icols;
	  cachedInStrideR = I.strideR;
	  cachedInStrideC = I.strideC;
	  cachedInPlace   = inplace;
	  cachedAlignment = alignment;
	}
	if (! cachedPlan) throw "Fourier: Unable to generate a plan.";

	// Run it
	fftw_execute_dft_c2r (cachedPlan, (fftw_complex *) Idata, (double *) Odata);
  }
}


#endif
