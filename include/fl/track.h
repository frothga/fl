/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_track_h
#define fl_track_h


#include "fl/convolve.h"

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
  // Generic tracking interface -----------------------------------------------

  class SHARED PointTracker
  {
  public:
	virtual void nextImage (const Image & image) = 0;  ///< Push the current image back to the "previous" position, and make the given image the "current" image.
	virtual void track (Point & point) = 0;  ///< Assuming point is in the "previous" image, update it so that it marks the same surface feature in the "current" image.
  };


  // KLT ----------------------------------------------------------------------

  /**
	 The Kanade-Lucas-Tomasi tracker.  This implementation is inspired by
	 the Birchfield implementation.  The Birchfield software is a complete
	 tracker, including point selection, replenishment, tracking, and
	 verification.  However, this class only does the job of
	 estimating an updated location.  The client program must generate
	 and manage interest points, as well as verify the new locations.
   **/
  class SHARED KLT : public PointTracker
  {
  public:
	KLT (int windowRadius = 3, int searchRadius = 15, float scaleRatio = 1.0f);
	~KLT ();

	virtual void nextImage (const Image & image);
	virtual void track (Point & point);
	float track (const Point & point0, const int level, Point & point1);  ///< Subroutine of track().

	std::vector<ImageOf<float> > pyramid0;  ///< "previous" image.  First entry is full sized image, and each subsequent entry is downsampled by 2.
	std::vector<ImageOf<float> > pyramid1;  ///< "current" image.  Same structure as pyramid0
	std::vector<Gaussian1D *> blurs;  ///< Blurring kernel for each level of the pyramid.  Brings some information from each pixel in one image to the position of the corresponding pixel in the other image.
	int pyramidRatio;  ///< Ratio between number of pixels in adjacent levels of pyramid.
	int windowRadius;  ///< Number of pixels from center to edge of search window.
	float minDeterminant;  ///< Smallest allowable determinant of second moment matrix.
	float minDisplacement;  ///< Convergence threshold on change in location.
	int maxIterations;  ///< To quit iterating even without convergence.
	float maxError;  ///< Largest allowable root mean squared error of pixel intensity within the window.  Note that intensity is in the range [0,1].
  };
}


#endif
