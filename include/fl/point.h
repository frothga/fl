#ifndef point_h
#define point_h


#include "fl/matrix.h"


namespace fl
{
  class Point : public MatrixAbstract<float>
  {
  public:
	Point ();
	Point (float x, float y);
	Point (const MatrixAbstract<float> & A);
	Point (const MatrixAbstract<double> & A);
	Point (std::istream & stream);

	//operator Vector<float> ();
	//operator Vector<double> ();

	virtual float & operator () (const int row, const int column) const;
    virtual float & operator [] (const int row) const;
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<float> * duplicate () const;
	virtual void resize (const int rows, const int columns = 1);  // We only have one size, so this may produce an exception.

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);  // withName is ignored, and no class id is ever written for Points

	float distance (const Point & that) const;  // Euclidean distance between two points
	float angle (const Point & that) const;  // Determine angle of vector (that - this)

	// Point uses float values for x and y, so we define the
	// following convention for interpreting the fractional part of a pixel
	// coordinate:  The image coordinate system starts in the upper left
	// corner.  Positive x goes to the right, and positive y goes down.
	// Integer pixel coordinates (eg: 0, 1.0, 2.0, etc.) refer to the
	// center of the pixel.  IE: a pixel begins at -0.5 and ends at 0.5.
	// This convention allows easy conversion between ints and floats
	// that is also geometrically consistent and valid.  The only thing
	// it makes more difficult is determining the edges and center of an
	// image, usually one-time calculations.
	float x;
	float y;

	// For use in pretending to be a homogenous coordinate vector
	static float zero;
	static float one;
  };

  class PointInterest : public Point
  {
  public:
	PointInterest ();
	PointInterest (const Point & p);
	PointInterest (std::istream & stream);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);

	float weight;  // strength of response of interest operator
	float scale;  // "characteristic scale" of image around interest point

	bool operator < (const PointInterest & that) const
	{
	  return weight < that.weight;
	}
  };

  // Encapsulate additional information needed to correct the patch around a
  // point so the x and y gradients are equal.
  class PointInterestAffine : public PointInterest
  {
  public:
	PointInterestAffine ();
	PointInterestAffine (const Point & p);
	PointInterestAffine (const PointInterest & p);
	PointInterestAffine (std::istream & stream);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false);

	// The matrix A is the 2x2 transformation from a rectified patch back to
	// the original image.  It is the same as the "U" matrix in Krystian
	// Mikolajczyk's paper "An affine invariant interest point detector".
	Matrix2x2<double> A;
	float angle;  // characteristic angle; generally the direction of the gradient
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
  operator * (const Matrix2x2<T> & M, const Point & p)
  {
	Point result;
	result.x = M.data[0][0] * p.x + M.data[1][0] * p.y;
	result.y = M.data[0][1] * p.x + M.data[1][1] * p.y;
	return result;
  }
}


#endif
