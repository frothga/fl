/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef point_h
#define point_h


#include "fl/matrix.h"


namespace fl
{
  /**
	 A point in a pixel raster.

	 Point uses float values for x and y, so we define the
	 following convention for interpreting the fractional part of a pixel
	 coordinate:  The image coordinate system starts in the upper left
	 corner.  Positive x goes to the right, and positive y goes down.
	 Integer pixel coordinates (eg: 0, 1.0, 2.0, etc.) refer to the
	 center of the pixel.  IE: a pixel begins at -0.5 and ends at 0.5.
	 This convention allows easy conversion between ints and floats
	 that is also geometrically consistent and valid.  The only thing
	 it makes more difficult is determining the edges and center of an
	 image, usually one-time calculations.
  **/
  class Point : public MatrixAbstract<float>
  {
  public:
	Point ();
	Point (float x, float y);
	template<class T>
	Point (const MatrixAbstract<T> & A)
	{
	  x = A[0];
	  y = A[1];
	}
	Point (std::istream & stream);

	virtual MatrixAbstract<float> * clone (bool deep = false) const;

	virtual float & operator () (const int row, const int column) const
	{
	  return const_cast<float &> ((&x)[row]);
	}
	virtual float & operator [] (const int row) const
	{
	  return const_cast<float &> ((&x)[row]);
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual void resize (const int rows, const int columns = 1);  ///< We only have one size.  This will throw an exception if (rows * columns) != 2.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	virtual MatrixResult<float> homogeneous () const;

	float distance (const Point & that) const;  ///< Euclidean distance between two points
	float angle (const Point & that) const;  ///< Determines angle of vector (that - this)
	float angle () const;  ///< Determines angle of vector from origin to this point.

	float x;
	float y;
  };

  class PointInterest : public Point
  {
  public:
	PointInterest ();
	PointInterest (const Point & p);
	PointInterest (std::istream & stream);
	virtual ~PointInterest ();

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	float weight;  ///< strength of response of interest operator
	float scale;  ///< "characteristic scale" of image around interest point

	enum DetectorType
	{
	  Unknown,
	  Corner,
	  Blob,
	  MSER
	};
	DetectorType detector;  ///< which interest point detector found this point?

	bool operator < (const PointInterest & that) const
	{
	  return weight < that.weight;
	}
  };

  class PointAffine : public PointInterest
  {
  public:
	PointAffine ();
	PointAffine (const Point & p);
	PointAffine (const PointInterest & p);
	PointAffine (std::istream & stream);
	PointAffine (const Matrix<double> & S);  ///< Constructs from a patch matrix.  S = ! this->rectification()

	Matrix<double> rectification () const;  ///< Computes the 3x3 (affine) homography from the image patch indicated by this point to the normalized form.
	Matrix<double> projection () const;  ///< Computes the 3x3 (affine) homography from the normalized form back into the image patch indicated by this point.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	/**
	   The matrix A is the 2x2 transformation from a rectified patch back to
	   the original image.  It is the same as the "U" matrix in Krystian
	   Mikolajczyk's paper "An affine invariant interest point detector".
	**/
	MatrixFixed<double,2,2> A;
	float angle;  ///< characteristic angle; generally the direction of the gradient.
  };

  class PointMSER : public PointAffine
  {
  public:
	PointMSER ();
	PointMSER (const Point & p);
	PointMSER (const PointInterest & p);
	PointMSER (const PointAffine & p);
	PointMSER (int index, unsigned char threshold, bool sign = true);
	PointMSER (std::istream & stream);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	int index;  ///< of a pixel actually inside the region.  The (x,y) value inherited from PointAffine may not satisfy this property.  index translates to a pixel value as (index % width, index / width).
	unsigned char threshold;  ///< gray-level value
	bool sign;  ///< true means threshold is upper bound on intensity (ie: this is an MSER+); false means lower bound (MSER-)
  };
}


#endif
