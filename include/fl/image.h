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
	Image ();  // Creates a new image of GrayChar, but with no buffer memory allocated.
	Image (const PixelFormat & format);  // Same as above, but with given PixelFormat
	Image (int width, int height);  // Allocates buffer of size width x height x GrayChar.depth bytes.
	Image (int width, int height, const PixelFormat & format);  // Same as above, but with given PixelFormat
	Image (const Image & that);  // Points our buffer to same location as "that" and copies all of its metadata.
	Image (unsigned char * buffer, int width, int height, const PixelFormat & format);  // Binds to an external block of memory.
	Image (const std::string & fileName);  // Create image initialized with contents of file.

	void read (const std::string & fileName);  // Read image from fileName.  Format will be determined automatically.
	void read (std::istream & stream);
	void write (const std::string & fileName, const std::string & formatName = "pgm") const;  // Write image to fileName.
	void write (std::ostream & stream, const std::string & formatName = "pgm") const;

	Image & operator <<= (const Image & that);  // Direct assignment by shallow copy.  Same semantics as "=".  By using a different operator than "=", we allow subclasses to inherit this function.
	void copyFrom (const Image & that);  // Duplicates another Image.  Copy all raster info into private buffer, and copy all other metadata.
	void copyFrom (unsigned char * buffer, int width, int height, const PixelFormat & format);  // Copy from a non-Image source.  Determine size of buffer in bytes as width x height x depth.
	void attach (unsigned char * buffer, int width, int height, const PixelFormat & format);  // Binds to an external block of memory.
	void detach ();  // Set the state of this image as if it has no buffer.  Releases (but only frees if appropriate) any memory.

	void resize (int width, int height);  // Changes image to new size.  Any pixels that are still visible are aligned correctly.  Any newly exposed pixels are set to black.
	void bitblt (const Image & that, int toX = 0, int toY = 0, int fromX = 0, int fromY = 0, int width = -1, int height = -1);  // -1 for width or height means "maximum possible value"
	void clear ();  // Initialize buffer, if it exists, to zero.  This generally results in black pixels.

	Image operator + (const Image & that);  // Creates an image that sums pixel values from this and that.
	Image operator - (const Image & that);  // Creates an image whose pixels are the difference between this and that.
	Image & operator *= (double factor);  // Scales each pixel by factor.  Ie: factor == 1 -> no change; factor == 2 -> bright spots are twice as bright, and dark spots (negative values relative to bias) are twice as dark.
	Image & operator += (double value);  // Adds value to each pixel

	Pixel        operator () (int x, int y) const;  // Returns a Pixel object that wraps (x,y).
	unsigned int getRGBA (int x, int y) const;  // The "RGB" functions are intended for abstract access to the buffer.  They perform conversion to whatever the buffer's format is.
	void         setRGBA (int x, int y, unsigned int rgba);  // The "rgb" format is always 24-bit RGB, 8 bits per field.  Blue is in the least significant byte, then green, then red.

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
  // sort of the combination of a pointer to memory and a PixelFormat.
  // A PixelFormat describes the entire collection of pixels in an image, and
  // we use it to interpret each pixel in the image.
  // PixelFormat extends Filter so it can be used directly to convert image
  // format.  The "from*" methods implement an n^2 set of direct conversions
  // between selected formats.  These conversions occur frequently when
  // displaying images on an X windows system.
  // For the present, all formats exept for XYZ make sRGB assumptions (see
  // www.srgb.com).  In addition, all integer values are non-linear (with
  // gamma = 2.2 as per sRGB spec), and all floating point values are linear.
  // We can add parameters to the formats if we need to distinguish more color
  // spaces.
  // TODO:
  // * think about adding arbitrary information channels besides alpha
  //   (eg: depth).  alpha gets special treatment because it has a specific,
  //   well-defined effect on how pixels are combined.
  // * add alpha blend methods (operator << and >>) to Pixel
  // * Think about naming conventions for formats.  Should letters such as
  //   R, G, B and A follow the layout of the machine word?  Of actual memory?
  //   Does it matter?  (Yes, it matters a little.  If structure is clearly
  //   indicated in the format title, it is easier to think about how the
  //   format interacts with the needs of other software packages.)
  class PixelFormat : public Filter
  {
  public:
	virtual Image filter (const Image & image);  // Return an Image in this format
	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBAChar (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;
	virtual void fromAny (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;  // Checks if this and that describe the same actual format.
	bool operator != (const PixelFormat & that) const
	{
	  return ! operator == (that);
	}

	virtual unsigned int  getRGBA (void * pixel) const = 0;  // Return value is always assumed to be non-linear sRGB.  Same for other RGB methods below.
	virtual void          getRGBA (void * pixel, float values[]) const;  // "values" must have at least three elements.  Each returned value is in [0,1].
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float * value) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const = 0;
	virtual void          setRGBA (void * pixel, float values[]) const;  // Each value must be in [0,1].  Values outside this range will be clamped and modified directly in the array.
	virtual void          setXYZ  (void * pixel, float values[]) const;

	int depth;  // Number of bytes in one pixel, including any padding
	int precedence;  // Imposes a (partial?) order on formats according to information content.  Bigger numbers have more information.
	// The following two flags could be implemented several different ways.
	// One alternative would be to make a more complicated class hierarchy
	// that implies the information.  Eg: have intermediate classes
	// PixelFormatMonochrome and PixelFormatColor.  Another alternative is
	// to put channel information into a bitmap and use masks to determine
	// various states.
	bool monochrome;  // Indicates that this format has no color components.
	bool hasAlpha;  // Indicates that this format has a real alpha channel (as apposed to a default alpha value).
  };

  // The naming convention for specific PixelFormats is
  // PixelFormat<Color Model><Type of one color channel>

  class PixelFormatGrayChar : public PixelFormat
  {
  public:
	PixelFormatGrayChar ();

	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBAChar (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;
	virtual void fromAny (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float * value) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
  };

  class PixelFormatGrayFloat : public PixelFormat
  {
  public:
	PixelFormatGrayFloat ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBAChar (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;
	virtual void fromAny (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float * value) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
  };

  class PixelFormatGrayDouble : public PixelFormat
  {
  public:
	PixelFormatGrayDouble ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromRGBAChar (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;
	virtual void fromAny (const Image & image, Image & result) const;

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual void          getRGBA (void * pixel, float values[]) const;
	virtual void          getXYZ  (void * pixel, float values[]) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          getGray (void * pixel, float * value) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
	virtual void          setRGBA (void * pixel, float values[]) const;
	virtual void          setXYZ  (void * pixel, float values[]) const;
  };

  class PixelFormatRGBAChar : public PixelFormat
  {
  public:
	PixelFormatRGBAChar ();

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int getRGBA (void * pixel) const;
	virtual void         setRGBA (void * pixel, unsigned int rgba) const;

	static void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift);  // Shifts are set to move bits from this format to the one indicated by the masks.
  };

  class PixelFormatRGBABits : public PixelFormat
  {
  public:
	PixelFormatRGBABits (int depth, unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask);

	virtual void fromGrayChar (const Image & image, Image & result) const;
	virtual void fromGrayFloat (const Image & image, Image & result) const;
	virtual void fromGrayDouble (const Image & image, Image & result) const;
	virtual void fromRGBAChar (const Image & image, Image & result) const;
	virtual void fromRGBABits (const Image & image, Image & result) const;

	virtual bool operator == (const PixelFormat & that) const;

	virtual unsigned int getRGBA (void * pixel) const;
	virtual void         setRGBA (void * pixel, unsigned int rgba) const;

	void shift (unsigned int redMask, unsigned int greenMask, unsigned int blueMask, unsigned int alphaMask, int & redShift, int & greenShift, int & blueShift, int & alphaShift) const;

	unsigned int redMask;
	unsigned int greenMask;
	unsigned int blueMask;
	unsigned int alphaMask;
  };

  class PixelFormatRGBAFloat : public PixelFormat
  {
  public:
	PixelFormatRGBAFloat ();

	virtual unsigned int getRGBA (void * pixel) const;
	virtual void         getRGBA (void * pixel, float values[]) const;
	virtual void         setRGBA (void * pixel, unsigned int rgba) const;
	virtual void         setRGBA (void * pixel, float values[]) const;
  };

  // Assumes that pixel pairs are 32-bit word aligned.  So, if the pixel
  // address falls in the center of a 32-bit word it must refer to the "YV"
  // portion of the pair.  Likewise, an address that falls on a 32-bit boundary
  // refers to the "YU" portion.
  class PixelFormatYVYUChar : public PixelFormat
  {
  public:
	PixelFormatYVYUChar ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
  };

  // Same as YVYU, but with different ordering within the word
  class PixelFormatVYUYChar : public PixelFormat
  {
  public:
	PixelFormatVYUYChar ();

	virtual unsigned int  getRGBA (void * pixel) const;
	virtual unsigned char getGray (void * pixel) const;
	virtual void          setRGBA (void * pixel, unsigned int rgba) const;
  };

  extern PixelFormatGrayChar   GrayChar;
  extern PixelFormatGrayFloat  GrayFloat;
  extern PixelFormatGrayDouble GrayDouble;
  extern PixelFormatRGBAChar   RGBAChar;
  extern PixelFormatRGBABits   BGRChar;  // Compact 3 byte format with red in LSB.  For talking to libjpeg and GL.
  extern PixelFormatRGBABits   ABGRChar;  // Similar to BGRChar, but assumes an alpha channel comes first.  For talking to libtiff.
  extern PixelFormatRGBAFloat  RGBAFloat;
  extern PixelFormatYVYUChar   YVYUChar;
  extern PixelFormatVYUYChar   VYUYChar;

  // Naming convention for RGBBits (other than BGRChar):
  // R<red bits>G<green bits>B<blue bits>
  // EG: R5G5B5 would be a 15 bit RGB format.

  // Another comment on naming: the order of the letters in names like "RGB"
  // refer to an imaginary machine word that is laid out in front of our
  // mind's eye with the most significant bits to the left.  This is how
  // most people tend to think of machine words.  On a little-endian machine,
  // which is where this software was developed, RGB will translate into "B"
  // being in the least signficant byte and "R" in the most significant byte.
  // BGRChar exists as an endian adjustment.  More work needs to be done to
  // make this software endian neutral.

  // Rant: The only reason different "endians" exist is that different
  // engineers conceived of different ways of converting this imaginary
  // machine word into physical storage.  "Little-endians" imagined that
  // storage starts on the left, while "big-endians" imagined that storage
  // starts on the right.  Of course, the organization of storage has nothing
  // to do with handedness.  We could just as well imagined that storage
  // starts at the bottom or top.  The most accurate conception of storage is
  // a collection of random points in space that are directly addressable by
  // a number, sort of like how telephone numbers address people.  There is
  // some order, but it is usually not useful to think about that order
  // (except that it is forced on us by the "endians").


  // Pixel --------------------------------------------------------------------

  // Provides convenient access to the functions of PixelFormat for a specific
  // datum.
  // All linear operations between pixels take place in RGB space.  It would
  // be better colorwise to do them in XYZ space, but most formats are closer
  // to RGB than XYZ numerically (ie: require less conversion), so it is
  // cheaper to do them in RGB.
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

	Pixel & operator = (const Pixel & that);  // Set self to have color contained in that.
	Pixel & operator += (const Pixel & that);  // Set self to sum of respective color channels.
	Pixel operator + (const Pixel & that) const;  // Add respective color channels.
	Pixel operator * (const Pixel & that) const;  // Multiply respective color channels.
	Pixel operator * (float scalar) const;  // Scale each channel.
	Pixel operator / (float scalar) const;  // Scale each channel.
	Pixel operator << (const Pixel & that);  // Alpha blend that into this.  The alpha value the governs the blending comes from that.

	const PixelFormat * format;
	void * pixel;  // always points to target data: either the union below or some point in an Image buffer

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

	virtual void read (const std::string & fileName, Image & image) const;
	virtual void read (std::istream & stream, Image & image) const = 0;
	virtual void write (const std::string & fileName, const Image & image) const;
	virtual void write (std::ostream & stream, const Image & image) const = 0;
	virtual bool isIn (std::istream & stream) const = 0;  // Determines if this format is on the stream.  Always rewinds stream back to where it was when function was called.
	virtual bool handles (const std::string & formatName) const = 0;  // Determines if this object handles the format with the given human readable name.

	static ImageFileFormat * find (const std::string & fileName);  // Determines what format the stream is in.
	static ImageFileFormat * find (std::istream & stream);  // Ditto.  Always returns stream to original position.
	static ImageFileFormat * findName (const std::string & formatName);  // Determines what format to use based on given name.
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

  inline Pixel
  Image::operator () (int x, int y) const
  {
	return Pixel (*format, & ((char *) buffer)[(y * width + x) * format->depth]);
  }
}


#endif
