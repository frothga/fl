#ifndef fl_searchable_numeric_tcc
#define fl_searchable_numeric_tcc


#include "fl/search.h"

#include <float.h>


namespace fl
{
  // class SearchableNumeric<T> -----------------------------------------------

  SearchableNumeric<float>::SearchableNumeric (float perturbation)
  {
	if (perturbation == -1)
	{
	  perturbation = sqrtf (FLT_EPSILON);
	}
	this->perturbation = perturbation;
  }

  SearchableNumeric<double>::SearchableNumeric (double perturbation)
  {
	if (perturbation == -1)
	{
	  perturbation = sqrt (DBL_EPSILON);
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
	int m = dimension ();
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
	  T h = perturbation * fabs (temp);
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
	int m = dimension ();
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
	  T h = perturbation * fabs (temp);
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
