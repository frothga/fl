#include "fl/point.h"


using namespace std;
using namespace fl;


// class Point ----------------------------------------------------------------

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

Point::Point (std::istream & stream)
{
  read (stream);
}

/*
Point::operator Vector<double> () const
{
  Vector<double> result (2);
  result[0] = x;
  result[1] = y;
  return result;
}
*/

Vector<float>
Point::homogenous (float third) const
{
  Vector<float> result (3);
  result[0] = x;
  result[1] = y;
  result[2] = third;
  return result;
}

Vector<float>
Point::homogenous (float third, float fourth) const
{
  Vector<float> result (3);
  result[0] = x;
  result[1] = y;
  result[2] = third;
  result[3] = fourth;
  return result;
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
  detector = Unknown;
}

PointInterest::PointInterest (const Point & p)
: Point (p)
{
  weight = 0;
  scale = 1;
  detector = Unknown;
}

PointInterest::PointInterest (std::istream & stream)
{
  read (stream);
}

void
PointInterest::read (std::istream & stream)
{
  Point::read (stream);
  stream.read ((char *) &weight,   sizeof (weight));
  stream.read ((char *) &scale,    sizeof (scale));
  stream.read ((char *) &detector, sizeof (detector));
}

void
PointInterest::write (std::ostream & stream, bool withName)
{
  Point::write (stream);
  stream.write ((char *) &weight,   sizeof (weight));
  stream.write ((char *) &scale,    sizeof (scale));
  stream.write ((char *) &detector, sizeof (detector));
}


// class PointAffine ----------------------------------------------------------

PointAffine::PointAffine ()
: PointInterest ()
{
  A.identity ();
  angle = 0;
}

PointAffine::PointAffine (const Point & p)
: PointInterest (p)
{
  A.identity ();
  angle = 0;
}

PointAffine::PointAffine (const PointInterest & p)
: PointInterest (p)
{
  A.identity ();
  angle = 0;
}

PointAffine::PointAffine (std::istream & stream)
{
  read (stream);
}

void
PointAffine::read (std::istream & stream)
{
  PointInterest::read (stream);
  A.read (stream);
  stream.read ((char *) &angle, sizeof (angle));
}

void
PointAffine::write (std::ostream & stream, bool withName)
{
  PointInterest::write (stream);
  A.write (stream, false);
  stream.write ((char *) &angle, sizeof (angle));
}
