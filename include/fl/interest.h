/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5  thru 1.11 Copyright 2005 Sandia Corporation.
Revisions 1.13 thru 1.15 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.15  2007/03/23 11:38:06  Fred
Correct which revisions are under Sandia copyright.

Revision 1.14  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.13  2006/01/22 05:23:51  Fred
Add "fast" mode to InterestDOG.

Revision 1.12  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.11  2005/10/09 03:42:22  Fred
Move the file name LICENSE up to previous line, for better symmetry with UIUC
notice.

Revision 1.10  2005/10/08 19:33:33  Fred
Update revision history and add Sandia copyright notice.

Commit to using ImageCache.

Add more constraints to InterestMSER.

Revision 1.9  2005/06/14 02:33:13  Fred
Move supporting subroutines into InterestMSER class.  Deal with known memory
leak in merge() by creating a string of Roots that have been subsumed.  Also
create a string of Roots that have been discarded.  By re-using these, we can
reduce calls to the memory allocator.

Revision 1.8  2005/06/12 20:24:42  Fred
Add neighbors parameter to control finding local minima.

Use Gaussian information stored in Root objects associated with subsumed
regions to avoid counting pixels more than once.  This is an "intermediate"
checkin to preserve a milestone.  The next step is to rewrite all the
supporting routines to share a data structure containing all important
variables.  This could be the InterestMSER class itself, but then it wouldn't
be thread safe, so will use a private structure in the implementation file.

Revision 1.7  2005/06/10 15:20:47  Fred
Save/reuse previous center and covariance of a region as it grows.  However, to
really become efficient, must transfer such information when regions merge.

Revision 1.6  2005/06/07 04:00:04  Fred
Added MSER and changed interface to pass collections of pointers (allows
polymorphism in type of returend interest points).

Revision 1.5  2005/05/29 21:54:11  Fred
Fix comment on InterestDOG.

Revision 1.4  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.3  2003/12/30 16:32:06  rothgang
Add InterestHessian and InterestDOG.  Convert comments to doxygen format.

Revision 1.2  2003/07/24 19:15:08  rothgang
Make InterestHarrisLaplacian and InterestLaplacian work in very similar manner.
Find local maximum rather than global maximum when determining scale.  Develop
appropriate statistic for determining threshold.  Deprecate findScale().

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef interest_h
#define interest_h


#include "fl/convolve.h"
#include "fl/matrix.h"
#include "fl/imagecache.h"

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

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;
  };


  // Specific interest operators ------------------------------------------------

  class InterestHarris : public InterestOperator
  {
  public:
	InterestHarris (int neighborhood = 5, int maxPoints = 5000, float thresholdFactor = 0.02);

	virtual void run (const Image & image, InterestPointSet & result);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	NonMaxSuppress nms;
	FilterHarris filter;
	int maxPoints;  ///< Max number of interest points allowable
	float thresholdFactor;  ///< Percent of max interest response level at which to cut off interest points.
  };

  class InterestHarrisLaplacian : public InterestOperator
  {
  public:
	InterestHarrisLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);
	void init ();

	virtual void run (const Image & image, InterestPointSet & result);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	std::vector<FilterHarris> filters;  ///< FilterHarris clearly outperforms FilterHarrisEigen in tests.
	std::vector<Laplacian> laplacians;
	int maxPoints;
	float thresholdFactor;
	float neighborhood;
	int firstStep;
	int lastStep;
	int extraSteps;
	float stepSize;
  };

  class InterestLaplacian : public InterestOperator
  {
  public:
	InterestLaplacian (int maxPoints = 5000, float thresholdFactor = 0.02, float neighborhood = 1, float firstScale = 1, float lastScale = 25, int extraSteps = 20, float stepSize = -1);  ///< neighborhood >= 0 means fixed size (min = 1 pixel); neighborhood < 0 means multiple of scale.

	virtual void run (const Image & image, InterestPointSet & result);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

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

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

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
  class InterestDOG : public InterestOperator
  {
  public:
	InterestDOG (float firstScale = 1.6f, float lastScale = INFINITY, int extraSteps = 3);  ///< extraSteps gives the number of sub-levels between octaves.

	virtual void run (const Image & image, InterestPointSet & result);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

	bool isLocalMax (float value, ImageOf<float> & dog, int x, int y);
	bool notOnEdge (ImageOf<float> & dog, int x, int y);
	float fitQuadratic (std::vector< ImageOf<float> > & dogs, int s, int x, int y, Vector<float> & result);

	float firstScale;
	float lastScale;
	int steps;  ///< Number of scale steps between octaves.
	int crop;  ///< Number of pixels from border to ignore maxima.
	float thresholdEdge;  ///< Gives smallest permissible ratio of det(H) / trace(H)^2, where H is the Hessian of the DoG function on intensity.
	float thresholdPeak;  ///< Minimum permissible strength of DoG function at a local maximum.
	bool fast;  ///< Indicates to use fast mode:  23% faster,  23% more points.  Under strictest conditions (matching scale), repeatability goes down.  However, larger numer of points detected compensates for this as scale criterion is relaxed.
  };

  class InterestMSER : public InterestOperator
  {
  public:
	InterestMSER (int delta = 5, float sizeRatio = 0.9f);

	virtual void run (const Image & image, InterestPointSet & result);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream) const;

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
