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
Canvas::drawPoint (const Point & p, uint32_t color)
{
  throw "drawPoint not implemented for this type of Canvas";
}

void
Canvas::drawSegment (const Point & a, const Point & b, uint32_t color)
{
  throw "drawSegment not implemented for this type of Canvas";
}

void
Canvas::drawLine (const Point & a, const Point & b, uint32_t color)
{
  float l1 = b.y - a.y;
  float l2 = a.x - b.x;
  float l3 = - (l1 * a.x + l2 * a.y);
  drawLine (l1, l2, l3, color);
}

void
Canvas::drawLine (float a, float b, float c, uint32_t color)
{
  throw "drawLine not implemented for this type of Canvas";
}

void
Canvas::drawRay (const Point & p, float angle, uint32_t color)
{
  throw "drawRay not implemented for this type of Canvas";
}

void
Canvas::drawPolygon (const std::vector<Point> & points, uint32_t color)
{
  throw "drawPolygon not implemented for this type of Canvas";
}

void
Canvas::drawFilledPolygon (const std::vector<Point> & points, uint32_t color)
{
  throw "drawFilledPolygon not implemented for this type of Canvas";
}

void
Canvas::drawParallelogram (const Matrix<double> & S, float radius, uint32_t color)
{
  Vector<double> tl (3);
  tl[0] = -radius;
  tl[1] =  radius;
  tl[2] =  1;
  Vector<double> tr (3);
  tr[0] =  radius;
  tr[1] =  radius;
  tr[2] =  1;
  Vector<double> bl (3);
  bl[0] = -radius;
  bl[1] = -radius;
  bl[2] =  1;
  Vector<double> br (3);
  br[0] =  radius;
  br[1] = -radius;
  br[2] =  1;

  Point ptl = S * tl;
  Point ptr = S * tr;
  Point pbl = S * bl;
  Point pbr = S * br;

  drawSegment (ptl, ptr, color);
  drawSegment (ptr, pbr, color);
  drawSegment (pbr, pbl, color);
  drawSegment (pbl, ptl, color);
}

void
Canvas::drawParallelogram (const PointAffine & p, float radius, uint32_t color)
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
Canvas::drawFilledRectangle (const Point & corner0, const Point & corner1, uint32_t colorFill)
{
  throw "drawFilledRectangle not implemented for this type of Canvas";
}

void
Canvas::drawCircle (const Point & center, float radius, uint32_t color, float startAngle, float endAngle)
{
  MatrixFixed<double,2,2> A;
  A.identity ();
  drawEllipse (center, A, radius, color, startAngle, endAngle);
}

void
Canvas::drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius, uint32_t color, float startAngle, float endAngle, bool inverse)
{
  throw "drawEllipse not implemented for this type of Canvas";
}

void
Canvas::drawEllipse (const Matrix<double> & S, float radius, uint32_t color)
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
Canvas::drawMSER (const PointMSER & p, const Image & image, uint32_t colorFill, uint32_t colorBorder)
{
  throw "drawMSER not implemented for this type of Canvas";
}

void
Canvas::drawImage (const Image & image, Point & p, float width, float height)
{
  throw "drawImage not implemented for this type of Canvas";
}

void
Canvas::drawText (const std::string & text, const Point & point, uint32_t color, float angle)
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
