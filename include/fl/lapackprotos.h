/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_lapackproto_single_h
#define fl_lapackproto_single_h


extern "C"
{
  void ssyev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   float        a[],
			   const int &  lda,
			   float        w[],
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
}


#endif
