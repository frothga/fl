#ifndef fl_image_h
#define fl_image_h


#include <fl/pointer.h>

#include <iostream>
#include <string>
#include <vector>


namespace fl
{
  // Forward declaration for use by Image
  class PixelFormat;
  class ImageFileFormat;
  

  // Image --------------------------------------------------------------------

  class Image
  {
  public:
	Image ();  // Creates a new image of GrayChar, but with no buffer memory allocated.
	Image (const PixelFormat & format);  // Same as above, but with given PixelFormat
	Image (int width, int height);  // Allocates buffer of size width x height x GrayChar.depth bytes.
	Image (int width, int height, const PixelFormat & format);  // Same as above, but with given PixelFormat
	Image (const Image & that);  // Points our buffer to same location as "that" and copies all of its metadata.

	void read (const std::string & fileName);  // Read image from fileName.  Format will be determined automatically.
	void read (std::istream & stream);
	void write (const std::string & fileName, const std::string & formatName = "pgm") const;  // Write image to fileName.
	void write (std::ostream & stream, const std::string & formatName = "pgm") const;

	Image & operator <<= (const Image & that);  // Direct assignment by shallow copy.  Same semantics as "=".  By using a different operator than "=", we allow subclasses to inherit this function.
	void copyFrom (const Image & that);  // Duplicates another Image.  Copy all raster info into private buffer, and copy all other metadata.
	void copyFrom (unsigned char * buffer, int width, int height, const PixelFormat & format);  // Copy from a non-Image source.  Determine size of buffer in bytes as width x height x depth.

	void resize (int width, int height);  // Changes image to new size.  Any pixels that are still visible are aligned correctly.  Any newly exposed pixels are set to black.
	void bitblt (const Image & that, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = -1, int height = -1);  // -1 for width or height means "maximum possible value"
	void clear ();  // Initialize buffer, if it exists, to zero.  This generally results in black pixels.

	Image operator + (const Image & that);  // Creates an image that sums pixel values from this and that.
	Image operator - (const Image & that);  // Creates an image whose pixels are the difference between this and that.
	Image & operator *= (double factor);  // Scales each pixel by factor.  Ie: factor == 1 -> no change; factor == 2 -> bright spots are twice as bright, and dark spots (negative values relative to bias) are twice as dark.
	Image & operator += (double value);  // Adds value to each pixel

	void *       operator () (int x, int y) const;  // Returns a pointer to the beginning of the pixel at (x,y).
	unsigned int getRGB (int x, int y) const;  // The "RGB" functions are intended for abstract access to the buffer.  They perform conversion to whatever the buffer's format is.
	void         setRGB (int x, int y, unsigned int rgb);  // The "rgb" format is always 24-bit RGB, 8 bits per field.  Blue is in the least significant byte, then green, then red.  This may eventually include an alpha channel.

	// Data
	Pointer             buffer;
	const PixelFormat * format;
	int                 width;
	int                 height;
	float               timestamp;  // Time when image was captured.
  };

  // A simple wrap around Image that makes it easier to access pixels directly.
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

  // Base class for reified functions that take as input an image and output
  // another image.
  class Filter
  {
  public:
	virtual Image filter (const Image & image) = 0;  // This could be const, but it is useful to allow filters to collect statistics.  Note that such filters are not thread safe.
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

  // A PixelFormat wraps access to an element of an Image.  A pixel itself is
  // sort of the combination of a pointer to memory and a PixelFormat.  We
  // don't define a Pixel class or pass Pixel objects, although that is a
  // possible approach.  Instead, we define an object that describes the entire
  // collection of pixels in an image and then use that object to interpret
  // each pixel in an image.
  // PixelFormat extends Filter so it can be used directly to convert image
  // format.
  class PixelFormat : public Filter
  {
  public:
	virtual Image filter (const Image & image);  // Return an Image in this format
	virtual void fromGrayChar (const Image & image, Image & result) const = 0;
	virtual void fromGrayFloat (const Image & image, Image & result) const = 0;
	virtual void fromGrayDouble (const Image & image, Image & result) const = 0;
	virtual void fromRGBChar (const Image & image, Image & result) const = 0;
	virtual void fromRGBBits (const Image & image, Image & result) const = 0;

	virtual bool operator == (const PixelFormat & that) const;  // Checks if this and that describe the same actual format.
	bool operator != (const PixelFormat & that) const
	{
	  return ! operator == (that);
	}

	virtual unsigned int getRGB (void * pixel) const = 0;
	virtual void         setRGB (void * pixel, unsigned int rgb) const = 0;

	int depth;  // Number of bytes in one pixel, including any padding
	int precedence;  // Imposes a (partial?) order on formats according to information content.  Bigger numbers have more information.
  };

  // The naming convention for specific PixelFormats is
  // PixelFormat<Color Model><Type of one color channel>

  class PixelFormatGrayChar : public PixelFormat
  {
  public:
	PixelFormatGrayChar ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBChar (const Image & image, Image & result) const;
	virtual void fromRGBBits (const Image & image, Image & result) const;

	virtual unsigned int getRGB (void * pixel) const;
	virtual void         setRGB (void * pixel, unsigned int rgb) const;
  };

  class PixelFormatGrayFloat : public PixelFormat
  {
  public:
	PixelFormatGrayFloat ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBChar (const Image & image, Image & result) const;
	virtual void fromRGBBits (const Image & image, Image & result) const;

	virtual unsigned int getRGB (void * pixel) const;
	virtual void         setRGB (void * pixel, unsigned int rgb) const;

	static void range (const Image & image, float & scale, float & bias);
  };

  class PixelFormatGrayDouble : public PixelFormat
  {
  public:
	PixelFormatGrayDouble ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBChar (const Image & image, Image & result) const;
	virtual void fromRGBBits (const Image & image, Image & result) const;

	virtual unsigned int getRGB (void * pixel) const;
	virtual void         setRGB (void * pixel, unsigned int rgb) const;

	static void range (const Image & image, double & scale, double & bias);
  };

  class PixelFormatRGBChar : public PixelFormat
  {
  public:
	PixelFormatRGBChar ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBChar (const Image & image, Image & result) const;
	virtual void fromRGBBits (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int getRGB (void * pixel) const;
	virtual void         setRGB (void * pixel, unsigned int rgb) const;

	static void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, int & redShift, int & greenShift, int & blueShift);  // Shifts are set to move bits from this format to the one indicated by the masks.
  };

  class PixelFormatRGBBits : public PixelFormat
  {
  public:
	PixelFormatRGBBits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask);

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBChar (const Image & image, Image & result) const;
	virtual void fromRGBBits (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int getRGB (void * pixel) const;
	virtual void         setRGB (void * pixel, unsigned int rgb) const;

	void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, int & redShift, int & greenShift, int & blueShift) const;

	unsigned int  redMask;
	unsigned int  greenMask;
	unsigned int  blueMask;
  };

  extern PixelFormatGrayChar   GrayChar;
  extern PixelFormatGrayFloat  GrayFloat;
  extern PixelFormatGrayDouble GrayDouble;
  extern PixelFormatRGBChar    RGBChar;
  extern PixelFormatRGBBits    BGRChar;  // Compact 3 byte format with red in LSB.  For talking to libjpeg and GL.
  // Naming convention for RGBBits (other than BGRChar):
  // R<red bits>G<green bits>B<blue bits>
  // EG: R5G5B5 would be a 15 bit RGB format.


  // Pixel --------------------------------------------------------------------

  // This section is experimental.  The idea is that a generic Pixel would
  // contain two things: 1) knowledge of its format, 2) the actual color data.
  // It would have much the same interface as PixelFormat, but w/o the need
  // to pass a pointer to the data.
  // The "knowledge of its format" can either be an explicit pointer to a
  // PixelFormat or the inherent pointer to a vtable.

  class Pixel
  {
  public:
  };

  // A lightweight Pixel for pointing into an Image buffer.
  class PixelPointer : public Pixel
  {
  public:
	PixelFormat * format;
	void * data;
  };

  class PixelRGBChar : public Pixel
  {
  public:
	unsigned char red;
	unsigned char green;
	unsigned char blue;
  };


  // File formats -------------------------------------------------------------

  // Danger!  Not thread-safe.  Need a mutex around modifications and accesses
  // to static variable "formats".  Add if there's ever a need to create
  // and destroy formats on the fly with multiple threads trying to access
  // the list.
  // Translation of the above: it is very unlikely that in practice there
  // will be a problem with the "unsafe" versin.  :)
  class ImageFileFormat
  {
  public:
	ImageFileFormat ();
	virtual ~ImageFileFormat ();

	virtual void read (std::istream & stream, Image & image) const = 0;
	virtual void write (std::ostream & stream, const Image & image) const = 0;
	virtual bool isIn (std::istream & stream) const = 0;  // Determines if this format is on the stream.  Always rewinds stream back to where it was when function was called.
	virtual bool handles (const std::string & formatName) const = 0;  // Determines if this object handles the format with the given human readable name.

	static ImageFileFormat * find (std::istream & stream);  // Determines what format the stream is in.  Always returns stream to original position.
	static ImageFileFormat * find (const std::string & formatName);  // Determines what format to use based on given name.
	static void getMagic (std::istream & stream, std::string & magic);  // Attempts to read magic.size () worth of bytes from stream and return them in magic.  Always returns stream to original position.

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

  inline void
  Image::clear ()
  {
	buffer.clear ();
  }

  inline void *
  Image::operator () (int x, int y) const
  {
	return & ((char *) buffer)[(y * width + x) * format->depth];
  }
}


#endif
