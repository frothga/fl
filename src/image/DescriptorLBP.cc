#include "fl/descriptor.h"
#include "fl/canvas.h"
#include "fl/pi.h"
#include "fl/lapackd.h"

// for debugging
#include "fl/time.h"


using namespace fl;
using namespace std;


// class DescriptorLBP --------------------------------------------------------

DescriptorLBP::DescriptorLBP (int P, float R, float supportRadial, int supportPixel)
{
  this->P             = P;
  this->R             = R;
  this->supportRadial = supportRadial;
  this->supportPixel  = supportPixel;

  initialize ();
}

void
DescriptorLBP::initialize ()
{
}

inline void
DescriptorLBP::add (const Image & image, const int x, const int y, Vector<float> & result)
{
  vector<bool> bits (P);
  int ones = 0;

  float center;
  image.getGray (x, y, center);
  for (int i = 0; i < P; i++)
  {
	// This set of calculations to find {xf, xl, xh, yf, yl, yh} can be done
	// once and stored.  Move to initialize() once this code is working well.
	float angle = i * 2 * PI / P;
	float xf = x + R * cosf (angle);
	float yf = y + R * sinf (angle);
	int xl = (int) floorf (xf);
	int yl = (int) floorf (yf);
	xf -= xl;
	yf -= yl;
	int xh = xl + 1;
	int yh = yl + 1;

	float pll;
	float plh;
	float phl;
	float phh;
	image.getGray (xl, yl, pll);
	image.getGray (xl, yh, plh);
	image.getGray (xh, yl, phl);
	image.getGray (xh, yh, phh);
	float xf1 = 1.0f - xf;
	float p = (pll * xf1 + phl * xf) * (1.0f - yf) + (plh * xf1 + phh * xf) * yf;
	bool sign = p >= center;
	bits[i] = sign;
	if (sign) ones++;
  }

  int transitions = bits.back () == bits.front () ? 0 : 1;
  for (int i = 1; i < P; i++)
  {
	if (bits[i-1] != bits[i])
	{
	  transitions++;
	}
  }

  if (transitions > 2)
  {
	ones = P + 1;
  }

  result[ones]++;
}

Vector<float>
DescriptorLBP::value (const Image & image, const PointAffine & point)
{
  Matrix<double> S = ! point.rectification ();
  S(2,0) = 0;
  S(2,1) = 0;
  S(2,2) = 1;
  
  int sourceT;
  int sourceL;
  int sourceB;
  int sourceR;
  Image patch;
  if (S(0,1) == 0  &&  S(1,0) == 0)  // Special case: indicates that point describes a rectangular region in the image, rather than a patch.
  {
	double h = fabs (S(0,0) * supportRadial);
	double v = fabs (S(1,1) * supportRadial);
	sourceL = (int) rint (S(0,2) - h >? R);
	sourceR = (int) rint (S(0,2) + h <? image.width - 1 - R);
	sourceT = (int) rint (S(1,2) - v >? R);
	sourceB = (int) rint (S(1,2) + v <? image.height - 1 - R);
	patch = image;
  }
  else  // Shape change, so we must compute a transformed patch
  {
	int patchSize = 2 * supportPixel;
	double scale = supportPixel / supportRadial;
	Transform t (S, scale);
	t.setWindow (0, 0, patchSize, patchSize);
	Image patch = image * t;
	patch *= GrayFloat;

	sourceT = (int) ceilf (R);
	sourceL = sourceT;
	sourceB = (int) floorf (patchSize - 1 - R);
	sourceR = sourceB;
  }

  // Gather LBP values into histogram
  Vector<float> result (P + 1);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  add (patch, x, y, result);
	}
  }
  result /= result.frob (1);
  return result;
}

Vector<float>
DescriptorLBP::value (const Image & image)
{
double start = getTimestamp ();
  int sourceL = (int) ceilf (R);
  int sourceR = (int) floorf (image.width - 1 - R);
  int sourceT = sourceL;
  int sourceB = (int) floorf (image.height - 1 - R);
  cerr << sourceL << " " << sourceR << " " << sourceT << " " << sourceB << endl;
  Vector<float> result (P + 2);
  result.clear ();
  for (int y = sourceT; y <= sourceB; y++)
  {
	for (int x = sourceL; x <= sourceR; x++)
	{
	  if (image.getAlpha (x, y))
	  {
		add (image, x, y, result);
	  }
	}
  }
  cerr << "before normalization = " << result << " = " << result.frob (1) << endl;
  result /= result.frob (1);
  cerr << "time = " << getTimestamp () - start << endl;
  return result;
}

Image
DescriptorLBP::patch (const Vector<float> & value)
{
  Image result;
  return result;
}

Comparison *
DescriptorLBP::comparison ()
{
  return new ChiSquared;
}

int
DescriptorLBP::dimension ()
{
  return P + 2;
}

void
DescriptorLBP::read (std::istream & stream)
{
  stream.read ((char *) &P,             sizeof (P));
  stream.read ((char *) &R,             sizeof (R));
  stream.read ((char *) &supportRadial, sizeof (supportRadial));
  stream.read ((char *) &supportPixel,  sizeof (supportPixel));

  initialize ();
}

void
DescriptorLBP::write (std::ostream & stream, bool withName)
{
  if (withName)
  {
	stream << typeid (*this).name () << endl;
  }

  stream.write ((char *) &P,             sizeof (P));
  stream.write ((char *) &R,             sizeof (R));
  stream.write ((char *) &supportRadial, sizeof (supportRadial));
  stream.write ((char *) &supportPixel,  sizeof (supportPixel));
}
