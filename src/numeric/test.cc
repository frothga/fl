#include "fl/matrix.h"
#include "fl/random.h"
#include "fl/search.h"
#include "fl/lapackd.h"
#include "fl/time.h"
#include "fl/complex.h"

//#include "fl/MatrixSparse.tcc"


#include <iostream>
#include <fstream>


using namespace std;
using namespace fl;


/*  This class structure demonstrates the overloaded function resolution
	"problem".  If you write:
	B a;
	B b;
	a * b;
	You will get a compile error.

class A
{
public:
  void operator * (const A & b) const;
};

class B : public A
{
public:
  void operator * (float f) const;
};
*/


// Searchable class for testing numeric search methods
// Expected result = 0.08241057  1.133037  2.343695
//class Test : public SearchableSparse
class Test : public SearchableNumeric
{
public:
  /*
  Test () : SearchableSparse (1e-3)
  {
	cover ();

	cerr << "after cover" << endl;
	cerr << parameters << endl;
	for (int i = 0; i < parms.size (); i++)
	{
	  for (int j = 0; j < parms[i].size (); j++)
	  {
		cerr << i << " " << j << " " << parms[i][j] << endl;
	  }
	}
  }
  */

  virtual int dimension ()
  {
	return 15;
  }

  virtual void value (const Vector<double> & x, Vector<double> & result)
  {
	const double y[] = {0.14, 0.18, 0.22, 0.25, 0.29, 0.32, 0.35, 0.39,
						0.37, 0.58, 0.73, 0.96, 1.34, 2.10, 4.39};

	result.resize (15);
	for (int i = 0; i < 15; i++)
	{
	  double t0 = i + 1;
	  double t1 = 15 - i;
	  double t2 = t0;
	  if (i > 7)
	  {
		t2 = t1;
	  }
	  result[i] = y[i] - (x[0] + t0 / (x[1] * t1 + x[2] * t2));
	}
	cerr << ".";
  }

  virtual MatrixSparse<int> interaction ()
  {
	MatrixSparse<int> result (15, 3);

	for (int i = 0; i < 15; i++)
	{
	  for (int j = 0; j < 3; j++)
	  {
		if (randfb () > 0.75)
		{
		  result.set (i, j, 1);
		}
	  }
	}
	cerr << "i=" << result << endl;

	return result;
  }
};








inline double
randbad ()
{
  double sign = randfb ();
  double a = fabs (sign);
  sign /= a;
  double b = 1 * randfb ();
  double result = sign * pow (a, b);
  return result;
}

inline Matrix<double>
makeMatrix (const int m, const int n)
{
  Matrix<double> A (m, n);
  for (int r = 0; r < m; r++)
  {
	for (int c = 0; c < n; c++)
	{
	  //A(r,c) = randbad ();
	  A(r,c) = randfb ();
	}
  }
  //cerr << A << endl << endl;

  return A;
}


int
main (int argc, char * argv[])
{
  #define parmFloat(n,d) (argc > n ? atof (argv[n]) : (d))
  #define parmInt(n,d) (argc > n ? atoi (argv[n]) : (d))
  #define parmChar(n,d) (argc > n ? argv[n] : (d))


  int seed = parmInt (3, time (NULL));
  srand (seed);
  cerr << "Random seed = " << seed << endl;

  int m = parmInt (1, 4);  // rows
  int n = parmInt (2, 4);  // columns



  Matrix2x2<float> A = makeMatrix (2, 2);
  cerr << "A=" << A << endl << endl;

  Matrix3x3<float> B;
  B.identity ();
  cerr << "B=" << B << endl << endl;
  cerr << "region=" << B.region (0, 0, 1, 1) << endl << endl;
  cerr << &B << endl;

  B.region (0, 0, 1, 1) = A;
  cerr << "region=" << B.region (0, 0, 1, 1) << endl << endl;


  /*
  //AnnealingAdaptive method;
  //LevenbergMarquardt method;
  LevenbergMarquardtMinpack method;
  //LevenbergMarquardtSparseBK method;
  Test t;
  Vector<double> point (3);
  point[0] = 1;
  point[1] = 1;
  point[2] = 1;
  method.search (t, point);
  cerr << point << endl;
  */
}
