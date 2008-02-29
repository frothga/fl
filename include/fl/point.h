/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.8 thru 1.9 Copyright 2005 Sandia Corporation.
Revisions 1.11         Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.10  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.9  2005/10/09 03:28:51  Fred
Update revision history and add Sandia copyright notice.

Revision 1.8  2005/06/07 04:01:11  Fred
Added PointMSER.  Changed DetectorType names to be more generic, and added MSER
regions to list.

Revision 1.7  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.6  2004/08/30 00:08:45  rothgang
Add projection(), and function similar to rectification() except that it
returns the inverse.  Typically, we do computations on affine patches using
this form.  Eliminates two matrix inversions in the process of converting a
PointAffine into the desired form.

Added a constructor for PointAffine that takes the kind of matrix produced by
projection().

Revision 1.5  2003/12/30 16:23:45  rothgang
Add method to compute rectifying transformation for PointAffine.

Revision 1.4  2003/09/07 22:19:24  rothgang
Convert comments to doxygen format.  Add forms of distance() and angle() that
compute relative to origin rather than a given point.

Revision 1.3  2003/08/11 13:48:49  rothgang
Added a detector type of Unknown.

Revision 1.2  2003/07/09 15:03:15  rothgang
Modified Point to work more easily with other matrix classes.  (Still needs
work in this regard.)  Added field to PointInterest to remember what type of
detector it came from.  Changed name of PointInterestAffine to PointAffine in
anticipation of adding Point classes specific to detector type.  However,
didn't proceed any further in developing the specific classes because the only
way they would be useful would be to store all PointInterests as separate heap
objects, which is "heavy".

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
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

	//operator Vector<double> () const;
	Vector<float> homogenous (float third) const;
	Vector<float> homogenous (float third, float fourth) const;

	virtual float & operator () (const int row, const int column) const
	{
	  return const_cast<float *> (&x) [row];
	}
    virtual float & operator [] (const int row) const
	{
	  return const_cast<float *> (&x) [row];
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<float> * duplicate () const;
	virtual void resize (const int rows, const int columns = 1);  ///< We only have one size.  This will throw an exception if (rows * columns) != 2.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);  ///< withName is ignored, and no class id is ever written for Points

	float distance (const Point & that) const;  ///< Euclidean distance between two points
	float distance () const;  ///< Euclidean distance from origin.
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
	virtual void write (std::ostream & stream, bool withName = false);

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
	virtual void write (std::ostream & stream, bool withName = false);

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
	virtual void write (std::ostream & stream, bool withName = false);

	int index;  ///< of a pixel actually inside the region.  The (x,y) value inherited from PointAffine may not satisfy this property.  index translates to a pixel value as (index % width, index / width).
	unsigned char threshold;  ///< gray-level value
	bool sign;  ///< true means threshold is upper bound on intensity (ie: this is an MSER+); false means lower bound (MSER-)
  };


  // Inlines for class Point --------------------------------------------------

  template <class T>
  inline Point
  operator * (const MatrixAbstract<T> & M, const Point & p)
  {
	Point result;
	if (M.columns () == 2)
	{
	  result.x = M(0,0) * p.x + M(0,1) * p.y;
	  result.y = M(1,0) * p.x + M(1,1) * p.y;
	}
	else  // We assume columns > 2; ie: no true bounds checking.
	{
	  result.x = M(0,0) * p.x + M(0,1) * p.y + M(0,2);
	  result.y = M(1,0) * p.x + M(1,1) * p.y + M(1,2);
	}
	return result;
  }

  template <class T>
  inline Point
  operator * (const Matrix<T> & M, const Point & p)
  {
	Point result;
	if (M.columns () == 2)
	{
	  result.x = M(0,0) * p.x + M(0,1) * p.y;
	  result.y = M(1,0) * p.x + M(1,1) * p.y;
	}
	else  // We assume columns > 2; ie: no true bounds checking.
	{
	  result.x = M(0,0) * p.x + M(0,1) * p.y + M(0,2);
	  result.y = M(1,0) * p.x + M(1,1) * p.y + M(1,2);
	}
	return result;
  }

  template <class T>
  inline Point
  operator * (const MatrixFixed<T,2,2> & M, const Point & p)
  {
	Point result;
	result.x = M.data[0][0] * p.x + M.data[1][0] * p.y;
	result.y = M.data[0][1] * p.x + M.data[1][1] * p.y;
	return result;
  }
}


#endif
