/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision 1.1 and 1.2 Copyright 2005 Sandia Corporation.
Revision 1.4 and 1.5 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 10:57:30  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.3  2005/10/13 04:14:26  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/10/13 03:47:38  Fred
Allow overwirting of input matrix.  Directly pass job types rather than
assigning to temporary variables.

Add Sandia copyright notice.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool destroyA)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<double> tempA;
	const Matrix<double> * pA;
	if (destroyA  &&  (pA = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  tempA = *pA;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
	int info = 0;

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			lda,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			n,
			work,
			lwork,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, bool destroyA)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<double> tempA;
	const Matrix<double> * pA;
	if (destroyA  &&  (pA = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  tempA = *pA;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
	int info = 0;

	dgeev_ ('N',
			'N',
			n,
			& tempA[0],
			lda,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			0,
			1,  // ldvr >= 1.  ditto
			work,
			lwork,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<std::complex<double> > & eigenvalues, Matrix<double> & eigenvectors, bool destroyA)
  {
	int lda = A.rows ();
	int n = std::min (lda, A.columns ());

	Matrix<double> tempA;
	const Matrix<double> * pA;
	if (destroyA  &&  (pA = dynamic_cast<const Matrix<double> *> (&A)))
	{
	  tempA = *pA;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wr (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = 5 * n;
	double * work = (double *) malloc (lwork * sizeof (double));
	int info = 0;

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			lda,
			& wr[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			n,
			work,
			lwork,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}

	for (int i = 0; i < n; i++)
	{
	  eigenvalues[i] = std::complex<double> (wr[i], wi[i]);
	}
  }
}
