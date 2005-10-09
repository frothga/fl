/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


6/2005 Fred Rothganger -- Added PointMSER.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


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
Point::distance () const
{
  return sqrtf (x * x + y * y);
}

float
Point::angle (const Point & that) const
{
  float dx = that.x - x;
  float dy = that.y - y;
  return atan2f (dy, dx);
}

float
Point::angle () const
{
  return atan2f (y, x);
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

PointInterest::~PointInterest ()
{
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

PointAffine::PointAffine (const Matrix<double> & S)
{
  x = S(0,2);
  y = S(1,2);
  scale = sqrt (S(0,0) * S(1,1) - S(1,0) * S(0,1));
  A = S;  // takes upper-left 2x2 region
  A /= scale;
  angle = 0;  // do not attempt to separate rotation from rest of transformation
}

Matrix<double>
PointAffine::rectification () const
{
  Matrix<double> R (3, 3);
  R.identity ();
  R(0,0) = cos (- angle);
  R(0,1) = -sin (- angle);
  R(1,0) = -R(0,1);
  R(1,1) = R(0,0);

  Matrix<double> A (3, 3);
  MatrixRegion<double> M (A, 0, 0, 1, 1);
  M = ! this->A / scale;
  A.region (0, 2, 1, 2) = ((M * (*this)) *= -1);
  A(2,0) = 0;
  A(2,1) = 0;
  A(2,2) = 1;

  return R * A;
}

Matrix<double>
PointAffine::projection () const
{
  Matrix<double> R (2, 2);
  R(0,0) = cos (angle);
  R(0,1) = -sin (angle);
  R(1,0) = -R(0,1);
  R(1,1) = R(0,0);

  R *= scale;

  Matrix<double> result (3, 3);
  result.region (0, 0) = A * R;
  result(0,2) = x;
  result(1,2) = y;
  result(2,0) = 0;
  result(2,1) = 0;
  result(2,2) = 1;

  return result;
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


// class PointMSER ------------------------------------------------------------

PointMSER::PointMSER ()
{
  index = -1;
  threshold = 0;
  sign = true;
}

PointMSER::PointMSER (const Point & p)
{
  index = -1;
  threshold = 0;
  sign = true;
}

PointMSER::PointMSER (const PointInterest & p)
{
  index = -1;
  threshold = 0;
  sign = true;
}

PointMSER::PointMSER (const PointAffine & p)
: PointAffine (p)
{
  index = -1;
  threshold = 0;
  sign = true;
}

PointMSER::PointMSER (int index, unsigned char threshold, bool sign)
{
  this->index     = index;
  this->threshold = threshold;
  this->sign      = sign;
}

PointMSER::PointMSER (std::istream & stream)
{
  read (stream);
}

void
PointMSER::read (std::istream & stream)
{
  PointAffine::read (stream);
  stream.read ((char *) &index,     sizeof (index));
  stream.read ((char *) &threshold, sizeof (threshold));
  stream.read ((char *) &sign,      sizeof (sign));
}

void
PointMSER::write (std::ostream & stream, bool withName)
{
  PointAffine::write (stream);
  stream.write ((char *) &index,     sizeof (index));
  stream.write ((char *) &threshold, sizeof (threshold));
  stream.write ((char *) &sign,      sizeof (sign));
}
