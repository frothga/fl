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


#include "fl/search.h"
#include "fl/random.h"
#include "fl/lapack.h"
#include "fl/fourier.h"
#include "fl/cluster.h"

#include <limits>
#include <complex>
#include <typeinfo>


using namespace std;
using namespace fl;


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
class TestFunction
{
public:
  Vector<T> endPoint;
  T         endResidual;
};

/**
   Searchable class for testing numeric search methods, translated from MINPACK.
**/
template<class T>
class MinpackTestFunction : public SearchableNumeric<T>, public TestFunction<T>, public SearchableGreedy<T>
{
public:
  MinpackTestFunction (T perturbation = -1)
  : SearchableNumeric<T> (perturbation)
  {
	TestFunction<T>::endPoint     = Vector<T> ("[0.08241058  1.133037  2.343695]");
	TestFunction<T>::endResidual  = 0.09063596;

	SearchableGreedy<T>::bestResidual = INFINITY;
  }

  virtual MatrixResult<T> start ()
  {
	return new Vector<T> ("[0 1 2]");
  }

  virtual int dimension (const Vector<T> & x)
  {
	return 15;
  }

  virtual MatrixResult<T> value (const Vector<T> & x)
  {
	const T y[] = {0.14, 0.18, 0.22, 0.25, 0.29, 0.32, 0.35, 0.39,
				   0.37, 0.58, 0.73, 0.96, 1.34, 2.10, 4.39};

	Vector<T> * result = new Vector<T> (15);
	for (int i = 0; i < 15; i++)
	{
	  T t0 = i + 1;
	  T t1 = 15 - i;
	  T t2 = i > 7 ? t1 : t0;
	  (*result)[i] = y[i] - (x[0] + t0 / (x[1] * t1 + x[2] * t2));
	}

	this->update (result->norm (2), x);
	cerr << ".";
	return result;
  }
};

/**
   Each output element is a sum of polynomials of the given degree, one
   polynomial per input element.  This problem can be made almost arbitrarily
   hard, such that only an exhaustive search can find the true global minimum.
   Gradient methods should be able to solve polynomials of degree 2 where
   all the coefficients are positive, since there should be only a single
   minimum.
**/
template<class T>
class PolynomialTestFunction : public SearchableNumeric<T>, public TestFunction<T>
{
public:
  PolynomialTestFunction (int rows = 15, int degree = 3, T perturbation = -1)
  : rows (rows),
	degree (degree),
	SearchableNumeric<T> (perturbation)
  {
	TestFunction<T>::endPoint     = Vector<T> ("[1 2 3]");
	TestFunction<T>::endResidual  = 0;  // by design, this problem can be solved exactly

	A = makeMatrix (rows, (degree + 1) * start ().rows ());
	correctValue = f (TestFunction<T>::endPoint);
  }

  virtual MatrixResult<T> start ()
  {
	return new Vector<T> ("[10 10 10]");  // starting far away seems to help
  }

  virtual int dimension (const Vector<T> & x)
  {
	return rows;
  }

  virtual MatrixResult<T> value (const Vector<T> & x)
  {
	cerr << ".";
	return f (x) - correctValue;
  }

  MatrixResult<T> f (const Vector<T> & x)
  {
	Vector<T> powers (A.columns ());
	assert (A.columns () == x.rows () * (degree + 1));
	for (int i = 0; i < x.rows (); i++)
	{
	  for (int p = 0; p <= degree; p++)
	  {
		powers[i * (degree + 1) + p] = pow (x[i], (T) p);
	  }
	}
	return A * powers;
  }

  int rows;
  int degree;
  Matrix<T> A;
  Vector<T> correctValue;
};

template<class T>
class SparseTestFunction : public SearchableSparse<T>, public TestFunction<T>
{
public:
  SparseTestFunction (T perturbation = -1)
  : SearchableSparse<T> (perturbation)
  {
	TestFunction<T>::endPoint     = Vector<T> ("[0.08241058  1.133037  2.343695]");
	TestFunction<T>::endResidual  = 0;
  }

  virtual MatrixResult<T> start ()
  {
	return new Vector<T> ("[0 1 2]");
  }

  virtual int dimension (const Vector<T> & x)
  {
	return 15;
  }

  virtual MatrixResult<T> value (const Vector<T> & x)
  {
	Vector<T> * result = new Vector<T> (15);
	for (int i = 0; i < 15; i++)
	{
	  int j = i / 5;
	  (*result)[i] = pow (abs (x[j] - TestFunction<T>::endPoint[j]), (T) (1.0 + i / 15));
	}
	cerr << ".";
	//cerr << result.norm (2) << endl;
	return result;
  }

  virtual MatrixSparse<bool> interaction ()
  {
	MatrixSparse<bool> result (15, 3);
	for (int i = 0; i < 15; i++)
	{
	  int j = i / 5;
	  result.set (i, j, true);
	}

	return result;
  }
};

template<class T>
class ConstrictionTestFunction : public SearchableConstriction<T>, public TestFunction<T>
{
public:
  ConstrictionTestFunction (Searchable<T> * searchable)
  : SearchableConstriction<T> (*searchable, Vector<T> (), Vector<T> ())
  {
	TestFunction<T> * t = dynamic_cast<TestFunction<T> *> (searchable);
	if (! t) throw "searchable is not a TestFunction";
	SearchableConstriction<T>::a = t->endPoint / 2;
	SearchableConstriction<T>::b = t->endPoint;

	TestFunction<T>::endPoint     = Vector<T> ("[0.5]");
	TestFunction<T>::endResidual  = t->endResidual;
  }
};

template<class T>
void
testSearch ()
{
  int seed = 0;  //time (0);
  srand (seed);
  cerr << "random seed for testSearch is " << seed << endl;

  vector<Searchable<T> *> searchables;
  searchables.push_back (new MinpackTestFunction<T>);
  searchables.push_back (new PolynomialTestFunction<T>);
  searchables.push_back (new SparseTestFunction<T>);
  searchables.push_back (new ConstrictionTestFunction<T> (searchables[0]));

  vector<Search<T> *> searches;
  searches.push_back (new AnnealingAdaptive<T>);  // Stochastic search along a single dimension doesn't seem to work so well.
  searches.push_back (new GradientDescent<T> (1e-4));  // The default toleranceX for double is too tight.
  searches.push_back (new LevenbergMarquardtSparse<T>);
  searches.push_back (new ParticleSwarm<T>);
# ifdef HAVE_LAPACK
  searches.push_back (new LevenbergMarquardt<T>);
  searches.push_back (new NewtonRaphson<T>);
  searches.push_back (new ConjugateGradient<T>);
# endif

  // Need a better method for representing expectations.  Perhaps define a
  // function in the TestFunction class that gives back a value based on
  // runtime type of search.
  const int m = searchables.size ();
  const int n = searches.size ();
  Matrix<T> epsilons (m, n);
  epsilons.clear (INFINITY);  // by default, disable all tests; then enable them below
  epsilons.column (0).clear (1e-2);  // AnnealingAdaptive
  epsilons.column (1).clear (1e-2);  // GradientDescent
  epsilons.column (2).clear (1e-6);  // LevenbergMarquardtBK
  epsilons.column (3).clear (1e-2);  // ParticleSwarm
# ifdef HAVE_LAPACK
  epsilons.column (4).clear (1e-6);  // LevenbergMarquardt
  epsilons.column (5).clear (1e-3);  // NewtonRaphson
  epsilons.column (6).clear (1e-2);  // ConjugateGradient
# endif
  epsilons(3,0) = INFINITY;  // AnnealingAdaptive can't solve a line search
  epsilons(3,3) = INFINITY;  // neither can ParticleSwarm
  epsilons.row (1).clear (INFINITY);  // very few methods can solve PolynomialTestFunction ...
  epsilons(1,2) = 1e-5;               // except LM
# ifdef HAVE_LAPACK
  epsilons(1,4) = 1e-5;
# endif

  for (int i = 0; i < m; i++)
  {
	Searchable<T> * function = searchables[i];
	TestFunction<T> * t = dynamic_cast<TestFunction<T> *> (function);
	SearchableGreedy<T> * greedy = dynamic_cast<SearchableGreedy<T> *> (function);

	for (int j = 0; j < n; j++)
	{
	  T epsilon = epsilons(i,j);
	  Search<T> * s = searches[j];
	  cerr << typeid (*s).name () << " searching " << typeid (*function).name () << endl;
	  if (isinf (epsilon))
	  {
		cerr << "  skipping" << endl;
		continue;
	  }

	  if (greedy) greedy->bestResidual = INFINITY;  // Have to reset this, because we are reusing the object.
	  srand (seed);

	  Vector<T> point = function->start ();

	  s->search (*function, point);
	  cerr << endl;

	  Vector<T> error = function->value (point);
	  double e = error.norm (2);
	  double d = e - t->endResidual;
	  cerr << "distance to best residual = " << d << " = " << e << " - " << t->endResidual << endl;
	  cerr << "distance to best point    = " << (point - t->endPoint).norm (2) << " = " << point << " - " << t->endPoint << endl;
	  if (d > epsilon) throw "Search fails";
	}
  }

  for (int i = 0; i < n; i++) delete searches[i];
  for (int i = m - 1; i >= 0; i--) delete searchables[i];

  cout << "Search passes" << endl;
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
  matrices.push_back ((~((Matrix<T> *) matrices[0])->region (1, 1, 2, 2)).clone ());
  matrices.push_back (new MatrixPacked<T> (*matrices[0]));
  matrices.push_back (new MatrixSparse<T> (*matrices[0]));
  matrices.push_back (new MatrixDiagonal<T> (*matrices[1]));
  matrices.push_back (new MatrixIdentity<T> (3));
  matrices.push_back (new MatrixTranspose<T> (matrices[0]->clone ()));
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
	Matrix<T> result;
#   ifdef HAVE_LAPACK
	int expectedSize;
	if (Arows <= Acols)  // right inverse
	{
	  result = A * !A;
	  expectedSize = Arows;
	}
	else  // left inverse
	{
	  result = !A * A;
	  expectedSize = Acols;
	}
	if (result.rows () != expectedSize  ||  result.columns () != expectedSize) throw "A * !A (or !A * A) is wrong size";
	for (int c = 0; c < expectedSize; c++)
	{
	  for (int r = 0; r < expectedSize; r++)
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
#   endif

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
	  MatrixAbstract<T> * selfTimes = A.clone (true);
	  MatrixAbstract<T> * selfOver  = A.clone (true);
	  MatrixAbstract<T> * selfPlus  = A.clone (true);
	  MatrixAbstract<T> * selfMinus = A.clone (true);
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

	  // TODO: test cross product (^) once it is generalized to any dimension (wedge product)
	  MatrixResult<T> resultElTimes = A & B;
	  MatrixResult<T> resultTimes   = A * B;
	  MatrixResult<T> resultOver    = A / B;
	  MatrixResult<T> resultPlus    = A + B;
	  MatrixResult<T> resultMinus   = A - B;
	  MatrixAbstract<T> * selfElTimes = A.clone (true);
	  MatrixAbstract<T> * selfTimes   = A.clone (true);
	  MatrixAbstract<T> * selfOver    = A.clone (true);
	  MatrixAbstract<T> * selfPlus    = A.clone (true);
	  MatrixAbstract<T> * selfMinus   = A.clone (true);
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

  for (int i = matrices.size () - 1; i >= 0; i--) delete matrices[i];

  cout << "operators pass" << endl;
}

template<class T>
void
testReshape ()
{
  Matrix<T> A (3, 3);
  for (int i = 0; i < 9; i++) A[i] = i;

  // rows and columns unchanged
  Matrix<T> B = A.reshape (3, 3);
  if (B.rows () != 3  ||  B.columns () != 3) throw "reshape 3x3 unexpected size";
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 3; r++)
	{
	  if (B(r,c) != (c * 3 + r) % 9) throw "reshape 3x3 unexpected value";
	}
  }

  // rows same, fewer columns
  B = A.reshape (3, 2);
  if (B.rows () != 3  ||  B.columns () != 2) throw "reshape 3x2 unexpected size";
  for (int c = 0; c < 2; c++)
  {
	for (int r = 0; r < 3; r++)
	{
	  if (B(r,c) != (c * 3 + r) % 9) throw "reshape 3x2 unexpected value";
	}
  }

  // inPlace mode, fewer rows, columns same
  B = A.reshape (2, 3, true);
  if (B.rows () != 2  ||  B.columns () != 3) throw "reshape in place 2x3 unexpected size";
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 2; r++)
	{
	  if (B(r,c) != (c * 3 + r) % 9) throw "reshape in place 2x3 unexpected value";
	}
  }

  // fewer rows, fewer columns
  B = A.reshape (2, 2);
  if (B.rows () != 2  ||  B.columns () != 2) throw "reshape 2x2 unexpected size";
  if (B(0,0) != 0  ||  B(1,0) != 1  ||  B(0,1) != 2  ||  B(1,1) != 3) throw "reshape 2x2 unexpected value";

  // more rows, fewer columns
  B = A.reshape (9, 1);
  if (B.rows () != 9  ||  B.columns () != 1) throw "reshape 9x1 unexpected size";
  for (int i = 0; i < 9; i++) if (B(i,0) != i) throw "reshape 9x1 unexpected value";

  // more rows, more columns
  B = A.reshape (7, 7);
  if (B.rows () != 7  ||  B.columns () != 7) throw "reshape 7x7 unexpected size";
  for (int c = 0; c < 7; c++)
  {
	for (int r = 0; r < 7; r++)
	{
	  if (B(r,c) != (c * 7 + r) % 9) throw "reshape 7x7 unexpected value";
	}
  }

  // fewer rows, more columns
  B = A.reshape (2, 5);
  if (B.rows () != 2  ||  B.columns () != 5) throw "reshape 2x5 unexpected size";
  for (int c = 0; c < 5; c++)
  {
	for (int r = 0; r < 2; r++)
	{
	  if (B(r,c) != (c * 2 + r) % 9) throw "reshape 2x5 unexpected value";
	}
  }

  cout << "reshape passes" << endl;
}

template<class T>
void
testStrided ()
{
  Matrix<T> A (7, 5);
  for (int i = 0; i < 35; i++) A[i] = i;
  A = A.reshape (4, 5, true);

  MatrixStrided<T> B = ~A.region (1, 1, 3, 2);
  if (B.rows () != 2  ||  B.columns () != 3) throw "strided transpose unexpected size";
  if (B(0,0) != 8  ||  B(1,0) != 15  ||  B(0,1) != 9  ||  B(1,1) != 16  ||  B(0,2) != 10  ||  B(1,2) != 17) throw "strided transpose unexpected value";

  B = A.row (1);
  if (B.rows () != 1  ||  B.columns () != 5) throw "strided row unexpected size";
  for (int i = 0; i < 5; i++) if (B[i] != i * 7 + 1) throw "strided row unexpected value";

  B = A.column (1);
  if (B.rows () != 4  ||  B.columns () != 1) throw "strided column unexpected size";
  for (int i = 0; i < 4; i++) if (B[i] != i + 7) throw "strided column unexpected value";

  cout << "MatrixStrided passes" << endl;
}

template<class T>
void
testNorm ()
{
  T epsilon = sqrt (numeric_limits<T>::epsilon ());

  vector<MatrixAbstract<T> *> matrices;
  matrices.push_back (new Matrix<T> ("[0 1 2; 3 4 5; 6 7 8]"));  // a matrix with easily calculated norms
  matrices.push_back (new Vector<T> ("~[0 1 2 3 4 5 6 7 8]"));
  matrices.push_back (new Matrix<T> (" [0 1 2 3 4 5 6 7 8]"));
  matrices.push_back ((~ *matrices[0]).clone ());
  matrices.push_back ((~ *matrices[1]).clone ());
  matrices.push_back ((~ *matrices[2]).clone ());

  for (int i = 0; i < matrices.size (); i++)
  {
	MatrixAbstract<T> * A = matrices[i];

	if (     A->norm (0)        != 8                          ) throw "norm(0) unexpected value";
	if (     A->norm (1)        != 36                         ) throw "norm(1) unexpected value";
	if (abs (A->norm (1.5)       - 19.1877274154004) > epsilon) throw "norm(1.5) unexpected value";
	if (abs (A->norm (2)         - 14.2828568570857) > epsilon) throw "norm(2) unexpected value";
	if (     A->norm (INFINITY) != 8                          ) throw "norm(INFINITY) unexpected value";

	delete A;
  }

  cout << "norm passes" << endl;
}

template<class T>
void
testClear ()
{
  Matrix<T> A (4, 3);
  A.clear ();
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 4; r++)
	{
	  if (A(r,c)) throw "not cleared to zero";
	}
  }
  A.clear (1);
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 4; r++)
	{
	  if (A(r,c) != 1) throw "not cleared to one";
	}
  }

  A.reshape (3, 3, true);
  A.clear ();
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 3; r++)
	{
	  if (A(r,c)) throw "strided not cleared to zero";
	}
  }
  A.clear (1);
  for (int c = 0; c < 3; c++)
  {
	for (int r = 0; r < 3; r++)
	{
	  if (A(r,c) != 1) throw "strided not cleared to one";
	}
  }

  cout << "clear passes" << endl;
}

template<class T>
void
testSumSquares ()
{
  T epsilon = sqrt (numeric_limits<T>::epsilon ());
  Matrix<T> A (3, 3);
  T answer = 0;
  for (int i = 0; i < 9; i++)
  {
	answer += i * i;
	A[i] = i;
  }
  if (abs (A.sumSquares () - answer) > epsilon) throw "sumSquares unexpected value";
  cout << "sumSquares passes" << endl;
}

/**
   @deprecated Dot product should go away, replaced simply by ~x * y
 **/
template<class T>
void
testDot ()
{
  T epsilon = sqrt (numeric_limits<T>::epsilon ());
  Vector<T> a (3);
  Vector<T> b (3);
  T answer = 0;
  for (int i = 0; i < 3; i++)
  {
	answer += i * (2 - i);
	a[i] = i;
	b[i] = 2 - i;
  }
  if (abs (a.dot (b) - answer) > epsilon) throw "dot unexpected value";

  MatrixRegion<T> R (b);
  if (abs (a.dot (R) - answer) > epsilon) throw "dot(Region) unexpected value";

  cout << "dot passes" << endl;
}

template<class T>
void
testLAPACK ()
{
  T epsilon   = sqrt (numeric_limits<T>::epsilon ());
  T epsilon50 = 50 *  numeric_limits<T>::epsilon ();

  // Inversion is tested by testOperator() above.  It covers the LAPACK
  // routines gesvd, getrf, and getri

  // Test gelss
  //   Uniquely determined
  Matrix<T> A = makeMatrix (3, 3);
  Matrix<T> B = makeMatrix (3, 3);
  Matrix<T> X;
  gelss (A, X, B);  // preserve A and B
  Matrix<T> R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelss 3x3";

  //  Underdetermined
  A = makeMatrix (3, 7);
  B = makeMatrix (3, 3);
  T residual;
  gelss (A, X, B, &residual);  // preserve A and B
  R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelss 3x7";
  if (residual) throw "gelss 3x7 has unexpected residual";

  //  Overdetermined
  A = makeMatrix (7, 3);
  B = A * makeMatrix (3, 3);  // Make it possible to solve exactly
  gelss (A, X, B, &residual);  // preserve A and B
  R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelss 7x3";
  if (abs (sqrt (residual) - R.norm (2)) > epsilon50) throw "gelss 7x3 unexpected residual";

  //  An ill-formed problem that gelss should be able to handle
  B = A.region (0, 0, 4, 2) * Matrix<T> (makeMatrix (3, 3));  // By selecting a subset of rows of A, we make a B that doesn't match row count of A.  gelss() should solve this by using the smaller number of rows.
  gelss (A, X, B, &residual);
  if (X.rows () != 3  ||  X.columns () != 3) throw "gelss (7x3)*X=(5x3) has unexpected size";
  R = A.region (0, 0, 4, 2) * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelss (7x3)*X-(5x3)";
  if (abs (sqrt (residual) - R.norm (2)) > epsilon50) throw "gelss (7x3)*X=(5x3) unexpected residual";


  // Test gelsd  (same set of tests as gelss)
  //   Uniquely determined
  A = makeMatrix (3, 3);
  B = makeMatrix (3, 3);
  gelsd (A, X, B);  // preserve A and B
  R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelsd 3x3";

  //  Underdetermined
  A = makeMatrix (3, 7);
  B = makeMatrix (3, 3);
  gelsd (A, X, B, &residual);  // preserve A and B
  R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelsd 3x7";
  if (residual) throw "gelsd 3x7 has unexpected residual";

  //  Overdetermined
  A = makeMatrix (7, 3);
  B = A * makeMatrix (3, 3);  // Make it possible to solve exactly
  gelsd (A, X, B, &residual);  // preserve A and B
  R = A * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelsd 7x3";
  if (abs (sqrt (residual) - R.norm (2)) > epsilon50) throw "gelsd 7x3 unexpected residual";

  //  An ill-formed problem that gelss should be able to handle
  B = A.region (0, 0, 4, 2) * Matrix<T> (makeMatrix (3, 3));  // By selecting a subset of rows of A, we make a B that doesn't match row count of A.  gelss() should solve this by using the smaller number of rows.
  gelsd (A, X, B, &residual);
  if (X.rows () != 3  ||  X.columns () != 3) throw "gelsd (7x3)*X=(5x3) has unexpected size";
  R = A.region (0, 0, 4, 2) * X - B;
  if (R.norm (2) > epsilon) throw "excessive residual in gelsd (7x3)*X-(5x3)";
  if (abs (sqrt (residual) - R.norm (2)) > epsilon50) throw "gelsd (7x3)*X=(5x3) unexpected residual";


  // Test geev
  //   Create a 3D rotation matrix, which will have known eigenvalues
  Vector<T> x = makeMatrix (3, 1);
  Vector<T> y = makeMatrix (3, 1);
  Vector<T> z = x ^ y;
  x = y ^ z;
  x.normalize ();
  y.normalize ();
  z.normalize ();
  A.resize (7, 3);
  A = A.reshape (3, 3, true);
  A.column (0) = x;
  A.column (1) = y;
  A.column (2) = z;

  //   geev with (right) eigenvectors and complex eigenvalues
  Matrix<T> eigenvectors;
  Matrix<complex<T> > zeigenvalues;
  geev (A, zeigenvalues, eigenvectors);
  if (zeigenvalues.rows () != 3  ||  zeigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (abs (zeigenvalues[i]) - 1) > epsilon) throw "geev unexpected magnitude of eigenvalue";
  //     don't bother checking actual values of eigenvectors
  if (eigenvectors.rows () != 3  ||  eigenvectors.columns () != 3) throw "geev unexpected size of eigenvectors";

  //   geev with eigenvectors and real eigenvalues
  Matrix<T> eigenvalues;
  geev (A, eigenvalues, eigenvectors);  // preserve A
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - zeigenvalues[i].real ()) > epsilon) throw "geev unexpected eigenvalue";
  if (eigenvectors.rows () != 3  ||  eigenvectors.columns () != 3) throw "geev unexpected size of eigenvectors";

  //   geev with just real eigenvalues
  eigenvalues.clear ();
  geev (A, eigenvalues);
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - zeigenvalues[i].real ()) > epsilon) throw "geev unexpected eigenvalue";


  // Test spev
  MatrixPacked<T> P (3);
  P.identity ();
  syev (P, eigenvalues);
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - 1) > epsilon) throw "spev unexpected eigenvalue";


  // Test syev
  //   eigenvalues and eigenvectors
  A.identity ();
  syev (A, eigenvalues, eigenvectors);
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - 1) > epsilon) throw "syev unexpected eigenvalue";
  if (eigenvectors.rows () != 3  ||  eigenvectors.columns () != 3) throw "syev unexpected size of eigenvectors";

  //   just eigenvalues
  syev (A, eigenvalues);
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - 1) > epsilon) throw "syev unexpected eigenvalue";


  // Test sygv
  B = A * 2;
  sygv (A, B, eigenvalues, eigenvectors);
  if (eigenvalues.rows () != 3  ||  eigenvalues.columns () != 1) throw "unexpected size of eigenvalues";
  for (int i = 0; i < 3; i++) if (abs (eigenvalues[i] - 0.5) > epsilon) throw "sygv unexpected eigenvalue";
  if (eigenvectors.rows () != 3  ||  eigenvectors.columns () != 3) throw "sygv unexpected size of eigenvectors";


  cout << "LAPACK passes" << endl;
}

#ifdef HAVE_FFTW
template<class T>
void
testFourier ()
{
  T epsilon = 5 * numeric_limits<T>::epsilon ();

  // Generate a matrix of random numbers
  Matrix<T> A = makeMatrix (100, 100);

  // Do a round-trip pair of transforms
  Fourier<T> F;
  Matrix<complex<T> > C;
  Matrix<T> B;
  F.dft (A, C);
  F.dft (C, B);

  // Compare result with original matrix
  if (A.rows () != B.rows ()  ||  A.columns () != B.columns ()) throw "Unexpected size of output from round-trip through Fourier.";
  if ((A - B).norm (INFINITY) > epsilon) throw "Excessive difference between elements in round-trip through Fourier.";
  cout << "Fourier passes" << endl;
}
#endif

void
testCluster ()
{
  // Generate data
  // Gaussian ball at 2 on each axis.  Ball is small enough to provide
  // adequate separation in any dimension.  Advantage of this form is that
  // all balls are equal in size (amenable to KMeans) and we know exactly
  // how many clusters there should be.
  const int dimension = 3;
  const int count = 1000;
  const float separation = 2;
  vector<Vector<float> > data;
  vector<int> classes (dimension * count);
  int * c = &classes[0];
  for (int d = 0; d < dimension; d++)
  {
	for (int i = 0; i < count; i++)
	{
	  Vector<float> datum (dimension);
	  for (int r = 0; r < dimension; r++) datum[r] = randfb ();  // more guaranteed results than randGaussian()
	  datum[d] += separation;
	  data.push_back (datum);
	  *c++ = d;
	}
  }

  // Test KMeans
  KMeans kmeans (dimension);
  kmeans.run (data);
  if (kmeans.classCount () != dimension) throw "KMeans wrong number of clusters";
  for (int i = 0; i < dimension; i++)
  {
	Vector<float> point (dimension);
	point.clear ();
	point[i] = separation;
	Vector<float> center = kmeans.representative (kmeans.classify (point));
	if ((point - center).norm (2) > 0.2) throw "KMeans cluster is missing";
  }

  // Test KMeansTree
  KMeansTree ktree (dimension, 2);
  ktree.run (data);
  if (ktree.classCount () != dimension * dimension) throw "KMeansTree wrong number of clusters";
  Vector<bool> unfound (dimension);
  unfound.clear (true);
  for (int i = 0; i < dimension; i++)
  {
	Vector<float> point (dimension);
	point.clear ();
	point[i] = separation;
	int g = ktree.kmeans.classify (point);
	unfound[g] = false;

	Vector<float> center (dimension);
	center.clear ();
	for (int j = 0; j < dimension; j++)
	{
	  center += ktree.representative (g * dimension + j);
	}
	center /= dimension;
	if ((point - center).norm (2) > 0.3) throw "KMeansTree cluster in unexpected position";
  }
  if (unfound.norm (1)) throw "KMeansTree cluster is missing";

  // test SVM
  SVM svm;
  svm.run (data, classes);

  if (svm.classCount () != 3) throw "Unexpected class count in SVM";
  if (svm.classify (svm.representative (0)) != 0) throw "SVM did not classify representative of 0 correctly";
  if (svm.classify (svm.representative (1)) != 1) throw "SVM did not classify representative of 1 correctly";
  if (svm.classify (svm.representative (2)) != 2) throw "SVM did not classify representative of 2 correctly";

  Archive ("test.svm", "w") & svm;
  SVM svm2;
  Archive ("test.svm", "r") & svm2;

  int correct = 0;
  for (int i = 0; i < data.size (); i++)
  {
	if (svm2.classify (data[i]) == classes[i]) correct++;
	//cerr << i << " " << data[i] << " " << classes[i] << " " << svm2.classify (data[i]) << " " << svm2.distribution (data[i]) << endl;
  }
  float ratio = (float) correct / data.size ();
  cerr << "ratio = " << ratio << endl;
  if (ratio < 0.99) throw "SVM does not classify enough test points correctly.";

  cout << "ClusterMethods pass" << endl;
}

template<class T>
void
testAll ()
{
  testSearch<T> ();
  testOperator<T> ();
  testReshape<T> ();
  testStrided<T> ();
  testNorm<T> ();
  testClear<T> ();
  testSumSquares<T> ();
  testDot<T> ();
# ifdef HAVE_LAPACK
  testLAPACK<T> ();
# endif
# ifdef HAVE_FFTW
  testFourier<T> ();
# endif
}


int
main (int argc, char * argv[])
{
  try
  {
	cout << "running all tests for float" << endl;
	testAll<float> ();
	testCluster ();  // right now, ClusterMethod is only in float

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
