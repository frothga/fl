/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.5 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/08 19:40:02  Fred
Update revision history and add Sandia copyright notice.

Add sgelsd_

Revision 1.4  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.3  2004/04/19 16:37:17  rothgang
Add symmetric generalized eigenvalue problems.

Revision 1.2  2003/08/09 21:54:10  rothgang
Bring up to date with lapackprotod.h

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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
