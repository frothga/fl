#include "fl/descriptor.h"
#include "fl/lapackd.h"


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
  // Determine square region in source image to scan

  Matrix<double> R = point.rectification ();
  Matrix<double> S = !R;

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

  int sourceL = (int) rint (ptl.x <? ptr.x <? pbl.x <? pbr.x >? 0);
  int sourceR = (int) rint (ptl.x >? ptr.x >? pbl.x >? pbr.x <? image.width - 1);
  int sourceT = (int) rint (ptl.y <? ptr.y <? pbl.y <? pbr.y >? 0);
  int sourceB = (int) rint (ptl.y >? ptr.y >? pbl.y >? pbr.y <? image.height - 1);

  R.region (0, 0, 1, 2) *= binsRadial / supportRadial;  // Now R maps directly to radial bin values

  // Determine mapping between pixel values and intensity bins
  ImageOf<float> that (image);
  float average = 0;
  float count = 0;
  Point q;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).distance ();
	  if (radius < binsRadial)
	  {
		float weight = 1.0f - radius / binsRadial;
		average += that(x,y) * weight;
		count += weight;
	  }
	}
  }
  average /= count;
  float deviation = 0;
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float radius = (R * q).distance ();
	  if (radius < binsRadial)
	  {
		float d = that(x,y) - average;
		float weight = 1.0f - radius / binsRadial;
		deviation += d * d * weight;
	  }
	}
  }
  deviation = sqrt (deviation / count);
  float range = 2.0f * supportIntensity * deviation;
  if (range == 0.0f)  // In case the image is completely flat
  {
	range = 1.0f;
  }
  float quantum = range / binsIntensity;
  float minIntensity = average - range / 2 + 0.5f * quantum;

  // Bin up all the pixels
  Matrix<float> result (binsIntensity, binsRadial);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	q.y = y;
	for (int x = sourceL; x <= sourceR; x++)
	{
	  q.x = x;
	  float rf = (R * q).distance () - 0.5f;
	  if (rf < binsRadial)
	  {
		int rl = (int) floorf (rf);
		int rh = rl + 1;
		rf -= rl;
		float rf1 = 1.0f - rf;
		if (rh > binsRadial - 1)
		{
		  rh = binsRadial - 1;
		  rf = 0.0f;
		}
		rl = max (rl, 0);

		float df = (that(x,y) - minIntensity) / quantum;
		int dl = (int) floorf (df);
		int dh = dl + 1;
		df -= dl;
		float df1 = 1.0f - df;
		if (dl < 0)
		{
		  dl = 0;
		  dh = 0;
		}
		if (dh > binsIntensity - 1)
		{
		  dl = binsIntensity - 1;
		  dh = binsIntensity - 1;
		}

		result(dl,rl) += df1 * rf1;
		result(dl,rh) += df1 * rf;
		result(dh,rl) += df  * rf1;
		result(dh,rh) += df  * rf;
	  }
	}
  }

  // Convert to probabilities
  for (int r = 0; r < binsRadial; r++)
  {
	float sum = result.column (r).frob (1);
	result.column (r) /= sum;
  }

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

Comparison *
DescriptorSpin::comparison ()
{
  //return new HistogramIntersection;
  return new ChiSquared;
}

int
DescriptorSpin::dimension ()
{
  return binsRadial * binsIntensity;
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
