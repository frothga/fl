/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


12/2004 Fred Rothganger -- Compilability fix for MSVC.
05/2005 Fred Rothganger -- Implement drawText() via Freetype2.
09/2005 Fred Rothganger -- Add drawMSER.  Break dependency of canvas.h on
        Freetype2 include files.  Change lapackd.h to lapack.h
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/canvas.h"
#include "fl/lapack.h"
#include "fl/string.h"

#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H
#ifdef WIN32
#  include <windows.h>
#  undef min
#  undef max
#else
#  include <dirent.h>
#endif


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
	angle += (float) (2 * PI);
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
CanvasImage::drawFilledRectangle (const Point & corner0, const Point & corner1, unsigned int colorFill)
{
  int x0 = (int) rint (corner0.x);
  int x1 = (int) rint (corner1.x);
  int y0 = (int) rint (corner0.y);
  int y1 = (int) rint (corner1.y);

  if (x0 > x1)
  {
	swap (x0, x1);
  }
  if (y0 > y1)
  {
	swap (y0, y1);
  }

  for (int y = y0; y <= y1; y++)
  {
	for (int x = x0; x <= x1; x++)
	{
	  setRGBA (x, y, colorFill);
	}
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
	endAngle += (float) (2 * PI);
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

/**
   \todo One key optimization would be to make the "visited" image only
   big enough to hold the actual region.
 **/
void
CanvasImage::drawMSER (const PointMSER & point, const Image & image, unsigned int colorFill, unsigned int colorBorder)
{
  Image grayImage = image * GrayChar;
  unsigned char * g = (unsigned char *) grayImage.buffer;

  int width = image.width;
  int height = image.height;
  int lastX = width - 1;
  int lastY = height - 1;

  Image visited (width, height, GrayChar);
  visited.clear ();
  unsigned char * v = (unsigned char *) visited.buffer;

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
	int left = (int) rint (pen.x + slot->bitmap_left);
	int top  = (int) rint (pen.y - slot->bitmap_top);

	switch (bitmap.pixel_mode)
	{
	  case FT_PIXEL_MODE_MONO:
	  {
		for (int y = 0; y < bitmap.rows; y++)
		{
		  unsigned char * alpha = bitmap.buffer + bitmap.pitch * y;
		  int x = 0;
		  while (x < bitmap.width)
		  {
			unsigned char mask = 0x80;
			while (mask  &&  x < bitmap.width)
			{
			  if (*alpha & mask) setRGBA (left + x, top + y, color);
			  mask >>= 1;
			  x++;
			}
			alpha++;
		  }
		}
		break;
	  }
	  default:  // Assume gray
	  {
		for (int y = 0; y < bitmap.rows; y++)
		{
		  unsigned char * alpha = bitmap.buffer + bitmap.pitch * y;
		  for (int x = 0; x < bitmap.width; x++)
		  {
			C.setAlpha (*alpha++);
			Pixel p = (*this) (left + x, top + y);
			p = p << C;
		  }
		}
	  }
	}

    pen.x += slot->advance.x / 64.0f;
    pen.y -= slot->advance.y / 64.0f;
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

void
CanvasImage::setFont (const std::string & name, float size)
{
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
							  (int) rint (size * 64),
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
	  error = FT_Set_Pixel_Sizes ((FT_Face) face, 0, (int) rint (bestSize->y_ppem / 64.0f));
	}
  }
  if (error) throw "Requested font size is not available";
}

void
CanvasImage::initFontLibrary ()
{
  if (library != 0) return;

  FT_Error error = FT_Init_FreeType ((FT_Library *) &library);
  if (error) throw error;

  // Scan default list of likely font directories...
# ifdef WIN32
  scanFontDirectory ("/WINDOWS/Fonts");
# else
  scanFontDirectory ("/cygdrive/c/WINDOWS/Fonts");
  scanFontDirectory ("/usr/X11R6/lib/X11/fonts/TTF");
  scanFontDirectory ("/usr/X11R6/lib/X11/fonts/Type1");
  // Do these once scanFontDirectory looks for "fonts.dir".  Otherwise, takes too long.
  //scanFontDirectory ("/usr/X11R6/lib/X11/fonts/100dpi");
  //scanFontDirectory ("/usr/X11R6/lib/X11/fonts/75dpi");
# endif
}

void
CanvasImage::addFontFile (const string & path)
{
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
}

void
CanvasImage::scanFontDirectory (const string & path)
{
  cerr << "Scanning " << path;

# ifdef WIN32

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

# else

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

# endif

  cerr << endl;
}
