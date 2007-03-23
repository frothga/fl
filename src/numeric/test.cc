/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revision  1.4         Copyright 2005 Sandia Corporation.
Revisions 1.6 and 1.7 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.6  2006/02/17 18:52:53  Fred
Test operand preservation and residual calculation in gels*().

Revision 1.5  2005/10/13 04:18:26  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.4  2005/10/13 04:14:25  Fred
Numerous miscellaneous changes.  The most recent are various tests for updated
LAPACK bridge.

Revision 1.3  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.2  2004/04/06 21:28:43  rothgang
Test neural networks.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/zroots.h"
#include "fl/matrix.h"
#include "fl/random.h"
#include "fl/search.h"
#include "fl/lapack.h"
#include "fl/time.h"
#include "fl/pi.h"
#include "fl/neural.h"
#include "fl/factory.h"
#include "fl/reduce.h"
#include "fl/cluster.h"

#include <iostream>
#include <fstream>
#include <complex>


using namespace std;
using namespace fl;


// Searchable class for testing numeric search methods
// Expected result = 0.08241057  1.133037  2.343695
template<class T>
//class Test : public SearchableSparse
class Test : public SearchableNumeric<T>
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
	  //A(r,c) = randfb ();
	  A(r,c) = randf ();
	}
  }
  //cerr << A << endl << endl;

  return A;
}

void
throwPoint (const Vector<float> & center, const float radius, vector< Vector<float> > & data)
{
  Vector<float> point (center.rows ());
  for (int i = 0; i < point.rows (); i++)
  {
	point[i] = randGaussian (); // * (i ? 1 : 2);
  }

  point.normalize (radius);
  point += center;

  data.push_back (point);
}

void
makeClusters (int clusters, int dimension, vector< Vector<float> > & data)
{
  vector< Vector<float> > center;
  for (int i = 0; i < clusters; i++)
  {
	Vector<float> point (dimension);
	point.clear ();
	point[i] = 1;
	center.push_back (point);
  }

  int iterations = dimension * dimension;  // We need d + d * (d + 1) / 2 data points per cluster to do k-means.  We get iterations * centers data points.
  float radius = 0.1;
  for (int i = 0; i < iterations; i++)
  {
    for (int j = 0; j < center.size (); j++)
	{
	  throwPoint (center[j], radius, data);
    }
  }
}

void
regress (vector<float> & data, float & ave, float & std)
{
  ave = 0;
  for (int i = 0; i < data.size (); i++)
  {
	ave += data[i];
  }
  ave /= data.size ();

  std = 0;
  for (int i = 0; i < data.size (); i++)
  {
	float d = data[i] - ave;
	std += d * d;
  }
  std /= data.size ();
}

void
regress (vector<Vector<float> > & data, Vector<float> & ave, Matrix<float> & std)
{
  int dimension = data[0].rows ();
  ave.resize (dimension);
  ave.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	ave += data[i];
  }
  ave /= data.size ();

  std.resize (dimension, dimension);
  std.clear ();
  for (int i = 0; i < data.size (); i++)
  {
	Vector<float> d = data[i] - ave;
	std += d * ~d;
  }
  std /= data.size ();
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

  Matrix<float> A = makeMatrix (m, n);
  Matrix<float> b = makeMatrix (m, 3);

  Matrix<float> x;
  float residual;
  gelss (A, x, b, &residual);
  cerr << "x by gelss:" << endl << x << endl;
  cerr << "residual = " << residual << endl;

  gelsd (A, x, b, &residual, true, true);
  cerr << "x by gelsd:" << endl << x << endl;
  cerr << "residual = " << residual << endl;



  /*
  vector<Vector<float> > data1;
  for (int i = 0; i < 1000; i++)
  {
	Vector<float> point (2);
	point[0] = randGaussian ();
	point[1] = randGaussian ();
	point *= 2;
	data1.push_back (point);
  }
  Vector<float> ave1;
  Matrix<float> std1;
  regress (data1, ave1, std1);
  cerr << "ave1 = " << ave1 << endl;
  cerr << "std1: " << endl << std1 << endl;

  vector<Vector<float> > data2;
  for (int i = 0; i < 100; i++)
  {
	Vector<float> point (2);
	point[0] = randGaussian () + 4;
	point[1] = randGaussian () - 3;
	data2.push_back (point);
  }
  Vector<float> ave2;
  Matrix<float> std2;
  regress (data2, ave2, std2);
  cerr << "ave2 = " << ave2 << endl;
  cerr << "std2: " << endl << std2 << endl;

  Vector<float> ave;
  Matrix<float> std;
  vector<Vector<float> > data = data1;
  data.insert (data.end (), data2.begin (), data2.end ());
  regress (data, ave, std);
  cerr << "ave = " << ave << endl;
  cerr << "std: " << endl << std << endl;


  float w1 = data1.size ();
  float w2 = data2.size ();
  float w = w1 + w2;
  Vector<float> ave3 = (ave1 * w1 + ave2 * w2) / w;
  cerr << "weighted mean = " << ave3 << endl;
  Matrix<float> shift1 = ave1 - ave3;
  Matrix<float> shift2 = ave2 - ave3;
  shift1 *= ~shift1;
  shift2 *= ~shift2;
  Matrix<float> std3 = ((std1 + shift1) * w1 + (std2 + shift2) * w2) / w;
  cerr << "std3 = " << endl << std3 << endl;
  */


  /*
  AnnealingAdaptive<double> method;
  //LevenbergMarquardt<float> method;
  //LevenbergMarquardtSparseBK<float> method;
  Test<double> t;
  Vector<double> point (3);
  point[0] = 1;
  point[1] = 1;
  point[2] = 1;
  method.search (t, point);
  cerr << point << endl;
  */
}
