/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.6, 1.8 thru 1.12 Copyright 2005 Sandia Corporation.
Revisions 1.14, 1.15         Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 11:38:06  Fred
Correct which revisions are under Sandia copyright.

Revision 1.14  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.13  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.12  2005/10/09 03:43:27  Fred
Add  Sandia distribution terms.

Revision 1.11  2005/09/10 17:06:30  Fred
Break dependency of canvas.h on freetype2 include files.

Revision 1.10  2005/09/06 03:21:24  Fred
Add drawMSER() to visualize Matas regions.

Revision 1.9  2005/09/06 03:20:18  Fred
Add Sandia copyright notice for revisions.  This will probably have to be
modified before actual release.  Clarify revision history.

Revision 1.8  2005/05/29 21:59:57  Fred
Replace explicit color values with symbolic constants from color.h.

Use freetype2 to support CanvaImage::drawText().  Changed interface to
drawText() and added setFont().

Revision 1.7  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.6  2005/01/12 04:55:16  rothgang
Add drawEllipse() that directly takes an S matrix.

Revision 1.5  2004/04/06 20:12:08  rothgang
Add lineWidth to postscript Canvas.

Revision 1.4  2004/01/13 19:44:21  rothgang
Provide a default scale for drawParallelogram().

Revision 1.3  2003/12/30 16:49:14  rothgang
Add method to fill a rectangular region with a color.

Revision 1.2  2003/08/15 20:06:42  rothgang
Add drawParallelogram().  Change comments to doxygen format.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef canvas_h
#define canvas_h


#include "fl/point.h"
#include "fl/matrix.h"
#include "fl/image.h"
#include "fl/pi.h"
#include "fl/color.h"

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
	virtual void drawPoint (const Point & p, unsigned int color = WHITE);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = WHITE);
	virtual void drawLine (const Point & a, const Point & b, unsigned int color = WHITE);  ///< Draws a line thru a and b
	virtual void drawLine (float a, float b, float c, unsigned int color = WHITE);  ///< Draws the set ax + by + c = 0
	virtual void drawRay (const Point & p, float angle, unsigned int color = WHITE);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = WHITE);
	virtual void drawParallelogram (const Matrix<double> & S, float radius = 1.0f, unsigned int color = WHITE);  ///< S projects a unit square centered at the origin into the image.  radius scales up the unit square.
	virtual void drawParallelogram (const PointAffine & p, float radius = 1.0f, unsigned int color = WHITE);  ///< Determines an S based on the shape and position of p, then calls drawParallelogram (S, ...).
	virtual void drawFilledRectangle (const Point & corner0, const Point & corner1, unsigned int colorFill = WHITE);
	virtual void drawCircle (const Point & center, float radius, unsigned int color = WHITE, float startAngle = 0, float endAngle = 2 * PI);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1.0f, unsigned int color = WHITE, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);  ///< Draws the set ~x * !shape * x == radius^2.  shape has same semantics as a covariance matrix.  It transforms a circle into an ellipse.  radius, startAngle and endAngle are relative to that circle before it is transformed.
	virtual void drawEllipse (const Matrix<double> & S, float radius = 1.0f, unsigned int color = WHITE);  ///< S projects a unit circle centered at the origin into the image.  radius scales up the unit circle.  This is a convenience function for marking affine-adapted patches.
	virtual void drawMSER (const PointMSER & p, const Image & image, unsigned int colorFill = GRAY50, unsigned int colorBorder = WHITE);
	virtual void drawText (const std::string & text, const Point & point, unsigned int color = WHITE, float angle = 0);
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);  ///< width or height == -1 means size is same number of units as pixels in image

	// State information
	virtual void setTranslation (float x, float y);  ///< Location of origin in this Canvas' coordinate system.
	virtual void setScale (float x, float y);  ///< Multiply all coordinates by a factor.  Scaling is done before translation.
	virtual void setLineWidth (float width);  ///< Width of pen for stroking lines, in native units.
	virtual void setPointSize (float radius);  ///< Set distance away from position of point that marker may extend.
	virtual void setFont (const std::string & name, float size);  ///< Select the type face and size for drawText().
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

	virtual void drawPoint (const Point & p, unsigned int color = WHITE);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = WHITE);
	virtual void drawLine (float a, float b, float c, unsigned int color = WHITE);
	virtual void drawRay (const Point & p, float angle, unsigned int color = WHITE);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = WHITE);
	virtual void drawFilledRectangle (const Point & corner0, const Point & corner1, unsigned int colorFill = WHITE);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1, unsigned int color = WHITE, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);
	virtual void drawMSER (const PointMSER & p, const Image & image, unsigned int colorFill = GRAY50, unsigned int colorBorder = WHITE);
	virtual void drawText (const std::string & text, const Point & point, unsigned int color = WHITE, float angle = 0);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);
	virtual void setLineWidth (float width);
	virtual void setPointSize (float radius);
	virtual void setFont (const std::string & name, float size);

	Point trans (const Point & p);
	void pen (const Point & p, unsigned int color);

	float transX;
	float transY;
	float scaleX;
	float scaleY;
	float lineWidth;
	ImageOf<unsigned char> penTip;
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
  class CanvasPS : public Canvas
  {
  public:
	CanvasPS (const std::string & fileName, float width, float height);  ///< width and height are in points.  They are used to determine %%BoundingBox.
	virtual ~CanvasPS ();
	virtual void drawDone ();

	virtual void drawPoint (const Point & p, unsigned int color = BLACK);
	virtual void drawSegment (const Point & a, const Point & b, unsigned int color = BLACK);
	virtual void drawPolygon (const std::vector<Point> & points, unsigned int color = BLACK);
	virtual void drawCircle (const Point & center, float radius, unsigned int color = BLACK, float startAngle = 0, float endAngle = 2 * PI);
	virtual void drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius = 1, unsigned int color = BLACK, float startAngle = 0, float endAngle = 2 * PI, bool inverse = false);
	virtual void drawImage (const Image & image, Point & p, float width = -1, float height = -1);

	virtual void setTranslation (float x, float y);
	virtual void setScale (float x, float y);
	virtual void setLineWidth (float width);

	void expandColor (unsigned int color);

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
