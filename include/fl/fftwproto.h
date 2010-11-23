/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_fftwproto_h
#define fl_fftwproto_h


#include <fftw3.h>


namespace fl
{
  template<class T>
  struct traitsFFTW
  {
	typedef fftw_plan     plan;
	typedef fftw_iodim    iodim;
	typedef fftw_r2r_kind kind;
	typedef fftw_complex  complex;
  };

  template<>
  struct traitsFFTW<float>
  {
	typedef fftwf_plan     plan;
	typedef fftwf_iodim    iodim;
	typedef fftwf_r2r_kind kind;
	typedef fftwf_complex  complex;
  };

  template<>
  struct traitsFFTW<long double>
  {
	typedef fftwl_plan     plan;
	typedef fftwl_iodim    iodim;
	typedef fftwl_r2r_kind kind;
	typedef fftwl_complex  complex;
  };

  // The FFTW interface is presented in C, which lacks function overloading.
  // However, we need function overloading to make the Fourier template work.
  // Therefore, we wrap each distinct function we call in a C++ style
  // overload.

  static inline traitsFFTW<double>::plan
  plan (int rank,
		const traitsFFTW<double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<double>::iodim * howmany_dims,
		traitsFFTW<double>::complex * in,
		traitsFFTW<double>::complex * out,
		int sign,
		unsigned flags)
  {
	return fftw_plan_guru_dft (rank, dims, howmany_rank, howmany_dims, in, out, sign, flags);
  }

  static inline traitsFFTW<float>::plan
  plan (int rank,
		const traitsFFTW<float>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<float>::iodim * howmany_dims,
		traitsFFTW<float>::complex * in,
		traitsFFTW<float>::complex * out,
		int sign,
		unsigned flags)
  {
	return fftwf_plan_guru_dft (rank, dims, howmany_rank, howmany_dims, in, out, sign, flags);
  }

  static inline traitsFFTW<long double>::plan
  plan (int rank,
		const traitsFFTW<long double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<long double>::iodim * howmany_dims,
		traitsFFTW<long double>::complex * in,
		traitsFFTW<long double>::complex * out,
		int sign,
		unsigned flags)
  {
	return fftwl_plan_guru_dft (rank, dims, howmany_rank, howmany_dims, in, out, sign, flags);
  }

  static inline traitsFFTW<double>::plan
  plan (int rank,
		const traitsFFTW<double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<double>::iodim * howmany_dims,
		double * in,
		traitsFFTW<double>::complex * out,
		unsigned flags)
  {
	return fftw_plan_guru_dft_r2c (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<float>::plan
  plan (int rank,
		const traitsFFTW<float>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<float>::iodim * howmany_dims,
		float * in,
		traitsFFTW<float>::complex * out,
		unsigned flags)
  {
	return fftwf_plan_guru_dft_r2c (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<long double>::plan
  plan (int rank,
		const traitsFFTW<long double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<long double>::iodim * howmany_dims,
		long double * in,
		traitsFFTW<long double>::complex * out,
		unsigned flags)
  {
	return fftwl_plan_guru_dft_r2c (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<double>::plan
  plan (int rank,
		const traitsFFTW<double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<double>::iodim * howmany_dims,
		traitsFFTW<double>::complex * in,
		double * out,
		unsigned flags)
  {
	return fftw_plan_guru_dft_c2r (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<float>::plan
  plan (int rank,
		const traitsFFTW<float>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<float>::iodim * howmany_dims,
		traitsFFTW<float>::complex * in,
		float * out,
		unsigned flags)
  {
	return fftwf_plan_guru_dft_c2r (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<long double>::plan
  plan (int rank,
		const traitsFFTW<long double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<long double>::iodim * howmany_dims,
		traitsFFTW<long double>::complex * in,
		long double * out,
		unsigned flags)
  {
	return fftwl_plan_guru_dft_c2r (rank, dims, howmany_rank, howmany_dims, in, out, flags);
  }

  static inline traitsFFTW<double>::plan
  plan (int rank,
		const traitsFFTW<double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<double>::iodim * howmany_dims,
		double * in,
		double * out,
		const traitsFFTW<double>::kind * kind,
		unsigned flags)
  {
	return fftw_plan_guru_r2r (rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
  }

  static inline traitsFFTW<float>::plan
  plan (int rank,
		const traitsFFTW<float>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<float>::iodim * howmany_dims,
		float * in,
		float * out,
		const traitsFFTW<float>::kind * kind,
		unsigned flags)
  {
	return fftwf_plan_guru_r2r (rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
  }

  static inline traitsFFTW<long double>::plan
  plan (int rank,
		const traitsFFTW<long double>::iodim * dims,
		int howmany_rank,
		const traitsFFTW<long double>::iodim * howmany_dims,
		long double * in,
		long double * out,
		const traitsFFTW<long double>::kind * kind,
		unsigned flags)
  {
	return fftwl_plan_guru_r2r (rank, dims, howmany_rank, howmany_dims, in, out, kind, flags);
  }

  static inline void
  execute (const traitsFFTW<double>::plan plan,
		   traitsFFTW<double>::complex * in,
		   traitsFFTW<double>::complex * out)
  {
	fftw_execute_dft (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<float>::plan plan,
		   traitsFFTW<float>::complex * in,
		   traitsFFTW<float>::complex * out)
  {
	fftwf_execute_dft (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<long double>::plan plan,
		   traitsFFTW<long double>::complex * in,
		   traitsFFTW<long double>::complex * out)
  {
	fftwl_execute_dft (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<double>::plan plan,
		   double * in,
		   traitsFFTW<double>::complex * out)
  {
	fftw_execute_dft_r2c (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<float>::plan plan,
		   float * in,
		   traitsFFTW<float>::complex * out)
  {
	fftwf_execute_dft_r2c (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<long double>::plan plan,
		   long double * in,
		   traitsFFTW<long double>::complex * out)
  {
	fftwl_execute_dft_r2c (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<double>::plan plan,
		   traitsFFTW<double>::complex * in,
		   double * out)
  {
	fftw_execute_dft_c2r (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<float>::plan plan,
		   traitsFFTW<float>::complex * in,
		   float * out)
  {
	fftwf_execute_dft_c2r (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<long double>::plan plan,
		   traitsFFTW<long double>::complex * in,
		   long double * out)
  {
	fftwl_execute_dft_c2r (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<double>::plan plan,
		   double * in,
		   double * out)
  {
	fftw_execute_r2r (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<float>::plan plan,
		   float * in,
		   float * out)
  {
	fftwf_execute_r2r (plan, in, out);
  }

  static inline void
  execute (const traitsFFTW<long double>::plan plan,
		   long double * in,
		   long double * out)
  {
	fftwl_execute_r2r (plan, in, out);
  }

  static inline void
  destroy_plan (traitsFFTW<double>::plan plan)
  {
	fftw_destroy_plan (plan);
  }

  static inline void
  destroy_plan (traitsFFTW<float>::plan plan)
  {
	fftwf_destroy_plan (plan);
  }

  static inline void
  destroy_plan (traitsFFTW<long double>::plan plan)
  {
	fftwl_destroy_plan (plan);
  }
}


#endif
