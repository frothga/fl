/*
Author: Fred Rothganger
Created 02/06/2005
Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
*/


#ifndef fl_imagecache_h
#define fl_imagecache_h


#include "fl/image.h"

#include <map>
#include <ostream>


namespace fl
{
  /**
	 An extended version of Image that holds metadata relevant to a scale
	 pyramid.  This avoids adding more metadata to the Image class itself
	 that is only used in this limited context.
   **/
  class PyramidImage : public Image
  {
  public:
	PyramidImage (const Image & that, float scale = 0.5f);

	float scale;
  };

  /**
	 A collection of the same image at a range of scales and sizes.

	 <P>Convention for scale:  Scale describes the "blur level" relative to the
	 physcial image.  Specifically, it measures the radius around a pixel
	 for which it possesses sufficient information.  A raw image from a sensor
	 is typically defined to have scale = 0.5, since pixels extend 1/2 pixel
	 out from their own center.

	 <P>If an image has a blur level >= 2 * original image, then it can be
	 downsampled without loss.  The blur level of the downsampled image
	 should be adjusted to account for the fact that the pixels are now
	 larger.  Let
	   cb = current blur level,
	   db = downsampled blur level,
	   os = original size (say the width in pixels),
	   cs = current size.
	 The new blur level would then be db = cb * cs / os.  However, when we
	 store downsampled images, we record the current blur level (cb) of the
	 full-sized image before downsampling.  To get blur level in terms of
	 size of pixels actually in the stored image, apply the formula given
	 above.

	 <P>Currently, all scale values stored in the pyramid must be unique.
	 However, it is possible to have two different sizes of images with
	 the same scale level: specifically an image before and after downsampling.
	 Right now we only allow one of the two to be stored.  Eventually, this
	 restriction should be relaxed, once we work out a good interface for
	 seeking images.
   **/
  class Pyramid
  {
  public:
	~Pyramid ();
	void clear ();

	void insert (PyramidImage * image);
	PyramidImage * find (float scale);  ///< Return image with scale closest to requested scale.  If there are no images, then returns null.
	PyramidImage * findLE (float scale);  ///< Returns image with scale <= requested scale.  If all images have scale larger than requested scale, then returns null.
	int findIndex (float scale);

	std::vector<PyramidImage *> images;  ///< Sorted by scale.
  };

  /**
	 A mechanism for holding on to intermediate results and sharing them
	 between image processing objects.
	 Associates various modified images with a given original image.  Each
	 modification process is defined by the client program and assigned an
	 integer id.  To facilitate information sharing within the image library,
	 some common processes are defined here.  Conceptually, the images are
	 organized as a collection of scale pyramids.  One pyramid is associated
	 with each id.  Within that pyramid, the images are sorted by
	 scale.
   **/
  class ImageCache
  {
  public:
	ImageCache ();
	~ImageCache ();
	void clear ();

	PyramidImage * add (const Image & image, int id = primary, float scale = 0.5f);  ///< By default, replaces the primary image.  If the new image actually appears to be different, then this will flush the cache.
	PyramidImage * get (int id = primary, float scale = 0.5f);  ///< Searches for image matching the id whose scale is closest to requested value.  If none is found, returns 0.
	PyramidImage * getLE (int id = primary, float scale = 0.5f);  ///< Searches for image matching the id whose scale is <= requested value.  If none is found, returns 0.
	Pyramid * getPyramid (int id);  ///< Ensures existance of Pyramid associated with given id, and returns it.

	enum
	{
	  primary,  ///< The original image.
	  color,  ///< The image in an arbitrary color format, preferrably the same as the original.
	  monochrome,  ///< The image in GrayFloat format.
	  gradientX,
	  gradientY,
	  gradientAngle,
	  gradientStrength,
	  user  ///< Start of user defined values.
	};

	PyramidImage * original;  ///< The image from which everything else is derived.  This may also appear multiple times in the "cache" collection.  All other images appear only once.

	typedef std::map<int, Pyramid *> IDmap;
	IDmap cache;

	static ImageCache shared;  ///< Global cache used by image library.
  };

  std::ostream & operator << (std::ostream & stream, const PyramidImage & data);
  std::ostream & operator << (std::ostream & stream, const Pyramid & data);
  std::ostream & operator << (std::ostream & stream, ImageCache & data);
}


#endif
