/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef match_h
#define match_h


#include "fl/point.h"
#include "fl/neighbor.h"

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
  class SHARED Match;
  class SHARED MatchSet;
  class SHARED Registration;
  class SHARED RegistrationMethod;
  class SHARED MatchFilter;
  class SHARED MatchFinder;

  /**
	 An ordered tuple of Points, each one from a different PointSet,
	 that are all projections of the same feature in the scene.
	 Can represent several things, all essentially the same:
	 <ul>
	 <li>a simple point matche between two images
	 <li>a match across a number of images
	 <li>a feature tracked across a number of frames in a video
	 </ul>
	 Generally does not take responsiblity for contained points, since they
	 are expected to be in some PointSet which does own them.
   **/
  class SHARED Match : public std::vector<Point *>
  {
  public:
	virtual ~Match ();
  };

  /**
	 A collection of matches that are related in some way.
	 In general they are self-consistent according to some Registration model.
	 Alternately they could be collection of proposed matches that will be
	 fed to a Registration.
   **/
  class SHARED MatchSet : public std::vector<Match *>
  {
  public:
	MatchSet ();
	virtual ~MatchSet ();
	void clear (bool destruct = false);  ///< Empty this collection, possibly destructing the contained matches.

	void set (Registration * model = 0);  ///< Replaces the current model.  To simply clear it, set to zero.

	Registration * model;  ///< Whether this model generated this set or is generated from it depends on history and is unspecified.  May also be null, even if there are matches, so always check before using.  We take ownership of the model and destruct it.
  };

  /**
	 A model that describes how points match between two or more point sets.
	 Examples include:
	 <ul>
	 <li>spline deformation model ("rubber sheet" matching)
	 <li>epipolar geometry
	 <li>homography (or affine transformation in general)
	 <li>3D model plus projection matrices
	 </ul>
   **/
  class SHARED Registration
  {
  public:
	virtual ~Registration ();

	virtual double test (const Match & match) const = 0;  ///< Measure the quality of a candidate match.  @return average reprojection error in pixels

	double error;  ///< Average reprojection error in pixels in the set used to construct this registration.
  };

  /**
	 Generates a particular kind of registration model.
   **/
  class SHARED RegistrationMethod
  {
  public:
	virtual ~RegistrationMethod ();

	virtual Registration * construct (const MatchSet & matches) const = 0;
	virtual int minMatches () const = 0;  ///< @return the smallest number of matches sufficient to create a registration
  };

  /**
	 Determines a maximal subset of the given matches that are self-consistent
	 under the given registraton method.
   **/
  class SHARED MatchFilter
  {
  public:
	MatchFilter (RegistrationMethod * method);  ///< @param method Takes ownership of the given object, and destructs it when done.
	virtual ~MatchFilter ();

	/**
	   @param source The full set of matches to be filtered.  May be empty for
	   filters that generate new matches.
	   @param result The chosen subset, along with the associated model.
	   Whether the model generates the set or is generated from it depends
	   on the specific match filter.
	   A filter may also generate new matches, which only appear in the result.
	   Any filter that generates new matches will probably need to be
	   parameterized with additional data, such as images.
	 **/
	virtual void run (const MatchSet & source, MatchSet & result) const = 0;

	RegistrationMethod * method;
  };

  /**
	 Generates a new set of matches based on two given point sets.
	 The "reference" point set remains with this object and may be used
	 repeatedly to match with different queries.
	 <p>Note: Objects of this class could be used together in a larger image
	 retrieval framework.  It would not be compulsory to use brute-force
	 matching for retrieval.  Instead this class could be extended to support
	 voting or some other scheme.
	 <p>Note: Even though a special functionis provided for setting the
	 reference image, it might make sense in derived classes to provide a
	 constructor that does this.
   **/
  class SHARED MatchFinder
  {
  public:
	virtual ~MatchFinder ();

	virtual void set (PointSet & reference) = 0;  ///< Creates the internal structures needed to find matches.  reference should remain alive as long as this object is alive.
	virtual void run (PointSet & query, MatchSet & matches) const = 0;  ///< Initializes matches with all hypothesized correspondences (many of which may be wrong).
  };


  // Registrations ------------------------------------------------------------

  /**
	 Places the second point into the frame of the first using a plane
	 transformation.
   **/
  class SHARED Homography : public Registration
  {
  public:
	virtual double test (const Match & match) const;

	Matrix<double> H;
  };

  class SHARED HomographyMethod : public RegistrationMethod
  {
  public:
	HomographyMethod (int dof);

	virtual Registration * construct (const MatchSet & matches) const;
	virtual int minMatches () const;

	int dof;  ///< Degrees of freedom in homography: 2=translation, 3=translation and rotation, 4=translation and uniform scaling, 6=full affine, 8=perspective
  };


  // MatchFilters -------------------------------------------------------------

  class SHARED Ransac : public MatchFilter
  {
  public:
	Ransac (RegistrationMethod * method);

	virtual void run (const MatchSet & source, MatchSet & result) const;

	// Parameters
	int    k;  ///< fixed number of iterations.  If negative, then compute number of iterations based on -k standard deviations.
	double w;  ///< inlier rate, that is, the ratio of inlier count over total number of data
	double p;  ///< desired probability that a model will be formed from all inliers
	double t;  ///< maximum amount of error a match may have and still be included in consensus set.  Default is 1 pixel.
	int    d;  ///< mimimum number of data in consensus set required to consider the model.  Default value is model->minMatches().
  };

  /**
	 Refines an existing model using repeated estimation based on the entire
	 consensus set, until it no longer changes (much).
   **/
  class SHARED FixedPoint : public MatchFilter
  {
  public:
	FixedPoint (RegistrationMethod * method);

	/**
	   @param result Must contain the most recent consensus set.
	   @param model The updated model.  Any previous model is ignored and overwritten.
	 **/
	virtual void run (const MatchSet & source, MatchSet & result) const;

	int maxIterations;  ///< When to terminate if we don't reach fixed-point.
	double t;  ///< maximum amount of error a match may have and still be included in consensus set.  Default is 1 pixel.
  };


  // MatchFinders -------------------------------------------------------------

  class SHARED NearestDescriptors : public MatchFinder
  {
  public:
	NearestDescriptors (PointSet & reference);
	~NearestDescriptors ();
	void clear ();

	virtual void set (PointSet & reference);
	virtual void run (PointSet & query, MatchSet & result) const;

	KDTree tree;
	std::vector<MatrixAbstract<float> *> data;
	double threshold;  ///< descriptors have to be closer than this to pass.  default = 1.0
	double ratio;  ///< of nearest descriptor over next nearest must be less than this.  default = 0.8
  };
}


#endif
