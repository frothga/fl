/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4, 1.6 thru 1.10 Copyright 2005 Sandia Corporation.
Revisions Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.12  2007/03/23 02:32:03  Fred
Use CVS Log to generate revision history.

Revision 1.11  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.10  2005/10/09 04:17:48  Fred
Add Sandia distribution terms.

Revision 1.9  2005/09/06 03:26:42  Fred
Add comment on drawMSER() to revision history.

Revision 1.8  2005/09/06 03:23:17  Fred
Add empty implementation of drawMSER().

Revision 1.7  2005/09/02 22:21:02  Fred
Make revision history more explicit, and add Sandia copyright notice.  Probably
needs more work to get notice correct.

Revision 1.6  2005/05/29 15:58:51  Fred
Change interface to drawText().  Add setFont().

Revision 1.5  2005/04/23 19:36:45  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/12 05:08:44  rothgang
Add drawEllipse() that directly takes an S matrix.

Revision 1.3  2003/12/29 23:30:16  rothgang
Add function to fill rectangular region with a color.

Revision 1.2  2003/09/07 21:55:31  rothgang
Add drawParallelogram().

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/canvas.h"


using namespace fl;
using namespace std;


// class Canvas ---------------------------------------------------------------

Canvas::~Canvas ()
{
}

void
Canvas::drawDone ()
{
  // Do nothing
}

void
Canvas::drawPoint (const Point & p, unsigned int color)
{
  throw "drawPoint not implemented for this type of Canvas";
}

void
Canvas::drawSegment (const Point & a, const Point & b, unsigned int color)
{
  throw "drawSegment not implemented for this type of Canvas";
}

void
Canvas::drawLine (const Point & a, const Point & b, unsigned int color)
{
  float l1 = b.y - a.y;
  float l2 = a.x - b.x;
  float l3 = - (l1 * a.x + l2 * a.y);
  drawLine (l1, l2, l3, color);
}

void
Canvas::drawLine (float a, float b, float c, unsigned int color)
{
  throw "drawLine not implemented for this type of Canvas";
}

void
Canvas::drawRay (const Point & p, float angle, unsigned int color)
{
  throw "drawRay not implemented for this type of Canvas";
}

void
Canvas::drawPolygon (const std::vector<Point> & points, unsigned int color)
{
  throw "drawPolygon not implemented for this type of Canvas";
}

void
Canvas::drawParallelogram (const Matrix<double> & S, float radius, unsigned int color)
{
  Point tl (-radius,  radius);
  Point tr ( radius,  radius);
  Point bl (-radius, -radius);
  Point br ( radius, -radius);

  tl = S * tl;
  tr = S * tr;
  bl = S * bl;
  br = S * br;

  drawSegment (tl, tr, color);
  drawSegment (tr, br, color);
  drawSegment (br, bl, color);
  drawSegment (bl, tl, color);
}

void
Canvas::drawParallelogram (const PointAffine & p, float radius, unsigned int color)
{
  MatrixFixed<double,2,2> R;
  R(0,0) = cos (p.angle);
  R(0,1) = -sin (p.angle);
  R(1,0) = -R(0,1);
  R(1,1) = R(0,0);

  Matrix<double> S (3, 3);
  S.identity ();
  S.region (0, 0, 1, 1) = p.A * R * p.scale;
  S.region (0, 2, 1, 2) = p;

  drawParallelogram (S, radius, color);
}

void
Canvas::drawFilledRectangle (const Point & corner0, const Point & corner1, unsigned int colorFill)
{
  throw "drawFilledRectangle not implemented for this type of Canvas";
}

void
Canvas::drawCircle (const Point & center, float radius, unsigned int color, float startAngle, float endAngle)
{
  MatrixFixed<double,2,2> A;
  A.identity ();
  drawEllipse (center, A, radius, color, startAngle, endAngle);
}

void
Canvas::drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius, unsigned int color, float startAngle, float endAngle, bool inverse)
{
  throw "drawEllipse not implemented for this type of Canvas";
}

void
Canvas::drawEllipse (const Matrix<double> & S, float radius, unsigned int color)
{
  Point center (S(0,2), S(1,2));
  MatrixFixed<double,2,2> shape = S.region (0, 0, 1, 1);
  shape = shape * ~shape;
  drawEllipse (center, shape, radius, color);
}

/**
   Paints the pixels inside the region with colorFill, and the pixels just
   outside the region with colorBorder.
   \param image The original image in which the MSER was found, preferably
   converted to GrayChar.
   \param colorFill If the alpha channel is zero, then the interior will not
   be marked.
   \param colorBorder If the alpha channel is zero, then the border will not
   be marked.
**/
void
Canvas::drawMSER (const PointMSER & p, const Image & image, unsigned int colorFill, unsigned int colorBorder)
{
  throw "drawMSER not implemented for this type of Canvas";
}

void
Canvas::drawImage (const Image & image, Point & p, float width, float height)
{
  throw "drawImage not implemented for this type of Canvas";
}

void
Canvas::drawText (const std::string & text, const Point & point, unsigned int color, float angle)
{
  throw "drawText not implemented for this type of Canvas";
}

void
Canvas::setTranslation (float x, float y)
{
  // Do nothing
}

void
Canvas::setScale (float x, float y)
{
  // Do nothing
}

void
Canvas::setLineWidth (float width)
{
  // Do nothing
}

void
Canvas::setPointSize (float radius)
{
  // Do nothing
}

void
Canvas::setFont (const string & name, float size)
{
  // Do nothing
}
