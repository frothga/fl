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


#ifndef fl_image_h
#define fl_image_h


#include "fl/pointer.h"
#include "fl/metadata.h"

#include <iostream>
#include <string>
#include <vector>

#include <assert.h>

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
  // Forward declaration for use by Image
  class SHARED PixelBuffer;
  class SHARED PixelFormat;
  class SHARED Pixel;
  class SHARED ImageFileFormat;


  // Image --------------------------------------------------------------------

  class SHARED Image
  {
  public:
	Image ();  ///< Creates a new image of GrayChar, but with no buffer memory allocated.
	Image (const PixelFormat & format);  ///< Same as above, but with given PixelFormat
	Image (int width, int height);  ///< Allocates buffer of size width x height x GrayChar.depth bytes.
	Image (int width, int height, const PixelFormat & format);  ///< Same as above, but with given PixelFormat
	Image (const Image & that);  ///< Points our buffer to same location as "that" and copies all of its metadata.
	Image (void * block, int width, int height, const PixelFormat & format);  ///< Binds to an external block of memory.  Implies PixelBufferPacked.
	Image (const std::string & fileName);  ///< Create image initialized with contents of file.

	void read (const std::string & fileName);  ///< Read image from fileName.  Format will be determined automatically.
	void read (std::istream & stream);
	void write (const std::string & fileName, const std::string & formatName = "") const;  ///< Write image to fileName.
	void write (std::ostream & stream, const std::string & formatName = "pgm") const;

	void copyFrom (const Image & that);  ///< Duplicates another Image.  Copy all raster info into private buffer, and copy all other metadata.
	void copyFrom (void * block, int width, int height, const PixelFormat & format);  ///< Copy from a non-Image source.  Determine size of buffer in bytes as width x height x depth.
	void attach (void * block, int width, int height, const PixelFormat & format);  ///< Binds to an external block of memory.
	void detach ();  ///< Set the state of this image as if it has no buffer.  Releases (but only frees if appropriate) any memory.
	Image roi (int fromX = 0, int fromY = 0, int width = -1, int height = -1);  ///< Limited support for region-of-interest processing.  This image must have a packed buffer type, and must remain in existence as long as the returned image exists.  Parameters have similar semantics to bitblt().

	void resize (int width, int height, bool preserve = false);
	void bitblt (const Image & that, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = -1, int height = -1);  ///< -1 for width or height means "maximum possible value"
	void clear (unsigned int rgba = 0);  ///< Initialize buffer, if it exists, to given color.  In case rgba == 0, simply zeroes out memory, since this generally results in black in most pixel formats.

	Image operator + (const Image & that);  ///< Creates an image that sums pixel values from this and that.
	Image operator - (const Image & that);  ///< Creates an image whose pixels are the difference between this and that.
	Image operator * (double factor);  ///< Creates an image containing this images's pixels scaled by factor.  Ie: factor == 1 -> no change; factor == 2 -> bright spots are twice as bright, and dark spots (negative values relative to bias) are twice as dark.
	Image & operator *= (double factor);  ///< Scales each pixel by factor.
	Image & operator += (double value);  ///< Adds value to each pixel
	bool operator == (const Image & that) const;  ///< Determines if both images have exactly the same metadata and buffer.  This is a strong, but not perfect, indicator that no change has occurred to the contents between the construction of the respective objects.
	bool operator != (const Image & that) const;  ///< Negated form of operator ==.

	// Note that bounds checking for these accessor functions is the responsibility of the caller.
	Pixel         operator () (int x, int y) const;  ///< Returns a Pixel object that wraps (x,y).
	unsigned int  getRGBA  (int x, int y) const;
	void          getRGBA  (int x, int y, float values[]) const;
	void          getXYZ   (int x, int y, float values[]) const;
	unsigned int  getYUV   (int x, int y) const;
	unsigned char getGray  (int x, int y) const;
	void          getGray  (int x, int y, float & gray) const;
	unsigned char getAlpha (int x, int y) const;
	void          setRGBA  (int x, int y, unsigned int rgba);
	void          setRGBA  (int x, int y, float values[]);
	void          setXYZ   (int x, int y, float values[]);
	void          setYUV   (int x, int y, unsigned int yuv);
	void          setGray  (int x, int y, unsigned char gray);
	void          setGray  (int x, int y, float gray);
	void          setAlpha (int x, int y, unsigned char alpha);

	// Data
	PointerPoly<PixelBuffer>       buffer;
	PointerPoly<const PixelFormat> format;
	int                            width;  ///< This should be viewed as cached information from the PixelBuffer.  Only make changes via resize().
	int                            height;  ///< This should be viewed as cached information from the PixelBuffer.  Only make changes via resize().
	double                         timestamp;  ///< Time when image was captured.  If part of a video, then time when image should be displayed.
  };


  // Filter -------------------------------------------------------------------

  /**
	 Base class for reified functions that take as input an image and output
	 another image.
  **/
  class SHARED Filter
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


  // PixelBuffer --------------------------------------------------------------

  /**
	 Changes the stride and height of a dense raster in memory.  Used mainly as
	 a utility function by PixelBuffer, but sometimes useful elsewhere.
  **/
  extern SHARED void reshapeBuffer (Pointer & memory, int oldStride, int newStride, int newHeight, int pad = 0);

  /**
	 An interface for various classes that manage the storage of image data.
   **/
  class SHARED PixelBuffer : public ReferenceCounted
  {
  public:
	virtual ~PixelBuffer ();

	/**
	   Maps (x,y) coordinates to a pointer that tells the PixelFormat where
	   to get the color information.  See comment on planes for details.

	   This function is not thread-safe for two reasons.  First, if the
	   underlying storage moves in memory (such as cached blocks from a large
	   disk file), then one call to this function could invalidate the
	   pointer(s) returned by another call.  Second, if planes != 1, then one
	   call to this function will change the array of pointers filled in by
	   another call.  For multi-threaded code, you should surround the call to
	   this function and the call to PixelFormat in a mutex.  Alternately, if
	   the underlying storage doesn't move in memory, then you could create
	   multiple instances of PixelBuffer that hold the same underlying storage.
	**/
	virtual void * pixel (int x, int y) = 0;
	virtual void resize (int width, int height, const PixelFormat & format, bool preserve = false) = 0;  ///< Same semantics as Image::resize()
	virtual PixelBuffer * duplicate () const = 0;  ///< Make a copy of self on the heap, with deap-copy semantics.
	virtual void clear () = 0;  ///< Fill buffer(s) with zeros.
	virtual bool operator == (const PixelBuffer & that) const;
	bool operator != (const PixelBuffer & that) const
	{
	  return ! (*this == that);
	}

	/**
	   Indicates the structure pointed to by pixel().  There are three forms:
	   <OL>
	   <LI>planes = 1 -- A direct pointer to a packed pixel containing all the
	   color channels.
	   <LI>planes > 1 -- A pointer to an array of pointers, each one
	   addressing one of the color channels.  The value of planes indicates the
	   size of the array.
	   <LI>planes < 1 -- A pointer to a structure.  Each kind of structure is
	   assigned a unique negative number.  Current structures are:
	   <UL>
	   <LI>-1 -- PixelBufferGroups::PixelData
	   </UL>
	   </OL>
	   Any value other than planes==1 is not thread safe.  The structure or
	   array is a member of the PixelBuffer object, and its contents will be
	   changed by the next call to pixel().  For safe operation, you must
	   create a separate PixelBuffer object for each thread.  The default
	   mode when assigning Image objects is to share the same PixelBuffer,
	   so you must explicitly duplicate it.

	   <p>A PixelFormat must have exactly the same value of planes to be
	   compatible.
	**/
	int planes;
  };

  /**
	 The default structure for most Images.  Each pixel contains its color
	 channels contiguously, and pixels are arranged contiguously in memory.
   **/
  class SHARED PixelBufferPacked : public PixelBuffer
  {
  public:
	PixelBufferPacked (int depth = 1);
	PixelBufferPacked (int stride, int height, int depth);
	PixelBufferPacked (void * buffer, int stride, int height, int depth);  ///< Binds to an external block of memory.
	PixelBufferPacked (const Pointer & buffer, int stride, int depth, int offset = 0);
	virtual ~PixelBufferPacked ();

	virtual void * pixel (int x, int y);
	virtual void resize (int width, int height, const PixelFormat & format, bool preserve = false);  ///< We assume that stride must now be set to width.  The alternative, if width < stride, would be to take no action.
	virtual PixelBuffer * duplicate () const;
	virtual void clear ();
	virtual bool operator == (const PixelBuffer & that) const;

	void copyFrom (void * buffer, int stride, int height, int depth);  ///< Makes a duplicate of the block of memory.
	void * base () const;

	int offset;
	int stride;
	int depth;
	Pointer memory;
  };

  /**
	 Each color channel is stored in a separate block of memory.  The blocks
	 are not necessarily contiguous with each other, and they don't
	 necessarily have stride == width.  This structure is typical for YUV
	 data used in video processing (see VideoFileFormatFFMPEG).

	 Assumes only three color channels.  Treats first channel as "full size",
	 while the second and third channels have vertical and horizontal devisors.
	 Assumes that the second and third channels have exactly the same geometry.
	 Assumes that the depth of any given channel is exactly one byte.
	 When resizing or intializing without specifying stride, determines a
	 stride0 that is a multiple of 16 bytes.
   **/
  class SHARED PixelBufferPlanar : public PixelBuffer
  {
  public:
	PixelBufferPlanar ();
	PixelBufferPlanar (int stride, int height, int ratioH = 1, int ratioV = 1);
	PixelBufferPlanar (void * buffer0, void * buffer1, void * buffer2, int stride0, int stride12, int height, int ratioH = 1, int ratioV = 1);
	virtual ~PixelBufferPlanar ();

	virtual void * pixel (int x, int y);
	virtual void resize (int width, int height, const PixelFormat & format, bool preserve = false);  ///< Assumes that ratioH and ratioV are already set correctly.
	virtual PixelBuffer * duplicate () const;
	virtual void clear ();
	virtual bool operator == (const PixelBuffer & that) const;

	Pointer plane0;
	Pointer plane1;
	Pointer plane2;
	int stride0;
	int stride12;  ///< Can be derived from stride0 / ratioH, but more efficient to pre-compute it.
	int ratioH;
	int ratioV;

	void * pixelArray[3];  ///< Temporary storage for marshalled addresses.  Not thread-safe.
  };

  /**
	 Describes a packed buffer in which the pixels on each row are divided
	 into functionally inseparable groups.  There are at least two cases
	 where this happens:
	 <ul>
	 <li>gray formats where each pixel is smaller than one byte, for example
	 1-bit monochrome with 8 pixels per byte.
	 <li>YUV formats where several Y values share a common pair of U and V
	 values.
	 </ul>
  **/
  class SHARED PixelBufferGroups : public PixelBuffer
  {
  public:
	PixelBufferGroups (int pixels, int bytes);
	PixelBufferGroups (int stride, int height, int pixels, int bytes);
	PixelBufferGroups (void * buffer, int stride, int height, int pixels, int bytes);
	virtual ~PixelBufferGroups ();

	virtual void * pixel (int x, int y);
	virtual void resize (int width, int height, const PixelFormat & format, bool preserve = false);  ///< stride will be set to ceil (width * groupBytes / groupPixels).
	virtual PixelBuffer * duplicate () const;
	virtual void clear ();
	virtual bool operator == (const PixelBuffer & that) const;

	int stride;
	int pixels;  ///< Pixels per group.
	int bytes;  ///< Bytes per group.
	Pointer memory;

	struct PixelData
	{
	  unsigned char * address;  ///< Pointer to first byte of pixel group.
	  int             index;  ///< Indicates which pixel in group to select.  Defined simply as x % groupPixels.
	};
	PixelData pixelData;
  };

  /**
	 Encapsulates an image that is too big to fit in memory.  Instead, the
	 image resides on disk, and a subset of its blocks are kept in cache at
	 any given time.
   **/
  class SHARED PixelBufferBig : public PixelBuffer
  {
  public:
	PixelBufferBig ();
	virtual ~PixelBufferBig ();
  };


  // ImageOf ------------------------------------------------------------------

  /**
	 An Image designed to make (x,y) style access to packed formats as
	 efficient as possible.  To be maximally efficient, one should instead
	 iterate over the buffer directly using pointers.
   **/
  template<class T>
  class ImageOf : public Image
  {
  public:
	// These constructors blindly wrap the constructors of Image, without
	// regard to the size or type of data returned by operator (x,y).
	ImageOf ()
	{
	  memory = 0;
	  stride = 0;
	}

	ImageOf (const PixelFormat & format) : Image (format)
	{
	  memory = 0;
	  stride = 0;
	}

	ImageOf (int width, int height) : Image (width, height)
	{
	  selfBind ();
	}

	ImageOf (int width, int height, const PixelFormat & format) : Image (width, height, format)
	{
	  if (! (PixelBufferPacked *) buffer)
	  {
		if (width == 0  ||  height == 0)
		{
		  memory = 0;
		  return;
		}
		throw "Can't wrap non-packed type images";
	  }
	  selfBind ();
	}

	ImageOf (const Image & that) : Image (that)
	{
	  if (! (PixelBufferPacked *) buffer)
	  {
		if (width == 0  ||  height == 0)
		{
		  memory = 0;
		  return;
		}
		throw "Can't wrap non-packed type images";
	  }
	  selfBind ();
	}

	void selfBind ()
	{
  	  memory = (char *) buffer->pixel (0, 0);
	  PixelBufferPacked * pbp = (PixelBufferPacked *) buffer;
	  if (! pbp) throw "Can't wrap non-packed type images";
	  stride = pbp->stride;
	}

	void read (const std::string & fileName)
	{
	  Image::read (fileName);
	  selfBind ();
	}

	void read (std::istream & stream)
	{
	  Image::read (stream);
	  selfBind ();
	}

	ImageOf<T> & operator = (const Image & that)
	{
	  Image::operator = (that);
	  selfBind ();
	  return *this;
	}

	void copyFrom (const Image & that)
	{
	  Image::copyFrom (that);
	  selfBind ();
	}

	void copyFrom (void * block, int width, int height, const PixelFormat & format)
	{
	  Image::copyFrom (block, width, height, format);
	  selfBind ();
	}

	void attach (void * block, int width, int height, const PixelFormat & format)
	{
	  Image::attach (buffer, width, height, format);
	  selfBind ();
	}

	void resize (int width, int height, bool preserve = false)
	{
	  Image::resize (width, height, preserve);
	  selfBind ();
	}

	T & operator () (int x, int y) const
	{
	  return ((T *) (memory + y * stride))[x];
	}

	char * memory;  ///< Points to the memory block held by Image::buffer, and must be constructed from it.
	int stride;
  };

  template<class T>
  inline ImageOf<T> &
  operator *= (ImageOf<T> & image, const Filter & filter)
  {
	return image = ((Filter &) filter).filter (image);
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

	 <p>Naming convention for PixelFormats
	   = <color space><basic C type for one channel>
       Color space names refer to sequence of channels (usually bytes) in
       memory, rather than in machine words (eg: registers in the processor).
       The leftmost letter in a name refers to the lowest numbered address.
	   If a channel is larger than one byte, then the bytes are laid out
	   within the channel according to the standard for the machine.

	 <p>Naming convention for accessor methods --
	   The data is in machine words, so names describe sequence
	   within machine words.  Accessors guarantee that the order in the
	   machine word will be the same, regardless of endian.  (This implies
	   that the implementation of an accessor changes with endian.)
	   The leftmost letter refers to the most significant byte in the word.
	   Some accessors take arrays, and since arrays are memory blocks they
	   follow the memory order convention.

	 <p>According to the Poynton color FAQ, YUV refers to a very specific
	 scaling of the color difference values for composite signals, and it is
	 improper to use these channel names for anything else.  However, in
	 colloquial usage U and V refer generically to color difference values
	 without regard to scaling.  If there is ever a need to deal with the
	 real YUV color space, everything will have to be renamed to make the
	 semantics clear.  Until then, in this code U and V will be synonyms
	 for Pb and Pr scaled to unsigned chars with a bias of 128.

	 <p>Another note on YUV: These formats are pretty specific to video,
	 and it is unlikely that there will ever be YUV formats with channel
	 sizes other than char, so these format names do not bear the obligatory
	 "Char" on the end.  This can also be changed once the first exception
	 becomes evident.

	 \todo Add accessor for numbered color channels.  This will be most
	 meaningful for hyperspectal data.
  **/
  class SHARED PixelFormat : public Filter, public ReferenceCounted
  {
  public:
	virtual ~PixelFormat ();

	virtual Image filter (const Image & image);  ///< Return an Image in this format
	virtual void fromAny (const Image & image, Image & result) const;

	virtual PixelBuffer * buffer () const;  ///< Construct a PixelBuffer suitable for holding data of the type described by this object.
	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;  ///< Creates a suitable PixelBuffer bound to the given external block of memory.  Makes best effort to guess the start of each plane in the case of planar formats.  (Default implementation assumes packed buffer.)

	virtual bool operator == (const PixelFormat & that) const;  ///< Checks if this and that describe the same interpretation of memory contents.
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

	int planes;  ///< The number of entries in the array passed through the "pixel" parameter.  See PixelBuffer::planes for semantics.  This format must agree with the PixelBuffer on the meaning of the pixel pointer.
	float depth;  ///< Number of bytes per pixel, including any padding.  This could have been defined as bits per pixel, but there actually exists a format (4CC==IF09) which has a non-integral number of bits per pixel.  Defined as bytes, this field allows one to compute the total number of bytes needed by the image (even for planar formats) as width * height * depth.
	int precedence;  ///< Imposes a (partial?) order on formats according to information content.  Bigger numbers have more information.
	// The following two flags could be implemented several different ways.
	// One alternative would be to make a more complicated class hierarchy
	// that implies the information.  Eg: have intermediate classes
	// PixelFormatMonochrome and PixelFormatColor.  Another alternative is
	// to put channel information into a bitmap and use masks to determine
	// various states.
	bool monochrome;  ///< Indicates that this format has no color components.
	bool hasAlpha;  ///< Indicates that this format has a real alpha channel (as apposed to a default alpha value).

	// Look up tables for conversion between linear and non-linear values.
	static unsigned char * lutFloat2Char;  ///< First convert float in [0,1] to unsigned short, then offset into this table to get unsigned char value.
	static float *         lutChar2Float;  ///< Use unsigned char value directly as index into this table of float values.
	static unsigned char * buildFloat2Char ();  ///< Construct lutFloat2Char during static initialization.
	static float *         buildChar2Float ();  ///< Construct lutChar2Float during static initialization.
  };

  /**
	 Specifies the interface required by PixelBufferGroups.
   **/
  class SHARED Macropixel
  {
  public:
	int pixels;  ///< Pixels per group.
	int bytes;  ///< Bytes per group.
  };

  class SHARED PixelFormatPalette : public PixelFormat, public Macropixel
  {
  public:
	PixelFormatPalette (unsigned char * r, unsigned char * g, unsigned char * b, int stride = 1, int bits = 8, bool bigendian = true);  ///< r, g, b and stride specify the structure of the source table, which we copy into our internal format.

	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;

	int           bits;  ///< Number of bits in one pixel.
	unsigned char masks[8];  ///< An array of bit masks for each pixel packed in one byte.  x % bits gives an offset into this array.  There may be some wasted entries, but this is a trivial loss in exchange for the simplicity of a fixed-size array.
	int           shifts[8];  ///< How far to downshift the indexed pixel to put it in the least significant position.
	unsigned int  palette[256];  ///< Colors stored as rgba values, that is, exactly the form returned by getRGBA().
  };

  /**
	 Interprets gray values consisting of fewer than 8 bits, such that several
	 (2, 4 or 8) gray pixels are packed into one byte.  This could
	 easily be generalized to chunks bigger than 1 byte, but presently there
	 is no need.  PixelFormatGrayChar is equivalent to this format with
	 bits == 8.  PixelFormatGrayShort also handles a set of significant bits
	 other than the full word, but it is a somewhat different layout, since
	 only one pixel occurs in any one word.
   **/
  class SHARED PixelFormatGrayBits : public PixelFormat, public Macropixel
  {
  public:
	/**
	   @param bigendian Indicates that the pixel with the smallest horizontal
	   coordinate (that is, X position) appears in the most significant bit(s)
	   of the byte.  Within a single pixel, the ordering of the bits is
	   unnaffected by this flag, and always follows the convention of the
	   machine.
	**/
	PixelFormatGrayBits (int bits = 1, bool bigendian = true);

	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;

	int           bits;  ///< Number of bits in one pixel.
	unsigned char masks[8];  ///< An array of bit masks for each pixel packed in one byte.  x % bits gives an offset into this array.  There may be some wasted entries, but this is a trivial loss in exchange for the simplicity of a fixed-size array.
	int           shifts[8];  ///< How far to upshift the indexed pixel to put it in the most significant position.
  };

  class SHARED PixelFormatGrayChar : public PixelFormat
  {
  public:
	PixelFormatGrayChar ();

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromGrayShort   (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;
	void fromRGBAChar    (const Image & image, Image & result) const;
	void fromRGBABits    (const Image & image, Image & result) const;
	void fromYCbCr       (const Image & image, Image & result) const;

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

  class SHARED PixelFormatGrayAlphaChar : public PixelFormat
  {
  public:
	PixelFormatGrayAlphaChar ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
  };

  class SHARED PixelFormatGrayShort : public PixelFormat
  {
  public:
	PixelFormatGrayShort (unsigned short grayMask = 0xFFFF);

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
	virtual void          setGray (void * pixel, unsigned char gray) const;
	virtual void          setGray (void * pixel, float gray) const;

	unsigned short grayMask;  ///< Indicates what (contiguous) bit in the pixel actually carry intensity info.
	int grayShift;  ///< How many bits to shift grayMask to align the msb with bit 15.
  };

  class SHARED PixelFormatGrayAlphaShort : public PixelFormat
  {
  public:
	PixelFormatGrayAlphaShort ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
  };

  class SHARED PixelFormatGrayFloat : public PixelFormat
  {
  public:
	PixelFormatGrayFloat ();

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayShort   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;
	void fromRGBAChar    (const Image & image, Image & result) const;
	void fromRGBChar     (const Image & image, Image & result) const;
	void fromRGBABits    (const Image & image, Image & result) const;
	void fromYCbCr       (const Image & image, Image & result) const;

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

  class SHARED PixelFormatGrayDouble : public PixelFormat
  {
  public:
	PixelFormatGrayDouble ();

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayShort   (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromRGBAChar    (const Image & image, Image & result) const;
	void fromRGBChar     (const Image & image, Image & result) const;
	void fromRGBABits    (const Image & image, Image & result) const;
	void fromYCbCr       (const Image & image, Image & result) const;

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

  /**
	 Allows construction of arbitrary RGBA formats.  Mainly used to support
	 X windows interface.  This class is named "RGBA", but this just
	 indicates what channels are supported.  The order of the
	 channels is actually arbitrary.  Bitmasks define the position of each
	 channel, and are by nature in terms of machine words.  Therefore,
	 a particular set of bitmasks will have different meanings on different
	 endian machines.
  **/
  class SHARED PixelFormatRGBABits : public PixelFormat
  {
  public:
	PixelFormatRGBABits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask);

	virtual Image filter (const Image & image);
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayShort   (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;
	void fromRGBABits    (const Image & image, Image & result) const;
	void fromYCbCr       (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;

	void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift) const;
	static int countBits (unsigned int mask);

	unsigned int redMask;
	unsigned int greenMask;
	unsigned int blueMask;
	unsigned int alphaMask;
	int redBits;
	int greenBits;
	int blueBits;
	int alphaBits;
  };

  class SHARED PixelFormatRGBAChar : public PixelFormatRGBABits
  {
  public:
	PixelFormatRGBAChar ();

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;
	void fromRGBChar     (const Image & image, Image & result) const;
	void fromPackedYUV   (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;
  };

  class SHARED PixelFormatRGBChar : public PixelFormatRGBABits
  {
  public:
	PixelFormatRGBChar ();

	virtual Image filter (const Image & image);
	void fromGrayChar    (const Image & image, Image & result) const;
	void fromGrayShort   (const Image & image, Image & result) const;
	void fromGrayFloat   (const Image & image, Image & result) const;
	void fromGrayDouble  (const Image & image, Image & result) const;
	void fromRGBAChar    (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
  };

  class SHARED PixelFormatRGBAShort : public PixelFormat
  {
  public:
	PixelFormatRGBAShort ();

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual unsigned char getAlpha (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
	virtual void          setAlpha (void * pixel, unsigned char alpha) const;
  };

  class SHARED PixelFormatRGBShort : public PixelFormat
  {
  public:
	PixelFormatRGBShort ();

	virtual unsigned int  getRGBA  (void * pixel) const;
	virtual void          setRGBA  (void * pixel, unsigned int rgba) const;
  };

  class SHARED PixelFormatRGBAFloat : public PixelFormat
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

  class SHARED PixelFormatYUV : public PixelFormat
  {
  public:
	PixelFormatYUV (int ratioH, int ratioV) : ratioH (ratioH), ratioV (ratioV) {}

	int ratioH;  ///< How many horizontal luma samples per chroma sample.
	int ratioV;  ///< How many vertical luma samples per chroma sample.
  };

  class SHARED PixelFormatPackedYUV : public PixelFormatYUV, public Macropixel
  {
  public:
	struct YUVindex
	{
	  int y;
	  int u;
	  int v;
	};

	PixelFormatPackedYUV (YUVindex * table);

	virtual Image filter (const Image & image);
	virtual void fromAny (const Image & image, Image & result) const;
	void fromYUV         (const Image & image, Image & result) const;

	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned int  getYUV  (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setYUV  (void * pixel, unsigned int yuv) const;

	YUVindex * table;
  };

  class SHARED PixelFormatPlanarYUV : public PixelFormatYUV
  {
  public:
	PixelFormatPlanarYUV (int ratioH, int ratioV);

	virtual void fromAny (const Image & image, Image & result) const;

	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned int  getYUV  (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setYUV  (void * pixel, unsigned int yuv) const;
  };

  /**
	 Same as PixelFormatPlanarYUV, except that 16 <= Y <= 235 and
	 16 <= U,V <= 240.  Video standards call for footroom and headroom
	 to allow for analog signal overshoots.
   **/
  class SHARED PixelFormatPlanarYCbCr : public PixelFormatYUV
  {
  public:
	PixelFormatPlanarYCbCr (int ratioH, int ratioV);

	virtual void fromAny (const Image & image, Image & result) const;

	virtual PixelBuffer * attach (void * block, int width, int height, bool copy = false) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned int  getYUV  (void * pixel) const;
	virtual void          getGray (void * pixel, float & gray) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setYUV  (void * pixel, unsigned int yuv) const;
	virtual void          setGray (void * pixel, float gray) const;

	static unsigned char * lutYin;
	static unsigned char * lutUVin;
	static unsigned char * lutYout;
	static unsigned char * lutUVout;
	static float *         lutGrayOut;
	static unsigned char * buildAll ();  ///< Returns the value of lutYin, but actually constructs and assigns all 6 luts.
  };

  class SHARED PixelFormatHLSFloat : public PixelFormat
  {
  public:
	PixelFormatHLSFloat ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;

	float HLSvalue (const float & n1, const float & n2, float h) const;  ///< Subroutine of getRGBA(floats).
  };

  extern SHARED PixelFormatGrayChar           GrayChar;
  extern SHARED PixelFormatGrayAlphaChar      GrayAlphaChar;
  extern SHARED PixelFormatGrayShort          GrayShort;
  extern SHARED PixelFormatGrayAlphaShort     GrayAlphaShort;
  extern SHARED PixelFormatGrayFloat          GrayFloat;
  extern SHARED PixelFormatGrayDouble         GrayDouble;
  extern SHARED PixelFormatRGBAChar           RGBAChar;
  extern SHARED PixelFormatRGBAShort          RGBAShort;
  extern SHARED PixelFormatRGBAFloat          RGBAFloat;
  extern SHARED PixelFormatRGBChar            RGBChar;
  extern SHARED PixelFormatRGBShort           RGBShort;
  extern SHARED PixelFormatRGBABits           B5G5R5;
  extern SHARED PixelFormatRGBABits           BGRChar;
  extern SHARED PixelFormatRGBABits           BGRChar4;
  extern SHARED PixelFormatRGBABits           BGRAChar;
  extern SHARED PixelFormatPackedYUV          UYVY;
  extern SHARED PixelFormatPackedYUV          YUYV;
  extern SHARED PixelFormatPackedYUV          UYV;
  extern SHARED PixelFormatPackedYUV          UYYVYY;
  extern SHARED PixelFormatPackedYUV          UYVYUYVYYYYY;
  extern SHARED PixelFormatPlanarYCbCr        YUV420;
  extern SHARED PixelFormatPlanarYCbCr        YUV411;
  extern SHARED PixelFormatHLSFloat           HLSFloat;

  // Naming convention for RGBABits:
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
  class SHARED Pixel
  {
  public:
	Pixel ();
	Pixel (unsigned int rgba);
	Pixel (const Pixel & that);
	Pixel (const PixelFormat & format, void * pixel);

	unsigned int  getRGBA  () const;
	void          getRGBA  (float values[]) const;
	void          getXYZ   (float values[]) const;
	unsigned char getAlpha () const;
	void          setRGBA  (unsigned int rgba);
	void          setRGBA  (float values[]);
	void          setXYZ   (float values[]);
	void          setAlpha (unsigned char alpha);

	Pixel & operator = (const Pixel & that);  ///< Set self to have color contained in that.
	Pixel & operator += (const Pixel & that);  ///< Set self to sum of respective color channels.
	Pixel operator + (const Pixel & that) const;  ///< Add respective color channels.
	Pixel operator * (const Pixel & that) const;  ///< Multiply respective color channels.
	Pixel operator * (float scalar) const;  ///< Scale each channel.
	Pixel operator / (float scalar) const;  ///< Scale each channel.
	Pixel operator << (const Pixel & that);  ///< Alpha blend that into this.  The alpha value that governs the blending comes from that.

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
	 Helper class for ImageFile that actually implements the methods for a
	 specific codec.
  **/
  class SHARED ImageFileDelegate : public ReferenceCounted, public Metadata
  {
  public:
	virtual ~ImageFileDelegate ();

	virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0) = 0;
	virtual void write (const Image & image, int x = 0, int y = 0) = 0;
  };

  /**
	 Read or write an image stored in a file or stream.
	 While both reading and writing
	 functions appear here, in general only one will work for any given
	 open file and the other will throw exceptions if called.

	 Metadata -- There are get() and set() functions for accessing named
	 values associated with or stored in the file.  Some of these entries
	 may control the behavior of the storage or retrieval process, and others
	 may be descriptive information actually stored in the file.  This
	 interface makes no distinction between the two; the individual image
	 codecs determine the semantics of the entries.  When writing a file,
	 you should set all the metadata first before calling the write() method.
	 If you specify a metadata entry that is not present or not recognized
	 by the codec, it will silently ignore the request.  In addition, the
	 get() methods will leave the value parameter unmodified, allowing you
	 to set a fallback value before making the get() call.

	 Big images -- Some very large rasters are typically broken up into
	 blocks.  The read() and write() functions include optional parameters
	 that allow you specify a portion of the full raster.  The most efficient
	 strategy in this case is to address an integer number of blocks and
	 position the image at a block boundary.  However, this is not required.
	 There are some reserved metadata entries for specifying or querying
	 block structure:
	 <ul>
	 <li>width -- total horizontal pixels.  Same semantics as Image::width.
	 <li>height -- total vertical pixels.  Same semantics as Image::height.
	 <li>blockWidth -- horizontal pixels in one block.  If the image is
	 stored in "strips", then this will be the same as width.
	 <li>blockHeight -- vertical pixels in one tile
	 <li>
	 </ul>
	 These entries will always have the semantics described above, regardless
	 of the image codec.  The image codec may also specify other entries
	 with the same meanings.

	 Coordinates -- Some codecs (namely TIFF) allow a rich set of image
	 origins and axis orientations.  These apply only to the display of
	 images.  As far as the image raster is concerned, these mean nothing.
	 There are simply two axis that start at zero and count up.
	 In memory, the raster is stored in row-major order.  To simplify the
	 description of this interface, assume the most common arrangement:
	 the origin is in the upper-left corner, x increases to the right and
	 y increases downward.
   **/
  class SHARED ImageFile : public Metadata
  {
  public:
	ImageFile ();
	ImageFile (const std::string & fileName, const std::string & mode = "r", const std::string & formatName = "");
	ImageFile (std::istream & stream);
	ImageFile (std::ostream & stream, const std::string & formatName = "pgm");

	/**
	   Open a file for reading or writing.  When writing, the file suffix
	   indicates the format, but the formatName may optionally override this.
	   When reading, the format is determined primarily by the magic string at
	   the start of the file.  The file suffix provides secondary guidance,
	   and the formatName is ignored.
	 **/
	void open (const std::string & fileName, const std::string & mode = "r", const std::string & formatName = "");
	void open (std::istream & stream);
	void open (std::ostream & stream, const std::string & formatName = "pgm");
	void close ();

	/**
	   Fill in image with pixels from the file.  This function by default
	   retrieves the entire raster.  However, if the codec supports big
	   images, is possible to select just a portion of it by using the
	   optional parameters.  If the codec does not support big images,
	   it will silently ignore the optional parameters and retrieve the
	   entire raster.
	   \param x The horizontal start position in the raster.
	   \param y The vertical start position in the raster.
	   \param width The number of horizontal pixels to retrieve.  If 0 (the
	   default) then retrieve all the image that lies to the right of the
	   start position.
	   \param height The number of vertical pixels to retrieve.  If 0 (the
	   default) then retrieve all the image that lies below the start
	   position.
	**/
	void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);

	/**
	   Place the contents of image into this file's raster.  If you are
	   writing a big raster (one that uses multiple blocks), you should
	   specify imageWidth and imageHeight before writing.  The block size
	   will be set to the size of the given image.  If you do not specify
	   imageWidth and imageHeight, and do not specify the position for the
	   given image, then this function will treat the given image as the
	   entire raster, which is the common case.
	   \param x The horizontal start position in the raster.  This should
	   be an integer multiple of image.width.
	   \param y The vertical start position in the raster.  This should be
	   an integer multiple of image.height.
	**/
	void write (const Image & image, int x = 0, int y = 0);

	void get (const std::string & name,       std::string & value);
	void set (const std::string & name, const std::string & value);
	using Metadata::get;
	using Metadata::set;

	PointerPoly<ImageFileDelegate> delegate;
	double timestamp;  ///< When it can be determined from the filesystem, apply it to the image.
  };

  /**
	 \todo Add a mutex around the static variable formats.
  **/
  class SHARED ImageFileFormat
  {
  public:
	virtual ~ImageFileFormat ();

	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const = 0;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const = 0;
	virtual float isIn (std::istream & stream) const = 0;  ///< Determines probability that this format is on the stream.  Always rewinds stream back to where it was when function was called.
	virtual float handles (const std::string & formatName) const = 0;  ///< Determines probability that this object handles the format with the given human readable name.

	static float find (const std::string & fileName, ImageFileFormat *& result);  ///< Determines what format the stream is in.
	static float find (std::istream & stream, ImageFileFormat *& result);  ///< Ditto.  Always returns stream to original position.
	static float findName (const std::string & formatName, ImageFileFormat *& result);  ///< Determines what format to use based on given name.
	static void getMagic (std::istream & stream, std::string & magic);  ///< Attempts to read magic.size () worth of bytes from stream and return them in magic.  Always returns stream to original position.

	static std::vector<ImageFileFormat *> formats;
  };

  class SHARED ImageFileFormatBMP : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatPGM : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatRRIF : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatPNG : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatEPS : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatJPEG : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatTIFF : public ImageFileFormat
  {
  public:
	static void use ();
	ImageFileFormatTIFF ();  ///< To initialize libgeotiff, if it is available.
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatNITF : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };

  class SHARED ImageFileFormatMatlab : public ImageFileFormat
  {
  public:
	static void use ();
	virtual ImageFileDelegate * open (std::istream & stream, bool ownStream = false) const;
	virtual ImageFileDelegate * open (std::ostream & stream, bool ownStream = false) const;
	virtual float isIn (std::istream & stream) const;
	virtual float handles (const std::string & formatName) const;
  };


  // Image inlines ------------------------------------------------------------

  inline bool
  Image::operator == (const Image & that) const
  {
	bool buffersAllocated = buffer.memory  &&  that.buffer.memory;
	bool buffersEqual = buffersAllocated ? *buffer == *that.buffer : buffer.memory == that.buffer.memory;  // The last term of the trinary operator is saying, implicitly, that both buffers must point to 0.
	return    buffersEqual
	       && *format   == *that.format
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
	return Pixel (*format, buffer->pixel (x, y));
  }

  inline unsigned int
  Image::getRGBA (int x, int y) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	return format->getRGBA (buffer->pixel (x, y));
  }

  inline void
  Image::getRGBA (int x, int y, float values[]) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->getRGBA (buffer->pixel (x, y), values);
  }

  inline void
  Image::getXYZ (int x, int y, float values[]) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->getXYZ (buffer->pixel (x, y), values);
  }

  inline unsigned int
  Image::getYUV (int x, int y) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	return format->getYUV (buffer->pixel (x, y));
  }

  inline unsigned char
  Image::getGray (int x, int y) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	return format->getGray (buffer->pixel (x, y));
  }

  inline void
  Image::getGray (int x, int y, float & gray) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->getGray (buffer->pixel (x, y), gray);
  }

  inline unsigned char
  Image::getAlpha (int x, int y) const
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	return format->getAlpha (buffer->pixel (x, y));
  }

  inline void
  Image::setRGBA (int x, int y, unsigned int rgba)
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setRGBA (buffer->pixel (x, y), rgba);
  }

  inline void
  Image::setRGBA (int x, int y, float values[])
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setRGBA (buffer->pixel (x, y), values);
  }

  inline void
  Image::setXYZ (int x, int y, float values[])
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setXYZ (buffer->pixel (x, y), values);
  }

  inline void
  Image::setYUV (int x, int y, unsigned int yuv)
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setYUV (buffer->pixel (x, y), yuv);
  }

  inline void
  Image::setGray (int x, int y, unsigned char gray)
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setGray (buffer->pixel (x, y), gray);
  }

  inline void
  Image::setGray (int x, int y, float gray)
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setGray (buffer->pixel (x, y), gray);
  }

  inline void
  Image::setAlpha (int x, int y, unsigned char alpha)
  {
	assert (x >= 0  &&  x < width  &&  y >= 0  &&  y < height);
	format->setAlpha (buffer->pixel (x, y), alpha);
  }
}


#endif
