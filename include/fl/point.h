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

#undef SHARED
#ifdef _MSC_VER
#  ifdef flImage_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  class SHARED Point;
  class SHARED PointSet;
  class SHARED Point3;
  class SHARED PointInterest;
  class SHARED PointAffine;
  class SHARED PointMSER;

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
  class SHARED Point : public MatrixAbstract<double>
  {
  public:
	Point ();
	Point (double x, double y);
	template<class T>
	Point (const MatrixAbstract<T> & A)
	{
	  x = A[0];
	  y = A[1];
	}
	virtual ~Point ();

	static uint32_t serializeVersion;
	void serialize (Archive & archive, uint32_t version);

	virtual MatrixAbstract<double> * clone (bool deep = false) const;
	virtual double & operator () (const int row, const int column) const
	{
	  return const_cast<double &> ((&x)[row]);
	}
	virtual double & operator [] (const int row) const
	{
	  return const_cast<double &> ((&x)[row]);
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual void resize (const int rows, const int columns = 1);  ///< We only have one size.  This will throw an exception if (rows * columns) != 2.

	virtual void print (std::ostream & stream) const;
	virtual MatrixResult<double> homogeneous () const;
	virtual Vector<float> * descriptor () const;  ///< Zero if this point lacks a descriptor.  Otherwise, a pointer to the vector, which should generally exist as long as this point exists.

	double distance (const Point & that) const;  ///< Euclidean distance between two points
	double angle (const Point & that) const;  ///< Determines angle of vector (that - this)
	double angle () const;  ///< Determines angle of vector from origin to this point.

	double x;
	double y;
  };
  inline std::ostream & operator << (std::ostream & stream, const Point & p)
  {
	p.print (stream);
	return stream;
  }

  /**
	 A collection of feature points, typically all from the same image.
	 This collection takes responsibility for the points given to it, and
	 destucts them when it is destructed.
   **/
  class SHARED PointSet : public std::vector<Point *>
  {
  public:
	~PointSet ();  ///< Always destruct our points
	void clear (bool destruct = true);  ///< Empty this collection, possibly without destructing the contained points.

	void add (const std::multiset<PointInterest> & points);
  };

  /**
	 A point in 3D space.
	 @todo Note the use of the inherited operator[] in this class.  This makes
	 the fragile assumption that our z will be placed immediately after the
	 inherited x and y in memory.  May need to change this if a compiler
	 does something different.
  **/
  class SHARED Point3 : public Point
  {
  public:
	Point3 ();
	Point3 (double x, double y, double z);
	template<class T>
	Point3 (const MatrixAbstract<T> & A)
	{
	  x = A[0];
	  y = A[1];
	  z = A[2];
	}

	void serialize (Archive & archive, uint32_t version);

	virtual MatrixAbstract<double> * clone (bool deep = false) const;
	virtual int rows () const;

	virtual void print (std::ostream & stream) const;
	virtual MatrixResult<double> homogeneous () const;

	double z;
  };

  /**
	 Version of Point3 that includes a descriptor.
   **/
  class SHARED Point3D : public Point3
  {
  public:
	Point3D ();
	Point3D (double x, double y, double z);
	template<class T>
	Point3D (const MatrixAbstract<T> & A)
	{
	  x = A[0];
	  y = A[1];
	  z = A[2];
	  descriptor_ = 0;
	}
	virtual ~Point3D ();

	void serialize (Archive & archive, uint32_t version);

	virtual Vector<float> * descriptor () const;

	Vector<float> * descriptor_;
  };

  class SHARED PointInterest : public Point
  {
  public:
	PointInterest ();
	PointInterest (const Point & p);
	virtual ~PointInterest ();

	void serialize (Archive & archive, uint32_t version);

	virtual Vector<float> * descriptor () const;

	Vector<float> * descriptor_;
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

  class SHARED PointAffine : public PointInterest
  {
  public:
	PointAffine ();
	PointAffine (const Point & p);
	PointAffine (const PointInterest & p);
	PointAffine (const Matrix<double> & S);  ///< Constructs from a patch matrix.  S = ! this->rectification()

	void serialize (Archive & archive, uint32_t version);

	Matrix<double> rectification () const;  ///< Computes the 3x3 (affine) homography from the image patch indicated by this point to the normalized form.
	Matrix<double> projection () const;  ///< Computes the 3x3 (affine) homography from the normalized form back into the image patch indicated by this point.

	/**
	   The matrix A is the 2x2 transformation from a rectified patch back to
	   the original image.  It is the same as the "U" matrix in Krystian
	   Mikolajczyk's paper "An affine invariant interest point detector".
	**/
	MatrixFixed<double,2,2> A;
	float angle;  ///< characteristic angle; generally the direction of the gradient.
  };

  class SHARED PointMSER : public PointAffine
  {
  public:
	PointMSER ();
	PointMSER (const Point & p);
	PointMSER (const PointInterest & p);
	PointMSER (const PointAffine & p);
	PointMSER (int index, unsigned char threshold, bool sign = true);

	void serialize (Archive & archive, uint32_t version);

	int index;  ///< of a pixel actually inside the region.  The (x,y) value inherited from PointAffine may not satisfy this property.  index translates to a pixel value as (index % width, index / width).
	unsigned char threshold;  ///< gray-level value
	bool sign;  ///< true means threshold is upper bound on intensity (ie: this is an MSER+); false means lower bound (MSER-)
  };

  class SHARED Rectangle
  {
  public:
	Rectangle (int D);
	Rectangle (const Vector<double> & a, const Vector<double> & b);
	Rectangle (const std::string & Astring);  ///< For convenience initialization.  Specify vectors a and b as first and second rows of a matrix in string form.
	void clear ();  ///< Set empty

	void copyFrom (const Rectangle & that);

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	Rectangle intersect (const Rectangle & that) const;
	Rectangle unite (const Rectangle & that) const;
	void stretch (const MatrixAbstract<double> & point);  ///< Extends bounds to contain the given point
	bool empty () const;
	bool contains (const MatrixAbstract<double> & point) const;
	MatrixResult<double> size () const;

	Vector<double> a;
	Vector<double> b;

	const char * toString (std::string & buffer) const;
  };
  inline std::ostream & operator << (std::ostream & out, const Rectangle & rect)
  {
	return out << rect.a << " " << rect.b;
  }
}


#endif
