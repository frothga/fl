#ifndef interest_h
#define interest_h


#include "fl/convolve.h"
#include "fl/matrix.h"

#include <set>


namespace fl
{
  // General interest operator interface ----------------------------------------

  class InterestOperator
  {
  public:
	virtual void run (const Image & image, std::multiset<PointInterest> & result) = 0;
	virtual void run (const Image & image, std::vector<PointInterest> & result);  // Same as above, just different type of collection for return value.
  };


  // Specific interest operators ------------------------------------------------

  class InterestHarris : public InterestOperator
  {
  public:
	InterestHarris (int neighborhood = 5, int maxPoints = 5000, float thresholdFactor = 0.02);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  // Max number of interest points allowable
	float thresholdFactor;  // Percent of max interest response level at which to cut off interest points.
  };

  class InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	void findScale (const Image & image, PointInterest & p);

	std::vector<FilterHarris> filters;  // FilterHarris clearly outperforms FilterHarrisEigen in tests.
	std::vector<Laplacian> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	int firstStep;
	int extraSteps;
	float stepSize;
  };

  class InterestLaplacian : public InterestOperator
  {
  public:
	InterestLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  // neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	void findScale (const Image & image, PointInterest & p);

	std::vector<Laplacian> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	int firstStep;
	int extraSteps;
	float stepSize;
  };
}


#endif
