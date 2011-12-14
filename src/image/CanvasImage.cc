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
#include "fl/lapack.h"
#include "fl/string.h"

#include <algorithm>

#ifdef HAVE_FREETYPE
#  include <ft2build.h>
#  include FT_FREETYPE_H
#  ifdef WIN32
#    include <windows.h>
#    undef min
#    undef max
#  else
#    include <dirent.h>
#  endif
#endif


using namespace fl;
using namespace std;


// Utiltiy functions ----------------------------------------------------------

static inline void
flipX (float & angle)
{
  // Act as if circle has been flipped along vertical axis
  if (angle < M_PI)
  {
	angle = M_PI - angle;
  }
  else
  {
	angle = 3 * M_PI - angle;
  }
}

static inline void
flipY (float & angle)
{
  // Act as if circle has been flipped along horizontal axis
  angle = TWOPIf - angle;
}

static inline bool
inRange (float angle, const float & startAngle, const float & endAngle)
{
  while (angle < startAngle)
  {
	angle += TWOPIf;
  }
  return angle <= endAngle;
}


// class CanvasImage ----------------------------------------------------------

// Most of the implementations here are hacks based on float.  They could be
// made more efficient for changing to Bresenhem-like approaches.

void * CanvasImage::library = 0;
map<string, string> CanvasImage::fontMap;

CanvasImage::CanvasImage (const PixelFormat & format)
: Image (format)
{
  initialize ();
}

CanvasImage::CanvasImage (int width, int height, const PixelFormat & format)
: Image (width, height, format)
{
  initialize ();
}

CanvasImage::CanvasImage (const Image & that)
: Image (that)
{
  initialize ();
}

void
CanvasImage::initialize ()
{
  transX = 0;
  transY = 0;
  scaleX = 1;
  scaleY = 1;

  setLineWidth (1);
  setPointSize (2);

  face = 0;
}

CanvasImage::~CanvasImage ()
{
}

inline Point
CanvasImage::trans (const Point & p)
{
  Point result;
  result.x = p.x * scaleX + transX;
  result.y = p.y * scaleY + transY;
  return result;
}

inline void
CanvasImage::pen (const Point & p, unsigned int color)
{
  if (lineWidth == 1)  // hack for simple line drawing
  {
	int x = (int) roundp (p.x);
	int y = (int) roundp (p.y);
	if (x >= 0  &&  x < width  &&  y >= 0  &&  y < height) setRGBA (x, y, color);
  }
  else
  {
	int h = penTip.width / 2;
	int px = (int) roundp (p.x) - h;
	int py = (int) roundp (p.y) - h;

	int xl = max (0,                 -px);
	int xh = min (penTip.width - 1,  (width - 1) - px);
	int yl = max (0,                 -py);
	int yh = min (penTip.height - 1, (height - 1) - py);

	Pixel C (color);
	for (int x = xl; x <= xh; x++)
	{
	  for (int y = yl; y <= yh; y++)
	  {
		C.setAlpha (penTip (x, y));
		Pixel p = (*this) (px + x, py + y);
		p = p << C;
	  }
	}
  }
}

void
CanvasImage::drawPoint (const Point & p, unsigned int color)
{
  Point step (pointRadius / scaleX, pointRadius / scaleY);
  Point p1 (p.x - step.x, p.y - step.y);
  Point p2 (p.x - step.x, p.y + step.y);
  Point p3 (p.x + step.x, p.y - step.y);
  Point p4 (p.x + step.x, p.y + step.y);
  drawSegment (p1, p4, color);
  drawSegment (p2, p3, color);
}

// The following is an implementation of the Cohen-Sutherland clipping
// algorithm, for use by drawSegment().  We use Cohen-Sutherland because
// it has faster early-out than other clipping algorithms, and we assume
// that most segments do not cross the boundary of the drawable area.

static const int LEFT   = 0x1;
static const int RIGHT  = 0x2;
static const int TOP    = 0x4;
static const int BOTTOM = 0x8;

static inline int
clipCode (const double & fWidth, const double & fHeight, const Point & a)
{
  int result = 0;
  if      (a.x < -0.5)    result |= LEFT;
  else if (a.x > fWidth)  result |= RIGHT;
  if      (a.y < -0.5)    result |= TOP;
  else if (a.y > fHeight) result |= BOTTOM;
  return result;
}

static inline bool
clip (const int & width, const int & height, Point & a, Point & b)
{
  const double fWidth  = width  - 0.500001;
  const double fHeight = height - 0.500001;
  int clipA = clipCode (fWidth, fHeight, a);
  int clipB = clipCode (fWidth, fHeight, b);
  while (true)
  {
	if (! (clipA | clipB)) return true;
	if (   clipA & clipB ) return false;

	double x;
	double y;
	int endpoint = clipA ? clipA : clipB;
	double slope = (b.y - a.y) / (b.x - a.x);
	if (endpoint & LEFT)
	{
	  x = 0;
	  y = a.y - a.x             * (b.y - a.y) / (b.x - a.x);
	}
	else if (endpoint & RIGHT)
	{
	  x = fWidth;
	  y = a.y + (fWidth - a.x)  * (b.y - a.y) / (b.x - a.x);
	}
	else if (endpoint & TOP)
	{
	  x = a.x - a.y             * (b.x - a.x) / (b.y - a.y);
	  y = 0;
	}
	else  // endpoint & BOTTOM
	{
	  x = a.x + (fHeight - a.y) * (b.x - a.x) / (b.y - a.y);
	  y = fHeight;
	}

	if (endpoint == clipA)
	{
	  a.x = x;
	  a.y = y;
	  clipA = clipCode (fWidth, fHeight, a);
	}
	else  // endpoint == clipB
	{
	  b.x = x;
	  b.y = y;
	  clipB = clipCode (fWidth, fHeight, b);
	}
  }
}

void
CanvasImage::drawSegment (const Point & a, const Point & b, unsigned int color)
{
  Point ta = trans (a);
  Point tb = trans (b);

  if (! clip (width, height, ta, tb)) return;

  double dx = tb.x - ta.x;
  double dy = tb.y - ta.y;

  if (dx == 0  &&  dy == 0)
  {
	pen (ta, color);
	return;
  }

  Point p;
  if (fabs (dx) > fabs (dy))
  {
	if (dx < 0)
	{
	  for (p.x = tb.x; p.x <= ta.x; p.x++)
	  {
		p.y = (p.x - tb.x) * dy / dx + tb.y;
		pen (p, color);
	  }
	}
	else
	{
	  for (p.x = ta.x; p.x <= tb.x; p.x++)
	  {
		p.y = (p.x - ta.x) * dy / dx + ta.y;
		pen (p, color);
	  }
	}
  }
  else
  {
	if (dy < 0)
	{
	  for (p.y = tb.y; p.y <= ta.y; p.y++)
	  {
		p.x = (p.y - tb.y) * dx / dy + tb.x;
		pen (p, color);
	  }
	}
	else
	{
	  for (p.y = ta.y; p.y <= tb.y; p.y++)
	  {
		p.x = (p.y - ta.y) * dx / dy + ta.x;
		pen (p, color);
	  }
	}
  }
}

void
CanvasImage::drawLine (float a, float b, float c, unsigned int color)
{
  a /= scaleX;
  b /= scaleY;
  c -= a * transX + b * transY;

  Point p;
  if (fabs (a) < fabs (b))
  {
	a /= -b;
	c /= -b;
	for (p.x = 0; p.x < width; p.x++)
	{
	  p.y = a * p.x + c;
	  pen (p, color);
	}
  }
  else
  {
	b /= -a;
	c /= -a;
	for (p.y = 0; p.y < height; p.y++)
	{
	  p.x = b * p.y + c;
	  pen (p, color);
	}
  }
}

void
CanvasImage::drawRay (const Point & p, float angle, unsigned int color)
{
  Point center = trans (p);

  angle = mod2pi (angle);
  float c = cosf (angle) * scaleX;
  float s = sinf (angle) * scaleY;

  if (fabsf (c) > fabsf (s))
  {
	float step = s / c;
	if (c < 0)
	{
	  do
	  {
		pen (center, color);
		center.x--;
		center.y -= step;
	  }
	  while (center.x >= 0);
	}
	else
	{
	  do
	  {
		pen (center, color);
		center.x++;
		center.y += step;
	  }
	  while (center.x < width);
	}
  }
  else
  {
	float step = c / s;
	if (s < 0)
	{
	  do
	  {
		pen (center, color);
		center.x -= step;
		center.y--;
	  }
	  while (center.y >= 0);
	}
	else
	{
	  do
	  {
		pen (center, color);
		center.x += step;
		center.y++;
	  }
	  while (center.y < height);
	}
  }
}

void
CanvasImage::drawPolygon (const vector<Point> & points, unsigned int color)
{
  for (int i = 0; i < points.size () - 1; i++)
  {
	drawSegment (points[i], points[i + 1], color);
  }
  if (points.size () >= 3)
  {
	drawSegment (points.front (), points.back (), color);
  }
}

struct Segment
{
  float x;
  float slope;
};

struct Vertex
{
  Point p;
  Vertex * pred;
  Vertex * succ;
  vector<Segment *> active;
};

static inline void
advanceX (float deltaY, vector<Segment *> & active)
{
  // Increment X positions
  for (int i = 0; i < active.size (); i++)
  {
	active[i]->x += active[i]->slope * deltaY;
  }

  // Bubble sort
  bool changed = true;
  while (changed)
  {
	changed = false;
	for (int i = 1; i < active.size (); i++)
	{
	  if (active[i-1]->x > active[i]->x)
	  {
		swap (active[i-1], active[i]);
		changed = true;
	  }
	}
  }
}

static inline void
insertSegment (Vertex * smallerY, Vertex * biggerY, vector<Segment *> & active)
{
  Segment * s = new Segment;
  s->x = smallerY->p.x;
  s->slope = (biggerY->p.x - smallerY->p.x) / (biggerY->p.y - smallerY->p.y);
  biggerY->active.push_back (s);

  // Insert into active in X order.
  // This linear search is reasonably efficient when there are only 2 or 4 active segments.
  // Should to use the more efficient binary search for more complex polygons
  int i;
  for (i = 0; i < active.size (); i++)
  {
	if (s->x <= active[i]->x)
	{
	  active.insert (active.begin () + i, s);
	  break;
	}
  }
  if (i >= active.size ()) active.push_back (s);
}

/**
   The basic approach of this function is to implement a plane-sweep algorithm
   combined with scanline rendering.  Sorts vertices into ascending Y order.
   Each Y value is an event that changes the structure of the bounding
   segments.  Between these events the set of segments remains constant,
   though their order may change (segments may cross over other segments).
 **/
void
CanvasImage::drawFilledPolygon (const vector<Point> & points, unsigned int color)
{
  if (points.size () < 3) throw "drawFilledPolygon requires at least 3 points";

  // Convert input to vertex ring and sort by Y
  multimap<float, Vertex *> sorted;
  Vertex * root = new Vertex;
  root->p = trans (points[0]);
  root->pred = root;
  root->succ = root;
  sorted.insert (make_pair (root->p.y, root));
  for (int i = 1; i < points.size (); i++)
  {
	Vertex * v = new Vertex;
	v->p = trans (points[i]);
	v->succ = root;
	v->pred = root->pred;
	root->pred->succ = v;
	root->pred = v;
	sorted.insert (make_pair (v->p.y, v));
  }

  // Scan through sorted vertex list, drawing scanlines
  float Y = -INFINITY;
  vector<Segment *> active;
  float lastQueuedY = sorted.rbegin ()->first;
  float alpha = (color & 0xFF) / 255.0f;
  Pixel C (color);
  while (sorted.size ())
  {
	// Advance Y to next queued vertex
	float nextQueuedY = sorted.begin ()->first;
	advanceX (nextQueuedY - Y, active);
	Y = nextQueuedY;
	if (Y > height - 1) break;

	// Update active segment list
	while (sorted.size ()  &&  sorted.begin ()->first == nextQueuedY)
	{
	  Vertex * v = sorted.begin ()->second;
	  sorted.erase (sorted.begin ());

	  // Remove any segments that terminate at v
	  for (int i = 0; i < v->active.size (); i++)
	  {
		Segment * kill = v->active[i];
		for (int j = 0; j < active.size (); j++)
		{
		  if (active[j] == kill)
		  {
			active.erase (active.begin () + j);
			delete kill;
			break;
		  }
		}
	  }

	  // Add segments associated with any neighbors of v with strictly larger y values (implies that we never add horizontal segments).
	  if (v->pred->p.y > v->p.y) insertSegment (v, v->pred, active);
	  if (v->succ->p.y > v->p.y) insertSegment (v, v->succ, active);
	}
	if (sorted.size ()) nextQueuedY = sorted.begin ()->first;
	else                nextQueuedY = lastQueuedY;

	// Determine next Y quantum (center of pixel row)
	int Yquantum = (int) ceil (Y);
	if (Yquantum < 0)
	{
	  int jumpY = min (0, (int) floor (nextQueuedY));
	  Yquantum = max (Yquantum, jumpY);
	}

	// Draw scanlines
	if (nextQueuedY > height) nextQueuedY = height;
	while (Yquantum < nextQueuedY)
	{
	  advanceX (Yquantum - Y, active);
	  Y = Yquantum;
	  if (Yquantum >= 0)
	  {
		// Fill scanline
		int i = 0;
		while (i < active.size ())
		{
		  float l = active[i++]->x;
		  float r = active[i++]->x;

		  l = max (-0.499999f,        l);
		  r = min (width - 0.500001f, r);
		  if (r < l) continue;

		  int intL = (int) roundp (l);
		  int intR = (int) roundp (r);
		  if (intL == intR)
		  {
			C.setAlpha ((int) (0xFF * (r - l) * alpha));
			Pixel p = (*this) (intL, Yquantum);
			p = p << C;
		  }
		  else
		  {
			// draw left pixel
			C.setAlpha ((int) (0xFF * (intL + 0.5f - l) * alpha));
			Pixel p0 = (*this) (intL, Yquantum);
			p0 = p0 << C;

			// draw middle pixels
			C.setAlpha (color & 0xFF);
			for (int x = intL + 1; x < intR; x++)
			{
			  Pixel p1 = (*this) (x, Yquantum);
			  p1 = p1 << C;
			}

			// draw right pixel;
			C.setAlpha ((int) (0xFF * (r - intR + 0.5f) * alpha));
			Pixel p2 = (*this) (intR, Yquantum);
			p2 = p2 << C;
		  }
		}
	  }
	  Yquantum++;
	}
  }

  // Delete structures
  Vertex * v = root;
  do
  {
	Vertex * kill = v;
	v = v->succ;
	delete kill;
  }
  while (v != root);
}

void
CanvasImage::drawFilledRectangle (const Point & corner0, const Point & corner1, unsigned int colorFill)
{
  int x0 = (int) roundp (corner0.x);
  int x1 = (int) roundp (corner1.x);
  int y0 = (int) roundp (corner0.y);
  int y1 = (int) roundp (corner1.y);

  if (x0 > x1)
  {
	swap (x0, x1);
  }
  if (y0 > y1)
  {
	swap (y0, y1);
  }

  if (x1 < 0  ||  x0 >= width  ||  y1 < 0  ||  y0 >= height) return;

  x0 = max (x0, 0);
  x1 = min (x1, width - 1);
  y0 = max (y0, 0);
  y1 = min (y1, height - 1);

  for (int y = y0; y <= y1; y++)
  {
	for (int x = x0; x <= x1; x++)
	{
	  setRGBA (x, y, colorFill);
	}
  }
}

void
CanvasImage::drawEllipse (const Point & center, const MatrixFixed<double,2,2> & shape, float radius, unsigned int color, float startAngle, float endAngle, bool inverse)
{
# ifdef HAVE_LAPACK
  // Adjust for scaling and translation
  Point tcenter = trans (center);
  MatrixFixed<double,2,2> tshape;
  if (inverse)
  {
	tshape(0,0) = shape(0,0) / (scaleX * scaleX);
	tshape(0,1) = shape(0,1) / (scaleX * scaleY);
	tshape(1,0) = shape(1,0) / (scaleY * scaleX);
	tshape(1,1) = shape(1,1) / (scaleY * scaleY);
  }
  else
  {
	tshape(0,0) = shape(0,0) * (scaleX * scaleX);
	tshape(0,1) = shape(0,1) * (scaleX * scaleY);
	tshape(1,0) = shape(1,0) * (scaleY * scaleX);
	tshape(1,1) = shape(1,1) * (scaleY * scaleY);
  }

  // Prepare ellipse parameters
  float r2 = radius * radius;
  float a2 = r2;
  float b2 = r2;

  /*
  Matrix<double> U;
  Matrix<double> D;
  Matrix<double> rot;
  gesvd (tshape, U, D, rot);
  */

  Matrix<double> D;
  Matrix<double> rot;
  geev (tshape, D, rot);

  if (inverse)
  {
	a2 /= D[0];
	b2 /= D[1];
  }
  else
  {
	a2 *= D[0];
	b2 *= D[1];
  }

  float a  = sqrt (a2);
  float b  = sqrt (b2);
  float a2_b2 = a2 / b2;
  float b2_a2 = b2 / a2;
  float ratio = a / b;

  // Prepare angle ranges
  startAngle = mod2pi (startAngle);
  endAngle   = mod2pi (endAngle);
  if (scaleX < 0)
  {
	flipX (startAngle);
	flipX (endAngle);
	swap (startAngle, endAngle);
  }
  if (scaleY < 0)
  {
	flipY (startAngle);
	flipY (endAngle);
	swap (startAngle, endAngle);
  }
  startAngle = mod2pi (startAngle);
  endAngle   = mod2pi (endAngle);
  if (endAngle <= startAngle)
  {
	endAngle += TWOPIf;
  }

  // Determine where to switch from tracking x-axis to tracking y-axis
  float maxA = a / sqrt (b2_a2 + 1);
  float maxB = b / sqrt (a2_b2 + 1);

  // Do the actual drawing
  Point p;
  for (float i = 0; i <= maxA; i++)  // i < a
  {
    float yt = sqrt (b2 - b2_a2 * i * i);
	float angle = atan ((yt / i) * ratio);

	if (inRange (angle, startAngle, endAngle))
	{
	  p.x = tcenter.x + rot(0,0) * i + rot(0,1) * yt;
	  p.y = tcenter.y + rot(1,0) * i + rot(1,1) * yt;
	  pen (p, color);
	}
	if (inRange (M_PI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * i + rot(0,1) * yt;
	  p.y = tcenter.y - rot(1,0) * i + rot(1,1) * yt;
	  pen (p, color);
	}
	if (inRange (M_PI + angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * i - rot(0,1) * yt;
	  p.y = tcenter.y - rot(1,0) * i - rot(1,1) * yt;
	  pen (p, color);
	}
	if (inRange (TWOPI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x + rot(0,0) * i - rot(0,1) * yt;
	  p.y = tcenter.y + rot(1,0) * i - rot(1,1) * yt;
	  pen (p, color);
	}
  }
  for (float j = 0; j <= maxB; j++)  // j < b
  {
    float xt = sqrt (a2 - a2_b2 * j * j);
	float angle = atan ((j / xt) * ratio);

	if (inRange (angle, startAngle, endAngle))
	{
	  p.x = tcenter.x + rot(0,0) * xt + rot(0,1) * j;
	  p.y = tcenter.y + rot(1,0) * xt + rot(1,1) * j;
	  pen (p, color);
	}
	if (inRange (M_PI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * xt + rot(0,1) * j;
	  p.y = tcenter.y - rot(1,0) * xt + rot(1,1) * j;
	  pen (p, color);
	}
	if (inRange (M_PI + angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * xt - rot(0,1) * j;
	  p.y = tcenter.y - rot(1,0) * xt - rot(1,1) * j;
	  pen (p, color);
	}
	if (inRange (TWOPI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x + rot(0,0) * xt - rot(0,1) * j;
	  p.y = tcenter.y + rot(1,0) * xt - rot(1,1) * j;
	  pen (p, color);
	}
  }
# endif
}

/**
   \todo One key optimization would be to make the "visited" image only
   big enough to hold the actual region.
 **/
void
CanvasImage::drawMSER (const PointMSER & point, const Image & image, unsigned int colorFill, unsigned int colorBorder)
{
  Image grayImage = image * GrayChar;
  PixelBufferPacked * buffer = (PixelBufferPacked *) grayImage.buffer;
  if (! buffer) throw "Can't draw MSER on anything besides a packed buffer for now.";
  unsigned char * g = (unsigned char *) buffer->memory;

  int width = image.width;
  int height = image.height;
  int lastX = width - 1;
  int lastY = height - 1;

  Image visited (width, height, GrayChar);
  visited.clear ();
  unsigned char * v = (unsigned char *) ((PixelBufferPacked *) visited.buffer)->memory;

  vector<int> frontier;
  vector<int> newFrontier;
  frontier.push_back (point.index);
  v[point.index] = 1;

  while (frontier.size ())
  {
	newFrontier.clear ();
	newFrontier.reserve ((int) ceil (frontier.size () * 1.2));
	for (int i = 0; i < frontier.size (); i++)
	{
	  int index = frontier[i];
	  int x = index % width;
	  int y = index / width;

	  if (point.sign ? g[index] > point.threshold : g[index] < point.threshold)
	  {
		if (colorBorder & 0xFF) setRGBA (x, y, colorBorder);
	  }
	  else
	  {
		if (colorFill & 0xFF) setRGBA (x, y, colorFill);

		#define visit(n) \
		if (! v[n]) \
		{ \
		  v[n] = 1; \
		  newFrontier.push_back (n); \
		}

		if (x > 0)     visit (index - 1);
		if (x < lastX) visit (index + 1);
		if (y > 0)     visit (index - width);
		if (y < lastY) visit (index + width);
	  }
	}
	swap (frontier, newFrontier);
  }
}

/**
   \todo Take into account current transformation when computing matrix.
   Also, CanvasImage and CanvasPS should be interchangeable in all regards.
   However, not sure that is the case right now.
 **/
void
CanvasImage::drawText (const string & text, const Point & point, unsigned int color, float angle)
{
# ifdef HAVE_FREETYPE
  if (face == 0)
  {
	setFont ("Helvetica", 12);
  }

  FT_Matrix matrix;
  matrix.xx = (FT_Fixed) ( cos (angle) * 0x10000L);
  matrix.xy = (FT_Fixed) ( sin (angle) * 0x10000L);
  matrix.yx = (FT_Fixed) (-sin (angle) * 0x10000L);
  matrix.yy = (FT_Fixed) ( cos (angle) * 0x10000L);

  FT_GlyphSlot slot = ((FT_Face) face)->glyph;

  Point pen = point;
  Pixel C (color);
  for (int i = 0; i < text.size (); i++)
  {
    FT_Set_Transform ((FT_Face) face, &matrix, 0);

	FT_Error error = FT_Load_Char ((FT_Face) face, text[i], FT_LOAD_RENDER);
    if (error) continue;

	FT_Bitmap & bitmap = slot->bitmap;
	int left = (int) roundp (pen.x + slot->bitmap_left);
	int top  = (int) roundp (pen.y - slot->bitmap_top);

	int xl = max (0,            -left);
	int xh = min (bitmap.width, width - left) - 1;
	int yl = max (0,            -top);
	int yh = min (bitmap.rows,  height - top) - 1;

	switch (bitmap.pixel_mode)
	{
	  case FT_PIXEL_MODE_MONO:
	  {
		cerr << "mono" << endl;
		for (int y = yl; y <= yh; y++)
		{
		  int x = xl;
		  unsigned char * alpha = bitmap.buffer + bitmap.pitch * y + (x / 8);
		  unsigned char mask = 0x80 >> (x % 8);
		  while (x <= xh)
		  {
			while (mask  &&  x <= xh)
			{
			  if (*alpha & mask) setRGBA (left + x, top + y, color);
			  mask >>= 1;
			  x++;
			}
			alpha++;
			mask = 0x80;
		  }
		}
		break;
	  }
	  default:  // Assume gray
	  {
		cerr << "gray" << endl;
		for (int y = yl; y <= yh; y++)
		{
		  int x = xl;
		  unsigned char * alpha = bitmap.buffer + bitmap.pitch * y + x;
		  while (x <= xh)
		  {
			C.setAlpha (*alpha++);
			Pixel p = (*this) (left + x, top + y);
			p = p << C;
			x++;
		  }
		}
	  }
	}

    pen.x += slot->advance.x / 64.0f;
    pen.y -= slot->advance.y / 64.0f;
  }
# endif
}

void
CanvasImage::setTranslation (float x, float y)
{
  transX = x;
  transY = y;
}

void
CanvasImage::setScale (float x, float y)
{
  scaleX = x;
  scaleY = y;
}

void
CanvasImage::setLineWidth (float width)
{
  if (lineWidth == width) return;
  lineWidth = width;

  float radius = width / 2;
  float sigma2 = radius * radius;

  int h = (int) ceil (radius);
  int w = 2 * h + 1;

  ImageOf<float> temp (w, w, GrayFloat);
  for (int x = 0; x < w; x++)
  {
	for (int y = 0; y < w; y++)
	{
	  float dx = x - h;
	  float dy = y - h;
	  float t = expf (0.5 - (dx * dx + dy * dy) / (2 * sigma2));
	  t = min (1.0f, t);
	  temp (x, y) = t;
	}
  }

  penTip = temp * GrayChar;
}

void
CanvasImage::setPointSize (float radius)
{
  pointRadius = radius;
}


void
CanvasImage::setFont (const std::string & name, float size)
{
# ifdef HAVE_FREETYPE
  initFontLibrary ();
  if (fontMap.size () < 1) throw "No fonts";

  // Attempt direct match
  string queryName = name;
  lowercase (queryName);
  map<string, string>::iterator it = fontMap.find (queryName);
  // Fallback 1: Attempt substring match
  if (it == fontMap.end ())
  {
	int bestLength = INT_MAX;
	map<string, string>::iterator bestEntry = fontMap.end ();
	for (it = fontMap.begin (); it != fontMap.end (); it++)
	{
	  if (it->first.find (queryName) != string::npos  &&  it->first.size () < bestLength)
	  {
		bestLength = it->first.size ();
		bestEntry = it;
	  }
	}
	it = bestEntry;
  }
  // Fallback 2: Use font substitution table
  if (it == fontMap.end ())
  {
	// Not yet implemented
  }
  // Fallback 3: Use courier (substring match)
  if (it == fontMap.end ())
  {
	int bestLength = INT_MAX;
	map<string, string>::iterator bestEntry = fontMap.end ();
	for (it = fontMap.begin (); it != fontMap.end (); it++)
	{
	  if (it->first.find ("courier") != string::npos  &&  it->first.size () < bestLength)
	  {
		bestLength = it->first.size ();
		bestEntry = it;
	  }
	}
	it = bestEntry;
  }
  // Fallback 4: Take anything
  if (it == fontMap.end ())
  {
	it = fontMap.begin ();
  }
  cerr << "got " << it->first << endl;

  if (face)
  {
	FT_Done_Face ((FT_Face) face);
  }
  FT_Error error = FT_New_Face ((FT_Library) library, it->second.c_str (), 0, (FT_Face *) &face);
  if (error  ||  face == 0) throw "Can't load font";

  error = 1;
  if (((FT_Face) face)->face_flags & FT_FACE_FLAG_SCALABLE)
  {
	error = FT_Set_Char_Size ((FT_Face) face,
							  (int) roundp (size * 64),
							  0,
							  96,  // estimated pixels per inch of image
							  0);
  }
  else if (((FT_Face) face)->face_flags & FT_FACE_FLAG_FIXED_SIZES)
  {
	// Enumerate fixed sizes to find one closest to requested size
	FT_Bitmap_Size * bestSize = 0;
	float bestDistance = INFINITY;
	for (int i = 0; i < ((FT_Face) face)->num_fixed_sizes; i++)
	{
	  FT_Bitmap_Size & bs = ((FT_Face) face)->available_sizes[i];
	  float distance = fabs (bs.size / 64.0f - size);
	  if (distance < bestDistance)
	  {
		bestDistance = distance;
		bestSize = &bs;
	  }
	}

	if (bestSize)
	{
	  error = FT_Set_Pixel_Sizes ((FT_Face) face, 0, (int) roundp (bestSize->y_ppem / 64.0f));
	}
  }
  if (error) throw "Requested font size is not available";
# endif
}

void
CanvasImage::initFontLibrary ()
{
# ifdef HAVE_FREETYPE
  if (library != 0) return;

  FT_Error error = FT_Init_FreeType ((FT_Library *) &library);
  if (error) throw error;

  // Scan default list of likely font directories...
#   ifdef WIN32
  scanFontDirectory ("/WINDOWS/Fonts");
#   else
  scanFontDirectory ("/cygdrive/c/WINDOWS/Fonts");
  scanFontDirectory ("/usr/X11R6/lib/X11/fonts/TTF");
  scanFontDirectory ("/usr/X11R6/lib/X11/fonts/Type1");
  scanFontDirectory ("/usr/share/fonts/default/Type1");
  // Do these once scanFontDirectory looks for "fonts.dir".  Otherwise, takes too long.
  //scanFontDirectory ("/usr/X11R6/lib/X11/fonts/100dpi");
  //scanFontDirectory ("/usr/X11R6/lib/X11/fonts/75dpi");
#   endif
# endif
}

void
CanvasImage::addFontFile (const string & path)
{
# ifdef HAVE_FREETYPE
  // Probe file to see if it is a font
  FT_Face face;
  FT_Error error = FT_New_Face ((FT_Library) library, path.c_str (), 0, (FT_Face *) &face);
  if (error  ||  face == 0) return;  // not a valid font file

  // Determine Postscript name
  const char * name = FT_Get_Postscript_Name (face);
  string postscriptName;
  if (name)
  {
	postscriptName = name;
  }
  else
  {
	const char * name  = face->family_name;
	const char * style = face->style_name;

	// Strip spaces
	const char * c = name;
	while (*c != 0)
	{
	  if (*c != ' ') postscriptName += *c;
	  c++;
	}

	if (style)
	{
	  postscriptName += "-";
	  postscriptName += style;
	}
  }

  lowercase (postscriptName);
  fontMap.insert (make_pair (postscriptName, path));
  cerr << ".";
# endif
}

void
CanvasImage::scanFontDirectory (const string & path)
{
# ifdef HAVE_FREETYPE
  cerr << "Scanning " << path << endl;

#   ifdef WIN32

  WIN32_FIND_DATA fd;
  HANDLE hFind = ::FindFirstFile ((path + "/*.*").c_str (), &fd);
  if (hFind == INVALID_HANDLE_VALUE)
  {
	return;
  }

  do
  {
	if (! (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
	  addFontFile (path + "/" + fd.cFileName);
	}
  }
  while (::FindNextFile (hFind, &fd));

  ::FindClose (hFind);

#   else

  DIR * dirh = opendir (path.c_str ());
  if (! dirh)
  {
	return;
  }

  struct dirent * dirp = readdir (dirh);
  while (dirp != NULL)
  {
	addFontFile (path + "/" + dirp->d_name);
	dirp = readdir (dirh);
  }

#   endif

  cerr << endl;
# endif
}
