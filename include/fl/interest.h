/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef interest_h
#define interest_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/imagecache.h"

#include <set>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flImage_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
  // General interest operator interface ----------------------------------------

  class SHARED InterestPointSet : public std::vector<PointInterest *>
  {
  public:
	InterestPointSet ();
	~InterestPointSet ();  ///< Destroy the points if we own them.

	void add (const std::multiset<PointInterest> & points);

	bool ownPoints;  ///< true if we are responsible to destroy the points (default value).
  };

  class SHARED InterestOperator
  {
  public:
	virtual ~InterestOperator ();

	/**
	   Returns a collection of interest points.
	   \param result If this class sorts points internally,
	   then the collection will be in ascending order by weight.  If the
	   collection already contains entries, then the newly detected points
	   will be appended to the end.
	 **/
	virtual void run (ImageCache & cache, InterestPointSet & result) = 0;
	/**
	   Convenience function that calls run() using the internal shared cache.
	   NOT thread-safe!
	 **/
	void run (const Image & image, InterestPointSet & result);

	void serialize (Archive & archive, uint32_t version);
	static uint32_t serializeVersion;
  };


  // Specific interest operators ------------------------------------------------

  class SHARED InterestHarris : public InterestOperator
  {
  public:
	InterestHarris (int neighborhood = 5, int maxPoints = 5000, float thresholdFactor = 0.02);

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  ///< Max number of interest points allowable
	float thresholdFactor;  ///< Percent of max interest response level at which to cut off interest points.
  };

  class SHARED InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 0.5, float lastScale = INFINITY, int steps = 2, int extraSteps = 20);
	~InterestHarrisLaplacian ();
	void init ();
	void clear ();

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

	std::vector<FilterHarris *> filters;  ///< FilterHarris clearly outperforms FilterHarrisEigen in tests.
	std::vector<Laplacian *> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	float firstScale;
	float lastScale;
	int steps;
	int extraSteps;
  };

  class SHARED InterestLaplacian : public InterestOperator
  {
  public:
	InterestLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  ///< neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

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
  class SHARED InterestHessian : public InterestOperator
  {
  public:
	InterestHessian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  ///< neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

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
  **/
  class SHARED InterestDOG : public InterestOperator
  {
  public:
	InterestDOG (float firstScale = 1.6f, float lastScale = INFINITY, int extraSteps = 3);  ///< extraSteps gives the number of sub-levels between octaves.

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

	bool isLocalMax (float value, ImageOf<float> & dog, int x, int y);
	bool notOnEdge (ImageOf<float> & dog, int x, int y);
	float fitQuadratic (ImageOf<float> & dog0, ImageOf<float> & dog1, ImageOf<float> & dog2, int x, int y, Vector<float> & result);

	float firstScale;
	float lastScale;
	int steps;  ///< Number of scale steps between octaves.
	int crop;  ///< Number of pixels from border to ignore maxima.
	float thresholdEdge;  ///< Gives smallest permissible ratio of det(H) / trace(H)^2, where H is the Hessian of the DoG function on intensity.
	float thresholdPeak;  ///< Minimum permissible strength of DoG function at a local maximum.
	bool fast;  ///< Indicates to use fast mode:  23% faster,  23% more points.  Under strictest conditions (matching scale), repeatability goes down.  However, larger numer of points detected compensates for this as scale criterion is relaxed.
  };

  class SHARED InterestMSER : public InterestOperator
  {
  public:
	InterestMSER (int delta = 5, float sizeRatio = 0.9f);

	virtual void run (ImageCache & cache, InterestPointSet & result);
	using InterestOperator::run;

	void serialize (Archive & archive, uint32_t version);

	// Parameters
	int delta;  ///< Amount of gray-level distance above and below current gray-level to check when computing rate of change in region size.
	float sizeRatio;  ///< Ratio of the pixel count of a candidate local minimum to pixel counts of the upper and lower levels that bracket the range in which its rate must be less than any other.
	float minScale;  ///< Smallest scale region to admit into resulting list of interest points.  Guards against long skinny structures and structures with too few pixels to be worth noting.
	int minSize;  ///< Smallest number of pixels permitted in a region.
	float maxSizeRatio;  ///< Largest number of pixels permitted in a region, as a portion of the total number of pixels in the image.  For example, 0.01 means regions never exceed 1% of the size of the image.
	int minLevels;  ///< Smallest number of intensity levels between the first pixel in the region and the level at which it is generated.
	float maxRate;  ///< Largest rate of change permitted for a region.

	// Working data and subroutines of run().  All structures are created
	// and destroyed by run(), not the constructor/destructor of this class
	// as a whole.
	// Note: storing working data here keeps objects of this
	// class from being thread safe.  IE: an instance can only process one
	// image on one thread.

	struct Node;

	/**
	   Structure to track meta-data associated with a region (tree) in the
	   union-find algorithm.
	**/
	struct Root
	{
	  Root * next;
	  Root * previous;

	  unsigned char level;  ///< The gray-level where this region was created.
	  unsigned char lower;  ///< Lower bound of scan for local minimum
	  unsigned char center;  ///< Curent gray-level that is a candidate local minimum

	  int size;  ///< Number of pixels in this tree
	  int sizes[256];  ///< History of sizes for all gray-levels
	  float rates[256];  ///< History of change rates w.r.t. gray-level.  Calculated from sizes[].

	  // Info for generating Gaussians
	  Node * head;  ///< Start of LIFO linked list of pixels.  IE: points to most recently added pixels.
	  Node * heads[256];  ///< History of composition of region at each gray-level.
	  Node * tail;  ///< Head of most recent ellipse.  Indicates stopping point when generating next Gaussian.
	  int tailSize;  ///< Number of pixels in most recent Gaussian.
	  float x;  ///< Center of most recent Gaussian.
	  float y;  ///< Center of most recent Gaussian.
	  float xx;  ///< Covariance of most recent Gaussian.
	  float xy;  ///< Covariance of most recent Gaussian.
	  float yy;  ///< Covariance of most recent Gaussian.
	};

	/**
	   Structure for keeping track of the state of one pixel.
	**/
	struct Node
	{
	  Node * parent;
	  Node * next;
	  Root * root;  ///< If this is a root node, then this points to the associated metadata;
	};

	Node * nodes;  ///< An image of union-find nodes, one per pixel in input image.
	Root roots;  ///< meta-data for active regions
	Root subsumed;  ///< Root objects subsumed but not yet deleted
	Root deleted;  ///< Root objects available for re-use

	int width;  ///< of input image
	int height;  ///< of input image
	int maxSize;  ///< Largest number of pixels in a region.  Computed based on number of pixels in image.  See maxSizeRatio.
	int * lists[257];  ///< start of array of pixel indices for each gray-level; includes a stop point at the end

	void clear (Root * head);  ///< deletes all Root structures in the given list
	void move (Root * root, Root & head);  ///< moves a single Root structure to the give list.
	void releaseAll (Root * head);  ///< moves all Root structures in the given list to the "deleted" list, where they can be re-used.
	void merge (Node * grow, Node * destroy);  ///< Make necessary structural adjustments to combine one region into another.
	Node * findSet (Node * n);
	void join (Node * i, Node * n);
	void addGrayLevel (unsigned char level, bool sign, std::vector<PointMSER *> & regions);
  };
}


#endif
