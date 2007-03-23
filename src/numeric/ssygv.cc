/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.1 and 1.2 Copyright 2005 Sandia Corporation.
Revisions 1.4 and 1.5 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.5  2007/03/23 10:57:26  Fred
Use CVS Log to generate revision history.

Revision 1.4  2006/02/16 04:45:26  Fred
Change all "copy" options into "destroy".  These functions now have about the
same semantics as before the copy option was added, except now the programmer
can explicitly indicate that a parameter can be overwritten safely.

Revision 1.3  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.2  2005/10/13 04:07:51  Fred
Allow overwriting of input matrices.  Change interface to take MatrixAbstract.

Add Sandia copyright notice.

Revision 1.1  2005/09/10 17:40:49  Fred
Create templates for LAPACK bridge, rather than using type specific inlines. 
Break lapackd.h and lapacks.h into implementation files.
-------------------------------------------------------------------------------
*/


#include "fl/lapack.h"
#include "fl/lapackprotos.h"


namespace fl
{
  template<>
  void
  sygv (const MatrixAbstract<float> & A, const MatrixAbstract<float> & B, Matrix<float> & eigenvalues, Matrix<float> & eigenvectors, bool destroyA, bool destroyB)
  {
	const Matrix<float> * p;
	if (destroyA  &&  (p = dynamic_cast<const Matrix<float> *> (&A)))
	{
	  eigenvectors = *p;
	}
	else
	{
	  eigenvectors.copyFrom (A);
	}

	Matrix<float> tempB;
	if (destroyB  &&  (p = dynamic_cast<const Matrix<float> *> (&B)))
	{
	  tempB = *p;
	}
	else
	{
	  tempB.copyFrom (B);
	}

	int n = eigenvectors.rows ();  // or eigenvectors.columns (); they should be equal
	eigenvalues.resize (n);

	int lwork = -1;
	float optimalSize = 0;
	int info = 0;

	// Space query
	ssygv_ (1,
			'V',
			'U',
			n,
			& eigenvectors[0],
			n,
			& tempB[0],
			n,
			& eigenvalues[0],
			& optimalSize,
			lwork,
			info);

	lwork = (int) optimalSize;
	float * work = (float *) malloc (lwork * sizeof (float));

	// Actual computation, using optimal work space.
	ssygv_ (1,
			'V',
			'U',
			n,
			& eigenvectors[0],
			n,
			& tempB[0],
			n,
			& eigenvalues[0],
			work,
			lwork,
			info);

	free (work);

	if (info)
	{
	  throw info;
	}
  }
}
