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
	InterestHarris (int neighborhood = 5, int maxPoints = 300, float thresholdFactor = 0.02);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  // Max number of interest points allowable
	float thresholdFactor;  // Percent of max interest response level at which to cut off interest points.
  };

  class InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 300, float thresholdFactor = 0.02, int neighborhood = 1, int firstStep = 0, int lastStep = 7, float stepSize = -1);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	void findScale (const Image & image, PointInterest & p);

	NonMaxSuppress nms;
	std::vector<FilterHarrisEigen> filters;
	std::vector<Laplacian> laplacians;
	int neighborhood;
	int maxPoints;
	float thresholdFactor;
	float halfStep;
  };

  class InterestLaplacian : public InterestOperator
  {
  public:
	InterestLaplacian (int maxPoints = 300, float thresholdFactor = 0.02, int neighborhood = 1, int firstStep = 0, int lastStep = 7, float stepSize = -1, int extraSteps = 20);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	void findScale (const Image & image, PointInterest & p);

	NonMaxSuppress nms;
	std::vector<Laplacian> laplacians;
	int neighborhood;
	int maxPoints;
	float thresholdFactor;
	int extraSteps;
  };
}


#endif
