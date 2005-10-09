/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fixes for MSVC and Cygwin
08/2005 Fred Rothganger -- Compilability fix for GCC 3.4.4
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file
LICENSE for details.
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
