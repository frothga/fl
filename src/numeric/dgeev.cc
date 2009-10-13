/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/lapack.h"
#include "fl/lapackprotod.h"


namespace fl
{
  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, Matrix<double> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<double> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<double> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	double optimalLwork;
	int info = 0;

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			tempA.strideC,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			eigenvectors.strideC,
			&optimalLwork,
			lwork,
			info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	double * work = (double *) malloc (lwork * sizeof (double));

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			tempA.strideC,
			& eigenvalues[0],
			& wi[0],
			0,
			1,
			& eigenvectors[0],
			eigenvectors.strideC,
			work,
			lwork,
			info);

	free (work);

	if (info) throw info;
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<double> & eigenvalues, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<double> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<double> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wi (n);

	int lwork = -1;
	double optimalLwork;
	int info = 0;

	dgeev_ ('N',
			'N',
			n,
			& tempA[0],
			tempA.strideC,
			& eigenvalues[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			0,
			1,  // ditto
			&optimalLwork,
			lwork,
			info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	double * work = (double *) malloc (lwork * sizeof (double));

	dgeev_ ('N',
			'N',
			n,
			& tempA[0],
			tempA.strideC,
			& eigenvalues[0],
			& wi[0],
			0,
			1,
			0,
			1,
			work,
			lwork,
			info);

	free (work);

	if (info) throw info;
  }

  template<>
  void
  geev (const MatrixAbstract<double> & A, Matrix<std::complex<double> > & eigenvalues, Matrix<double> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<double> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<double> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<double> wr (n);
	Matrix<double> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	double optimalLwork;
	int info = 0;

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			tempA.strideC,
			& wr[0],
			& wi[0],
			0,
			1,  // ldvl >= 1.  A lie to make lapack happy.
			& eigenvectors[0],
			eigenvectors.strideC,
			&optimalLwork,
			lwork,
			info);

	if (info) throw info;
	lwork = (int) optimalLwork;
	double * work = (double *) malloc (lwork * sizeof (double));

	dgeev_ ('N',
			'V',
			n,
			& tempA[0],
			tempA.strideC,
			& wr[0],
			& wi[0],
			0,
			1,
			& eigenvectors[0],
			eigenvectors.strideC,
			work,
			lwork,
			info);

	free (work);

	if (info) throw info;

	for (int i = 0; i < n; i++)
	{
	  eigenvalues[i] = std::complex<double> (wr[i], wi[i]);
	}
  }
}
