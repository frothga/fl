#include "fl/point.h"


using namespace std;
using namespace fl;


// class Point ----------------------------------------------------------------

float Point::zero = 0;
float Point::one  = 1;

Point::Point ()
{
  x = 0;
  y = 0;
}

Point::Point (float x, float y)
{
  this->x = x;
  this->y = y;
}

Point::Point (const MatrixAbstract<float> & A)
{
  x = A[0];
  y = A[1];
}

Point::Point (const MatrixAbstract<double> & A)
{
  x = A[0];
  y = A[1];
}

Point::Point (std::istream & stream)
{
  read (stream);
}

/*
Point::operator Vector<float> ()
{
  Vector<float> result (3);
  result[0] = x;
  result[1] = y;
  result[2] = 1;
}

Point::operator Vector<double> ()
{
  Vector<double> result (3);
  result[0] = x;
  result[1] = y;
  result[2] = 1;
}
*/

float &
Point::operator () (const int row, const int column) const
{
  switch (row)
  {
    case 0:
	  return const_cast<float &> (x);
    case 1:
	  return const_cast<float &> (y);
    case 2:
	  return const_cast<float &> (one);
    default:
	  return const_cast<float &> (zero);
  }
}

float &
Point::operator [] (const int row) const
{
  switch (row)
  {
    case 0:
	  return const_cast<float &> (x);
    case 1:
	  return const_cast<float &> (y);
    case 2:
	  return const_cast<float &> (one);
    default:
	  return const_cast<float &> (zero);
  }
}

int
Point::rows () const
{
  return 2;
}

int
Point::columns () const
{
  return 1;
}

MatrixAbstract<float> *
Point::duplicate () const
{
  return new Point (x, y);
}

void
Point::resize (const int rows, const int columns)
{
  if (columns != 1  ||  rows != 2)
  {
	throw "Can't resize Point";
  }
}

void
Point::read (std::istream & stream)
{
  stream.read ((char *) &x, sizeof (x));
  stream.read ((char *) &y, sizeof (y));
}

void
Point::write (std::ostream & stream, bool withName)
{
  stream.write ((char *) &x, sizeof (x));
  stream.write ((char *) &y, sizeof (y));
}

float
Point::distance (const Point & that) const
{
  float dx = that.x - x;
  float dy = that.y - y;
  return sqrtf (dx * dx + dy * dy);
}

float
Point::angle (const Point & that) const
{
  float dx = that.x - x;
  float dy = that.y - y;
  return atan2f (dy, dx);
}


// class PointInterest --------------------------------------------------------

PointInterest::PointInterest ()
: Point ()
{
  weight = 0;
  scale = 1;
}

PointInterest::PointInterest (const Point & p)
: Point (p)
{
  weight = 0;
  scale = 1;
}

PointInterest::PointInterest (std::istream & stream)
{
  read (stream);
}

void
PointInterest::read (std::istream & stream)
{
  Point::read (stream);
  stream.read ((char *) &weight, sizeof (weight));
  stream.read ((char *) &scale,  sizeof (scale));
}

void
PointInterest::write (std::ostream & stream, bool withName)
{
  Point::write (stream);
  stream.write ((char *) &weight, sizeof (weight));
  stream.write ((char *) &scale,  sizeof (scale));
}


// class PointInterestAffine -------------------------------------------------

PointInterestAffine::PointInterestAffine ()
: PointInterest ()
{
  A.identity ();
  angle = 0;
}

PointInterestAffine::PointInterestAffine (const Point & p)
: PointInterest (p)
{
  A.identity ();
  angle = 0;
}

PointInterestAffine::PointInterestAffine (const PointInterest & p)
: PointInterest (p)
{
  A.identity ();
  angle = 0;
}

PointInterestAffine::PointInterestAffine (std::istream & stream)
{
  read (stream);
}

void
PointInterestAffine::read (std::istream & stream)
{
  PointInterest::read (stream);
  A.read (stream);
  stream.read ((char *) &angle, sizeof (angle));
}

void
PointInterestAffine::write (std::ostream & stream, bool withName)
{
  PointInterest::write (stream);
  A.write (stream, false);
  stream.write ((char *) &angle, sizeof (angle));
}
