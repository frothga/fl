/*
Author: Fred Rothganger

Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#define __STDC_LIMIT_MACROS

#include <fl/parms.h>
#include <fl/vectorsparse.h>
#include <fl/serialize.h>

#include <iostream>
#include <fstream>


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

class A
{
public:
  A () {}
  virtual void read (istream & stream) {}
  virtual void write (ostream & stream) const {}
  virtual void serialize (Archive & a, uint32_t version)
  {
	cerr << "A version " << version << endl;
	a & number;
	a & name;
  }
  static uint32_t serializeCurrentVersion ()
  {
	return 1;
  }

  // Some basic types to serialize
  int32_t number;
  string name;
};

class B : public A
{
public:
  B () {}
};

class C : public A
{
public:
  C () {}
};

class D
{
public:
  virtual void serialize (Archive & a, uint32_t version)
  {
	cerr << "D version " << version << endl;
	a.registerClass<A> ();
	a.registerClass<B> ();
	a.registerClass<C> ("bob");
	a & collection;
  }
  static uint32_t serializeCurrentVersion ()
  {
	return 2;
  }

  bool operator == (const D & that)
  {
	if (collection.size ()  !=  that.collection.size ())
	{
	  cerr << "Collections are different sizes: " << collection.size () << " " << that.collection.size () << endl;
	  return false;
	}
	for (int i = 0; i < collection.size (); i++)
	{
	  A * a = collection[i];
	  A * b = that.collection[i];
	  if (typeid (*a)  !=  typeid (*b))
	  {
		cerr << "types are different: " << typeid (*a).name () << " " << typeid (*b).name () << endl;
		return false;
	  }
	  if (a->name != b->name) return false;
	  if (a->number != b->number) return false;
	}

	return true;
  }

  vector<A *> collection;
};

ostream &
operator << (ostream & stream, const D & data)
{
  cerr << typeid (data).name () << endl;
  cerr << data.collection.size () << endl;
  for (int i = 0; i < data.collection.size (); i++)
  {
	A * a = data.collection[i];
	cerr << hex << a << dec << " " << typeid (*a).name () << " " << a->name << " " << a->number << endl;
  }
  return stream;
}

void
testFactory ()
{
  Product<A, A>::add ("a");
  Product<A, B>::add ("b");
  Product<A, C>::add ("c");

  B b;
  ofstream ofs ("testBaseFile", ios::binary);
  Factory<A>::write (ofs, b);
  ofs.close ();

  ifstream ifs ("testBaseFile", ios::binary);
  A * a = Factory<A>::read (ifs);
  if (typeid (*a) != typeid (b))
  {
	cerr << "Unexpected class retrieved from stream" << endl;
	throw "Factory fails";
  }

  cout << "Factory passes" << endl;
}

void
testArchive ()
{
  A * a = new A;
  B * b = new B;
  C * c = new C;
  D before;
  before.collection.push_back (a);
  before.collection.push_back (b);
  before.collection.push_back (c);
  a->name = "a";
  b->name = "b";
  c->name = "c";
  a->number = 1;
  b->number = 2;
  c->number = 3;

  {
	Archive archive ("testBaseFile", "w");
	archive & before;
  }

  {
	Archive archive ("testBaseFile", "r");
	D after;
	archive & after;

	cerr << "before:" << endl << before << endl;
	cerr << "after:" << endl << after << endl;
	if (after == before) cout << "Archive passes" << endl;
	else throw "Archive fails";
  }
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
	//testFactory ();
	testArchive ();
	//testVectorsparse ();
  }
  catch (const char * message)
  {
	cout << "Exception: " << message << endl;
	return 1;
  }

  return 0;
}
