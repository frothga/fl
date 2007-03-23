/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.3, 1.5 thru 1.7 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/23 02:32:02  Fred
Use CVS Log to generate revision history.

Revision 1.8  2005/10/13 03:22:02  Fred
Place UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 04:40:19  Fred
Add Sandia distribution terms.

Rename lapack?.h to lapack.h

Revision 1.6  2005/09/10 17:07:21  Fred
Add detail to revision history.  Add Sandia copyright.  This will need to be
updated with license info before release.

Revision 1.5  2005/05/29 15:59:29  Fred
Adjust to new format for rgba constants.

Revision 1.4  2005/04/23 19:36:46  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.3  2005/01/22 21:05:32  Fred
MSVC compilability fix:  Cast to clarify use of float.  Replace GNU operator
with max().

Revision 1.2  2004/05/03 18:53:33  rothgang
Make interface more similar to CanvasImage.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/canvas.h"
#include "fl/lapack.h"

#include <algorithm>


using namespace fl;
using namespace std;


// class CanvasPS -------------------------------------------------------------

CanvasPS::CanvasPS (const std::string & fileName, float width, float height)
: psf (fileName.c_str ())
{
  scale = 1.0;
  lineWidth = 1.0;
  bboxL = 72;
  bboxB = 72;
  bboxR = bboxL + width;
  bboxT = bboxB + height;

  // Write Postscript header
  psf << "%!PS-Adobe-2.0" << endl;
  psf << "%%BoundingBox: " << bboxL << " " << bboxB << " " << bboxR << " " << bboxT << endl;
  psf << "%%EndComments" << endl;
  psf << endl;
  psf << "% Abbreviations" << endl;
  psf << "/cm {matrix currentmatrix} def" << endl;
  psf << "/cpst {setrgbcolor closepath stroke} def" << endl;
  psf << "/gr {grestore} def" << endl;
  psf << "/gs {gsave} def" << endl;
  psf << "/lt {lineto} def" << endl;
  psf << "/mt {moveto} def" << endl;
  psf << "/np {newpath} def" << endl;
  psf << "/rot {rotate} def" << endl;
  psf << "/sc {scale} def" << endl;
  psf << "/seg {setrgbcolor newpath moveto lineto stroke} def" << endl;
  psf << "/slw {setlinewidth} def" << endl;
  psf << "/sm {setmatrix} def" << endl;
  psf << "/st {setrgbcolor stroke} def" << endl;
  psf << "/tr {translate} def" << endl;
  psf << endl;

  // Set coordinate system similar to image rasters.  A freshly constructed
  // CanvasPS acts just like a CanvasImage.
  setTranslation (bboxL, bboxT);
  setScale (1, -1);
}

CanvasPS::~CanvasPS ()
{
  if (psf.is_open ())
  {
	drawDone ();
  }
}

void
CanvasPS::drawDone ()
{
  // Write Postscript trailer
  psf << "%%Trailer" << endl;
  psf << "%%EOF" << endl;

  psf.close ();
}

inline void
CanvasPS::expandColor (unsigned int color)
{
  int r = (color >> 24) & 0xFF;
  int g = (color >> 16) & 0xFF;
  int b = (color >> 8 ) & 0xFF;
  psf << (float) r / 255 << " " << (float) g / 255 << " " << (float) b / 255;
}

void
CanvasPS::drawPoint (const Point & p, unsigned int color)
{
  float step = 2.0 / scale;

  // "X" style
  /*
  Point p1 (p.x - step, p.y - step);
  Point p2 (p.x - step, p.y + step);
  Point p3 (p.x + step, p.y - step);
  Point p4 (p.x + step, p.y + step);
  drawSegment (p1, p4, color);
  drawSegment (p2, p3, color);
  */

  // "Dot" style
  psf << "np" << endl;
  psf << p.x << " " << p.y << " " << step << " 0 360 arc" << endl;
  expandColor (color);
  psf << " setrgbcolor" << endl;
  psf << "fill" << endl;
  psf << endl;
}

void
CanvasPS::drawSegment (const Point & a, const Point & b, unsigned int color)
{
  psf << a.x << " " << a.y << " " << b.x << " " << b.y << " ";
  expandColor (color);
  psf << " seg" << endl;
  psf << endl;
}

void
CanvasPS::drawPolygon (const std::vector<Point> & points, unsigned int color)
{
  if (points.size ())
  {
	psf << "np" << endl;
	psf << points[0].x << " " << points[0].y << " mt" << endl;
	for (int i = 1; i < points.size (); i++)
	{
	  psf << points[i].x << " " << points[i].y << " lt" << endl;
	}
	expandColor (color);
	psf << " cpst" << endl;
	psf << endl;
  }
}

void
CanvasPS::drawCircle (const Point & center, float radius, unsigned int color, float startAngle, float endAngle)
{
  startAngle *= (float) (180 / PI);
  endAngle   *= (float) (180 / PI);

  psf << "np" << endl;
  psf << center.x << " " << center.y << " " << radius << " " << startAngle << " " << endAngle << " arc" << endl;
  expandColor (color);
  psf << " st" << endl;
  psf << endl;
}

void
CanvasPS::drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius, unsigned int color, float startAngle, float endAngle, bool inverse)
{
  Matrix<double> D;
  Matrix<double> rot;
  geev (shape, D, rot);

  float a;
  float b;
  if (inverse)
  {
	a = sqrt (1.0 / D[0]);
	b = sqrt (1.0 / D[1]);
  }
  else
  {
	a = sqrt (D[0]);
	b = sqrt (D[1]);
  }
  float angle = atan2 (rot(1,0), rot(0,0));

  startAngle *= (float) (180 / PI);
  endAngle   *= (float) (180 / PI);

  psf << "np" << endl;
  psf << "cm" << endl;
  psf << center.x << " " << center.y << " tr" << endl;
  psf << angle << " rot" << endl;
  psf << a << " " << b << " sc" << endl;
  psf << "0 0 " << radius << " " << startAngle << " " << endAngle << " arc" << endl;
  psf << "sm" << endl;
  expandColor (color);
  psf << " st" << endl;
  psf << endl;
}

void
CanvasPS::drawImage (const Image & image, Point & p, float width, float height)
{
  throw "Not implemented";
}

void
CanvasPS::setTranslation (float x, float y)
{
  psf << x << " " << y << " translate" << endl;
  psf << endl;
}

void
CanvasPS::setScale (float x, float y)
{
  scale = max (x, y);

  psf << x << " " << y << " sc" << endl;
  psf << lineWidth / scale << " slw" << endl;
  psf << endl;
}

void
CanvasPS::setLineWidth (float width)
{
  lineWidth = width;
  psf << lineWidth / scale << " slw" << endl;
  psf << endl;
}
