#include "fl/search.h"

#include <float.h>


using namespace fl;


// For debugging
//#include <iostream>
//using namespace std;


// class SearchableNumeric ----------------------------------------------------

SearchableNumeric::SearchableNumeric (double perturbation)
{
  if (perturbation == -1)
  {
	perturbation = sqrt (DBL_EPSILON);
  }
  this->perturbation = perturbation;
}

void
SearchableNumeric::gradient (const Vector<double> & point, Vector<double> & result, const int index)
{
  // This approach is terribly inefficient, especially if dimension is larger
  // than 1.  :)  It's a good idea to directly implement gradient, even if it
  // is done by finite differences.

  Matrix<double> J;
  jacobian (point, J);

  result = J.row (index);
}

void
SearchableNumeric::jacobian (const Vector<double> & point, Matrix<double> & result, const Vector<double> * currentValue)
{
  int m = dimension ();
  int n = point.rows ();

  result.resize (m, n);

  Vector<double> oldValue;
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
	Vector<double> column (& result(0,i), m);

	double temp = point[i];
	double h = perturbation * fabs (temp);
	if (h == 0)
	{
	  h = perturbation;
	}
	const_cast<Vector<double> &> (point)[i] = temp + h;
	value (point, column);
	const_cast<Vector<double> &> (point)[i] = temp;

	column -= oldValue;
	column /= h;
  }
}

void
SearchableNumeric::jacobian (const Vector<double> & point, MatrixSparse<double> & result, const Vector<double> * currentValue)
{
  int m = dimension ();
  int n = point.rows ();

  result.resize (m, n);

  Vector<double> oldValue;
  if (currentValue)
  {
	oldValue = *currentValue;
  }
  else
  {
	value (point, oldValue);
  }

  Vector<double> column (m);
  for (int i = 0; i < n; i++)
  {
	double temp = point[i];
	double h = perturbation * fabs (temp);
	if (h == 0)
	{
	  h = perturbation;
	}
	const_cast<Vector<double> &> (point)[i] = temp + h;
	value (point, column);
	const_cast<Vector<double> &> (point)[i] = temp;

	for (int j = 0; j < m; j++)
	{
	  result.set (j, i, (column[j] - oldValue[j]) / h);
	}
  }
}

void
SearchableNumeric::hessian (const Vector<double> & point, Matrix<double> & result, const int index)
{
  throw "hessian not implemented yet";
}
