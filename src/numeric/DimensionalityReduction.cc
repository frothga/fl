#include "fl/reduce.h"


using namespace std;
using namespace fl;


void
DimensionalityReduction::read (istream & stream)
{
}

void
DimensionalityReduction::write (ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }
}
