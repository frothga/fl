#include "fl/descriptor.h"
#include "fl/pi.h"

// For debugging
#include "fl/time.h"
#include "fl/slideshow.h"
//#include "fl/canvas.h"


using namespace std;
using namespace fl;


// class DescriptorSpin -------------------------------------------------------

DescriptorSpin::DescriptorSpin (int binsRadial, int binsIntensity, float supportRadial, float supportIntensity)
{
  this->binsRadial       = binsRadial;
  this->binsIntensity    = binsIntensity;
  this->supportRadial    = supportRadial;
  this->supportIntensity = supportIntensity;
}

DescriptorSpin::DescriptorSpin (std::istream & stream)
{
  read (stream);
}

Vector<float>
DescriptorSpin::value (const Image & image, const PointAffine & point)
{
  double startTime = getTimestamp ();

  // Convert patch into a square
  // This is a temporary adapter until the rest of the code can be rewritten
  // to handle affine transformations.
  const int patchSize = 50;
  const float half = patchSize / 2.0;
  PointAffine center;
  center.x = (patchSize - 1) / 2.0;
  center.y = (patchSize - 1) / 2.0;
  center.scale = half / supportRadial;
  TransformGauss rectify (point.A / (half / (supportRadial * point.scale)), true);
  rectify.setPeg (point.x, point.y, patchSize, patchSize);
  ImageOf<float> patch = image * rectify;

  // Determine various sizes
  /*
  float width = supportRadial * point.scale;
  float binRadius = width / binsRadial;
  int x1 = (int) rint (point.x - width);
  int x2 = (int) rint (point.x + width);
  int y1 = (int) rint (point.y - width);
  int y2 = (int) rint (point.y + width);
  x1 = max (x1, 0);
  x2 = min (x2, image.width - 1);
  y1 = max (y1, 0);
  y2 = min (y2, image.height - 1);
  */
  float width = half;
  float binRadius = width / binsRadial;
  int x1 = 0;
  int x2 = patch.width - 1;
  int y1 = 0;
  int y2 = patch.width - 1;

  // Determine intensity bin values
  float minIntensity;
  float quantum;
  //rangeMeanDeviation (image, point, x1, y1, x2, y2, width, minIntensity, quantum);
  rangeMeanDeviation (patch, center, x1, y1, x2, y2, width, minIntensity, quantum);

  // Bin up all the pixels
  Vector<float> result (binsRadial * binsIntensity);
  result.clear ();
  for (int x = x1; x <= x2; x++)
  {
	float dx = x - center.x;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - center.y;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		int d = (int) ((patch(x,y) - minIntensity) / quantum);
		d = d >? 0;
		d = d <? binsIntensity - 1;
		int r = (int) (radius / binRadius);
		result[r * binsIntensity + d] += 1.0;
	  }
	  cerr << endl;
	}
  }

  // Convert to probabilities
cerr << "time = " << getTimestamp () - startTime << endl;
cerr << binRadius << endl;
float sum = 0;
  for (int r = 0; r < binsRadial; r++)
  {
	float total = 0;
	for (int d = 0; d < binsIntensity; d++)
	{
	  total += result[r * binsIntensity + d];
	}
sum += total;
cerr << r << "\t" << pow ((r + 1) * binRadius, 2) * PI << "\t" << total << "\t" << sum << endl;
	if (total > 0)
	{
	  for (int d = 0; d < binsIntensity; d++)
	  {
		result[r * binsIntensity + d] /= total;
	  }
	}
  }
cerr << endl;

  return result;
}

Image
DescriptorSpin::patch (const Vector<float> & value)
{
  ImageOf<float> result (binsRadial, binsIntensity, GrayFloat);

  for (int r = 0; r < binsRadial; r++)
  {
	for (int d = 0; d < binsIntensity; d++)
	{
	  result (r, d) = 1.0 - value[(r * binsIntensity) + (binsIntensity - d - 1)];
	}
  }

  return result;
}

void
DescriptorSpin::read (std::istream & stream)
{
  stream.read ((char *) &binsRadial,       sizeof (binsRadial));
  stream.read ((char *) &binsIntensity,    sizeof (binsIntensity));
  stream.read ((char *) &supportRadial,    sizeof (supportRadial));
  stream.read ((char *) &supportIntensity, sizeof (supportIntensity));
}

void
DescriptorSpin::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &binsRadial,       sizeof (binsRadial));
  stream.write ((char *) &binsIntensity,    sizeof (binsIntensity));
  stream.write ((char *) &supportRadial,    sizeof (supportRadial));
  stream.write ((char *) &supportIntensity, sizeof (supportIntensity));
}

void
DescriptorSpin::rangeMinMax (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum)
{
  ImageOf<float> that (image);
  minIntensity = 1;
  float maxIntensity = -1;
  for (int x = x1; x <= x2; x++)
  {
	float dx = x - point.x;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - point.y;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		minIntensity = minIntensity <? that (x, y);
		maxIntensity = maxIntensity >? that (x, y);
	  }
	}
  }
  float range = maxIntensity - minIntensity;
  if (range == 0)  // In case the image is completely flat
  {
	range = 1;
  }
  quantum = range / binsIntensity;
cerr << "Using min-max: " << minIntensity << " " << quantum << endl;
}

void
DescriptorSpin::rangeMeanDeviation (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float & minIntensity, float & quantum)
{
  ImageOf<float> that (image);
  float average = 0;
  float count = 0;
  for (int x = x1; x <= x2; x++)
  {
	float dx = x - point.x;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - point.y;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		float weight = 1.0 - radius / width;
		average += that (x, y) * weight;
		count += weight;
	  }
	}
  }
  average /= count;
  float deviation = 0;
  for (int x = x1; x <= x2; x++)
  {
	float dx = x - point.x;
	for (int y = y1; y <= y2; y++)
	{
	  float dy = y - point.y;
	  float radius = sqrt (dx * dx + dy * dy);
	  if (radius < width)
	  {
		float d = that (x, y) - average;
		float weight = 1.0 - radius / width;
		deviation += d * d * weight;
	  }
	}
  }
  deviation = sqrt (deviation / count);
  float range = 2.0 * supportIntensity * deviation;
  if (range == 0)  // In case the image is completely flat
  {
	range = 1.0;
  }

  quantum = range / binsIntensity;
  minIntensity = average - range / 2;
cerr << "Using average: " << minIntensity << " " << quantum << endl;
}

void
DescriptorSpin::doBinning (const Image & image, const PointAffine & point, int x1, int y1, int x2, int y2, float width, float minIntensity, float quantum, float binRadius, Vector<float> & result)
{
}
