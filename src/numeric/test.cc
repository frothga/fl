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


#include "fl/search.h"
#include "fl/random.h"

#include <limits>


using namespace std;
using namespace fl;


// Searchable class for testing numeric search methods
// Expected result = 0.08241057  1.133037  2.343695
template<class T>
class Test : public SearchableSparse<T>
{
public:
  Test ()
  //: SearchableSparse (1e-3)
  {
	this->cover ();
  }

  virtual int dimension ()
  {
	return 15;
  }

  virtual void value (const Vector<T> & x, Vector<T> & result)
  {
	const T y[] = {0.14, 0.18, 0.22, 0.25, 0.29, 0.32, 0.35, 0.39,
				   0.37, 0.58, 0.73, 0.96, 1.34, 2.10, 4.39};

	result.resize (15);
	for (int i = 0; i < 15; i++)
	{
	  T t0 = i + 1;
	  T t1 = 15 - i;
	  T t2 = t0;
	  if (i > 7)
	  {
		t2 = t1;
	  }
	  result[i] = y[i] - (x[0] + t0 / (x[1] * t1 + x[2] * t2));
	}
	cerr << ".";
  }

  virtual MatrixSparse<bool> interaction ()
  {
	MatrixSparse<bool> result (15, 3);
	result.clear (true);

	return result;
  }
};

template<class T>
void
testSearch ()
{
  vector<Search<T> *> searches;
  searches.push_back (new AnnealingAdaptive<T>);
  searches.push_back (new LevenbergMarquardt<T>);
  searches.push_back (new LevenbergMarquardtSparseBK<T>);

  Test<T> t;
  Matrix<T> expected ("~[0.08241057 1.133037 2.343695]");

  for (int i = 0; i < searches.size (); i++)
  {
	Vector<T> point (3);
	point[0] = 1;
	point[1] = 1;
	point[2] = 1;

	searches[i]->search (t, point);
	cerr << endl;
	double d = (point - expected).norm (2);
	if (d > 1e6)
	{
	  cerr << "Result not close enough: " << d << " = " << point << " - " << expected << endl;
	  throw "Search fails";
	}
  }
  cout << "Search passes" << endl;
}


inline Matrix<double>
makeMatrix (const int m, const int n)
{
  Matrix<double> A (m, n);
  for (int r = 0; r < m; r++)
  {
	for (int c = 0; c < n; c++)
	{
	  A(r,c) = randfb ();
	}
  }
  return A;
}

template<class T>
void
testOperator ()
{
  T epsilon = sqrt (numeric_limits<T>::epsilon ());
  cerr << "epsilon = " << epsilon << endl;

  // Instantiate various matrix types and sizes.  These will not be modified during the test.
  vector<MatrixAbstract<T> *> matrices;
  matrices.push_back (new Matrix<T> (makeMatrix (3, 3)));
  matrices.push_back (new Vector<T> ("[1 2 3]"));
  matrices.push_back (new MatrixPacked<T> (*matrices[0]));
  matrices.push_back (new MatrixSparse<T> (*matrices[0]));
  matrices.push_back (new MatrixDiagonal<T> (*matrices[1]));
  matrices.push_back (new MatrixIdentity<T> (3));
  matrices.push_back (new MatrixTranspose<T> (matrices[0]));
  matrices.push_back (new MatrixRegion<T> (*matrices[0], 1, 1, 2, 2));
  matrices.push_back (new MatrixFixed<T,2,2> (*matrices[0]));

  // Perform every operation between every combination of matrices.
  for (int i = 0; i < matrices.size (); i++)
  {
	MatrixAbstract<T> & A = *matrices[i];
	const uint32_t Aid = A.classID ();
	const int Arows = A.rows ();
	const int Acols = A.columns ();
	cerr << typeid (A).name () << endl;

	// Unary operations

	//   Inversion
	MatrixResult<T> result;
	if (Arows == Acols)
	{
	  result = A * !A;
	  if (result.rows () != result.columns ()) throw "A * !A is not square";
	  for (int c = 0; c < result.columns (); c++)
	  {
		for (int r = 0; r < result.rows (); r++)
		{
		  if (r == c)
		  {
			if (abs (result(r,c) - (T) 1) > epsilon) throw "A * !A diagonal is not 1";
		  }
		  else
		  {
			if (abs (result(r,c)) > epsilon) throw "A * !A off-diagonal is not 0";
		  }
		}
	  }
	}


	//   Transpose
	result = ~A;
	if (result.rows () != Acols  ||  result.columns () != Arows) throw "~A dimensions are wrong";
	for (int c = 0; c < result.columns (); c++)
	{
	  for (int r = 0; r < result.rows (); r++)
	  {
		if (abs (result(r,c) - A(c,r)) > epsilon) throw "~A unexpected element value";
	  }
	}
	

	// Binary operations with scalar
	{
	  T scalar = (T) 2;
	  MatrixResult<T> resultTimes = A * scalar;
	  MatrixResult<T> resultOver  = A / scalar;
	  MatrixResult<T> resultPlus  = A + scalar;
	  MatrixResult<T> resultMinus = A - scalar;
	  MatrixAbstract<T> * selfTimes = A.duplicate (true);
	  MatrixAbstract<T> * selfOver  = A.duplicate (true);
	  MatrixAbstract<T> * selfPlus  = A.duplicate (true);
	  MatrixAbstract<T> * selfMinus = A.duplicate (true);
	  (*selfTimes) *= scalar;
	  (*selfOver)  /= scalar;
	  (*selfPlus)  += scalar;
	  (*selfMinus) -= scalar;
	  if (resultTimes.rows () != Arows  ||  resultTimes.columns () != Acols) throw "A * scalar: dimensions are wrong";
	  if (resultOver .rows () != Arows  ||  resultOver .columns () != Acols) throw "A / scalar: dimensions are wrong";
	  if (resultPlus .rows () != Arows  ||  resultPlus .columns () != Acols) throw "A + scalar: dimensions are wrong";
	  if (resultMinus.rows () != Arows  ||  resultMinus.columns () != Acols) throw "A - scalar: dimensions are wrong";
	  if (selfTimes->rows () != Arows  ||  selfTimes->columns () != Acols) throw "A *= scalar: dimensions are wrong";
	  if (selfOver ->rows () != Arows  ||  selfOver ->columns () != Acols) throw "A /= scalar: dimensions are wrong";
	  if (selfPlus ->rows () != Arows  ||  selfPlus ->columns () != Acols) throw "A += scalar: dimensions are wrong";
	  if (selfMinus->rows () != Arows  ||  selfMinus->columns () != Acols) throw "A -= scalar: dimensions are wrong";
	  for (int c = 0; c < Acols; c++)
	  {
		for (int r = 0; r < Arows; r++)
		{
		  // Determine expected values
		  T & element  = A(r,c);
		  T product    = element * scalar;
		  T quotient   = element / scalar;
		  T sum        = element + scalar;
		  T difference = element - scalar;

		  if (abs (resultTimes (r,c) - product   ) > epsilon) throw "A * scalar: unexpected element value";
		  if (abs (resultOver  (r,c) - quotient  ) > epsilon) throw "A / scalar: unexpected element value";
		  if (abs (resultPlus  (r,c) - sum       ) > epsilon) throw "A + scalar: unexpected element value";
		  if (abs (resultMinus (r,c) - difference) > epsilon) throw "A - scalar: unexpected element value";
		  if (abs ((*selfTimes)(r,c) - product   ) > epsilon) throw "A *= scalar: unexpected element value";
		  if (abs ((*selfOver )(r,c) - quotient  ) > epsilon) throw "A /= scalar: unexpected element value";

		  // Don't test elements if A can't represent them.
		  if ((Aid & MatrixDiagonalID)  &&  r != c) continue;
		  if ((Aid & MatrixIdentityID)  &&  (r < Arows - 1  ||  c < Acols - 1)) continue;

		  if (abs ((*selfPlus )(r,c) - sum       ) > epsilon) throw "A += scalar: unexpected element value";
		  if (abs ((*selfMinus)(r,c) - difference) > epsilon) throw "A -= scalar: unexpected element value";
		}
	  }
	  delete selfTimes;
	  delete selfOver;
	  delete selfPlus;
	  delete selfMinus;
	}


	// Binary operations with matrix
	for (int j = 0; j < matrices.size (); j++)  // cover full set of matrices to ensure every one functions as both left and right operand
	{
	  MatrixAbstract<T> & B = *matrices[j];
	  cerr << "  " << typeid (B).name () << endl;
	  const int Brows = B.rows ();
	  const int Bcols = B.columns ();
	  const int Erows = min (Arows, Brows);  // size of overlap region for elementwise operations
	  const int Ecols = min (Acols, Bcols);
	  int Prows = Arows;  // expected size of self-product
	  int Pcols = Bcols;
	  if (Aid & MatrixPackedID) Prows = Pcols = min (Arows, Bcols);
	  if (Aid & MatrixIdentityID) Prows = Pcols = max (Arows, Bcols);
	  if (Aid & MatrixFixedID)
	  {
		Prows = Arows;
		Pcols = Acols;
	  }

	  MatrixResult<T> resultElTimes = A & B;
	  MatrixResult<T> resultTimes   = A * B;
	  MatrixResult<T> resultOver    = A / B;
	  MatrixResult<T> resultPlus    = A + B;
	  MatrixResult<T> resultMinus   = A - B;
	  MatrixAbstract<T> * selfElTimes = A.duplicate (true);
	  MatrixAbstract<T> * selfTimes   = A.duplicate (true);
	  MatrixAbstract<T> * selfOver    = A.duplicate (true);
	  MatrixAbstract<T> * selfPlus    = A.duplicate (true);
	  MatrixAbstract<T> * selfMinus   = A.duplicate (true);
	  (*selfElTimes) &= B;
	  (*selfTimes)   *= B;
	  (*selfOver)    /= B;
	  (*selfPlus)    += B;
	  (*selfMinus)   -= B;
	  if (resultElTimes.rows () != Arows  ||  resultElTimes.columns () != Acols) throw "A & B: dimensions are wrong";
	  if (resultTimes  .rows () != Arows  ||  resultTimes  .columns () != Bcols) throw "A * B: dimensions are wrong";
	  if (resultOver   .rows () != Arows  ||  resultOver   .columns () != Acols) throw "A / B: dimensions are wrong";
	  if (resultPlus   .rows () != Arows  ||  resultPlus   .columns () != Acols) throw "A + B: dimensions are wrong";
	  if (resultMinus  .rows () != Arows  ||  resultMinus  .columns () != Acols) throw "A - B: dimensions are wrong";
	  if (selfElTimes->rows () != Arows  ||  selfElTimes->columns () != Acols) throw "A &= B: dimensions are wrong";
	  if (selfTimes  ->rows () != Prows  ||  selfTimes  ->columns () != Pcols) throw "A *= B: dimensions are wrong";
	  if (selfOver   ->rows () != Arows  ||  selfOver   ->columns () != Acols) throw "A /= B: dimensions are wrong";
	  if (selfPlus   ->rows () != Arows  ||  selfPlus   ->columns () != Acols) throw "A += B: dimensions are wrong";
	  if (selfMinus  ->rows () != Arows  ||  selfMinus  ->columns () != Acols) throw "A -= B: dimensions are wrong";
	  for (int r = 0; r < Arows; r++)
	  {
		// Test standard matrix multiply
		const int w = min (Acols, Brows);
		for (int c = 0; c < Bcols; c++)
		{
		  T product = (T) 0;
		  for (int k = 0; k < w; k++) product += A(r,k) * B(k,c);

		  if (abs (resultTimes (r,c) - product) > epsilon) throw "A * B: unexpected element value";

		  if (r >= Prows  ||  c >= Pcols) continue;
		  if ((Aid & MatrixDiagonalID)  &&  r != c) continue;
		  if ((Aid & MatrixIdentityID)  &&  (r < Arows - 1  ||  c < Acols - 1)) continue;
		  if ((Aid & MatrixPackedID)    &&  r > c) continue;

		  if (abs ((*selfTimes)(r,c) - product) > epsilon) throw "A *= B: unexpected element value";
		}

		// Test elementwise operations
		for (int c = 0; c < Acols; c++)
		{
		  // Determine expected values
		  T & a = A(r,c);
		  T elproduct  = a;
		  T quotient   = a;
		  T sum        = a;
		  T difference = a;
		  if (r < Erows  &&  c < Ecols)
		  {
			T & b = B(r,c);
			elproduct  *= b;
			quotient   /= b;
			sum        += b;
			difference -= b;
		  }

		  if (abs (resultElTimes (r,c) - elproduct ) > epsilon) throw "A & B: unexpected element value";
		  if (abs (resultOver    (r,c) - quotient  ) > epsilon) throw "A / B: unexpected element value";
		  if (abs (resultPlus    (r,c) - sum       ) > epsilon) throw "A + B: unexpected element value";
		  if (abs (resultMinus   (r,c) - difference) > epsilon) throw "A - B: unexpected element value";

		  if ((Aid & MatrixPackedID)    &&  r > c) continue;
		  if ((Aid & MatrixIdentityID)  &&  (r < Arows - 1  ||  c < Acols - 1)) continue;

		  if (abs ((*selfElTimes)(r,c) - elproduct ) > epsilon) throw "A &= B: unexpected element value";
		  if (abs ((*selfOver )  (r,c) - quotient  ) > epsilon) throw "A /= B: unexpected element value";

		  if ((Aid & MatrixDiagonalID)  &&  r != c) continue;

		  if (abs ((*selfPlus )  (r,c) - sum       ) > epsilon) throw "A += B: unexpected element value";
		  if (abs ((*selfMinus)  (r,c) - difference) > epsilon) throw "A -= B: unexpected element value";
		}
	  }
	  delete selfElTimes;
	  delete selfTimes;
	  delete selfOver;
	  delete selfPlus;
	  delete selfMinus;
	}
  }

  cout << "operators pass" << endl;
}


template<class T>
void
testAll ()
{
  testSearch<T> ();
  testOperator<T> ();
}


int
main (int argc, char * argv[])
{
  try
  {
	cout << "running all tests for float" << endl;
	testAll<float> ();

	cout << "running all tests for double" << endl;
	testAll<double> ();
  }
  catch (const char * message)
  {
	cout << "Exception: " << message << endl;
	return 1;
  }
  catch (int message)
  {
	cout << "Numeric Exception: " << message << endl;
	return 1;
  }

  return 0;
}
