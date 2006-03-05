/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


05/2005 Fred Rothganger -- Fix bugs with reading.  Complete support for
        writing.  Use new PixelFormat names.
Revisions Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


02/2006 Fred Rothganger -- Change Image structure.  Separate ImageFile.
*/


#include "fl/image.h"
#include "fl/string.h"
#include "fl/math.h"
#include "fl/lapack.h"

#include <tiffio.h>
#include <tiffio.hxx>
#ifdef HAVE_GEOTIFF
#  include <xtiffio.h>
#  include <geotiff.h>
#endif

#include <strstream>


using namespace std;
using namespace fl;


// class ImageFileDelegateTIFF ------------------------------------------------

class ImageFileDelegateTIFF : public ImageFileDelegate
{
public:
  ImageFileDelegateTIFF (TIFF * tif, ios * stream)
  {
	this->tif = tif;
	this->stream = stream;

#   ifdef HAVE_GEOTIFF
	gtif = GTIFNew (tif);
#   endif
  }
  virtual ~ImageFileDelegateTIFF ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name, double & value);
  virtual void get (const string & name, string & value);
  virtual void get (const string & name, Matrix<double> & value);
  virtual void set (const string & name, double value);
  virtual void set (const string & name, const string & value);
  virtual void set (const string & name, const Matrix<double> & value);

  TIFF * tif;
  ios * stream;
# ifdef HAVE_GEOTIFF
  GTIF * gtif;
# endif
};

ImageFileDelegateTIFF::~ImageFileDelegateTIFF ()
{
# ifdef HAVE_GEOTIFF
  if (ostream * bogus = dynamic_cast<ostream *> (stream))
  {
	GTIFWriteKeys (gtif);
  }
  GTIFFree (gtif);
# endif
  TIFFClose (tif);
  if (stream) delete stream;
}

void
ImageFileDelegateTIFF::read (Image & image, int x, int y, int width, int height)
{
  if (! tif) throw "ImageFileDelegateTIFF not open";

  bool ok = true;

  uint32 w, h;
  ok &= TIFFGetField (tif, TIFFTAG_IMAGEWIDTH, &w);
  ok &= TIFFGetField (tif, TIFFTAG_IMAGELENGTH, &h);

  uint16 samplesPerPixel;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel);
  uint16 bitsPerSample;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_BITSPERSAMPLE, &bitsPerSample);
  uint16 sampleFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLEFORMAT, &sampleFormat);
  uint16 photometric;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PHOTOMETRIC, &photometric);
  uint16 planarConfig;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_PLANARCONFIG, &planarConfig);
  uint16 extra;
  uint16 * extraFormat;
  ok &= TIFFGetFieldDefaulted (tif, TIFFTAG_EXTRASAMPLES, &extra, &extraFormat);

  if (! ok)
  {
	throw "Unable to get needed tag values.";
  }
  if (photometric > 2)
  {
	throw "Can't handle color palettes, transparency masks, etc.";
	// But we should handle the YCbCr and L*a*b* color spaces.
  }
  if (planarConfig != PLANARCONFIG_CONTIG)
  {
	throw "Can't yet handle planar formats";
  }
  if (extra > 1)
  {
	throw "No PixelFormats currently support more than one channel beyond three colors.";
  }

  PixelFormat * format = 0;
  switch (bitsPerSample)
  {
	case 8:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayChar;
		  break;
		case 3:
		  format = &RGBChar;
		  break;
		case 4:
		  format = &RGBAChar;
	  }
	  break;
	case 16:
	  switch (samplesPerPixel)
	  {
		case 1:
		  format = &GrayShort;
		  break;
		case 3:
		  format = &RGBShort;
		  break;
		case 4:
		  format = &RGBAShort;
	  }
	  break;
	case 32:
	  if (sampleFormat == SAMPLEFORMAT_IEEEFP)
	  {
		switch (samplesPerPixel)
		{
		  case 1:
			format = &GrayFloat;
			break;
		  case 4:
			format = &RGBAFloat;
		}
	  }
  }
  if (! format)
  {
	throw "No PixelFormat available that matches file contents";
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
  image.format = format;
  image.resize (w, h);
  int stride = image.format->depth * w;

  if (TIFFIsTiled (tif))
  {
	uint32 blockWidth;
	TIFFGetField (tif, TIFFTAG_TILEWIDTH, &blockWidth);
	uint32 blockHeight;
	TIFFGetField (tif, TIFFTAG_TILELENGTH, &blockHeight);

	Image temp (blockWidth, blockHeight, *image.format);
	tsize_t size = blockWidth * blockHeight * image.format->depth;
	tdata_t buf = (tdata_t) ((PixelBufferPacked *) temp.buffer)->memory;

	ttile_t tile = 0;
	for (int y = 0; y < image.height; y += blockHeight)
	{
	  for (int x = 0; x < image.width; x += blockWidth)
	  {
		TIFFReadEncodedTile (tif, tile++, buf, size);
		image.bitblt (temp, x, y, 0, 0, min ((int) blockWidth, image.width - x), min ((int) blockHeight, image.height - y));
	  }
	}
  }
  else  // strip organization
  {
	uint32 rowsPerStrip;
	TIFFGetField (tif, TIFFTAG_ROWSPERSTRIP, &rowsPerStrip);

	int fullStrips = image.height / rowsPerStrip;
	int stripStride = stride * rowsPerStrip;
	unsigned char * p = (unsigned char *) buffer->memory;
	for (int s = 0; s < fullStrips; s++)
	{
	  TIFFReadEncodedStrip (tif, s, p, stripStride);
	  p += stripStride;
	}

	int spareRows = image.height - fullStrips * rowsPerStrip;
	if (spareRows)
	{
	  TIFFReadEncodedStrip (tif, fullStrips, p, spareRows * stride);
	}
  }
}

void
ImageFileDelegateTIFF::write (const Image & image, int x, int y)
{
  if (! tif) throw "ImageFileDelegateTIFF not open";

  if (image.format->monochrome)
  {
	if (*image.format != GrayChar  &&  *image.format != GrayShort  &&  *image.format != GrayFloat  &&  *image.format != GrayDouble)
	{
	  write (image * GrayChar);
	  return;
	}
  }
  else if (image.format->hasAlpha)
  {
	if (*image.format != RGBAChar  &&  *image.format != RGBAShort  &&  *image.format != RGBAFloat)
	{
	  write (image * RGBAChar);
	  return;
	}
  }
  else  // Three color channels
  {
	if (*image.format != RGBChar  &&  *image.format != RGBShort)
	{
	  write (image * RGBChar);
	  return;
	}
  }

  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
  if (! buffer) throw "TIFF only handles packed buffers for now";


  TIFFSetField (tif, TIFFTAG_IMAGEWIDTH, image.width);
  TIFFSetField (tif, TIFFTAG_IMAGELENGTH, image.height);
  TIFFSetField (tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
  TIFFSetField (tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

  if (image.format->monochrome)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);

	if (*image.format == GrayChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == GrayFloat)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
	else if (*image.format == GrayDouble)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 64);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else if (image.format->hasAlpha)
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 4);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == RGBAChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else if (*image.format == RGBAShort)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBAFloat
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 32);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
	}
  }
  else  // 3 color channels
  {
	TIFFSetField (tif, TIFFTAG_SAMPLESPERPIXEL, 3);
	TIFFSetField (tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);

	if (*image.format == RGBChar)
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 8);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
	else  // RGBShort
	{
	  TIFFSetField (tif, TIFFTAG_BITSPERSAMPLE, 16);
	  TIFFSetField (tif, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
	}
  }

  if (TIFFIsTiled (tif))
  {
	uint32 blockWidth;
	TIFFGetField (tif, TIFFTAG_TILEWIDTH, &blockWidth);
	uint32 blockHeight;
	TIFFGetField (tif, TIFFTAG_TILELENGTH, &blockHeight);

	Image temp (blockWidth, blockHeight, *image.format);
	tsize_t size = blockWidth * blockHeight * image.format->depth;
	int tempStride = blockWidth * image.format->depth;

	ttile_t tile = 0;
	for (int y = 0; y < image.height; y += blockHeight)
	{
	  for (int x = 0; x < image.width; x += blockWidth)
	  {
		int w = min ((int) blockWidth, image.width - x);
		int h = min ((int) blockHeight, image.height - y);
		temp.bitblt (image, 0, 0, x, y, w, h);
		unsigned char * buf = (unsigned char *) ((PixelBufferPacked *) temp.buffer)->memory;

		// Fill out partial block in a way that is friendly to compression.
		//   Fill bottom portion with black.
		int n = blockHeight - h;
		if (n)
		{
		  memset (buf + (h * tempStride), 0, n * tempStride);
		}
		//   Fill right side with copies of last pixel in row.
		int m = blockWidth - w;
		if (m)
		{
		  unsigned char * a = buf + ((w - 1) * image.format->depth);
		  unsigned char * b = a + image.format->depth;
		  int count = m * image.format->depth;
		  for (int i = 0; i < h; i++)
		  {
			unsigned char * aa = a;
			unsigned char * bb = b;
			unsigned char * end = b + count;
			while (bb < end) *bb++ = *aa++;
			a += tempStride;
			b += tempStride;
		  }
		}

		TIFFWriteEncodedTile (tif, tile++, buf, size);
	  }
	}
  }
  else  // strip organization
  {
	int stride = image.format->depth * buffer->stride;
	int rowsPerStrip = (int) ceil (8192.0 / stride);  // Manual recommends 8K strips
	TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (uint32) rowsPerStrip);

	int fullStrips = image.height / rowsPerStrip;
	int stripStride = stride * rowsPerStrip;
	unsigned char * p = (unsigned char *) buffer->memory;
	for (int s = 0; s < fullStrips; s++)
	{
	  TIFFWriteEncodedStrip (tif, s, p, stripStride);
	  p += stripStride;
	}

	int spareRows = image.height - fullStrips * rowsPerStrip;
	if (spareRows)
	{
	  TIFFWriteEncodedStrip (tif, fullStrips, p, spareRows * stride);
	}
  }
}

void
ImageFileDelegateTIFF::get (const string & name, double & value)
{
  Matrix<double> v;
  get (name, v);
  if (v.rows () > 0  &&  v.columns () > 0)
  {
	value = v(0,0);
  }
}

void
ImageFileDelegateTIFF::get (const string & name, string & value)
{
  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	if (fi->field_type == TIFF_ASCII)
	{
	  char * v;
	  if (TIFFGetFieldDefaulted (tif, fi->field_tag, &v)) value = v;
	}
	else
	{
	  Matrix<double> v;
	  get (name, v);
	  if (v.rows () > 0  &&  v.columns () > 0)
	  {
		ostrstream sv;
		sv << v;
		value = sv.str ();
	  }
	}
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	char buffer[100];
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		uint16 v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  sprintf (buffer, "%hu", v);
		  value = buffer;
		}
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  sprintf (buffer, "%lf", v);
		  value = buffer;
		}
		return;
	  }
	  case TYPE_ASCII:
	  {
		value.resize (length);
		GTIFKeyGet (gtif, (geokey_t) key, (void *) value.c_str (), 0, length);
		return;
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::get (const string & name, Matrix<double> & value)
{
  if (name == "GeoTransformationMatrix")
  {
	uint16 count;
	double * v;
	bool found = TIFFGetField (tif, 34264, &count, &v);  // GeoTransformationMatrix
	if (! found)
	{
	  found = TIFFGetField (tif, 33920, &count, &v)  &&  count == 16;  // Intergraph TransformationMatrix
	}
	if (found)
	{
	  value = ~Matrix<double> (v, 4, 4);
	  return;
	}

	if (TIFFGetField (tif, 33922, &count, &v)) // ModelTiepoint
	{
	  int points = count / 6;
	  if (points == 1)  // exactly one point indicates direct computation of transformation
	  {
		double * o;
		if (TIFFGetField (tif, 33550, &count, &o)) // ModelPixelScale
		{
		  value.resize (4, 4);
		  value.clear ();
		  value(0,0) = o[0];
		  value(1,1) = -o[1];
		  value(2,2) = o[2];
		  value(0,3) = v[3] - v[0] * o[0];
		  value(1,3) = v[4] + v[1] * o[1];
		  value(2,3) = v[5] - v[2] * o[2];
		  value(3,3) = 1;
		}
	  }
	  else if (points >= 3)  // 3 or more tiepoints, so solve for transformation using least squares
	  {
		// First determine level of model to compute
		bool allzero = true;
		double * p = &v[2];
		for (int i = 0; i < 2 * points; i++)
		{
		  allzero &= *p == 0;
		  p += 3;
		}

		if (allzero)  // solve just 6 parameters
		{
		  // Find center of each point cloud
		  Vector<double> centerIJ (2);
		  Vector<double> centerXY (2);
		  centerIJ.clear ();
		  centerXY.clear ();
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			centerIJ[0] += *p++;
			centerIJ[1] += *p++;
			p++;
			centerXY[0] += *p++;
			centerXY[1] += *p++;
			p++;
		  }
		  centerIJ /= points;
		  centerXY /= points;

		  // Build least squares problem for 2x2 xform
		  Matrix<double> AT (points, 2);
		  Matrix<double> BT (points, 2);
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			AT(i,0) = *p++ - centerIJ[0];
			AT(i,1) = *p++ - centerIJ[1];
			p++;
			BT(i,0) = *p++ - centerXY[0];
			BT(i,1) = *p++ - centerXY[1];
			p++;
		  }
		  Matrix<double> X;
		  gelss (AT, X, BT, (double *) 0, true, true);

		  // Assemble result
		  value.resize (4, 4);
		  value.clear ();
		  value.region (0, 0) = ~X;
		  value.region (0, 3) = centerXY - value.region (0, 0, 1, 1) * centerIJ;
		  value(3,3) = 1;
		}
		else if (points >= 4)  // solve all 12 parameters
		{
		  // Find center of each point cloud
		  Vector<double> centerIJK (3);
		  Vector<double> centerXYZ (3);
		  centerIJK.clear ();
		  centerXYZ.clear ();
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			centerIJK[0] += *p++;
			centerIJK[1] += *p++;
			centerIJK[2] += *p++;
			centerXYZ[0] += *p++;
			centerXYZ[1] += *p++;
			centerXYZ[2] += *p++;
		  }
		  centerIJK /= points;
		  centerXYZ /= points;

		  // Build least squares problem for 3x3 xform
		  Matrix<double> AT (points, 3);
		  Matrix<double> BT (points, 3);
		  p = v;
		  for (int i = 0; i < points; i++)
		  {
			AT(i,0) = *p++ - centerIJK[0];
			AT(i,1) = *p++ - centerIJK[1];
			AT(i,2) = *p++ - centerIJK[2];
			BT(i,0) = *p++ - centerXYZ[0];
			BT(i,1) = *p++ - centerXYZ[1];
			BT(i,2) = *p++ - centerXYZ[2];
		  }
		  Matrix<double> X;
		  gelss (AT, X, BT, (double *) 0, true, true);

		  // Assemble result
		  value.resize (4, 4);
		  value.clear ();
		  value.region (0, 0) = ~X;
		  value.region (0, 3) = centerXYZ - value.region (0, 0, 2, 2) * centerIJK;
		  value(3,3) = 1;
		}
	  }
	}

	return;
  }

  if (name == "width")
  {
	get ("ImageWidth", value);
	return;
  }
  if (name == "height")
  {
	get ("ImageLength", value);
	return;
  }
  if (name == "blockWidth")
  {
	if (TIFFIsTiled (tif)) get ("TileWidth",  value);
	else                   get ("ImageWidth", value);
	return;
  }
  if (name == "blockHeight")
  {
	if (TIFFIsTiled (tif)) get ("TileLength",   value);
	else                   get ("RowsPerStrip", value);
	return;
  }

  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	int count = fi->field_readcount;

	if (count == 1)
	{
#     define grabSingle(type) \
	  { \
		type v; \
		if (TIFFGetFieldDefaulted (tif, fi->field_tag, &v)) \
		{ \
		  value.resize (1, 1); \
		  value(0,0) = v; \
		} \
		return; \
	  }

	  switch (fi->field_type)
	  {
		case TIFF_BYTE:
		  grabSingle (uint8);
		case TIFF_SBYTE:
		  grabSingle (int8);
		case TIFF_SHORT:
		  grabSingle (uint16);
		case TIFF_SSHORT:
		  grabSingle (int16);
		case TIFF_LONG:
		case TIFF_IFD:
		  grabSingle (uint32);
		case TIFF_SLONG:
		  grabSingle (int32);
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		case TIFF_FLOAT:
		  grabSingle (float);
		case TIFF_DOUBLE:
		  grabSingle (double);
	  }
	  return;
	}

	int rows = 1;
	if (fi->field_readcount == TIFF_SPP)
	{
	  uint16 spp;
	  TIFFGetFieldDefaulted (tif, TIFFTAG_SAMPLESPERPIXEL, &spp);
	  rows = spp;
	}
	if (name == "GeoTiePoints") rows = 6;

	void * data;
	bool found = false;
	if (count > 1)
	{
	  found = TIFFGetFieldDefaulted (tif, fi->field_tag, &data);
	}
	else if (fi->field_passcount)
	{
	  if (fi->field_readcount == TIFF_VARIABLE2)
	  {
		uint32 c;
		found = TIFFGetFieldDefaulted (tif, fi->field_tag, &c, &data);
		count = c;
	  }
	  else
	  {
		uint16 c;
		found = TIFFGetFieldDefaulted (tif, fi->field_tag, &c, &data);
		count = c;
	  }
	}
	if (! found) return;

	if (fi->field_type != TIFF_DOUBLE)
	{
	  if (rows == 1) value.resize (count, 1);
	  else           value.resize (rows, count / rows);
	}

#   define readVector(type) \
	for (int i = 0; i < count; i++) \
	{ \
	  value[i] = ((type *) data)[i]; \
	} \
	return;

	switch (fi->field_type)
	{
	  case TIFF_BYTE:
		readVector (uint8);
	  case TIFF_SBYTE:
		readVector (int8);
	  case TIFF_SHORT:
		readVector (uint16);
	  case TIFF_SSHORT:
		readVector (int16);
	  case TIFF_LONG:
	  case TIFF_IFD:
		readVector (uint32);
	  case TIFF_SLONG:
		readVector (int32);
	  case TIFF_RATIONAL:
	  case TIFF_SRATIONAL:
	  case TIFF_FLOAT:
		readVector (float);
	  case TIFF_DOUBLE:
		if (rows == 1) value = Matrix<double> ((double *) data, count, 1);
		else           value = Matrix<double> ((double *) data, rows, count / rows);
		return;
	}
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		uint16 v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, (geokey_t) key, &v, 0, 1))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TYPE_ASCII:
	  {
		string v;
		v.resize (length);
		if (GTIFKeyGet (gtif, (geokey_t) key, (void *) v.c_str (), 0, length))
		{
		  value.resize (1, 1);
		  value(0,0) = atof (v.c_str ());
		}
		return;
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::set (const string & name, double value)
{
  Matrix<double> v (1, 1);
  v(0,0) = value;
  set (name, v);
}

void
ImageFileDelegateTIFF::set (const string & name, const string & value)
{
  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	if (fi->field_type == TIFF_ASCII)
	{
	  TIFFSetField (tif, fi->field_tag, (char *) value.c_str ());
	}
	else
	{
	  set (name, atof (value.c_str ()));
	}
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, (uint16) atoi (value.c_str ()));
		break;
	  }
	  case TYPE_DOUBLE:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, atof (value.c_str ()));
		break;
	  }
	  case TYPE_ASCII:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_ASCII, value.size (), value.c_str ());
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::set (const string & name, const Matrix<double> & value)
{
  if (name == "GeoTransformationMatrix")
  {
	Matrix<double> v = ~value;
	TIFFSetField (tif, 34264, (uint16) 16, &v(0,0));
	return;
  }

  if (name == "width")
  {
	set ("ImageWidth", value);
	return;
  }
  if (name == "height")
  {
	set ("ImageLength", value);
	return;
  }
  if (name == "blockWidth")
  {
	set ("TileWidth", value);
	return;
  }
  if (name == "blockHeight")
  {
	set ("TileLength", value);
	return;
  }

  const TIFFFieldInfo * fi = TIFFFindFieldInfoByName (tif, name.c_str (), TIFF_ANY);
  if (fi)
  {
	int count = value.rows () * value.columns ();
	if (! count) throw "Emptry matrix";

	void * data = 0;
	ostrstream adata;

#   define writeVector(type) \
	data = malloc (count * sizeof (type)); \
	for (int i = 0; i < count; i++) \
	{ \
	  ((type *) data)[i] = (type) rint (value[i]); \
	} \
	break;

	switch (fi->field_type)
	{
	  case TIFF_BYTE:
		writeVector (uint8);
	  case TIFF_SBYTE:
		writeVector (int8);
	  case TIFF_SHORT:
		writeVector (uint16);
	  case TIFF_SSHORT:
		writeVector (int16);
	  case TIFF_LONG:
	  case TIFF_IFD:
		writeVector (uint32);
	  case TIFF_SLONG:
		writeVector (int32);
	  case TIFF_RATIONAL:
	  case TIFF_SRATIONAL:
	  case TIFF_FLOAT:
		data = malloc (count * sizeof (float));
		for (int i = 0; i < count; i++)
		{
		  ((float *) data)[i] = value[i];
		}
		break;
	  case TIFF_DOUBLE:
		data = &value(0,0);
		break;
	  case TIFF_ASCII:
		adata << value;
		data = adata.str ();
	}

	if (fi->field_passcount)
	{
	  if (fi->field_writecount == TIFF_VARIABLE2)
	  {
		TIFFSetField (tif, fi->field_tag, (uint32) count, data);
	  }
	  else
	  {
		TIFFSetField (tif, fi->field_tag, (uint16) count, data);
	  }
	}
	else
	{
	  if (fi->field_writecount == 1)
	  {
		switch (fi->field_type)
		{
		  case TIFF_BYTE:
			TIFFSetField (tif, fi->field_tag, *(uint8 *) data);
			break;
		  case TIFF_SBYTE:
			TIFFSetField (tif, fi->field_tag, *(int8 *) data);
			break;
		  case TIFF_SHORT:
			TIFFSetField (tif, fi->field_tag, *(uint16 *) data);
			break;
		  case TIFF_SSHORT:
			TIFFSetField (tif, fi->field_tag, *(int16 *) data);
			break;
		  case TIFF_LONG:
		  case TIFF_IFD:
			TIFFSetField (tif, fi->field_tag, *(uint32 *) data);
			break;
		  case TIFF_SLONG:
			TIFFSetField (tif, fi->field_tag, *(int32 *) data);
			break;
		  case TIFF_RATIONAL:
		  case TIFF_SRATIONAL:
		  case TIFF_FLOAT:
			TIFFSetField (tif, fi->field_tag, *(float *) data);
			break;
		  case TIFF_DOUBLE:
			TIFFSetField (tif, fi->field_tag, *(double *) data);
		}
	  }
	  else
	  {
		TIFFSetField (tif, fi->field_tag, data);
	  }
	}

	if (data  &&  fi->field_type != TIFF_DOUBLE  &&  fi->field_type != TIFF_ASCII) free (data);
	return;
  }

# ifdef HAVE_GEOTIFF
  int key = GTIFKeyCode ((char *) name.c_str ());
  if (key < 0) key = GTIFKeyCode ((char *) (name + "GeoKey").c_str ());
  if (key >= 0)
  {
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, (geokey_t) key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_SHORT, 1, (uint16) rint (value(0,0)));
		break;
	  }
	  case TYPE_DOUBLE:
	  {
		GTIFKeySet (gtif, (geokey_t) key, TYPE_DOUBLE, 1, value(0,0));
		break;
	  }
	  case TYPE_ASCII:
	  {
		ostrstream data;
		data << value;
		GTIFKeySet (gtif, (geokey_t) key, TYPE_ASCII, data.pcount (), data.str ());
	  }
	}
  }
# endif
}


// class ImageFileFormatTIFF --------------------------------------------------

ImageFileFormatTIFF::ImageFileFormatTIFF ()
{
# ifdef HAVE_GEOTIFF
  _XTIFFInitialize ();
# endif
}

ImageFileDelegate *
ImageFileFormatTIFF::open (istream & stream, bool ownStream) const
{
  TIFF * tif = TIFFStreamOpen ("", &stream);
  if (! tif)
  {
	throw "Unable to open file.";
  }
  return new ImageFileDelegateTIFF (tif, ownStream ? &stream : 0);
}

ImageFileDelegate *
ImageFileFormatTIFF::open (ostream & stream, bool ownStream) const
{
  TIFF * tif = TIFFStreamOpen ("", &stream);
  if (! tif)
  {
	throw "Unable to open file.";
  }
  return new ImageFileDelegateTIFF (tif, ownStream ? &stream : 0);
}

float
ImageFileFormatTIFF::isIn (std::istream & stream) const
{
  string magic = "    ";  // 4 spaces
  getMagic (stream, magic);

  // Apparently, some implementation don't store the magic number 42 according
  // to the endian indicated by the first two bytes, so we have to handle the
  // wrong order as well.
  if (magic.substr (0, 2) == "II")  // little endian
  {
	if (magic[2] == '\x2A'  &&  magic[3] == '\x00') return 1;
	if (magic[2] == '\x00'  &&  magic[3] == '\x2A') return 0.8;
  }
  if (magic.substr (0, 2) == "MM")  // big endian
  {
	if (magic[2] == '\x2A'  &&  magic[3] == '\x00') return 0.8;
	if (magic[2] == '\x00'  &&  magic[3] == '\x2A') return 1;
  }

  return 0;
}

float
ImageFileFormatTIFF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "tiff") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "tif") == 0)
  {
	return 0.8;
  }
  return 0;
}
