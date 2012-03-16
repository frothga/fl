/*
Author: Fred Rothganger
Created 02/06/2005

Copyright 2005, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_imagecache_h
#define fl_imagecache_h


#include "fl/image.h"
#include "fl/convolve.h"

#include <set>
#include <ostream>

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
  class ImageCacheEntry;
  class ImageCache;
  class EntryPyramid;

  /**
	 Stores the result of a completed computation on an image.
	 Also acts as a query object into the cache.  In this role, the image
	 member is initially empty.  If the result is in the cache, then the
	 query object is deleted.  Otherwise, the generate function is called
	 to fill in the image member.
	 Alternately, it is permissible to preemptively fill the image member
	 and not do anything when the generate function is called.  In this case
	 the client code should be careful to avoid wasted computation.
   **/
  class SHARED ImageCacheEntry
  {
  public:
	virtual void  generate (ImageCache & cache);  ///< Fill the image member.
	virtual bool  compare  (const ImageCacheEntry & that) const;
	virtual float distance (const ImageCacheEntry & that) const;  ///< Returns zero if that is exactly same as this, otherwise a positive number that indicates how different they are.  Returns INFINITY if that is not same class as this, or otherwise not substitutable.
	virtual void  print    (std::ostream & stream) const;

	Image image; ///< The actual cached data.
  };
  SHARED std::ostream & operator << (std::ostream & stream, const ImageCacheEntry & data);

  /**
	 A mechanism for storing and sharing image-processing results.
   **/
  class SHARED ImageCache
  {
  public:
	ImageCache ();
	~ImageCache ();
	void clear ();

	void setOriginal (const Image & image, float scale = 0.5f);  ///< Compares image to original.  If same, does nothing.  If different, flushes cache and sets new original.
	ImageCacheEntry * get        (ImageCacheEntry * query);  ///< Ensures that the desired datum exists.  Assumes ownership of the query object.  Result will be equivalent, but may or may not be the same as the query object.
	ImageCacheEntry * getClosest (ImageCacheEntry * query);  ///< Returns entry with smallest distance from query, or null if no acceptable entry exists.  Always deletes query before returning.
	ImageCacheEntry * getLE      (ImageCacheEntry * query);  ///< Returns entry that satisfies the query, or the closest one for which entry < query, or null if no acceptable entry exists.  Always deletes query before returning.

	struct EntryCompare
	{
	  bool operator () (const ImageCacheEntry * a, const ImageCacheEntry * b) const
	  {
		return a->compare (*b);
	  }
	};

	EntryPyramid * original;  ///< Base image from which all others are derived
	typedef std::set<ImageCacheEntry *, EntryCompare> cacheType;
	cacheType cache;

	static ImageCache shared;  ///< Global cache used by image library.
  };
  SHARED std::ostream & operator << (std::ostream & stream, const ImageCache & data);

  /**
	 Same image as original with some variation of pixel format or scale.
	 Sort order is:
	 <ul>
	 <li>ascending by PixelFormat::precedence
	 <li>ascending by scale with respect to original width (that is, (current scale) * original.width / image.width)
	 <li>descending by width (assume isotropic scaling, so only need to check one dimension)
	 </ul>

	 <P>Convention for scale:  Scale describes the "blur level" relative to the
	 physcial image.  Specifically, it measures the radius (in pixels) around
	 a sample point for which that sample possesses sufficient information.
	 A raw image from a sensor is typically defined to have scale = 0.5, since
	 each sample extends 1/2 pixel out from its own center.
   **/
  class SHARED EntryPyramid : public ImageCacheEntry
  {
  public:
	EntryPyramid (const PixelFormat & format, float scale = 0.5f, int width = 0);  ///< Create a query object
	EntryPyramid (const Image & that, float scale = 0.5f);  ///< Directly store "that"

	// utility functions
	static int   targetWidth (float targetScale, int sourceWidth, float sourceScale = 0.5f);  ///< Utility function for calculating desired width of result.  Gives closest power-of-two size.
	static float ratioDistance (const float & a, const float & b);

	virtual void  generate (ImageCache & cache);
	virtual bool  compare  (const ImageCacheEntry & that) const;
	virtual float distance (const ImageCacheEntry & that) const;
	virtual void  print    (std::ostream & stream) const;

	void resample (ImageCache & cache, const EntryPyramid * source);

	float scale;  ///< Stores scale with respect to original width (no consideration for amount of downsampling)
	static bool fast;  ///< Allows use of BlurDecimate and DoubleSize, which are faster than Transform but also accumulate more error per level.
	static float toleranceScaleRatio;  ///< How close two scales have to be before they are considered the same, in terms of portion of an octave.
  };

  class SHARED EntryFiniteDifference : public ImageCacheEntry
  {
  public:
	EntryFiniteDifference (Direction direction, float scale = 0.5, int width = 0);

	virtual void  generate (ImageCache & cache);
	virtual bool  compare  (const ImageCacheEntry & that) const;
	virtual float distance (const ImageCacheEntry & that) const;
	virtual void  print    (std::ostream & stream) const;

	Direction direction;
	float scale;
  };

  class SHARED EntryDOG : public ImageCacheEntry
  {
  public:
	EntryDOG (float sigmaPlus, float sigmaMinus, int width = 0);

	virtual void  generate (ImageCache & cache);
	virtual bool  compare  (const ImageCacheEntry & that) const;
	virtual float distance (const ImageCacheEntry & that) const;
	virtual void  print    (std::ostream & stream) const;

	float sigmaPlus;
	float sigmaMinus;
	float scale;  ///< Calculated effective scale.  Used mainly for compare() and distance() operations.
  };
}


#endif
