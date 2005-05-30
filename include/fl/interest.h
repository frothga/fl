/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


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
	virtual void run (const Image & image, std::vector<PointInterest> & result);  ///< Same as above, just different type of collection for return value.
  };


  // Specific interest operators ------------------------------------------------

  class InterestHarris : public InterestOperator
  {
  public:
	InterestHarris (int neighborhood = 5, int maxPoints = 5000, float thresholdFactor = 0.02);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  ///< Max number of interest points allowable
	float thresholdFactor;  ///< Percent of max interest response level at which to cut off interest points.
  };

  class InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	std::vector<FilterHarris> filters;  ///< FilterHarris clearly outperforms FilterHarrisEigen in tests.
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
	InterestLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  ///< neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	std::vector<Laplacian> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	int firstStep;
	int extraSteps;
	float stepSize;
  };

  /**
	 Like InterestLaplacian, but uses a separable kernel.  Better for handling
	 larger scales.  Should deprecate InterestLaplacian.
  **/
  class InterestHessian : public InterestOperator
  {
  public:
	InterestHessian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  ///< neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	std::vector<FilterHessian> filters;
	std::vector<Laplacian> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	int firstStep;
	int extraSteps;
	float stepSize;
  };

  /**
	 Implements David Lowe's scale pyramid approach to finding difference of
	 Gaussian extrema.  The shape of a difference-of-Gaussian kernel is very
	 similar to a Laplacian of Gaussian.

	 The member "pyramid" stores blurred images that could be used to generate
	 gradient info for use DescriptorSIFT.
  **/
  class InterestDOG : public InterestOperator
  {
  public:
	InterestDOG (float firstScale = 1.6f, float lastScale = INFINITY, int extraSteps = 3);  ///< extraSteps gives the number of sub-levels between octaves.

	virtual void run (const Image & image, std::multiset<PointInterest> & result);

	bool isLocalMax (float value, ImageOf<float> & dog, int x, int y);
	bool notOnEdge (ImageOf<float> & dog, int x, int y);
	float fitQuadratic (std::vector< ImageOf<float> > & dogs, int s, int x, int y, Vector<float> & result);

	std::vector<Image> pyramid;  ///< A series of blurred/resampled images.  pyramid[0] contains the original image in GrayFloat (or GrayDouble if that was the original format).  The coordinates in pyramid[i] are (x/s,y/s) where (x,y) are the coordinates in pyramid[0] and s = pyramid[0].width / pyramid[i].width.
	std::vector<float> scales;  ///< Scale levels associated with the entries in pyramid
	float firstScale;
	float lastScale;
	int steps;  ///< Number of scale steps between octaves.
	int crop;  ///< Number of pixels from border to ignore maxima.
	bool storePyramid;  ///< Indicates that blurred/resampled images should be stored rather than thrown away.
	float thresholdEdge;  ///< Gives smallest permissible ratio of det(H) / trace(H)^2, where H is the Hessian of the DoG function on intensity.
	float thresholdPeak;  ///< Minimum permissible strength of DoG function at a local maximum.
  };
}


#endif
