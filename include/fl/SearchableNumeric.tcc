/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_searchable_numeric_tcc
#define fl_searchable_numeric_tcc


#include "fl/search.h"

#include <float.h>
#include <limits>


namespace fl
{
  // class SearchableNumeric<T> -----------------------------------------------

  template<class T>
  SearchableNumeric<T>::SearchableNumeric (T perturbation)
  {
	if (perturbation == -1) perturbation = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->perturbation = perturbation;
  }

  template<class T>
  void
  SearchableNumeric<T>::gradient (const Vector<T> & point, Vector<T> & result)
  {
	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

	int n = point.rows ();

	result.resize (n);

	Vector<T> v;
	value (point, v);
	T v0 = v.sumSquares ();

	Vector<T> newValue;
	for (int i = 0; i < n; i++)
	{
	  T temp = point[i];
	  T h = perturbation * std::fabs (temp);
	  if (h == 0) h = perturbation;
	  perturbedPoint[i] += h;
	  value (perturbedPoint, v);
	  perturbedPoint[i] = temp;
	  T v1 = v.sumSquares ();

	  result[i] = (v1 - v0) / h;
	}
  }

  template<class T>
  void
  SearchableNumeric<T>::jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue)
  {
	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

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
	  if (h == 0) h = perturbation;
	  perturbedPoint[i] += h;
	  value (perturbedPoint, column);
	  perturbedPoint[i] = temp;

	  result.column (i) = (column - oldValue) / h;
	}
  }

  template<class T>
  void
  SearchableNumeric<T>::jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue)
  {
	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

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
	  if (h == 0) h = perturbation;
	  perturbedPoint[i] += h;
	  value (perturbedPoint, column);
	  perturbedPoint[i] = temp;

	  for (int j = 0; j < m; j++)
	  {
		result.set (j, i, (column[j] - oldValue[j]) / h);
	  }
	}
  }

  template<class T>
  void
  SearchableNumeric<T>::hessian (const Vector<T> & point, Matrix<T> & result)
  {
	T perturbation2 = std::sqrt (perturbation);  // because hessian takes second derivative, we need to keep denominator from getting too small

	Vector<T> point00;
	Vector<T> point10;
	point00.copyFrom (point);
	point10.copyFrom (point);

	int n = point.rows ();
	result.resize (n, n);

	Vector<T> deltas (n);
	for (int i = 0; i < n; i++)
	{
	  T & delta = deltas[i];
	  delta = perturbation2 * std::fabs (point[i]);
	  if (delta == 0) delta = perturbation2;
	}

	Vector<T> v;
	value (point00, v);
	T v00 = v.sumSquares ();

	for (int i = 0; i < n; i++)
	{
	  T & deltaI = deltas[i];

	  point10[i] += deltaI;
	  value (point10, v);
	  T v10 = v.sumSquares ();

	  // Diagonals -- use central differences
	  point00[i] -= deltaI;
	  value (point00, v);
	  point00[i] = point[i];
	  result(i,i) = ((v10 - v00) / deltaI - (v00 - v.sumSquares ()) / deltaI) / deltaI;

	  // Off-diagonals
	  for (int j = i + 1; j < n; j++)
	  {
		T deltaJ = deltas[j];

		point00[j] += deltaJ;
		value (point00, v);
		point00[j] = point[j];
		T v01 = v.sumSquares ();

		T temp = point10[j];
		point10[j] += deltaJ;
		value (point10, v);
		point10[j] = temp;
		T v11 = v.sumSquares ();

		T h = ((v11 - v10) / deltaJ - (v01 - v00) / deltaJ) / deltaI;  // distribute the division by deltaJ for better scaling
		result(i,j) = h;
		result(j,i) = h;
	  }

	  point10[i] = point[i];
	}
  }

  /*
  template<class T>
  void
  SearchableNumeric<T>::hessian (const Vector<T> & point, Matrix<T> & result)
  {
	T perturbation2 = std::sqrt (perturbation);  // because hessian takes second derivative, we need to keep denominator from getting too small

	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

	int n = point.rows ();
	result.resize (n, n);

	Vector<T> g0;
	gradient (point, g0);

	for (int i = 0; i < n; i++)
	{
	  Vector<T> g;

	  T temp = point[i];
	  T h = perturbation2 * std::fabs (temp);
	  if (h == 0) h = perturbation2;
	  perturbedPoint[i] += h;
	  gradient (perturbedPoint, g);
	  perturbedPoint[i] = temp;

	  result.column (i) = (g - g0) / h;
	}

	// At this point, result is not necessarily symmetric.  We need to enforce
	// symmetry so that client code can depend on that property.
	for (int i = 0; i < n; i++)
	{
	  for (int j = i + 1; j < n; j++)
	  {
		T & a = result(i,j);
		T & b = result(j,i);
		a = b = (a + b) / (T) 2;
	  }
	}
  }
  */
}


#endif
