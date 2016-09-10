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


#ifndef canvas_h
#define canvas_h


#include "fl/point.h"
#include "fl/matrix.h"
#include "fl/image.h"
#include "fl/color.h"

#include <vector>
#include <string>
#include <fstream>

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
  // Generic Canvas interface -------------------------------------------------

  /**
	 An abstract 2D drawing surface.
  **/
  class SHARED Canvas
  {
  public:
	virtual ~Canvas ();
	virtual void drawDone ();  ///< Do any final steps to output the drawing.  After this, the effect of draw commands is undefined.

	// Drawing functions are the primary interface
	virtual void drawPoint (const Point & p, uint32_t color = WHITE);
	virtual void drawSegment (const Point & a, const Point & b, uint32_t color = WHITE);
	virtual void drawLine (const Point & a, const Point & b, uint32_t color = WHITE);  ///< Draws a line thru a and b
	virtual void drawLine (float a, float b, float c, uint32_t color = WHITE);  ///< Draws the set ax + by + c = 0
	virtual void drawRay (const Point & p, float angle, uint32_t color = WHITE);
	virtual void drawPolygon (const std::vector<Point> & points, uint32_t color = WHITE);
	virtual void drawFilledPolygon (const std::vector<Point> & points, uint32_t color = WHITE);
	virtual void drawParallelogram (const Matrix<double> & S, float radius = 1.0f, uint32_t color = WHITE);  ///< S projects a unit square centered at the origin into the image.  radius scales up the unit square.
	virtual void drawParallelogram (const PointAffine & p, float radius = 1.0f, uint32_t color = WHITE);  ///< Determines an S based on the shape and position of p, then calls drawParallelogram (S, ...).
	virtual void drawFilledRectangle (const Point & corner0, const Point & corner1, uint32_t colorFill = WHITE);
	virtual void drawCircle (const Point & center, float radius, uint32_t color = WHITE, float startAngle = 0, float endAngle = TWOPIf);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1.0f, uint32_t color = WHITE, float startAngle = 0, float endAngle = TWOPIf, bool inverse = false);  ///< Draws the set ~x * !shape * x == radius^2.  shape has same semantics as a covariance matrix.  It transforms a circle into an ellipse.  radius, startAngle and endAngle are relative to that circle before it is transformed.
	virtual void drawEllipse (const Matrix<double> & S, float radius = 1.0f, uint32_t color = WHITE);  ///< S projects a unit circle centered at the origin into the image.  radius scales up the unit circle.  This is a convenience function for marking affine-adapted patches.
	virtual void drawEllipse (const PointAffine & p,    float radius = 1.0f, uint32_t color = WHITE);
	virtual void drawMSER (const PointMSER & p, const Image & image, uint32_t colorFill = GRAY50, uint32_t colorBorder = WHITE);
	virtual void drawText (const std::string & text, const Point & point, uint32_t color = WHITE, float angle = 0);
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);  ///< width or height == -1 means size is same number of units as pixels in image

	// State information
	virtual void setTranslation (float x, float y);  ///< Location of origin in this Canvas' coordinate system.
	virtual void setScale (float x, float y);  ///< Multiply all coordinates by a factor.  Scaling is done before translation.
	virtual void setLineWidth (float width);  ///< Width of pen for stroking lines, in native units.
	virtual void setPointSize (float radius);  ///< Set distance away from position of point that marker may extend.
	virtual void setFont (const std::string & name, float size);  ///< Select the type face and size for drawText().
  };


  // Specific Canvas implementations ------------------------------------------

  class SHARED CanvasImage : public Canvas, public Image
  {
  public:
	CanvasImage (const PixelFormat & format = GrayChar);
	CanvasImage (int width, int height, const PixelFormat & format = GrayChar);
	CanvasImage (const Image & that);
	void initialize ();
	virtual ~CanvasImage ();

	virtual void drawPoint (const Point & p, uint32_t color = WHITE);
	virtual void drawSegment (const Point & a, const Point & b, uint32_t color = WHITE);
	virtual void drawLine (float a, float b, float c, uint32_t color = WHITE);
	virtual void drawRay (const Point & p, float angle, uint32_t color = WHITE);
	virtual void drawPolygon (const std::vector<Point> & points, uint32_t color = WHITE);
	virtual void drawFilledPolygon (const std::vector<Point> & points, uint32_t color = WHITE);
	virtual void drawFilledRectangle (const Point & corner0, const Point & corner1, uint32_t colorFill = WHITE);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1, uint32_t color = WHITE, float startAngle = 0, float endAngle = TWOPIf, bool inverse = false);
	using Canvas::drawEllipse;
	virtual void drawMSER (const PointMSER & p, const Image & image, uint32_t colorFill = GRAY50, uint32_t colorBorder = WHITE);
	virtual void drawText (const std::string & text, const Point & point, uint32_t color = WHITE, float angle = 0);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);
	virtual void setLineWidth (float width);
	virtual void setPointSize (float radius);
	virtual void setFont (const std::string & name, float size);

	Point trans (const Point & p);
	void scanCircle (const Point & p, double radius, uint32_t color, int x0, int y0, int x1, int y1);
	void scanCircle (const Point & p, double radius, uint32_t color);

	float transX;
	float transY;
	float scaleX;
	float scaleY;
	float lineWidth;
	float pointRadius;
	void * face;  ///< The currently selected font.  Actually an FT_FaceRec_ *.  Storing this as a void pointer is an ugly hack to avoid the dependency on the freetype includes in the client program.

	// Static interface for Freetype and font management

	static void initFontLibrary ();  ///< One-time initialization of the Freetype2 library.
	static void scanFontDirectory (const std::string & path);  ///< Identify all font files in a given directory and register the Postscript names in the map.  Can be called after initFontLibrary() to add fonts not in the hard-coded default font path.
	static void addFontFile (const std::string & path);  ///< If path points to valid font file, then add its Postscript name to the map.

	static void * library;  ///< Working memory for freetype.  Actually an FT_LibraryRec_ *.  Storing this as a void pointer is an ugly hack to avoid the dependency on the freetype include in the client program.
	static std::map<std::string, std::string> fontMap;  ///< Maps from font name to font file
  };

  /**
	 Outputs drawing as a Postscript file
  **/
  class SHARED CanvasPS : public Canvas
  {
  public:
	CanvasPS (const std::string & fileName, float width, float height);  ///< width and height are in points.  They are used to determine %%BoundingBox.
	virtual ~CanvasPS ();
	virtual void drawDone ();

	virtual void drawPoint (const Point & p, uint32_t color = BLACK);
	virtual void drawSegment (const Point & a, const Point & b, uint32_t color = BLACK);
	virtual void drawPolygon (const std::vector<Point> & points, uint32_t color = BLACK);
	virtual void drawCircle (const Point & center, float radius, uint32_t color = BLACK, float startAngle = 0, float endAngle = TWOPIf);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1, uint32_t color = BLACK, float startAngle = 0, float endAngle = TWOPIf, bool inverse = false);
	using Canvas::drawEllipse;
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);
	virtual void setLineWidth (float width);

	void expandColor (uint32_t color);

	std::ofstream psf;  ///< "Postscript file"
	float scale;  ///< Used to compute line widths.
	float lineWidth;
	float bboxT;  ///< Top of bounding box in points
	float bboxB;  ///< Bottom
	float bboxL;  ///< Left
	float bboxR;  ///< Right
  };
}


#endif
