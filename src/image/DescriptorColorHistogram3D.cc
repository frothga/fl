#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"


using namespace fl;
using namespace std;


// class DescriptorColorHistogram3D -------------------------------------------

DescriptorColorHistogram3D::DescriptorColorHistogram3D (int width, int height, float supportRadial)
{
  if (height < 1)
  {
	height = width;
  }

  this->width         = width;
  this->height        = height;
  this->supportRadial = supportRadial;

  initialize ();
}

DescriptorColorHistogram3D::DescriptorColorHistogram3D (istream & stream)
{
  read (stream);
}

DescriptorColorHistogram3D::~DescriptorColorHistogram3D ()
{
  if (valid)
  {
	delete[] valid;
  }

  if (histogram)
  {
	delete[] histogram;
  }
}

#define indexOf(u,v,y) ((u * width) + v) * height + y

void
DescriptorColorHistogram3D::initialize ()
{
  monochrome = false;

  histogram = new float[width * width * height];
  valid     = new bool [width * width * height];

  memset (valid, 0, width * width * height * sizeof (bool));
  dimension = 0;
  bool * vi = valid;
  for (int u = 0; u < width; u++)
  {
	float uf = (u + 0.5f) / width - 0.5f;
	for (int v = 0; v < width; v++)
	{
	  float vf = (v + 0.5f) / width - 0.5f;

	  // The following is based on the YUV-to-RGB conversion matrix found
	  // in PixelFormatYVYUChar::getRGBA().
	  // The idea is to find the range of Y values for which the YUV value
	  // converts into an RGB value that is in range.
	  float tr =                1.4022f * vf;
	  float tg = -0.3456f * uf -0.7145f * vf;
	  float tb =  1.7710f * uf;
	  float yl = 0;
	  float yh = 1;
	  yl = max (yl, 0.0f - tr);
	  yh = min (yh, 1.0f - tr);
	  yl = max (yl, 0.0f - tg);
	  yh = min (yh, 1.0f - tg);
	  yl = max (yl, 0.0f - tb);
	  yh = min (yh, 1.0f - tb);
	  for (int y = 0; y < height; y++)
	  {
		float yf = (y + 0.5f) / height;
		if (yf >= yl  &&  yf <= yh)
		{
		  *vi = true;
		  dimension++;
		}

		vi++;
	  }
	}
  }

  //cerr << dimension << " / " << width * width * height << endl;
}

void
DescriptorColorHistogram3D::clear ()
{
  memset (histogram, 0, width * width * height * sizeof (float));
}

union YUV
{
  unsigned int all;
  struct
  {
	unsigned char v;
	unsigned char u;
	unsigned char y;
  };
};

inline void
DescriptorColorHistogram3D::addToHistogram (const Image & image, const int x, const int y)
{
  YUV yuv;
  yuv.all = image.getYUV (x, y);
  float yf = yuv.y * height / 256.0f - 0.5f;
  float uf = yuv.u * width  / 256.0f - 0.5f;
  float vf = yuv.v * width  / 256.0f - 0.5f;
  int yl = (int) floorf (yf);
  int yh = yl + 1;
  int ul = (int) floorf (uf);
  int uh = ul + 1;
  int vl = (int) floorf (vf);
  int vh = vl + 1;
  yf -= yl;
  uf -= ul;
  vf -= vl;

  // If one of these values is clipped, then all the weight will go to
  // a single plane of 3D histogram.
  yl = max (yl, 0);
  yh = min (yh, height - 1);
  ul = max (ul, 0);
  uh = min (uh, width - 1);
  vl = max (vl, 0);
  vh = min (vh, width - 1);

  // Use bilinear method to distribute weight to 4 adjacent bins in
  // histogram.

  float uweight = 1.0f - uf;
  float vweight = (1.0f - vf) * uweight;
  histogram[indexOf(ul,vl,yl)] += (1.0f - yf) * vweight;
  histogram[indexOf(ul,vl,yh)] +=         yf  * vweight;
  vweight       =         vf  * uweight;
  histogram[indexOf(ul,vh,yl)] += (1.0f - yf) * vweight;
  histogram[indexOf(ul,vh,yh)] +=         yf  * vweight;

  uweight       =        uf;
  vweight       = (1.0f - vf) * uweight;
  histogram[indexOf(uh,vl,yl)] += (1.0f - yf) * vweight;
  histogram[indexOf(uh,vl,yh)] +=         yf  * vweight;
  vweight       =         vf  * uweight;
  histogram[indexOf(uh,vh,yl)] += (1.0f - yf) * vweight;
  histogram[indexOf(uh,vh,yh)] +=         yf  * vweight;
}

void
DescriptorColorHistogram3D::add (const Image & image, const int x, const int y)
{
  addToHistogram (image, x, y);
}

Vector<float>
DescriptorColorHistogram3D::finish ()
{
  Vector<float> result (dimension);
  int i = 0;
  bool *  vi = valid;
  float * hi = histogram;
  for (int u = 0; u < width; u++)
  {
	for (int v = 0; v < width; v++)
	{
	  for (int y = 0; y < height; y++)
	  {
		if (*vi++)
		{
		  result[i++] = *hi;
		}
		hi++;
	  }
	}
  }
  result /= result.frob (1);  // normalize to a probability distribution

  return result;
}

Vector<float>
DescriptorColorHistogram3D::value (const Image & image, const PointAffine & point)
{
  // Prepare to project patch into image.

  Matrix<double> R = point.rectification ();
  Matrix<double> S = ! R;
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;

  Vector<double> tl (3);
  tl[0] = -supportRadial;
  tl[1] = supportRadial;
  tl[2] = 1;
  Vector<double> tr (3);
  tr[0] = supportRadial;
  tr[1] = supportRadial;
  tr[2] = 1;
  Vector<double> bl (3);
  bl[0] = -supportRadial;
  bl[1] = -supportRadial;
  bl[2] = 1;
  Vector<double> br (3);
  br[0] = supportRadial;
  br[1] = -supportRadial;
  br[2] = 1;

  Point ptl = S * tl;
  Point ptr = S * tr;
  Point pbl = S * bl;
  Point pbr = S * br;

  sourceL = (int) floorf (ptl.x <? ptr.x <? pbl.x <? pbr.x >? 0);
  sourceR = (int) ceilf  (ptl.x >? ptr.x >? pbl.x >? pbr.x <? image.width - 1);
  sourceT = (int) floorf (ptl.y <? ptr.y <? pbl.y <? pbr.y >? 0);
  sourceB = (int) ceilf  (ptl.y >? ptr.y >? pbl.y >? pbr.y <? image.height - 1);

  // Gather color values into histogram
  clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (fabsf (q.x) <= supportRadial  &&  fabsf (q.y) <= supportRadial)
	  {
		addToHistogram (image, x, y);
	  }
	}
  }
  return finish ();
}

Vector<float>
DescriptorColorHistogram3D::value (const Image & image)
{
  clear ();
  if (image.format->hasAlpha)
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		if (image.getAlpha (x, y))
		{
		  addToHistogram (image, x, y);
		}
	  }
	}
  }
  else  // No alpha channel, so just process entire image w/o checking alpha.
  {
	for (int y = 0; y < image.height; y++)
	{
	  for (int x = 0; x < image.width; x++)
	  {
		addToHistogram (image, x, y);
	  }
	}
  }
  return finish ();
}

Image
DescriptorColorHistogram3D::patch (const Vector<float> & value)
{
  Image result (width, height * width, RGBAChar);
  result.clear ();

  float maximum = value.frob (INFINITY);

  YUV yuv;
  yuv.all = 0;
  int i = 0;
  bool * vi = valid;
  for (int u = 0; u < width; u++)
  {
	for (int v = 0; v < width; v++)
	{
	  for (int y = 0; y < height; y++)
	  {
		if (*vi++)
		{
		  yuv.y = (unsigned char) (255    * value[i++] / maximum);
		  if (yuv.y > 0)
		  {
			yuv.u = (unsigned char) (255.0f * (u + 0.5f) / width);
			yuv.v = (unsigned char) (255.0f * (v + 0.5f) / width);
			result.setYUV (u, (height - y - 1) * width + v, yuv.all);
		  }
		}
	  }
	}
  }

  return result;
}

Comparison *
DescriptorColorHistogram3D::comparison ()
{
  return new ChiSquared;
}

void
DescriptorColorHistogram3D::read (std::istream & stream)
{
  Descriptor::read (stream);

  stream.read ((char *) &width,         sizeof (width));
  stream.read ((char *) &height,        sizeof (height));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));

  initialize ();
}

void
DescriptorColorHistogram3D::write (std::ostream & stream, bool withName)
{
  Descriptor::write (stream, withName);

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &height,        sizeof (height));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
}
