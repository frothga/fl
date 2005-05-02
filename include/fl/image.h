/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.


4/2005 Revised by Fred Rothganger
*/


#ifndef fl_image_h
#define fl_image_h


#include "fl/pointer.h"

#include <iostream>
#include <string>
#include <vector>


namespace fl
{
  // Forward declaration for use by Image
  class PixelFormat;
  class Pixel;
  class ImageFileFormat;
  

  // Image --------------------------------------------------------------------

  class Image
  {
  public:
	Image ();  ///< Creates a new image of GrayChar, but with no buffer memory allocated.
	Image (const PixelFormat & format);  ///< Same as above, but with given PixelFormat
	Image (int width, int height);  ///< Allocates buffer of size width x height x GrayChar.depth bytes.
	Image (int width, int height, const PixelFormat & format);  ///< Same as above, but with given PixelFormat
	Image (const Image & that);  ///< Points our buffer to same location as "that" and copies all of its metadata.
	Image (unsigned char * buffer, int width, int height, const PixelFormat & format);  ///< Binds to an external block of memory.
	Image (const std::string & fileName);  ///< Create image initialized with contents of file.

	void read (const std::string & fileName);  ///< Read image from fileName.  Format will be determined automatically.
	void read (std::istream & stream);
	void write (const std::string & fileName, const std::string & formatName = "pgm") const;  ///< Write image to fileName.
	void write (std::ostream & stream, const std::string & formatName = "pgm") const;

	Image & operator <<= (const Image & that);  ///< Direct assignment by shallow copy.  Same semantics as "=".  By using a different operator than "=", we allow subclasses to inherit this function.
	void copyFrom (const Image & that);  ///< Duplicates another Image.  Copy all raster info into private buffer, and copy all other metadata.
	void copyFrom (unsigned char * buffer, int width, int height, const PixelFormat & format);  ///< Copy from a non-Image source.  Determine size of buffer in bytes as width x height x depth.
	void attach (unsigned char * buffer, int width, int height, const PixelFormat & format);  ///< Binds to an external block of memory.
	void detach ();  ///< Set the state of this image as if it has no buffer.  Releases (but only frees if appropriate) any memory.

	void resize (int width, int height);  ///< Changes image to new size.  Any pixels that are still visible are aligned correctly.  Any newly exposed pixels are set to black.
	void bitblt (const Image & that, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = -1, int height = -1);  ///< -1 for width or height means "maximum possible value"
	void clear (unsigned int rgba = 0);  ///< Initialize buffer, if it exists, to given color.  In case rgba == 0, simply zeroes out memory, since this generally results in black in most pixel formats.

	Image operator + (const Image & that);  ///< Creates an image that sums pixel values from this and that.
	Image operator - (const Image & that);  ///< Creates an image whose pixels are the difference between this and that.
	Image operator * (double factor);  ///< Creates an image containing this images's pixels scaled by factor.  Ie: factor == 1 -> no change; factor == 2 -> bright spots are twice as bright, and dark spots (negative values relative to bias) are twice as dark.
	Image & operator *= (double factor);  ///< Scales each pixel by factor.
	Image & operator += (double value);  ///< Adds value to each pixel
	bool operator == (const Image & that) const;  ///< Determines if both images have exactly the same metadata and buffer.  This is a strong, but not perfect, indicator that no change has occurred to the contents between the construction of the respective objects.
	bool operator != (const Image & that) const;  ///< Negated form of operator ==.

	Pixel         operator () (int x, int y) const;  ///< Returns a Pixel object that wraps (x,y).
	unsigned int  getRGBA  (int x, int y) const;
	void          getRGBA  (int x, int y, float values[]) const;
	unsigned int  getYUV   (int x, int y) const;
	unsigned char getGray  (int x, int y) const;
	void          getGray  (int x, int y, float & gray) const;
	unsigned char getAlpha (int x, int y) const;
	void          setRGBA  (int x, int y, unsigned int rgba);
	void          setRGBA  (int x, int y, float values[]);
	void          setYUV   (int x, int y, unsigned int yuv);
	void          setGray  (int x, int y, unsigned char gray);
	void          setGray  (int x, int y, float gray);
	void          setAlpha (int x, int y, unsigned char alpha);

	// Data
	Pointer             buffer;
	const PixelFormat * format;
	int                 width;  ///< The Image class guarantees that width * height is always non-negative and that the raster stored in buffer has enough allocated memory to contain width * height pixels.  (Of course, you can set this field directly, in which case the warranty is void. :)
	int                 height;  ///< See width for interface guarantees.
	double              timestamp;  ///< Time when image was captured.  If part of a video, then time when image should be displayed.
  };

  /**
	 A simple wrap around Image that makes it easier to access pixels directly.
   **/
  template<class T>
  class ImageOf : public Image
  {
  public:
	// These constructors blindly wrap the constructors of Image, without
	// regard to the size or type of data returned by operator (x,y).
	ImageOf () {}
	ImageOf (const PixelFormat & format) : Image (format) {}
	ImageOf (int width, int height) : Image (width, height) {}
	ImageOf (int width, int height, const PixelFormat & format) : Image (width, height, format) {}
	ImageOf (const Image & that) : Image (that) {}

	T & operator () (int x, int y) const
	{
	  return ((T *) buffer)[y * width + x];
	}
  };


  // Filter -------------------------------------------------------------------

  /**
	 Base class for reified functions that take as input an image and output
	 another image.
  **/
  class Filter
  {
  public:
	virtual Image filter (const Image & image) = 0;  ///< This could be const, but it is useful to allow filters to collect statistics.  Note that such filters are not thread safe.
  };

  inline Image
  operator * (const Filter & filter, const Image & image)
  {
	// We lie about Filter being const.  This allows you to construct a Filter
	// object right in the middle of an expression without getting complaints
	// from the compiler.  Ditto for the other operators below.
	return ((Filter &) filter).filter (image);
  }

  inline Image
  operator * (const Image & image, const Filter & filter)
  {
	return ((Filter &) filter).filter (image);
  }

  inline Image &
  operator *= (Image & image, const Filter & filter)
  {
	return image = image * ((Filter &) filter);
  }


  // PixelFormat --------------------------------------------------------------

  /**
	 A PixelFormat wraps access to an element of an Image.  A pixel itself is
	 sort of the combination of a pointer to memory and a PixelFormat.
	 A PixelFormat describes the entire collection of pixels in an image, and
	 we use it to interpret each pixel in the image.
	 PixelFormat extends Filter so it can be used directly to convert image
	 format.  The "from*" methods implement an n^2 set of direct conversions
	 between selected formats.  These conversions occur frequently when
	 displaying images on an X windows system.
	 For the present, all formats exept for XYZ make sRGB assumptions (see
	 www.srgb.com).  In addition, all integer values are non-linear (with
	 gamma = 2.2 as per sRGB spec), and all floating point values are linear.
	 We can add parameters to the formats if we need to distinguish more color
	 spaces.
	 Naming convention for PixelFormats
	   = <color space><basic C type for one channel>
       Color space names refer to sequence of channels (usually bytes) in
       memory, rather than in machine words (eg: registers in the processor).
       The leftmost letter in a name refers to the lowest numbered address.
	   If a channel is larger than one byte, then the bytes are laid out
	   within the channel according to the standard for the machine.
	 Naming convention for accessor methods
	   The data is in machine words, so names describe sequence
	   within machine words.  Accessors guarantee that the order in the
	   machine word will be the same, regardless of endian.  (This implies
	   that the implementation of an accessor changes with endian.)
	   The leftmost letter refers to the most significant byte in the word.
	   Some accessors take arrays, and since arrays are memory blocks they
	   follow the memory order convention.
	 TODO:
	 * think about adding arbitrary information channels besides alpha
       (eg: depth).  alpha gets special treatment because it has a specific,
       well-defined effect on how pixels are combined.
	 * add alpha blend methods (operator << and >>) to Pixel
  **/
  class PixelFormat : public Filter
  {
  public:
	virtual Image filter (const Image & image);  ///< Return an Image in this format
	void fromAny (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;  ///< Checks if this and that describe the same actual format.
	bool operator != (const PixelFormat & that) const
	{
	  return ! operator == (that);
	}

	virtual unsigned int  getRGBA  (void * pixel) const = 0;  ///< Return value is always assumed to be non-linear sRGB.  Same for other integer RGB methods below.
	virtual void          getRGBA  (void * pixel, float values[]) const;  ///< "values" must have at least four elements.  Each returned value is in [0,1].
	virtual void          getXYZ   (void * pixel, float values[]) const;
	virtual unsigned int  getYUV   (void * pixel) const;
	virtual unsigned char getGray  (void * pixel) const;
	virtual void          getGray  (void * pixel, float & gray) const;
	virtual unsigned char getAlpha (void * pixel) const;  ///< Returns fully opaque by default.  PixelFormats that actually have an alpha channel must override this to return correct value.
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const = 0;
	virtual void          setRGBA  (void * pixel, float values[]) const;  ///< Each value must be in [0,1].  Values outside this range will be clamped and modified directly in the array.
	virtual void          setXYZ   (void * pixel, float values[]) const;
	virtual void          setYUV   (void * pixel, unsigned int yuv) const;
	virtual void          setGray  (void * pixel, unsigned char gray) const;
	virtual void          setGray  (void * pixel, float gray) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;  ///< Ignored by default.  Formats that actually have an alpha channel must override this method.

	int depth;  ///< Number of bytes in one pixel, including any padding
	int precedence;  ///< Imposes a (partial?) order on formats according to information content.  Bigger numbers have more information.
	// The following two flags could be implemented several different ways.
	// One alternative would be to make a more complicated class hierarchy
	// that implies the information.  Eg: have intermediate classes
	// PixelFormatMonochrome and PixelFormatColor.  Another alternative is
	// to put channel information into a bitmap and use masks to determine
	// various states.
	bool monochrome;  ///< Indicates that this format has no color components.
	bool hasAlpha;  ///< Indicates that this format has a real alpha channel (as apposed to a default alpha value).
  };

  class PixelFormatGrayChar : public PixelFormat
  {
  public:
	PixelFormatGrayChar ();

	virtual Image filter (const Image & image);
	void fromGrayFloat  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromRGBAChar   (const Image & image, Image & result) const;
	void fromRGBABits   (const Image & image, Image & result) const;
	void fromAny        (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
	virtual void          setGray (void * pixel, unsigned char gray) const;
	virtual void          setGray (void * pixel, float gray) const;
  };

  class PixelFormatGrayShort : public PixelFormat
  {
  public:
	PixelFormatGrayShort ();

	virtual Image filter (const Image & image);
	void fromGrayChar   (const Image & image, Image & result) const;
	void fromGrayFloat  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromAny        (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
	virtual void          setGray (void * pixel, unsigned char gray) const;
	virtual void          setGray (void * pixel, float gray) const;
  };

  class PixelFormatGrayFloat : public PixelFormat
  {
  public:
	PixelFormatGrayFloat ();

	virtual Image filter (const Image & image);
	void fromGrayChar   (const Image & image, Image & result) const;
	void fromGrayShort  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromRGBAChar   (const Image & image, Image & result) const;
	void fromRGBABits   (const Image & image, Image & result) const;
	void fromAny        (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
	virtual void          setGray (void * pixel, unsigned char gray) const;
	virtual void          setGray (void * pixel, float gray) const;
  };

  class PixelFormatGrayDouble : public PixelFormat
  {
  public:
	PixelFormatGrayDouble ();

	virtual Image filter (const Image & image);
	void fromGrayChar  (const Image & image, Image & result) const;
	void fromGrayFloat (const Image & image, Image & result) const;
	void fromRGBAChar  (const Image & image, Image & result) const;
	void fromRGBABits  (const Image & image, Image & result) const;
	void fromAny       (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
	virtual void          setGray (void * pixel, unsigned char gray) const;
	virtual void          setGray (void * pixel, float gray) const;
  };

  class PixelFormatRGBAChar : public PixelFormat
  {
  public:
	PixelFormatRGBAChar ();

	virtual Image filter (const Image & image);
	void fromGrayChar   (const Image & image, Image & result) const;
	void fromGrayFloat  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromRGBChar    (const Image & image, Image & result) const;
	void fromRGBABits   (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;

	static void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift);  ///< Shifts are set to move bits from this format to the one indicated by the masks.
  };

  class PixelFormatRGBChar : public PixelFormat
  {
  public:
	PixelFormatRGBChar ();

	virtual Image filter (const Image & image);
	void fromGrayChar   (const Image & image, Image & result) const;
	void fromGrayShort  (const Image & image, Image & result) const;
	void fromGrayFloat  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromRGBAChar   (const Image & image, Image & result) const;
	void fromRGBABits   (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;

	static void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift);  ///< Shifts are set to move bits from this format to the one indicated by the masks.
  };

  /**
	 Allows construction of arbitrary RGBA formats.  Mainly used to support
	 X windows interface.  This class is named "RGBA", but this just
	 indicates what channels are supported.  The order of the
	 channels is actually arbitrary.  Bitmasks define the position of each
	 channel, and are by nature in terms of machine words.  Therefore,
	 a particular set of bitmasks will have different meanings on different
	 endian machines.
  **/
  class PixelFormatRGBABits : public PixelFormat
  {
  public:
	PixelFormatRGBABits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask);

	virtual Image filter (const Image & image);
	void fromGrayChar   (const Image & image, Image & result) const;
	void fromGrayFloat  (const Image & image, Image & result) const;
	void fromGrayDouble (const Image & image, Image & result) const;
	void fromRGBAChar   (const Image & image, Image & result) const;
	void fromRGBABits   (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;

	void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift) const;

	unsigned int redMask;
	unsigned int greenMask;
	unsigned int blueMask;
	unsigned int alphaMask;
  };

  class PixelFormatRGBAShort : public PixelFormat
  {
  public:
	PixelFormatRGBAShort ();

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;
  };

  class PixelFormatRGBShort : public PixelFormat
  {
  public:
	PixelFormatRGBShort ();

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
  };

  class PixelFormatRGBAFloat : public PixelFormat
  {
  public:
	PixelFormatRGBAFloat ();

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          getRGBA  (void * pixel, float values[]) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA  (void * pixel, float values[]) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;
  };

  /**
	 Assumes that pixel pairs are 32-bit word aligned.  So, if the pixel
	 address falls in the center of a 32-bit word it must refer to the "VY"
	 portion of the pair.  Likewise, an address that falls on a 32-bit boundary
	 refers to the "UY" portion.
  **/
  class PixelFormatUYVYChar : public PixelFormat
  {
  public:
	PixelFormatUYVYChar ();

	virtual Image filter (const Image & image);
	void fromYUYVChar (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned int  getYUV  (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setYUV  (void * pixel, unsigned int yuv) const;
  };

  /// Same as UYVY, but with different ordering within the dwords
  class PixelFormatYUYVChar : public PixelFormat
  {
  public:
	PixelFormatYUYVChar ();

	virtual Image filter (const Image & image);
	void fromUYVYChar (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned int  getYUV  (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setYUV  (void * pixel, unsigned int yuv) const;
  };

  class PixelFormatHLSFloat : public PixelFormat
  {
  public:
	PixelFormatHLSFloat ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;

	float HLSvalue (const float & n1, const float & n2, float h) const;  ///< Subroutine of getRGBA(floats).
  };

  extern PixelFormatGrayChar   GrayChar;
  extern PixelFormatGrayShort  GrayShort;
  extern PixelFormatGrayFloat  GrayFloat;
  extern PixelFormatGrayDouble GrayDouble;
  extern PixelFormatRGBAChar   RGBAChar;
  extern PixelFormatRGBAShort  RGBAShort;
  extern PixelFormatRGBAFloat  RGBAFloat;
  extern PixelFormatRGBChar    RGBChar;
  extern PixelFormatRGBShort   RGBShort;
  extern PixelFormatUYVYChar   UYVYChar;
  extern PixelFormatYUYVChar   YUYVChar;
  extern PixelFormatHLSFloat   HLSFloat;

  // Naming convention for RGBBits (other than BGRChar):
  // R<red bits>G<green bits>B<blue bits>
  // EG: R5G5B5 would be a 15 bit RGB format.


  // Pixel --------------------------------------------------------------------

  /**
	 Provides convenient access to the functions of PixelFormat for a specific
	 datum.
	 All linear operations between pixels take place in RGB space.  It would
	 be better colorwise to do them in XYZ space, but most formats are closer
	 to RGB than XYZ numerically (ie: require less conversion), so it is
	 cheaper to do them in RGB.

	 There are two possible ways to think of this class:
	 1) It provides a shorthand for applying the color access functions of
	    PixelFormat to a particular (x,y) position in an image.  E.g.:
		image(x,y).getRGBA () <=> image.format->getRGBA (image(x,y))
		assuming the image(x,y) is defined to return a void * to the
		correct address.
	 1.1) It provides a way of treating the operation image(x,y) as a
	    reference to the pixel without actually knowing its format.
	 2) It provides an abstract way to do numerical operations on pixels,
	    such as scaling, adding, and alpha-blending.
	    As such, it should really be an extension of Vector<float>.
	 The current implementation kind of munges these two ideas together,
	 creating something that is rather inefficient for either purpose.
	 Job 1 should be moved into class Image in the form of
	 more color accessor functions.  This makes most sense from an efficiency
	 standpoint.  It is not clear to me yet what to do about job 1.1 and job 2.
	 Currently, the only code that makes serious use of Pixel uses it for
	 both these jobs.  However, it does not do this efficiently.
	 Pixel would do jobs 1.1 and 2 more efficiently if we relieve it of the
	 burden of storing intermediate results.  We should instead store the
	 intermediate results in Vector<float> as RGB values.
  **/
  class Pixel
  {
  public:
	Pixel ();
	Pixel (unsigned int rgba);
	Pixel (const Pixel & that);
	Pixel (const PixelFormat & format, void * pixel);

	unsigned int getRGBA () const;
	void         getRGBA (float values[]) const;
	void         getXYZ  (float values[]) const;
	void         setRGBA (unsigned int rgba) const;
	void         setRGBA (float values[]) const;
	void         setXYZ  (float values[]) const;

	Pixel & operator = (const Pixel & that);  ///< Set self to have color contained in that.
	Pixel & operator += (const Pixel & that);  ///< Set self to sum of respective color channels.
	Pixel operator + (const Pixel & that) const;  ///< Add respective color channels.
	Pixel operator * (const Pixel & that) const;  ///< Multiply respective color channels.
	Pixel operator * (float scalar) const;  ///< Scale each channel.
	Pixel operator / (float scalar) const;  ///< Scale each channel.
	Pixel operator << (const Pixel & that);  ///< Alpha blend that into this.  The alpha value the governs the blending comes from that.

	const PixelFormat * format;
	void * pixel;  ///< always points to target data: either the union below or some point in an Image buffer

	// This union is rather gross, and it would be nice not carry the baggage
	// around.  However, this is cheaper than allocating a small piece on the
	// heap.  Since Pixel is mainly intended to allow linear ops on abstract
	// image elements, it will frequently need its own storage (for
	// intermediate values) as opposed to referrencing an Image buffer.
	// "Frequently" means in the middle of an O(n^3) loop where n is the width
	// or height of an Image.
	union
	{
	  unsigned char graychar;
	  float         grayfloat;
	  double        graydouble;
	  unsigned int  rgbchar;
	  float         rgbafloat[4];
	} data;
  };


  // File formats -------------------------------------------------------------

  /**
	 Danger!  Not thread-safe.  Need a mutex around modifications and accesses
	 to static variable "formats".  Add if there's ever a need to create
	 and destroy formats on the fly with multiple threads trying to access
	 the list.
	 Translation of the above: it is very unlikely that in practice there
	 will be a problem with the "unsafe" version.  :)
  **/
  class ImageFileFormat
  {
  public:
	ImageFileFormat ();
	virtual ~ImageFileFormat ();

	virtual void read (const std::string & fileName, Image & image) const;
	virtual void read (std::istream & stream, Image & image) const = 0;
	virtual void write (const std::string & fileName, const Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const = 0;
	virtual bool isIn (std::istream & stream) const = 0;  ///< Determines if this format is on the stream.  Always rewinds stream back to where it was when function was called.
	virtual bool handles (const std::string & formatName) const = 0;  ///< Determines if this object handles the format with the given human readable name.

	static ImageFileFormat * find (const std::string & fileName);  ///< Determines what format the stream is in.
	static ImageFileFormat * find (std::istream & stream);  ///< Ditto.  Always returns stream to original position.
	static ImageFileFormat * findName (const std::string & formatName);  ///< Determines what format to use based on given name.
	static void getMagic (std::istream & stream, std::string & magic);  ///< Attempts to read magic.size () worth of bytes from stream and return them in magic.  Always returns stream to original position.

	static std::vector<ImageFileFormat *> formats;
  };

  class ImageFileFormatPGM : public ImageFileFormat
  {
  public:
	virtual void read (std::istream & stream, Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const;
	virtual bool isIn (std::istream & stream) const;
	virtual bool handles (const std::string & formatName) const;
  };

  class ImageFileFormatEPS : public ImageFileFormat
  {
  public:
	virtual void read (std::istream & stream, Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const;
	virtual bool isIn (std::istream & stream) const;
	virtual bool handles (const std::string & formatName) const;
  };

  class ImageFileFormatJPEG : public ImageFileFormat
  {
  public:
	virtual void read (std::istream & stream, Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const;
	virtual bool isIn (std::istream & stream) const;
	virtual bool handles (const std::string & formatName) const;
  };

  class ImageFileFormatTIFF : public ImageFileFormat
  {
  public:
	// Note: This format can't read and write streams, so those two methods
	// will throw an exception.
	virtual void read (const std::string & fileName, Image & image) const;
	virtual void read (std::istream & stream, Image & image) const;
	virtual void write (const std::string & fileName, const Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const;
	virtual bool isIn (std::istream & stream) const;
	virtual bool handles (const std::string & formatName) const;
  };

  class ImageFileFormatMatlab : public ImageFileFormat
  {
  public:
	virtual void read (std::istream & stream, Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const;
	virtual bool isIn (std::istream & stream) const;
	virtual bool handles (const std::string & formatName) const;
	void parseType (int type, int & numericType) const;
  };


  // Image inlines ------------------------------------------------------------

  inline Image &
  Image::operator <<= (const Image & that)
  {
	buffer    = that.buffer;
	format    = that.format;
	width     = that.width;
	height    = that.height;
	timestamp = that.timestamp;
	return *this;
  }

  inline bool
  Image::operator == (const Image & that) const
  {
	return    buffer    == that.buffer
	       && format    == that.format
	       && width     == that.width
	       && height    == that.height
	       && timestamp == that.timestamp;
  }

  inline bool
  Image::operator != (const Image & that) const
  {
	return ! (*this == that);
  }

  inline Pixel
  Image::operator () (int x, int y) const
  {
	return Pixel (*format, & ((char *) buffer)[(y * width + x) * format->depth]);
  }

  inline void
  Image::getRGBA (int x, int y, float values[]) const
  {
	format->getRGBA (& ((char *) buffer)[(y * width + x) * format->depth], values);
  }

  inline unsigned int
  Image::getYUV (int x, int y) const
  {
	return format->getYUV (& ((char *) buffer)[(y * width + x) * format->depth]);
  }

  inline unsigned char
  Image::getGray (int x, int y) const
  {
	return format->getGray (& ((char *) buffer)[(y * width + x) * format->depth]);
  }

  inline void
  Image::getGray (int x, int y, float & gray) const
  {
	format->getGray (& ((char *) buffer)[(y * width + x) * format->depth], gray);
  }

  inline unsigned char
  Image::getAlpha (int x, int y) const
  {
	return format->getAlpha (& ((char *) buffer)[(y * width + x) * format->depth]);
  }

  inline void
  Image::setRGBA (int x, int y, float values[])
  {
	format->setRGBA (& ((char *) buffer)[(y * width + x) * format->depth], values);
  }

  inline void
  Image::setYUV (int x, int y, unsigned int yuv)
  {
	format->setYUV (& ((char *) buffer)[(y * width + x) * format->depth], yuv);
  }

  inline void
  Image::setGray (int x, int y, unsigned char gray)
  {
	format->setGray (& ((char *) buffer)[(y * width + x) * format->depth], gray);
  }

  inline void
  Image::setGray (int x, int y, float gray)
  {
	format->setGray (& ((char *) buffer)[(y * width + x) * format->depth], gray);
  }

  inline void
  Image::setAlpha (int x, int y, unsigned char alpha)
  {
	format->setAlpha (& ((char *) buffer)[(y * width + x) * format->depth], alpha);
  }
}


#endif
