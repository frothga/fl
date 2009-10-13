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
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  geev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<float> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<float> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<float> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	float optimalLwork;
	int info = 0;

	sgeev_ ('N',
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
	float * work = (float *) malloc (lwork * sizeof (float));

	sgeev_ ('N',
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
  geev (const MatrixAbstract<float> & A, Matrix<float> & eigenvalues, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<float> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<float> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<float> wi (n);

	int lwork = -1;
	float optimalLwork;
	int info = 0;

	sgeev_ ('N',
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
	float * work = (float *) malloc (lwork * sizeof (float));

	sgeev_ ('N',
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
  geev (const MatrixAbstract<float> & A, Matrix<std::complex<float> > & eigenvalues, Matrix<float> & eigenvectors, bool destroyA)
  {
	int n = std::min (A.rows (), A.columns ());

	Matrix<float> tempA;
	if (destroyA  &&  (A.classID () & MatrixID))
	{
	  tempA = (const Matrix<float> &) A;
	}
	else
	{
	  tempA.copyFrom (A);
	}

	eigenvalues.resize (n);
	Matrix<float> wr (n);
	Matrix<float> wi (n);

	eigenvectors.resize (n, n);

	int lwork = -1;
	float optimalLwork;
	int info = 0;

	sgeev_ ('N',
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
	float * work = (float *) malloc (lwork * sizeof (float));

	sgeev_ ('N',
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
	  eigenvalues[i] = std::complex<float> (wr[i], wi[i]);
	}
  }
}
