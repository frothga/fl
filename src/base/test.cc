#define __STDC_LIMIT_MACROS
#include <iostream>
#include <fl/parms.h>
#include <fl/vectorsparse.h>


using namespace std;
using namespace fl;


void
testParameters (int argc, char * argv[])
{
  Parameters parms (argc, argv);
  string bob = parms.getChar ("bob", "");
  if (bob.size () == 0)
  {
	cerr << "Parameter 'bob' not found.  Please pass 'include={path to}test.parms'" << endl;
	cerr << "on the command line to process the test parameter file." << endl;
	throw "Parameters class fails";
  }
  cout << "Parameters class passes" << endl;
}

inline void
integrity (vectorsparse<int> & v)
{
  //cerr << "size=" << v.contigs.size () << endl;
  for (int i = 0; i < v.contigs.size (); i++)
  {
	volatile vectorsparse<int>::Contig * c = v.contigs[i];
	volatile int index = c->index;
	volatile int count = c->count;
  }
}

const int maxElement = 1000;

inline void
generateRandomVector (vectorsparse<int> & test, vector<int> & truth, const int fillIn)
{
  const int iterations = fillIn * maxElement;
  truth.clear ();
  truth.resize (maxElement, 0);
  for (int i = 0; i < iterations; i++)
  {
	int index = rand () % maxElement;
	bool zero = rand () % fillIn;  // only non-zero 1/fillIn part of the time
	//cerr << index << " = " << (zero ? 0 : index);
	if (zero)
	{
	  truth[index] = 0;
	  test.clear (index);
	}
	else
	{
	  truth[index] = index;
	  test[index] = index;
	}
	cerr << ".";
  }
}

inline void
testVectorsparseStructure (const int fillIn)
{
  cerr << "vectorsparse structural test; fillIn = " << fillIn << endl;
  vectorsparse<int> test;
  vector<int> truth;
  generateRandomVector (test, truth, fillIn);
  cerr << "  Done filling.  Starting integrity check." << endl;
  for (int i = 0; i < maxElement; i++)
  {
	int value = ((const vectorsparse<int> &) test)[i];
	if (truth[i] != value)
	{
	  cerr << "  Unexpected element value: " << value << " at " << i << endl;
	  throw "vectorsparse fails";
	}
  }
}

void
testVectorsparse ()
{
  // Structural torture test
  testVectorsparseStructure (1);
  testVectorsparseStructure (10);
  testVectorsparseStructure (20);
  testVectorsparseStructure (30);

  // copy constructor
  {
	vectorsparse<int> test;
	vector<int> truth;
	generateRandomVector (test, truth, 20);
	vectorsparse<int> test2 (test);
	for (int i = 0; i < maxElement; i++)
	{
	  int value = ((const vectorsparse<int> &) test2)[i];
	  if (truth[i] != value)
	  {
		cerr << "  Unexpected element value: " << value << " at " << i << endl;
		throw "vectorsparse fails";
	  }
	}
  }

  // resize
  

  // Test for const and non-const access
  //   Make some const accesses, and verify that no fill-in has occurred
  //   Make some non-const accesses, and verify fill-in

  // const iterator


  // non-const iterator


  // sparse iterator


}

int main (int argc, char * argv[])
{
  try
  {
	//testParameters (argc, argv);
	testVectorsparse ();
  }
  catch (const char * message)
  {
	cout << "Exception: " << message << endl;
	return 1;
  }

  return 0;
}
