#include "fl/interest.h"
#include "fl/lapacks.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestDOG ----------------------------------------------------------

InterestDOG::InterestDOG (float firstScale, float lastScale, int extraSteps)
{
  this->firstScale = firstScale;
  this->lastScale  = lastScale;
  this->steps      = extraSteps;

  crop = (int) rint (Gaussian2D::cutoff);
  storePyramid = false;
  thresholdEdge = 0.06;
  thresholdPeak = 0.04 / steps;
}

inline bool
InterestDOG::isLocalMax (float value, ImageOf<float> & dog, int x, int y)
{
  if (value > 0)
  {
	return    dog(x-1,y-1) < value
	       && dog(x-1,y  ) < value
	       && dog(x-1,y+1) < value
	       && dog(x  ,y-1) < value
	       && dog(x  ,y+1) < value
	       && dog(x+1,y-1) < value
	       && dog(x+1,y  ) < value
	       && dog(x+1,y+1) < value;
  }
  else
  {
	return    dog(x-1,y-1) > value
	       && dog(x-1,y  ) > value
	       && dog(x-1,y+1) > value
	       && dog(x  ,y-1) > value
	       && dog(x  ,y+1) > value
	       && dog(x+1,y-1) > value
	       && dog(x+1,y  ) > value
	       && dog(x+1,y+1) > value;
  }
}

inline bool
InterestDOG::notOnEdge (ImageOf<float> & dog, int x, int y)
{
  float H00 = dog(x-1,y) - 2.0f * dog(x,y) + dog(x+1,y);
  float H11 = dog(x,y-1) - 2.0f * dog(x,y) + dog(x,y+1);
  float H01 = ((dog(x+1,y+1) - dog(x+1,y-1)) - (dog(x-1,y+1) - dog(x-1,y-1))) / 4.0f;
  float det = H00 * H11 - H01 * H01;
  float trace = H00 + H11;
  return det > thresholdEdge * trace * trace;
}

float
InterestDOG::fitQuadratic (vector< ImageOf<float> > & dogs, int s, int x, int y, Vector<float> & result)
{
  ImageOf<float> & dog0 = dogs[s-1];
  ImageOf<float> & dog1 = dogs[s];
  ImageOf<float> & dog2 = dogs[s+1];

  Vector<float> g (3);
  g[0] = (dog2(x,  y  ) - dog0(x,  y  )) / 2.0f;
  g[1] = (dog1(x+1,y  ) - dog1(x-1,y  )) / 2.0f;
  g[2] = (dog1(x,  y+1) - dog1(x,  y-1)) / 2.0f;

  Matrix<float> H (3, 3);
  H(0,0) = dog0(x,  y  ) - 2.0f * dog1(x,y) + dog2(x,  y  );
  H(1,1) = dog1(x-1,y  ) - 2.0f * dog1(x,y) + dog1(x+1,y  );
  H(2,2) = dog1(x,  y-1) - 2.0f * dog1(x,y) + dog1(x,  y+1);
  H(0,1) = ((dog2(x+1,y  ) - dog2(x-1,y  )) - (dog0(x+1,y  ) - dog0(x-1,y  ))) / 4.0f;
  H(0,2) = ((dog2(x,  y+1) - dog2(x,  y-1)) - (dog0(x,  y+1) - dog0(x,  y-1))) / 4.0f;
  H(1,2) = ((dog1(x+1,y+1) - dog1(x+1,y-1)) - (dog1(x-1,y+1) - dog1(x-1,y-1))) / 4.0f;
  H(1,0) = H(0,1);
  H(2,0) = H(0,2);
  H(2,1) = H(1,2);

  gelss (H, result, g * -1.0f);

  return dog1(x,y) + 0.5f * result.dot (g);
}

void
InterestDOG::run (const Image & image, std::multiset<PointInterest> & result)
{
  double startTime = getTimestamp ();

  pyramid.clear ();
  scales.clear ();
  result.clear ();

  ImageOf<float> work;
  work.copyFrom (image);
  work *= GrayFloat;
  if (firstScale != 0.5f)  // The blur level of a raw image is defined to be 0.5
  {
	Gaussian1D blur (sqrt (firstScale * firstScale - 0.25), GrayFloat, Horizontal, Boost);
	work *= blur;
	blur.direction = Vertical;
	work *= blur;
  }

  // Make a set of blurring kernels, one for each step of blurring while
  // processing one octave.  There are actually two more steps than a full
  // octave in order to accomodate search of scale space maxima in DoG images.
  vector<Gaussian1D> blurs;
  float scaleRatio = powf (2.0f, 1.0f / steps);
  float sigmaRatio = sqrtf (scaleRatio * scaleRatio - 1.0f);  // amount of blurring needed to go from current scale step to next scale step
  float scale = firstScale;
  for (int i = 0; i < steps + 2; i++)
  {
	blurs.push_back (Gaussian1D (scale * sigmaRatio, GrayFloat, Horizontal, Boost));
	scale *= scaleRatio;
  }

  // Step thru octaves until image is too small to process
  int minsize = 2 * crop + 3;
  float octave = 1.0f;  // actually, this variable stores 2^octave
  while (work.width >= minsize  &&  work.height >= minsize  &&  octave * firstScale <= lastScale)
  {
	// Build set of blurred images
	vector<Image> blurred (steps + 3);
	blurred[0].copyFrom (work);
	for (int i = 1; i < steps + 3; i++)
	{
	  Gaussian1D & blur = blurs[i-1];
	  blur.direction = Horizontal;
	  work *= blur;
	  blur.direction = Vertical;
	  work *= blur;
	  blurred[i].copyFrom (work);
	}
	work = blurred[steps];
	if (storePyramid)
	{
	  for (int i = 1; i <= steps; i++)
	  {
		pyramid.push_back (blurred[i]);
		scales.push_back (octave * firstScale * powf (scaleRatio, i));
	  }
	}

	// Build set of Difference-of-Gaussian images
	vector< ImageOf<float> > dogs (steps + 2);
	for (int i = 0; i < steps + 2; i++)
	{
	  dogs[i] = blurred[i] - blurred[i+1];
	}

	// Search for maxima in DoG at each scale level
	for (int i = 1; i <= steps; i++)
	{
	  ImageOf<float> & dog = dogs[i];
	  for (int y = crop; y < dog.height - crop; y++)
	  {
		for (int x = crop; x < dog.width - crop; x++)
		{
		  float & value = dog(x,y);
		  if (   fabsf (value) > 0.8 * thresholdPeak
			  && isLocalMax (value, dog, x, y)
			  && isLocalMax (value, dogs[i-1], x, y)
			  && isLocalMax (value, dogs[i+1], x, y)
			  && notOnEdge (dog, x, y))
		  {
			// Search for precise location of maximum using interpolation.
			Vector<float> offset;
			int u = x;
			int v = y;
			float peak;
			int count = 0;
			bool changed = true;
			while (changed  &&  count++ < 5)
			{
			  int oldu = u;
			  int oldv = v;
			  peak = fitQuadratic (dogs, i, u, v, offset);
			  if (offset[1] > 0.6f  &&  u < dog.width - crop)
			  {
				u++;
			  }
			  if (offset[1] < -0.6f  &&  u > crop)
			  {
				u--;
			  }
			  if (offset[2] > 0.6f  &&  v < dog.height - crop)
			  {
				v++;
			  }
			  if (offset[2] < -0.6f  &&  v > crop)
			  {
				v--;
			  }
			  changed =  u != oldu  ||  v != oldv;
			}

			// Store the point if quadratic interpolation returned reasonable
			// results and the DoG response is strong enough.
			if (fabs (offset[0]) < 1.5f  &&  fabs (offset[1]) < 1.5f  &&  fabs (offset[2]) < 1.5f  &&  fabs (peak) > thresholdPeak)
			{
			  PointInterest p;
			  p.scale = octave * firstScale * powf (scaleRatio, i + offset[0]);
			  p.x = (x + offset[1] + 0.5f) * octave - 0.5f;  // Coordinates all use center-of-pixel convention.  Must shift to left edge of pixel to properly overly images w/ different scales.
			  p.y = (y + offset[2] + 0.5f) * octave - 0.5f;
			  p.weight = peak;
			  p.detector = PointInterest::Laplacian;
			  result.insert (p);
			}
		  }
		}
	  }
	}

	// Downsample the working image
	ImageOf<float> downsampled (work.width / 2, work.height / 2, GrayFloat);
	int y2 = 0;
	for (int y = 0; y < downsampled.height; y++)
	{
	  int x2 = 0;
	  for (int x = 0; x < downsampled.width; x++)
	  {
		downsampled(x,y) = work(x2,y2);
		x2 += 2;
	  }
	  y2 += 2;
	}
	work = downsampled;

	octave *= 2.0f;
  }

  cerr << "DoG time = " << getTimestamp () - startTime << endl;
}
