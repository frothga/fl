#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"


using namespace fl;
using namespace std;



// class DescriptorOrientationHistogram ---------------------------------------

DescriptorOrientationHistogram::DescriptorOrientationHistogram (int bins, float supportRadial, int supportPixel)
{
  this->bins          = bins;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;

  cutoff = 0.8f;

  lastImage = 0;
}

void
DescriptorOrientationHistogram::computeGradient (const Image & image)
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
DescriptorOrientationHistogram::value (const Image & image, const PointAffine & point)
{
  // First, prepeare the derivative images I_x and I_y.
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  Point center;
  float sigma;
  float radius;
  if (point.A(0,0) == 1.0f  &&  point.A(0,1) == 0.0f  &&  point.A(1,0) == 0.0f  &&  point.A(1,1) == 1.0f)  // No shape change, so we can work in context of original image.
  {
	computeGradient (image);

	radius = point.scale * supportRadial;
	sourceL = (int) floorf (point.x - radius);
	sourceR = (int) ceilf  (point.x + radius);
	sourceT = (int) floorf (point.y - radius);
	sourceB = (int) ceilf  (point.y + radius);

	sourceL = max (sourceL, 0);
	sourceR = min (sourceR, I_x.width - 1);
	sourceT = max (sourceT, 0);
	sourceB = min (sourceB, I_x.height - 1);

	center = point;
	sigma = point.scale;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	Matrix<double> S = ! point.rectification ();
	S(2,0) = 0;
	S(2,1) = 0;
	S(2,2) = 1;

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

	center.x = supportPixel - 0.5f;
	center.y = center.x;
	sigma = supportPixel / supportRadial;
  }

  // Second, gather up the gradient histogram.
  float histogram[bins];
  memset (histogram, 0, sizeof (histogram));
  float radius2 = radius * radius;
  float sigma2 = 2.0 * sigma * sigma;
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  float cx = x - center.x;
	  float cy = y - center.y;
	  float d2 = cx * cx + cy * cy;
	  if (d2 < radius2)
	  {
		float dx = I_x(x,y);
		float dy = I_y(x,y);
		float angle = atan2 (dy, dx);
		int bin = (int) ((angle + PI) * bins / (2 * PI));
		bin = min (bin, bins - 1);  // Technically, these two lines should not be necessary.  They compensate for numerical jitter.
		bin = max (bin, 0);
		float weight = sqrtf (dx * dx + dy * dy) * expf (- (cx * cx + cy * cy) / sigma2);
		histogram[bin] += weight;
	  }
	}
  }

  // Smooth histogram
  for (int i = 0; i < 6; i++)
  {
	float previous = histogram[bins - 1];
	for (int j = 0; j < bins; j++)
	{
      float current = histogram[j];
      histogram[j] = (previous + current + histogram[(j + 1) % bins]) / 3.0;
      previous = current;
	}
  }

  // Find max value
  float maximum = 0;
  for (int i = 0; i < bins; i++)
  {
	maximum = max (maximum, histogram[i]);
  }
  float threshold = cutoff * maximum;

  // Collect peaks
  vector<float> angles;
  for (int i = 0; i < bins; i++)
  {
	float h0 = histogram[(i == 0 ? bins : i) - 1];
	float h1 = histogram[i];
	float h2 = histogram[(i + 1) % bins];
	if (h1 > h0  &&  h1 > h2  &&  h1 >= threshold)
	{
	  float peak = 0.5f * (h0 - h2) / (h0 - 2.0f * h1 + h2);
	  angles.push_back ((i + 0.5f + peak) * 2 * PI / bins - PI);
	}
  }

  // Store peak angles in result
  Vector<float> result (angles.size ());
  for (int i = 0; i < angles.size (); i++)
  {
	result[i] = angles[i];
  }
  return result;
}

Image
DescriptorOrientationHistogram::patch (const Vector<float> & value)
{
  CanvasImage result;

  // TODO: actually implement this method!

  result.clear ();
  return result;
}

void
DescriptorOrientationHistogram::read (std::istream & stream)
{
  stream.read ((char *) &bins,          sizeof (bins));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));
  stream.read ((char *) &cutoff,        sizeof (cutoff));
}

void
DescriptorOrientationHistogram::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &bins,          sizeof (bins));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
  stream.write ((char *) &cutoff,        sizeof (cutoff));
}
