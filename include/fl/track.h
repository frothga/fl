/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.4  2004/05/15 18:27:48  rothgang
Allow pyramidRatio to be a non-integer.  Gather residual on patch.

Revision 1.3  2004/05/07 21:24:13  rothgang
Use TransformGauss class to calculate the pyramid.  This is more efficient when
the pyramidRatio is large (4 or 8).

Revision 1.2  2004/05/06 16:23:52  rothgang
Use bilinear method to calculate derivatives, so remove derivative image
pyramids.

Revision 1.1  2004/05/06 16:21:00  rothgang
Create point trackers.
-------------------------------------------------------------------------------
*/


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
	float track (const Point & point0, const int level, Point & point1);  ///< Subroutine of track().

	std::vector< ImageOf<float> > pyramid0;  ///< "previous" image.  First entry is full sized image, and each subsequent entry is downsampled by 2.
	std::vector< ImageOf<float> > pyramid1;  ///< "current" image.  Same structure as pyramid0
	Gaussian1D preBlur;  ///< For blurring full image at base of pyramid.  Purpose is to ensure smooth texture within search window.
	float pyramidRatio;  ///< Ratio between number of pixels in adjacent levels of pyramid.
	int windowRadius;  ///< Number of pixels from center to edge of search window.
	int windowWidth;  ///< Diameter of search window.
	float minDeterminant;  ///< Smallest allowable determinant of second moment matrix.
	float minDisplacement;  ///< Convergence threshold on change in location.
	int maxIterations;  ///< To quit iterating even without convergence.
	float maxError;  ///< Largest allowable root mean squared error of pixel intensity within the window.  Note that intensity is in the range [0,1].
  };
}


#endif
