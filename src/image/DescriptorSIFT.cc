#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"


using namespace fl;
using namespace std;



// class DescriptorSIFT ------------------------------------------------------

DescriptorSIFT::DescriptorSIFT (int width, int angles)
{
  this->width = width;
  this->angles = angles;

  supportRadial = 6;
  supportPixel = 32;
  sigmaWeight = width / 2.0;
  maxValue = 0.2f;

  lastImage = 0;
}

void
DescriptorSIFT::computeGradient (const Image & image)
{
  if (lastImage == &image)
  {
	return;
  }
  lastImage = &image;

  ImageOf<float> work = image * GrayFloat;

  I_x.format = &GrayFloat;
  I_y.format = &GrayFloat;
  I_x.resize (image.width, image.height);
  I_y.resize (image.width, image.height);

  // Compute all of I_x
  int lastX = image.width - 1;
  int penX  = lastX - 1;  // "pen" as in "penultimate"
  for (int y = 0; y < image.height; y++)
  {
	I_x(0,    y) = 2.0f * (work(1,    y) - work(0,   y));
	I_x(lastX,y) = 2.0f * (work(lastX,y) - work(penX,y));

	float * p = & work(2,y);
	float * m = & work(0,y);
	float * c = & I_x (1,y);
	float * end = c + (image.width - 2);
	while (c < end)
	{
	  *c++ = *p++ - *m++;
	}
  }

  // Compute top and bottom rows of I_y
  int lastY = image.height - 1;
  int penY  = lastY - 1;
  float * pt = & work(0,1);
  float * mt = & work(0,0);
  float * pb = & work(0,lastY);
  float * mb = & work(0,penY);
  float * ct = & I_y (0,0);
  float * cb = & I_y (0,lastY);
  float * end = ct + image.width;
  while (ct < end)
  {
	*ct++ = 2.0f * (*pt++ - *mt++);
	*cb++ = 2.0f * (*pb++ - *mb++);
  }

  // Compute everything else in I_y
  float * p = & work(0,2);
  float * m = & work(0,0);
  float * c = & I_y (0,1);
  end = &I_y(lastX,penY) + 1;
  while (c < end)
  {
	*c++ = *p++ - *m++;
  }
}

Vector<float>
DescriptorSIFT::value (const Image & image, const PointAffine & point)
{
  // First, prepeare the derivative images I_x and I_y, and prepare projection
  // information between the derivative images and the bins.

  const float center = (width - 1) / 2.0f;
  const float sigma2 = 2.0f * sigmaWeight * sigmaWeight;

  Matrix<double> R = point.rectification ();  // Later, minor changes to R will allow it to function as the rectification from image to histogram bins.
  Matrix<double> S = ! R;
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  float angleOffset;

  if (point.A(0,0) == 1.0f  &&  point.A(0,1) == 0.0f  &&  point.A(1,0) == 0.0f  &&  point.A(1,1) == 1.0f)  // No shape change, so we can work in context of original image.
  {
	computeGradient (image);

	// Project the patch into the gradient image in order to find the window
	// for scanning pixels there.

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

	sourceL = (int) rint (ptl.x <? ptr.x <? pbl.x <? pbr.x >? 0);
	sourceR = (int) rint (ptl.x >? ptr.x >? pbl.x >? pbr.x <? I_x.width - 1);
	sourceT = (int) rint (ptl.y <? ptr.y <? pbl.y <? pbr.y >? 0);
	sourceB = (int) rint (ptl.y >? ptr.y >? pbl.y >? pbr.y <? I_x.height - 1);

	R.region (0, 0, 1, 2) *= width / (2 * supportRadial);
	R(0,2) += center;
	R(1,2) += center;

	angleOffset = - point.angle;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (S, scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * GrayFloat * t;

	lastImage = 0;
	computeGradient (patch);

	sourceT = 0;
	sourceL = 0;
	sourceB = patchSize - 1;
	sourceR = sourceB;

	R.clear ();
	R(0,0) = (double) width / patchSize;
	R(1,1) = R(0,0);
	R(0,2) = 0.5 * R(0,0) - 0.5;
	R(1,2) = R(0,2);
	R(2,2) = 1;

	angleOffset = 0;
  }

  // Second, gather up the gradient histogram that constitutes the SIFT key.
  float histogram[width][width][angles];
  memset (histogram, 0, sizeof (histogram));
  const float binLimit = width - 0.5f;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  Point q = R * Point (x, y);
	  if (q.x >= -0.5f  &&  q.x < binLimit  &&  q.y >= -0.5f  &&  q.y < binLimit)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx) + angleOffset;
		mod2pi (angle);
		angle *= angles / (2 * PI);
		float xc = q.x - center;
		float yc = q.y - center;
		float weight = sqrtf (dx * dx + dy * dy) * expf (- (xc * xc + yc * yc) / sigma2);

		int xl = (int) floorf (q.x);
		int xh = xl + 1;
		int yl = (int) floorf (q.y);
		int yh = yl + 1;
		int al = (int) floorf (angle);
		int ah = al + 1;
		if (ah >= angles)
		{
		  ah = 0;
		}

		float xf = q.x - xl;
		float yf = q.y - yl;
		float af = angle - al;

		// Use trilinear method to distribute weight to 8 adjacent bins in histogram.
		if (xl >= 0)
		{
		  float xweight = (1.0f - xf) * weight;
		  if (yl >= 0)
		  {
			float yweight = (1.0f - yf) * xweight;
			histogram[xl][yl][al] += (1.0f - af) * yweight;
			histogram[xl][yl][ah] +=         af  * yweight;
		  }
		  if (yh < width)
		  {
			float yweight = yf * xweight;
			histogram[xl][yh][al] += (1.0f - af) * yweight;
			histogram[xl][yh][ah] +=         af  * yweight;
		  }
		}
		if (xh < width)
		{
		  float xweight = xf * weight;
		  if (yl >= 0)
		  {
			float yweight = (1.0f - yf) * xweight;
			histogram[xh][yl][al] += (1.0f - af) * yweight;
			histogram[xh][yl][ah] +=         af  * yweight;
		  }
		  if (yh < width)
		  {
			float yweight = yf * xweight;
			histogram[xh][yh][al] += (1.0f - af) * yweight;
			histogram[xh][yh][ah] +=         af  * yweight;
		  }
		}
	  }
	}
  }

  Vector<float> result (&histogram[0][0][0], width * width * angles);
  result.copyFrom (result);  // force creation of a new buffer (one that won't evaporate at the end of this method)
  result.normalize ();
  bool changed = false;
  for (int i = 0; i < result.rows (); i++)
  {
	if (result[i] > maxValue)
	{
	  result[i] = maxValue;
	  changed = true;
	}
  }
  if (changed)
  {
	result.normalize ();
  }

  return result;
}

Image
DescriptorSIFT::patch (const Vector<float> & value)
{
  CanvasImage result;

  // TODO: actually implement this method!

  result.clear ();
  return result;
}

void
DescriptorSIFT::read (std::istream & stream)
{
  stream.read ((char *) &width,         sizeof (width));
  stream.read ((char *) &angles,        sizeof (angles));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.read ((char *) &maxValue,      sizeof (maxValue));
}

void
DescriptorSIFT::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &width,         sizeof (width));
  stream.write ((char *) &angles,        sizeof (angles));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &sigmaWeight,   sizeof (sigmaWeight));
  stream.write ((char *) &maxValue,      sizeof (maxValue));
}
