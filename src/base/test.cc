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
#include <fl/archive.h>
#include <fl/factory.h>

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
  void serialize (Archive & a, uint32_t version)
  {
	a & number;
	a & name;
  }

  virtual bool operator == (const A & that)
  {
	if (typeid (*this) != typeid (that)) return false;
	if (number != that.number) return false;
	if (name != that.name) return false;
	return true;
  }

  static uint32_t serializeVersion;

  // Some basic types to serialize
  int number;
  string name;
};
uint32_t A::serializeVersion = 0;

class B : public A
{
public:
  B () {}
};

class C : public A
{
public:
  C ()
  {
	restoredVersion = serializeVersion;
  }
  void serialize (Archive & a, uint32_t version)
  {
	a & *((A *) this);
	if (version == 0) cerr << "not serializing 'truth'" << endl;
	else
	{
	  cerr << "serializing 'truth'" << endl;
	  a & truth;
	}
	restoredVersion = version;
  }
  virtual bool operator == (const A & that)
  {
	if (! A::operator == (that)) return false;
	cerr << "C::operator==  versions " << restoredVersion << " " << ((C *) &that)->restoredVersion << endl;
	if (restoredVersion == 0  ||  ((C *) &that)->restoredVersion == 0)
	{
	  cerr << "not comparing truth, because one of the objects is old version" << endl;
	  return true;
	}
	if (truth != ((C*) &that)->truth) return false;
	return true;
  }

  static uint32_t serializeVersion;

  uint32_t restoredVersion;
  bool truth;
};
uint32_t C::serializeVersion = 0;

class D
{
public:
  void serialize (Archive & archive, uint32_t version)
  {
	archive.registerClass<A> ();
	archive.registerClass<B> ();
	archive.registerClass<C> ("bob");
	archive.registerClass<C> ("sam");

	archive & collection;
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
	  if (a == 0  &&  b == 0) return true;
	  if (a == 0  ||  b == 0) return false;
	  if (! (*a == *b)) return false;
	}
	return true;
  }

  static uint32_t serializeVersion;
  vector<A *> collection;
};
uint32_t D::serializeVersion = 0;

ostream &
operator << (ostream & stream, const D & data)
{
  cerr << typeid (data).name () << endl;
  cerr << data.collection.size () << endl;
  for (int i = 0; i < data.collection.size (); i++)
  {
	A * a = data.collection[i];
	cerr << hex << a << dec;
	if (a) cerr << " " << typeid (*a).name () << " " << a->name << " " << a->number << endl;
	else cerr << endl;
  }
  return stream;
}

void
testFactory ()
{
  Factory<A>::add<A> ("a");
  Factory<A>::add<B> ("b");
  Factory<A>::add<C> ("c");

  A * a = Factory<A>::create ("b");
  if (typeid (*a) != typeid (B))
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
  before.collection.push_back (0);
  a->name = "a";
  b->name = "b";
  c->name = "c";
  a->number = 1;
  b->number = 2;
  c->number = 3;
  c->truth = false;

  cerr << "testing basic Achive" << endl;
  {
	Archive archive ("testBaseFile", "w");
	archive & before;

	if (archive.alias.size () != 4) throw "Unexpected number of aliases";
  }

  {
	Archive archive ("testBaseFile", "r");
	D after;
	archive & after;

	cerr << "before:" << endl << before << endl;
	cerr << "after:" << endl << after << endl;
	if (! (after == before)) throw "Archive fails";
  }

  cerr << "testing versioned Archive" << endl;
  {
	Archive archive ("testBaseFile", "w");
	uint32_t oldVersion = C::serializeVersion;
	C::serializeVersion = 1;
	archive & before;
	C::serializeVersion = oldVersion;
  }

  {
	Archive archive ("testBaseFile", "r");
	D after;
	archive & after;

	if (! (after == before)) throw "Archive fails";
  }

  cout << "Archive passes" << endl;
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
	testParameters (argc, argv);
	testFactory ();
	testArchive ();
	testVectorsparse ();
  }
  catch (const char * message)
  {
	cout << "Exception: " << message << endl;
	return 1;
  }

  return 0;
}
