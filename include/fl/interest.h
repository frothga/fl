/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


5/2005 Fred Rothganger -- Added MSER and changed interface to pass collections
       of pointers.
*/


#ifndef interest_h
#define interest_h


#include "fl/convolve.h"
#include "fl/matrix.h"

#include <set>


namespace fl
{
  // General interest operator interface ----------------------------------------

  class InterestPointSet : public std::vector<PointInterest *>
  {
  public:
	InterestPointSet ();
	~InterestPointSet ();  ///< Destroy the points if we own them.

	void add (const std::multiset<PointInterest> & points);

	bool ownPoints;  ///< true if we are responsible to destroy the points (default value).
  };

  class InterestOperator
  {
  public:
	/**
	   Returns a collection of interest points.
	   \param result If this class sorts points internally,
	   then the collection will be in ascending order by weight.  If the
	   collection already contains entries, then the newly detected points
	   will be appended to the end.
	 **/
	virtual void run (const Image & image, InterestPointSet & result) = 0;
  };


  // Specific interest operators ------------------------------------------------

  class InterestHarris : public InterestOperator
  {
  public:
	InterestHarris (int neighborhood = 5, int maxPoints = 5000, float thresholdFactor = 0.02);

	virtual void run (const Image & image, InterestPointSet & result);

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  ///< Max number of interest points allowable
	float thresholdFactor;  ///< Percent of max interest response level at which to cut off interest points.
  };

  class InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);

	virtual void run (const Image & image, InterestPointSet & result);

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

	virtual void run (const Image & image, InterestPointSet & result);

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

	virtual void run (const Image & image, InterestPointSet & result);

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

	virtual void run (const Image & image, InterestPointSet & result);

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

  class InterestMSER : public InterestOperator
  {
  public:
	InterestMSER (int delta = 1);

	virtual void run (const Image & image, InterestPointSet & result);

	/**
	   Structure to track meta-data associated with a region (tree) in the
	   union-find algorithm.
	 **/
	struct Node;
	struct Root
	{
	  Root * next;
	  Root * previous;

	  unsigned char level;  ///< The gray-level where this region was created.

	  int size;  ///< Number of pixels in this tree
	  int sizes[256];  ///< History of sizes for all gray-levels
	  float rates[256];  ///< History of changes rates w.r.t. gray-level.  Calculated from sizes[].

	  // Info for generating elliptical regions
	  Node * head;  ///< Start of LIFO linked list of pixels.  IE: points to most recently added pixels.
	  Node * heads[256];  ///< History of composition of region at each gray-level.
	  unsigned char oldLevel;  ///< Gray-level of most recent ellipse
	  float x;  ///< Center of most recent ellipse.
	  float y;  ///< Center of most recent ellipse.
	  float xx;  ///< Covariance of most recent ellipse.
	  float xy;  ///< Covariance of most recent ellipse.
	  float yy;  ///< Covariance of most recent ellipse.
	};

	struct Node
	{
	  Node * parent;
	  Node * next;
	  Root * root;  ///< If this is a root node, then this points to the associated metadata;
	};

	void addGrayLevel (unsigned char level, bool sign, int * lists[], Node * nodes, const int width, const int height, Root * roots, std::vector<PointMSER *> & regions);

	int delta;  ///< Amount of gray-level distance above and below current gray-level to check when computing rate of change in region size.
	float minScale;  ///< Smallest scale region to admit into resulting list of interest points.  Guards against long skinny structures and structures with too few pixels to be worth noting.
  };
}


#endif
