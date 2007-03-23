/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3 and 1.4 Copyright 2005 Sandia Corporation.
Revisions 1.6 and 1.7 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 11:38:06  Fred
Correct which revisions are under Sandia copyright.

Revision 1.6  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.5  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/08 19:39:33  Fred
Update revision history and add Sandia copyright notice.

Add dgelsd_

Revision 1.3  2005/09/10 16:30:27  Fred
Add dsygv_

Revision 1.2  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_lapackproto_double_h
#define fl_lapackproto_double_h


extern "C"
{
  void dsyev_ (const char & jobz,
			   const char & uplo,
			   const int &  n,
			   double       a[],
			   const int &  lda,
			   double       w[],
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
}


#endif
