/*
Author: Fred Rothganger
Created 01/30/2006 to provide a general interface for measuring distances
in R^n, and to help separate numeric and image libraries.
*/


#include "fl/metric.h"
#include "fl/factory.h"


using namespace fl;
using namespace std;


// class Metric ---------------------------------------------------------------

template class Factory<Metric>;
template <> Factory<Metric>::productMap Factory<Metric>::products;

Metric::~Metric ()
{
}

void
Metric::read (istream & stream)
{
}

void
Metric::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
