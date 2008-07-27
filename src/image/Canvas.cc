/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
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
