#include "fl/interest.h"

#include <math.h>

// Include for debugging
#include "fl/time.h"


using namespace std;
using namespace fl;


// class InterestHarrisLaplacian ----------------------------------------------

InterestHarrisLaplacian::InterestHarrisLaplacian (int maxPoints, float thresholdFactor, int neighborhood, int firstStep, int lastStep, float stepSize)
: nms (neighborhood)
{
  this->maxPoints       = maxPoints;
  this->thresholdFactor = thresholdFactor;

  if (stepSize < 0)
  {
	stepSize = sqrtf (2.0);
  }
  halfStep = sqrtf (stepSize);

  for (int s = firstStep; s <= lastStep; s++)
  {
	float scale = powf (stepSize, s);
	FilterHarrisEigen h (scale * 0.7125, scale);  // sigmaI seems to be the truer representative of characteristic scale
	filters.push_back (h);
  }

  float maxScale = powf (stepSize, lastStep + 0.5);
  stepSize = powf (stepSize, 1.0 / 3.0);
  lastStep = (int) (logf (maxScale) / logf (stepSize));
cerr << "lastStep = " << lastStep << endl;
cerr << "maxScale = " << maxScale << endl;
cerr << "creating scales: ";
  for (int s = 0; s <= lastStep; s++)
  {
	float scale = powf (stepSize, s);
cerr << scale << " ";
	Laplacian l (scale);
	l *= scale * scale;
	laplacians.push_back (l);
  }
cerr << endl;
}

inline void
InterestHarrisLaplacian::findScale (const Image & image, PointInterest & p)
{
  p.weight = 0;
  p.scale = 1.0;
  vector<Laplacian>::iterator i;
  for (i = laplacians.begin (); i < laplacians.end (); i++)
  {
	float response = fabs (i->response (image, p));
	if (response > p.weight)
	{
	  p.weight = response;
	  p.scale = i->sigma;
	}
  }

  if (p.scale <= laplacians.front ().sigma  ||  p.scale >= laplacians.back ().sigma)
  {
	p.scale = 1.0;
	p.weight = 0;
  }
}

/*
void
InterestHarrisLaplacian::run (const Image & image, std::multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;

  ImageOf<float> maxImage (work.width, work.height, GrayFloat);
  maxImage.clear ();
double startTime;
  for (int i = 0; i < filters.size (); i++)
  {
cerr << "filter " << i << endl;
startTime = getTimestamp ();
    //int offset = max (filters[i].G1.width, filters[i].dG.width) / 2;
    //int offsetX = filters[i].G.width / 2 + offset;
    //int offsetY = filters[i].G.height / 2 + offset;
    int offset = filters[i].offset;

	ImageOf<float> filtered = work * filters[i];
	filtered *= nms;
cerr << "  took " << getTimestamp () - startTime << endl;
startTime = getTimestamp ();

	for (int x = 0; x < filtered.width; x++)
	{
	  for (int y = 0; y < filtered.height; y++)
	  {
		//float & target = maxImage.pixelGrayFloat (x + offsetX, y + offsetY);
		float & target = maxImage (x + offset, y + offset);
		float & source = filtered (x, y);
		target = max (target, source);
	  }
	}
cerr << "  max " << getTimestamp () - startTime << endl;
  }

startTime = getTimestamp ();
  maxImage *= nms;
cerr << "nms " << getTimestamp () - startTime << endl;
  float threshold = nms.average * thresholdFactor;

  result.clear ();

startTime = getTimestamp ();
  for (int x = 0; x < maxImage.width; x++)
  {
	for (int y = 0; y < maxImage.height; y++)
	{
	  float pixel = maxImage (x, y);
	  if (pixel > threshold)
	  {
		PointInterest p;
		p.weight = pixel;
		p.x = x;
		p.y = y;
		result.insert (p);
		if (result.size () > maxPoints)
		{
		  result.erase (result.begin ());
		}
	  }
	}
  }
cerr << "thresholding " << getTimestamp () - startTime << endl;

  // Calculate natural scale for each selected interest point using Laplacian
  multiset<PointInterest>::iterator p;
  for (p = result.begin (); p != result.end (); p++)
  {
	findScale (image, (PointInterest &) *p);
  }
}
*/

void
InterestHarrisLaplacian::run (const Image & image, std::multiset<PointInterest> & result)
{
  ImageOf<float> work = image * GrayFloat;
  result.clear ();

  for (int i = 0; i < filters.size (); i++)
  {
cerr << "filter " << i;
double startTime = getTimestamp ();

    int offset = filters[i].offset;

	ImageOf<float> filtered = work * filters[i];
	filtered *= nms;
	float threshold = nms.average * thresholdFactor;

	for (int x = 0; x < filtered.width; x++)
	{
	  for (int y = 0; y < filtered.height; y++)
	  {
		float pixel = filtered (x, y);
		if (pixel > threshold)
		{
		  PointInterest p;
		  p.x = x + offset;
		  p.y = y + offset;
		  findScale (work, p);
		  float scaleRatio = p.scale / filters[i].sigmaI;
		  if (scaleRatio < halfStep  &&  scaleRatio > 1.0 / halfStep)
		  {
			// p.weight could either be max Laplacian response or max Harris response
			p.weight = pixel;
			p.detector = PointInterest::Harris;
			result.insert (p);
			if (result.size () > maxPoints)
			{
			  result.erase (result.begin ());
			}
		  }
		}
	  }
	}

cerr << " " << getTimestamp () - startTime << endl;
  }
}
