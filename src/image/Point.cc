/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/point.h"


using namespace std;
using namespace fl;


// class Point ----------------------------------------------------------------

uint32_t Point::serializeVersion = 0;

Point::Point ()
{
  x = 0;
  y = 0;
}

Point::Point (double x, double y)
{
  this->x = x;
  this->y = y;
}

MatrixAbstract<double> *
Point::clone (bool deep) const
{
  return new Point (x, y);
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

void
Point::resize (const int rows, const int columns)
{
  if (columns != 1  ||  rows != 2)
  {
	throw "Can't resize Point";
  }
}

void
Point::serialize (Archive & archive, uint32_t version)
{
  archive & x;
  archive & y;
}

MatrixResult<double>
Point::homogeneous () const
{
  Vector<double> * result = new Vector<double> (3);
  (*result)[0] = x;
  (*result)[1] = y;
  (*result)[2] = 1;
  return result;
}

double
Point::distance (const Point & that) const
{
  double dx = that.x - x;
  double dy = that.y - y;
  return sqrt (dx * dx + dy * dy);
}

double
Point::angle (const Point & that) const
{
  double dx = that.x - x;
  double dy = that.y - y;
  return atan2 (dy, dx);
}

double
Point::angle () const
{
  return atan2 (y, x);
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

PointInterest::~PointInterest ()
{
}

namespace fl
{
  template<>
  Archive &
  Archive::operator & (PointInterest::DetectorType & detector)
  {
	if (in) in ->read  ((char *) &detector, sizeof (detector));
	else    out->write ((char *) &detector, sizeof (detector));
	return *this;
  }
}

void
PointInterest::serialize (Archive & archive, uint32_t version)
{
  archive & *((Point *) this);
  archive & weight;
  archive & scale;
  archive & detector;
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
  A.region (0, 2, 1, 2) = ((M * Vector<double> (*this)) *= -1);
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
PointAffine::serialize (Archive & archive, uint32_t version)
{
  archive & *((PointInterest *) this);
  archive & A;
  archive & angle;
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

void
PointMSER::serialize (Archive & archive, uint32_t version)
{
  archive & *((PointAffine *) this);
  archive & index;
  archive & threshold;
  archive & sign;
}
