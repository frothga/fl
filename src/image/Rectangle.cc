/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/point.h"


using namespace std;
using namespace fl;


// class Retangle -------------------------------------------------------------

Rectangle::Rectangle (int D)
: a (D),
  b (D)
{
  clear ();
}

Rectangle::Rectangle (const Vector<double> & a, const Vector<double> & b)
{
  this->a.copyFrom (a);
  this->b.copyFrom (b);
}

Rectangle::Rectangle (const string & Astring)
{
  Matrix<double> A (Astring);
  a = ~A.row (0);
  b = ~A.row (1);
}

void
Rectangle::clear ()
{
  a.clear ( INFINITY);
  b.clear (-INFINITY);
}

void
Rectangle::copyFrom (const Rectangle & that)
{
  a.copyFrom (that.a);
  b.copyFrom (that.b);
}

Rectangle
Rectangle::intersect (const Rectangle & that) const
{
  const int D = a.rows ();
  Rectangle result (D);
  for (int i = 0; i < D; i++)
  {
	result.a[i] = max (a[i], that.a[i]);
	result.b[i] = min (b[i], that.b[i]);
  }
  return result;
}

Rectangle
Rectangle::unite (const Rectangle & that) const
{
  const int D = a.rows ();
  Rectangle result (D);
  for (int i = 0; i < D; i++)
  {
	result.a[i] = min (a[i], that.a[i]);
	result.b[i] = max (b[i], that.b[i]);
  }
  return result;
}

bool
Rectangle::empty () const
{
  const int D = a.rows ();
  for (int i = 0; i < D; i++) if (a[i] >= b[i]) return true;
  return false;
}

bool
Rectangle::contains (const MatrixAbstract<double> & point) const
{
  const int D = a.rows ();
  for (int i = 0; i < D; i++)
  {
	double & p = point[i];
	if (p < a[i]  ||  b[i] < p) return false;
  }
  return true;
}

uint32_t Rectangle::serializeVersion = 0;

void
Rectangle::serialize (Archive & archive, uint32_t version)
{
  archive & a;
  archive & b;
}

MatrixResult<double>
Rectangle::size () const
{
  return b - a;
}

const char *
Rectangle::toString (std::string & buffer) const
{
  std::ostringstream stream;
  stream << *this;
  buffer = stream.str ();
  return buffer.c_str ();
}
