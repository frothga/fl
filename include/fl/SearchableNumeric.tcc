/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 thru 1.8 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.8  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 03:41:12  Fred
Move the file name LICENSE up to previous line, for better symmetry with UIUC
notice.

Revision 1.6  2005/10/08 18:41:11  Fred
Update revision history and add Sandia copyright notice.

Revision 1.5  2005/08/07 03:10:08  Fred
GCC 3.4 compilability fix: explicitly specify "this" for inherited member
variables in a template.

Revision 1.4  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 20:41:09  Fred
MSVC compilability fix: Get rid of explicit template specializations to avoid
multiple definitions.

Revision 1.2  2005/01/12 04:59:59  rothgang
Use std versions of pow, sqrt, and fabs so that choice of type specific version
will be automatic when template is instantiated.  IE: std contains type
overloaded versions of the functions rather than separately named functions.

Revision 1.1  2004/04/19 21:22:43  rothgang
Create template versions of Search classes.
-------------------------------------------------------------------------------
*/


#ifndef fl_searchable_numeric_tcc
#define fl_searchable_numeric_tcc


#include "fl/search.h"

#include <float.h>


namespace fl
{
  // class SearchableNumeric<T> -----------------------------------------------

  template<class T>
  SearchableNumeric<T>::SearchableNumeric (T perturbation)
  {
	if (perturbation == -1)
	{
	  perturbation = sizeof (T) == sizeof (float) ? sqrtf (FLT_EPSILON) : sqrt (DBL_EPSILON);
	}
	this->perturbation = perturbation;
  }

  template<class T>
  void
  SearchableNumeric<T>::gradient (const Vector<T> & point, Vector<T> & result, const int index)
  {
	// This approach is terribly inefficient, especially if dimension is larger
	// than 1.  :)  It's a good idea to directly implement gradient, even if it
	// is done by finite differences.

	Matrix<T> J;
	jacobian (point, J);

	result = J.row (index);
  }

  template<class T>
  void
  SearchableNumeric<T>::jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue)
  {
	int m = this->dimension ();
	int n = point.rows ();

	result.resize (m, n);

	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

	for (int i = 0; i < n; i++)
	{
	  Vector<T> column (& result(0,i), m);

	  T temp = point[i];
	  T h = perturbation * std::fabs (temp);
	  if (h == 0)
	  {
		h = perturbation;
	  }
	  const_cast<Vector<T> &> (point)[i] = temp + h;
	  value (point, column);
	  const_cast<Vector<T> &> (point)[i] = temp;

	  column -= oldValue;
	  column /= h;
	}
  }

  template<class T>
  void
  SearchableNumeric<T>::jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue)
  {
	int m = this->dimension ();
	int n = point.rows ();

	result.resize (m, n);

	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  value (point, oldValue);
	}

	Vector<T> column (m);
	for (int i = 0; i < n; i++)
	{
	  T temp = point[i];
	  T h = perturbation * std::fabs (temp);
	  if (h == 0)
	  {
		h = perturbation;
	  }
	  const_cast<Vector<T> &> (point)[i] = temp + h;
	  value (point, column);
	  const_cast<Vector<T> &> (point)[i] = temp;

	  for (int j = 0; j < m; j++)
	  {
		result.set (j, i, (column[j] - oldValue[j]) / h);
	  }
	}
  }

  template<class T>
  void
  SearchableNumeric<T>::hessian (const Vector<T> & point, Matrix<T> & result, const int index)
  {
	throw "hessian not implemented yet";
  }
}


#endif
