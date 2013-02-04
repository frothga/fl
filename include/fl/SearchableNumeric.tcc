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
  Search<T> *
  SearchableNumeric<T>::search ()
  {
	return new LevenbergMarquardt<T>;
  }

  template<class T>
  void
  SearchableNumeric<T>::finish (const Vector<T> & point)
  {
  }

  template<class T>
  MatrixResult<T>
  SearchableNumeric<T>::scales (const Vector<T> & point)
  {
	Vector<T> * result = new Vector<T> (point.rows ());
	result->clear (1);
	return result;
  }

  template<class T>
  MatrixResult<T>
  SearchableNumeric<T>::gradient (const Vector<T> & point, const Vector<T> * currentValue)
  {
	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

	int n = point.rows ();

	Vector<T> * result = new Vector<T> (n);

	T v0;
	Vector<T> v;
	if (currentValue)
	{
	  v0 = currentValue->sumSquares ();
	}
	else
	{
	  v = this->value (point);
	  v0 = v.sumSquares ();
	}

	for (int i = 0; i < n; i++)
	{
	  T temp = point[i];
	  T h = perturbation * std::fabs (temp);
	  if (h == 0) h = perturbation;
	  perturbedPoint[i] += h;
	  v = this->value (perturbedPoint);
	  perturbedPoint[i] = temp;
	  T v1 = v.sumSquares ();

	  (*result)[i] = (v1 - v0) / h;
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  SearchableNumeric<T>::jacobian (const Vector<T> & point, const Vector<T> * currentValue)
  {
	Vector<T> perturbedPoint;
	perturbedPoint.copyFrom (point);

	Vector<T> oldValue;
	if (currentValue)
	{
	  oldValue = *currentValue;
	}
	else
	{
	  oldValue = this->value (point);
	}

	int m = oldValue.rows ();
	int n = point.rows ();

	Matrix<T> * result = new Matrix<T> (m, n);

	Vector<T> column (m);
	for (int i = 0; i < n; i++)
	{
	  T temp = point[i];
	  T h = perturbation * std::fabs (temp);
	  if (h == 0) h = perturbation;
	  perturbedPoint[i] += h;
	  column = this->value (perturbedPoint);
	  perturbedPoint[i] = temp;

	  result->column (i) = (column - oldValue) / h;
	}

	return result;
  }

  template<class T>
  MatrixResult<T>
  SearchableNumeric<T>::hessian (const Vector<T> & point, const Vector<T> * currentValue)
  {
	T perturbation2 = std::sqrt (perturbation);  // because hessian takes second derivative, we need to keep denominator from getting too small

	Vector<T> point00;
	Vector<T> point10;
	point00.copyFrom (point);
	point10.copyFrom (point);

	int n = point.rows ();
	Matrix<T> * result = new Matrix<T> (n, n);

	Vector<T> deltas (n);
	for (int i = 0; i < n; i++)
	{
	  T & delta = deltas[i];
	  delta = perturbation2 * std::fabs (point[i]);
	  if (delta == 0) delta = perturbation2;
	}

	T v00;
	if (currentValue)
	{
	  v00 = currentValue->sumSquares ();
	}
	else
	{
	  v00 = this->value (point00).sumSquares ();
	}

	for (int i = 0; i < n; i++)
	{
	  T & deltaI = deltas[i];

	  point10[i] += deltaI;
	  T v10 = this->value (point10).sumSquares ();

	  // Diagonals -- use central differences
	  point00[i] -= deltaI;
	  T v = this->value (point00).sumSquares ();
	  point00[i] = point[i];
	  (*result)(i,i) = ((v10 - v00) / deltaI - (v00 - v) / deltaI) / deltaI;

	  // Off-diagonals
	  for (int j = i + 1; j < n; j++)
	  {
		T deltaJ = deltas[j];

		point00[j] += deltaJ;
		T v01 = this->value (point00).sumSquares ();
		point00[j] = point[j];

		T temp = point10[j];
		point10[j] += deltaJ;
		T v11 = this->value (point10).sumSquares ();
		point10[j] = temp;

		T h = ((v11 - v10) / deltaJ - (v01 - v00) / deltaJ) / deltaI;  // distribute the division by deltaJ for better scaling
		(*result)(i,j) = h;
		(*result)(j,i) = h;
	  }

	  point10[i] = point[i];
	}

	return result;
  }
}


#endif
