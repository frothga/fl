#include "fl/canvas.h"
#include "fl/lapackd.h"

#include <algorithm>


using namespace fl;
using namespace std;


// Utiltiy functions ----------------------------------------------------------

static inline void
flipX (float & angle)
{
  // Act as if circle has been flipped along vertical axis
  if (angle < PI)
  {
	angle = PI - angle;
  }
  else
  {
	angle = 3 * PI - angle;
  }
}

static inline void
flipY (float & angle)
{
  // Act as if circle has been flipped along horizontal axis
  angle = 2 * PI - angle;
}

static inline bool
inRange (float angle, const float & startAngle, const float & endAngle)
{
  while (angle < startAngle)
  {
	angle += 2 * PI;
  }
  return angle <= endAngle;
}


// class CanvasImage ----------------------------------------------------------

// Most of the implementations here are hacks based on float.  They could be
// made more efficient for changing to Bresenhem-like approaches.

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
	setRGBA ((int) rint (p.x), (int) rint (p.y), color);
  }
  else
  {
	int h = penTip.width / 2;
	int px = (int) rint (p.x) - h;
	int py = (int) rint (p.y) - h;

	int xl = max (0,                 -px);
	int xh = min (penTip.width - 1,  (width - 1) - px);
	int yl = max (0,                 -py);
	int yh = min (penTip.height - 1, (height - 1) - py);

	for (int x = xl; x <= xh; x++)
	{
	  for (int y = yl; y <= yh; y++)
	  {
		unsigned int c = penTip (x, y);
		c <<= 24;
		c |= color & 0xFFFFFF;
		Pixel C;
		C.setRGBA (c);
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

void
CanvasImage::drawSegment (const Point & a, const Point & b, unsigned int color)
{
  Point ta = trans (a);
  Point tb = trans (b);

  float dx = tb.x - ta.x;
  float dy = tb.y - ta.y;

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
CanvasImage::drawPolygon (const std::vector<Point> & points, unsigned int color)
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

void
CanvasImage::drawEllipse (const Point & center, const Matrix2x2<double> & shape, float radius, unsigned int color, float startAngle, float endAngle, bool inverse)
{
  // Adjust for scaling and translation
  Point tcenter = trans (center);
  Matrix2x2<double> tshape;
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
	endAngle += 2 * PI;
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
	if (inRange (PI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * i + rot(0,1) * yt;
	  p.y = tcenter.y - rot(1,0) * i + rot(1,1) * yt;
	  pen (p, color);
	}
	if (inRange (PI + angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * i - rot(0,1) * yt;
	  p.y = tcenter.y - rot(1,0) * i - rot(1,1) * yt;
	  pen (p, color);
	}
	if (inRange (2 * PI - angle, startAngle, endAngle))
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
	if (inRange (PI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * xt + rot(0,1) * j;
	  p.y = tcenter.y - rot(1,0) * xt + rot(1,1) * j;
	  pen (p, color);
	}
	if (inRange (PI + angle, startAngle, endAngle))
	{
	  p.x = tcenter.x - rot(0,0) * xt - rot(0,1) * j;
	  p.y = tcenter.y - rot(1,0) * xt - rot(1,1) * j;
	  pen (p, color);
	}
	if (inRange (2 * PI - angle, startAngle, endAngle))
	{
	  p.x = tcenter.x + rot(0,0) * xt - rot(0,1) * j;
	  p.y = tcenter.y + rot(1,0) * xt - rot(1,1) * j;
	  pen (p, color);
	}
  }
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
