#ifndef fl_track_h
#define fl_track_h


#include "fl/convolve.h"


namespace fl
{
  // Generic tracking interface -----------------------------------------------

  class PointTracker
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
  class KLT : public PointTracker
  {
  public:
	KLT (int windowRadius = 3, int searchRadius = 15);

	virtual void nextImage (const Image & image);
	virtual void track (Point & point);
	void track (const Point & point0, const int level, Point & point1);  ///< Subroutine of track().

	std::vector< ImageOf<float> > pyramid0;  ///< "previous" image.  First entry is full sized image, and each subsequent entry is downsampled by 2.
	std::vector< ImageOf<float> > pyramid1;  ///< "current" image.  Same structure as pyramid0
	Gaussian1D preBlur;  ///< For blurring full image at base of pyramid.  Purpose is to ensure smooth texture within search window.
	int pyramidRatio;  ///< Ratio between number of pixels in adjacent levels of pyramid.
	int windowRadius;  ///< Number of pixels from center to edge of search window.
	int windowWidth;  ///< Diameter of search window.
	float minDeterminant;  ///< Smallest allowable determinant of second moment matrix.
	float minDisplacement;  ///< Convergence threshold on change in location.
	int maxIterations;  ///< To quit iterating even without convergence.
  };
}


#endif
