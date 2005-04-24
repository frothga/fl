/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#include "fl/complex.h"
#include "fl/matrix.h"
#include "fl/random.h"
#include "fl/search.h"
#include "fl/lapackd.h"
#include "fl/time.h"
#include "fl/zroots.h"
#include "fl/pi.h"
#include "fl/neural.h"



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







inline Matrix2x2<double>
rotationMatrix (const double angle)
{
  Matrix2x2<double> R;
  R(0,0) = cos (angle);
  R(0,1) = -sin (angle);
  R(1,0) = -R(0,1);
  R(1,1) = R(0,0);
  return R;
}

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








class NeuronInput : public NeuronBackprop
{
public:
  virtual float getOutput ()
  {
	if (line[position] == code)
	{
	  return 1.0;
	}
	else if (line[position] == '?')
	{
	  return unknownValue;
	}
	else
	{
	  return 0;
	}
  }

  char * line;
  int position;
  char code;
  float unknownValue;
};

class NeuronOutput : public NeuronBackprop
{
public:
  virtual float getDelta ()
  {
	float desired = line[0] == code ? 0.9f : -0.9f;
	delta = desired - getOutput ();
	return delta;
  }

  char * line;
  char code;
};

class TestNN : public NeuralNetworkBackprop
{
public:
  TestNN ()
  {
	char * values[] =
	{
	  "bcxfks",
	  "fgys",
	  "nbcgrpuewy",
	  "tf",
	  "alcyfmnps",
	  "adfn",
	  "cwd",
	  "bn",
	  "knbhgropuewy",
	  "et",
	  "bcuezr",  // also includes "?", but handled differently
	  "fyks",
	  "fyks",
	  "nbcgopewy",
	  "nbcgopewy",
	  "pu",
	  "nowy",
	  "not",
	  "ceflnpsz",
	  "knbhrouwy",
	  "acnsvy",
	  "glmpuwd"
	};

	for (int j = 0; j < 22; j++)
	{
	  for (int k = 0; values[j][k]; k++)
	  {
		NeuronInput * n = new NeuronInput;
		n->line         = buffer;
		n->position     = (j + 1) * 2;
		n->code         = values[j][k];
		n->unknownValue = 1.0 / strlen (values[j]);
		inputs.push_back (n);
	  }
	}

	NeuronOutput * e = new NeuronOutput;
	e->line = buffer;
	e->code = 'e';
	outputs.push_back (e);
	NeuronOutput * p = new NeuronOutput;
	p->line = buffer;
	p->code = 'p';
	outputs.push_back (p);

	vector<int> hiddenSizes;
	//hiddenSizes.push_back (40);
	//hiddenSizes.push_back (5);
	constructHiddenLayers (hiddenSizes);
  }

  virtual void startData ()
  {
	index = 0;
  }

  virtual bool nextDatum ()
  {
	if (index == dataCount)
	{
	  return false;
	}
	strcpy (buffer, data[index++]);
	return true;
  }

  virtual bool correct ()
  {
	return fabsf (outputs[0]->getDelta ()) < 0.4f  &&  fabsf (outputs[1]->getDelta ()) < 0.4f;
  }

  virtual void happyGraph (int iteration, float accuracy)
  {
	cerr << iteration << " " << accuracy << endl;
  }

  int index;
  int dataCount;
  char * data[9000];
  char buffer[80];
};











int
main (int argc, char * argv[])
{
  #define parmFloat(n,d) (argc > n ? atof (argv[n]) : (d))
  #define parmInt(n,d) (argc > n ? atoi (argv[n]) : (d))
  #define parmChar(n,d) (argc > n ? argv[n] : (d))


  int seed = parmInt (3, time (NULL));
  srand (seed);
  cerr << "Random seed = " << seed << endl;

  //int m = parmInt (1, 4);  // rows
  //int n = parmInt (2, 4);  // columns




  // Construct the NN
  srand ((unsigned) time (NULL));
  TestNN NN;

  char buffer[80];
  char * testset[9000];

  // Load data
  NN.dataCount = 0;
  ifstream trainStream ("trainset");
  while (! trainStream.eof ())
  {
	trainStream.getline (buffer, sizeof (buffer));
	if (strlen (buffer) > 10)
	{
	  NN.data[NN.dataCount++] = strdup (buffer);
	}
  }
  trainStream.close ();

  int testCount = 0;
  ifstream testStream ("testset");
  while (! testStream.eof ())
  {
	testStream.getline (buffer, sizeof (buffer));
	if (strlen (buffer) > 10)
	{
	  testset[testCount++] = strdup (buffer);
	}
  }
  testStream.close ();
  cerr << "testset size = " << testCount << endl;

  // Train
  NN.train ();

  // Test
  int accuracy = 0;
  for (int i = 0; i < testCount; i++)
  {
	strcpy (NN.buffer, testset[i]);
	NN.reset ();
	if (NN.correct ()) accuracy++;
  }
  cout << "Test accuracy: " << accuracy << " (" << (float) accuracy / testCount << ")" << endl;

  return 0;





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
