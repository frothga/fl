/*
Author: Fred Rothganger
Created 10/5/2005


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_lapackproto_h
#define fl_lapackproto_h

#ifdef HAVE_LAPACK

extern "C"
{
  void dgeev_ (const char & jobvl,
			   const char & jobvr,
			   const int &  n,
			   double       a[],
			   const int &  lda,
			   double       wr[],
			   double       wi[],
			   double       vl[],
			   const int &  ldvl,
			   double       vr[],
			   const int &  ldvr,
			   double       work[],
			   const int &  lwork,
			   int &        info);

  void dgelsd_ (const int &    m,
				const int &    n,
				const int &    nrhs,
				double         a[],
				const int &    lda,
				double         b[],
				const int &    ldb,
				double         s[],
				const double & rcond,
				int &          rank,
                double         work[],
				const int &    lwork,
				int            iwork[],
				int &          info);

  void dgelss_ (const int &    m,
				const int &    n,
				const int &    nrhs,
				double         a[],
				const int &    lda,
				double         b[],
				const int &    ldb,
				double         s[],
				const double & rcond,
				int &          rank,
                double         work[],
				const int &    lwork,
				int &          info);

  void dgeqp3_ (const int & m,
				const int & n,
				double      a[],
				const int & lda,
				int         jpvt[],
				double      tau[],
				double      work[],
				const int & lwork,
				int &       info);

  void dgesvd_ (const char & jobu,
				const char & jobvt,
				const int &  m,
				const int &  n,
				double       a[],
				const int &  lda,
				double       s[],
				double       u[],
				const int &  ldu,
				double       vt[],
				const int &  ldvt,
				double       work[],
				const int &  lwork,
				int &        info);

  void dgetrf_ (const int & m,
				const int & n,
				double      a[],
				const int & lda,
				int         ipiv[],
				int &       info);

  void dgetri_ (const int & n,
				double      a[],
				const int & lda,
				int         ipiv[],
				double      work[],
				const int & lwork,
				int &       info);

  void dgetrs_ (const char & trans,
				const int &  n,
				const int &  nrhs,
				const double a[],
				const int &  lda,
				const int    ipiv[],
				double       b[],
				const int &  ldb,
				int &        info);

  void dormqr_ (const char & side,
				const char & trans,
				const int &  m,
				const int &  n,
				const int &  k,
				double       a[],
				const int &  lda,
				double       tau[],
				double       c[],
				const int &  ldc,
				double       work[],
				const int &  lwork,
				int &        info);

  void dspev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   double       ap[],
			   double       w[],
			   double       z[],
			   const int &  ldz,
			   double       work[],
			   int &        info);

  void dsyev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   double       a[],
			   const int &  lda,
			   double       w[],
			   double       work[],
			   const int &  lwork,
			   int &        info);

  void dsygv_ (const int &  itype,
			   const char & jobz,
			   const char & uplo,
			   const int &  n,
			   double       a[],
			   const int &  lda,
			   double       b[],
			   const int &  ldb,
			   double       w[],
			   double       work[],
			   const int &  lwork,
			   int &        info);

  void dsytrf_ (const char & uplo,
				const int &  n,
				double       a[],
				const int &  lda,
				int          ipiv[],
				double       work[],
				const int &  lwork,
				int &        info);

  void dsytri_ (const char & uplo,
				const int &  n,
				double       a[],
				const int &  lda,
				int          ipiv[],
				double       work[],
				int &        info);

  void dsytrs_ (const char & uplo,
				const int &  n,
				const int &  nrhs,
				const double a[],
				const int &  lda,
				const int    ipiv[],
				double       b[],
				const int &  ldb,
				int &        info);

  /// @note This approach to specifying string sizes assumes LAPACK is compiled by g77
  int ilaenv_ (const int &  ispec,
			   const char * name,
			   const char * opts,
			   const int &  n1,
			   const int &  n2,
			   const int &  n3,
			   const int &  n4,
			   const int    length_name,
			   const int    length_opts);

  void sgeev_ (const char & jobvl,
			   const char & jobvr,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        wr[],
			   float        wi[],
			   float        vl[],
			   const int &  ldvl,
			   float        vr[],
			   const int &  ldvr,
			   float        work[],
			   const int &  lwork,
			   int &        info);

  void sgelsd_ (const int &   m,
				const int &   n,
				const int &   nrhs,
				float         a[],
				const int &   lda,
				float         b[],
				const int &   ldb,
				float         s[],
				const float & rcond,
				int &         rank,
                float         work[],
				const int &   lwork,
				int           iwork[],
				int &         info);

  void sgelss_ (const int &   m,
				const int &   n,
				const int &   nrhs,
				float         a[],
				const int &   lda,
				float         b[],
				const int &   ldb,
				float         s[],
				const float & rcond,
				int &         rank,
                float         work[],
				const int &   lwork,
				int &         info);

  void sgeqp3_ (const int & m,
				const int & n,
				float       a[],
				const int & lda,
				int         jpvt[],
				float       tau[],
				float       work[],
				const int & lwork,
				int &       info);

  void sgesvd_ (const char & jobu,
				const char & jobvt,
				const int &  m,
				const int &  n,
				float        a[],
				const int &  lda,
				float        s[],
				float        u[],
				const int &  ldu,
				float        vt[],
				const int &  ldvt,
				float        work[],
				const int &  lwork,
				int &        info);

  void sgetrf_ (const int & m,
				const int & n,
				float       a[],
				const int & lda,
				int         ipiv[],
				int &       info);

  void sgetri_ (const int & n,
				float       a[],
				const int & lda,
				int         ipiv[],
				float       work[],
				const int & lwork,
				int &       info);

  void sgetrs_ (const char & trans,
				const int &  n,
				const int &  nrhs,
				const float  a[],
				const int &  lda,
				const int    ipiv[],
				float        b[],
				const int &  ldb,
				int &        info);

  void sormqr_ (const char & side,
				const char & trans,
				const int &  m,
				const int &  n,
				const int &  k,
				float        a[],
				const int &  lda,
				float        tau[],
				float        c[],
				const int &  ldc,
				float        work[],
				const int &  lwork,
				int &        info);

  void sspev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   float        ap[],
			   float        w[],
			   float        z[],
			   const int &  ldz,
			   float        work[],
			   int &        info);

  void ssyev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        w[],
			   float        work[],
			   const int &  lwork,
			   int &        info);

  void ssygv_ (const int &  itype,
			   const char & jobz,
			   const char & uplo,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        b[],
			   const int &  ldb,
			   float        w[],
			   float        work[],
			   const int &  lwork,
			   int &        info);

  void ssytrf_ (const char & uplo,
				const int &  n,
				float        a[],
				const int &  lda,
				int          ipiv[],
				float        work[],
				const int &  lwork,
				int &        info);

  void ssytri_ (const char & uplo,
				const int &  n,
				float        a[],
				const int &  lda,
				int          ipiv[],
				float        work[],
				int &        info);

  void ssytrs_ (const char & uplo,
				const int &  n,
				const int &  nrhs,
				const float  a[],
				const int &  lda,
				const int    ipiv[],
				float        b[],
				const int &  ldb,
				int &        info);
}

namespace fl
{
  inline void
  geev (const char & jobvl,
		const char & jobvr,
		const int &  n,
		double       a[],
		const int &  lda,
		double       wr[],
		double       wi[],
		double       vl[],
		const int &  ldvl,
		double       vr[],
		const int &  ldvr,
		double       work[],
		const int &  lwork,
		int &        info)
  {
	dgeev_ (jobvl, jobvr, n, a, lda, wr, wi, vl, ldvl, vr, ldvr, work, lwork, info);
  }

  inline void
  geev (const char & jobvl,
		const char & jobvr,
		const int &  n,
		float        a[],
		const int &  lda,
		float        wr[],
		float        wi[],
		float        vl[],
		const int &  ldvl,
		float        vr[],
		const int &  ldvr,
		float        work[],
		const int &  lwork,
		int &        info)
  {
	sgeev_ (jobvl, jobvr, n, a, lda, wr, wi, vl, ldvl, vr, ldvr, work, lwork, info);
  }

  inline void
  gelsd (const int &    m,
		 const int &    n,
		 const int &    nrhs,
		 double         a[],
		 const int &    lda,
		 double         b[],
		 const int &    ldb,
		 double         s[],
		 const double & rcond,
		 int &          rank,
		 double         work[],
		 const int &    lwork,
		 int            iwork[],
		 int &          info)
  {
	dgelsd_ (m, n, nrhs, a, lda, b, ldb, s, rcond, rank, work, lwork, iwork, info);
  }

  inline void
  gelsd (const int &   m,
		 const int &   n,
		 const int &   nrhs,
		 float         a[],
		 const int &   lda,
		 float         b[],
		 const int &   ldb,
		 float         s[],
		 const float & rcond,
		 int &         rank,
		 float         work[],
		 const int &   lwork,
		 int           iwork[],
		 int &         info)
  {
	sgelsd_ (m, n, nrhs, a, lda, b, ldb, s, rcond, rank, work, lwork, iwork, info);
  }

  inline void
  gelss (const int &    m,
		 const int &    n,
		 const int &    nrhs,
		 double         a[],
		 const int &    lda,
		 double         b[],
		 const int &    ldb,
		 double         s[],
		 const double & rcond,
		 int &          rank,
		 double         work[],
		 const int &    lwork,
		 int &          info)
  {
	dgelss_ (m, n, nrhs, a, lda, b, ldb, s, rcond, rank, work, lwork, info);
  }

  inline void
  gelss (const int &   m,
		 const int &   n,
		 const int &   nrhs,
		 float         a[],
		 const int &   lda,
		 float         b[],
		 const int &   ldb,
		 float         s[],
		 const float & rcond,
		 int &         rank,
		 float         work[],
		 const int &   lwork,
		 int &         info)
  {
	sgelss_ (m, n, nrhs, a, lda, b, ldb, s, rcond, rank, work, lwork, info);
  }

  inline void
  geqp3 (const int & m,
		 const int & n,
		 double      a[],
		 const int & lda,
		 int         jpvt[],
		 double      tau[],
		 double      work[],
		 const int & lwork,
		 int &       info)
  {
	dgeqp3_ (m, n, a, lda, jpvt, tau, work, lwork, info);
  }

  inline void
  geqp3 (const int & m,
		 const int & n,
		 float       a[],
		 const int & lda,
		 int         jpvt[],
		 float       tau[],
		 float       work[],
		 const int & lwork,
		 int &       info)
  {
	sgeqp3_ (m, n, a, lda, jpvt, tau, work, lwork, info);
  }

  inline void
  gesvd (const char & jobu,
		 const char & jobvt,
		 const int &  m,
		 const int &  n,
		 double       a[],
		 const int &  lda,
		 double       s[],
		 double       u[],
		 const int &  ldu,
		 double       vt[],
		 const int &  ldvt,
		 double       work[],
		 const int &  lwork,
		 int &        info)
  {
	dgesvd_ (jobu, jobvt, m, n, a, lda, s, u, ldu, vt, ldvt, work, lwork, info);
  }

  inline void
  gesvd (const char & jobu,
		 const char & jobvt,
		 const int &  m,
		 const int &  n,
		 float        a[],
		 const int &  lda,
		 float        s[],
		 float        u[],
		 const int &  ldu,
		 float        vt[],
		 const int &  ldvt,
		 float        work[],
		 const int &  lwork,
		 int &        info)
  {
	sgesvd_ (jobu, jobvt, m, n, a, lda, s, u, ldu, vt, ldvt, work, lwork, info);
  }

  inline void
  getrf (const int & m,
		 const int & n,
		 double      a[],
		 const int & lda,
		 int         ipiv[],
		 int &       info)
  {
	dgetrf_ (m, n, a, lda, ipiv, info);
  }

  inline void
  getrf (const int & m,
		 const int & n,
		 float       a[],
		 const int & lda,
		 int         ipiv[],
		 int &       info)
  {
	sgetrf_ (m, n, a, lda, ipiv, info);
  }

  inline void
  getri (const int & n,
		 double      a[],
		 const int & lda,
		 int         ipiv[],
		 double      work[],
		 const int & lwork,
		 int &       info)
  {
	dgetri_ (n, a, lda, ipiv, work, lwork, info);
  }

  inline void
  getri (const int & n,
		 float       a[],
		 const int & lda,
		 int         ipiv[],
		 float       work[],
		 const int & lwork,
		 int &       info)
  {
	sgetri_ (n, a, lda, ipiv, work, lwork, info);
  }

  inline void
  getrs (const char & trans,
		 const int &  n,
		 const int &  nrhs,
		 const double a[],
		 const int &  lda,
		 const int    ipiv[],
		 double       b[],
		 const int &  ldb,
		 int &        info)
  {
	dgetrs_ (trans, n, nrhs, a, lda, ipiv, b, ldb, info);
  }

  inline void
  getrs (const char & trans,
		 const int &  n,
		 const int &  nrhs,
		 const float  a[],
		 const int &  lda,
		 const int    ipiv[],
		 float        b[],
		 const int &  ldb,
		 int &        info)
  {
	sgetrs_ (trans, n, nrhs, a, lda, ipiv, b, ldb, info);
  }

  inline int
  ilaenv (const int    ispec,
		  const char * name,
		  const char * opts,
		  const int    n1 = -1,
		  const int    n2 = -1,
		  const int    n3 = -1,
		  const int    n4 = -1)
  {
	return ilaenv_ (ispec, name, opts, n1, n2, n3, n4, strlen (name), strlen (opts));
  }

  inline void
  ormqr (const char & side,
		 const char & trans,
		 const int &  m,
		 const int &  n,
		 const int &  k,
		 double       a[],
		 const int &  lda,
		 double       tau[],
		 double       c[],
		 const int &  ldc,
		 double       work[],
		 const int &  lwork,
		 int &        info)
  {
	dormqr_ (side, trans, m, n, k, a, lda, tau, c, ldc, work, lwork, info);
  }

  inline void
  ormqr (const char & side,
		 const char & trans,
		 const int &  m,
		 const int &  n,
		 const int &  k,
		 float        a[],
		 const int &  lda,
		 float        tau[],
		 float        c[],
		 const int &  ldc,
		 float        work[],
		 const int &  lwork,
		 int &        info)
  {
	sormqr_ (side, trans, m, n, k, a, lda, tau, c, ldc, work, lwork, info);
  }

  inline void
  spev (const char & jobz,
		const char & uplo,
		const int &  n,
		double       ap[],
		double       w[],
		double       z[],
		const int &  ldz,
		double       work[],
		int &        info)
  {
	dspev_ (jobz, uplo, n, ap, w, z, ldz, work, info);
  }

  inline void
  spev (const char & jobz,
		const char & uplo,
		const int &  n,
		float        ap[],
		float        w[],
		float        z[],
		const int &  ldz,
		float        work[],
		int &        info)
  {
	sspev_ (jobz, uplo, n, ap, w, z, ldz, work, info);
  }

  inline void
  syev (const char & jobz,
		const char & uplo,
		const int &  n,
		double       a[],
		const int &  lda,
		double       w[],
		double       work[],
		const int &  lwork,
		int &        info)
  {
	dsyev_ (jobz, uplo, n, a, lda, w, work, lwork, info);
  }

  inline void
  syev (const char & jobz,
		const char & uplo,
		const int &  n,
		float        a[],
		const int &  lda,
		float        w[],
		float        work[],
		const int &  lwork,
		int &        info)
  {
	ssyev_ (jobz, uplo, n, a, lda, w, work, lwork, info);
  }

  inline void
  sygv (const int &  itype,
		const char & jobz,
		const char & uplo,
		const int &  n,
		double       a[],
		const int &  lda,
		double       b[],
		const int &  ldb,
		double       w[],
		double       work[],
		const int &  lwork,
		int &        info)
  {
	dsygv_ (itype, jobz, uplo, n, a, lda, b, ldb, w, work, lwork, info);
  }

  inline void
  sygv (const int &  itype,
		const char & jobz,
		const char & uplo,
		const int &  n,
		float        a[],
		const int &  lda,
		float        b[],
		const int &  ldb,
		float        w[],
		float        work[],
		const int &  lwork,
		int &        info)
  {
	ssygv_ (itype, jobz, uplo, n, a, lda, b, ldb, w, work, lwork, info);
  }

  inline void
  sytrf (const char & uplo,
		 const int &  n,
		 double       a[],
		 const int &  lda,
		 int          ipiv[],
		 double       work[],
		 const int &  lwork,
		 int &        info)
  {
	dsytrf_ (uplo, n, a, lda, ipiv, work, lwork, info);
  }

  inline void
  sytrf (const char & uplo,
		 const int &  n,
		 float        a[],
		 const int &  lda,
		 int          ipiv[],
		 float        work[],
		 const int &  lwork,
		 int &        info)
  {
	ssytrf_ (uplo, n, a, lda, ipiv, work, lwork, info);
  }

  inline void
  sytri (const char & uplo,
		 const int &  n,
		 double       a[],
		 const int &  lda,
		 int          ipiv[],
		 double       work[],
		 int &        info)
  {
	dsytri_ (uplo, n, a, lda, ipiv, work, info);
  }

  inline void
  sytri (const char & uplo,
		 const int &  n,
		 float        a[],
		 const int &  lda,
		 int          ipiv[],
		 float        work[],
		 int &        info)
  {
	ssytri_ (uplo, n, a, lda, ipiv, work, info);
  }

  inline void
  sytrs (const char & uplo,
		 const int &  n,
		 const int &  nrhs,
		 const double a[],
		 const int &  lda,
		 const int    ipiv[],
		 double       b[],
		 const int &  ldb,
		 int &        info)
  {
	dsytrs_ (uplo, n, nrhs, a, lda, ipiv, b, ldb, info);
  }

  inline void
  sytrs (const char & uplo,
		 const int &  n,
		 const int &  nrhs,
		 const float  a[],
		 const int &  lda,
		 const int    ipiv[],
		 float        b[],
		 const int &  ldb,
		 int &        info)
  {
	ssytrs_ (uplo, n, nrhs, a, lda, ipiv, b, ldb, info);
  }
}

#endif  // HAVE_LAPACK

#endif
