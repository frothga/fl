#ifndef canvas_h
#define canvas_h


#include "fl/point.h"
#include "fl/matrix.h"
#include "fl/image.h"
#include "fl/pi.h"

#include <vector>
#include <string>
#include <fstream>


namespace fl
{
  // Generic Canvas interface -------------------------------------------------

  /**
	 An abstract 2D drawing surface.
  **/
  class Canvas
  {
  public:
	virtual ~Canvas ();
	virtual void drawDone ();  ///< Do any final steps to output the drawing.  After this, the effect of draw commands is undefined.

	// Drawing functions are the primary interface
	virtual void drawPoint (const Point & p, unsigned int color = 0xFFFFFF);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = 0xFFFFFF);
	virtual void drawLine (const Point & a, const Point & b, unsigned int color = 0xFFFFFF);  ///< Draws a line thru a and b
	virtual void drawLine (float a, float b, float c, unsigned int color = 0xFFFFFF);  ///< Draws the set ax + by + c = 0
	virtual void drawRay (const Point & p, float angle, unsigned int color = 0xFFFFFF);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = 0xFFFFFF);
	virtual void drawParallelogram (const Matrix<double> & S, float radius, unsigned int color = 0xFFFFFF);  ///< S projects a unit square centered at the origin into the image.  radius scales up the unit square.
	virtual void drawParallelogram (const PointAffine & p, float radius, unsigned int color = 0xFFFFFF);  ///< Determines an S based on the shape and position of p, then calls drawParallelogram (S, ...).
	virtual void drawCircle (const Point & center, float radius, unsigned int color = 0xFFFFFF, float startAngle = 0, float endAngle = 2 * PI);
	virtual void drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius = 1, unsigned int color = 0xFFFFFF, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);  ///< Draws the set ~x * !shape * x == radius^2.  shape has same semantics as a covariance matrix.  It transforms a circle into an ellipse.  radius, startAngle and endAngle are relative to that circle before it is transformed.
	virtual void drawText (const std::string & text, const Point & point, float size = 10, float angle = 0, unsigned int color = 0xFFFFFF);
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);  ///< width or height == -1 means size is same number of units as pixels in image

	// State information
	virtual void setTranslation (float x, float y);  ///< Location of origin in this Canvas' coordinate system.
	virtual void setScale (float x, float y);  ///< Multiply all coordinates by a factor.  Scaling is done before translation.
	virtual void setLineWidth (float width);  ///< Width of pen for stroking lines, in native units.
	virtual void setPointSize (float radius);  ///< Set distance away from position of point that marker may extend.
  };


  // Specific Canvas implementations ------------------------------------------

  class CanvasImage : public Canvas, public Image
  {
  public:
	CanvasImage (const PixelFormat & format = GrayChar);
	CanvasImage (int width, int height, const PixelFormat & format = GrayChar);
	CanvasImage (const Image & that);
	void initialize ();
	virtual ~CanvasImage ();

	virtual void drawPoint (const Point & p, unsigned int color = 0xFFFFFF);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = 0xFFFFFF);
	virtual void drawLine (float a, float b, float c, unsigned int color = 0xFFFFFF);
	virtual void drawRay (const Point & p, float angle, unsigned int color = 0xFFFFFF);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = 0xFFFFFF);
	virtual void drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius = 1, unsigned int color = 0xFFFFFF, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);
	virtual void setLineWidth (float width);
	virtual void setPointSize (float radius);

	Point trans (const Point & p);
	void pen (const Point & p, unsigned int color);

	float transX;
	float transY;
	float scaleX;
	float scaleY;
	float lineWidth;
	ImageOf<unsigned char> penTip;
	float pointRadius;
  };

  /**
	 Outputs drawing as a Postscript file
  **/
  class CanvasPS : public Canvas
  {
  public:
	CanvasPS (const std::string & fileName, float width, float height);  ///< width and height are in points.  They are used to determine %%BoundingBox.
	virtual ~CanvasPS ();
	virtual void drawDone ();

	virtual void drawPoint (const Point & p, unsigned int color = 0);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = 0);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = 0);
	virtual void drawCircle (const Point & center, float radius, unsigned int color = 0, float startAngle = 0, float endAngle = 2 * PI);
	virtual void drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius = 1, unsigned int color = 0, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);

	void expandColor (unsigned int color);

	std::ofstream psf;  ///< "Postscript file"
	float scale;  ///< Used to compute line widths.
	float bboxT;  ///< Top of bounding box in points
	float bboxB;  ///< Bottom
	float bboxL;  ///< Left
	float bboxR;  ///< Right
  };
}


#endif
