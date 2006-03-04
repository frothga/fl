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

#include <tiffio.h>
#include <tiffio.hxx>
#ifdef HAVE_GEOTIFF
#  include <xtiffio.h>
#  include <geotiff.h>
#endif


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

  TIFF * tif;
  ios * stream;
# ifdef HAVE_GEOTIFF
  GTIF * gtif;
# endif
};

ImageFileDelegateTIFF::~ImageFileDelegateTIFF ()
{
# ifdef HAVE_GEOTIFF
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
	throw "Can't handle planar formats";
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

  unsigned char * p = (unsigned char *) buffer->memory;
  int stride = image.format->depth * w;
  for (int y = 0; y < h; y++)
  {
	TIFFReadScanline (tif, p, y);
	p += stride;
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

  TIFFSetField (tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);

  unsigned char * p = (unsigned char *) buffer->memory;
  int stride = image.format->depth * buffer->stride;
  TIFFSetField (tif, TIFFTAG_ROWSPERSTRIP, (int) ceil (8.0 * 1024 / stride));  // Manual recommends 8K strips
  for (int y = 0; y < image.height; y++)
  {
	TIFFWriteScanline (tif, p, y, 0);
    p += stride;
  }
}

enum tagtype
{
  TTYPE_INT,
  TTYPE_SHORT,
  TTYPE_SINT,
  TTYPE_ASCII,
  TTYPE_FLOAT,
  TTYPE_DOUBLE,
  TTYPE_VECTORINT,
  TTYPE_VECTORFLOAT,
  TTYPE_VECTORDOUBLE
};

struct TIFFmapping
{
  char *  name;
  ttag_t  tag;
  tagtype type;
};

static TIFFmapping TIFFmap[] =
{
  // Standard TIFF tags -------------------------------------------------------
  {"Artist",                    TIFFTAG_ARTIST,                 TTYPE_ASCII},
  {"BadFaxLines",               TIFFTAG_BADFAXLINES,            TTYPE_INT},
  {"BitsPerSample",             TIFFTAG_BITSPERSAMPLE,          TTYPE_SHORT},
  {"CleanFaxData",              TIFFTAG_CLEANFAXDATA,           TTYPE_SHORT},
  //{"ColorMap",                  TIFFTAG_COLORMAP,               TTYPE_},
  {"Compression",               TIFFTAG_COMPRESSION,            TTYPE_SHORT},
  {"ConsecutiveBadFaxLines",    TIFFTAG_CONSECUTIVEBADFAXLINES, TTYPE_INT},
  {"Copyright",                 TIFFTAG_COPYRIGHT,              TTYPE_ASCII},
  {"DataType",                  TIFFTAG_DATATYPE,               TTYPE_SHORT},
  {"DateTime",                  TIFFTAG_DATETIME,               TTYPE_ASCII},
  {"DocumentName",              TIFFTAG_DOCUMENTNAME,           TTYPE_ASCII},
  //{"DotRange",                  TIFFTAG_DOTRANGE,               TTYPE_},
  //{"ExtraSamples",              TIFFTAG_EXTRASAMPLES,           TTYPE_},
  {"FaxMode",                   TIFFTAG_FAXMODE,                TTYPE_SINT},
  {"FillOrder",                 TIFFTAG_FILLORDER,              TTYPE_SHORT},
  {"Group3Options",             TIFFTAG_GROUP3OPTIONS,          TTYPE_INT},
  {"Group4Options",             TIFFTAG_GROUP4OPTIONS,          TTYPE_INT},
  //{"HalftoneHints",             TIFFTAG_HALFTONEHINTS,          TTYPE_),
  {"HostComputer",              TIFFTAG_HOSTCOMPUTER,           TTYPE_ASCII},
  {"ImageDepth",                TIFFTAG_IMAGEDEPTH,             TTYPE_INT},
  {"ImageDescription",          TIFFTAG_IMAGEDESCRIPTION,       TTYPE_ASCII},
  {"ImageLength",               TIFFTAG_IMAGELENGTH,            TTYPE_INT},
  {"ImageWidth",                TIFFTAG_IMAGEWIDTH,             TTYPE_INT},
  {"InkNames",                  TIFFTAG_INKNAMES,               TTYPE_ASCII},
  {"InkSet",                    TIFFTAG_INKSET,                 TTYPE_SHORT},
  //{"JPEGTables",                TIFFTAG_JPEGTABLES,             TTYPE_},
  {"JPEGQuality",               TIFFTAG_JPEGQUALITY,            TTYPE_SINT},
  {"JPEGColorMode",             TIFFTAG_JPEGCOLORMODE,          TTYPE_SINT},
  {"JPEGTablesMode",            TIFFTAG_JPEGTABLESMODE,         TTYPE_SINT},
  {"Make",                      TIFFTAG_MAKE,                   TTYPE_ASCII},
  {"Matteing",                  TIFFTAG_MATTEING,               TTYPE_SHORT},
  {"MaxSampleValue",            TIFFTAG_MAXSAMPLEVALUE,         TTYPE_SHORT},
  {"MinSampleValue",            TIFFTAG_MINSAMPLEVALUE,         TTYPE_SHORT},
  {"Model",                     TIFFTAG_MODEL,                  TTYPE_ASCII},
  {"Orientation",               TIFFTAG_ORIENTATION,            TTYPE_SHORT},
  {"PageName",                  TIFFTAG_PAGENAME,               TTYPE_ASCII},
  {"PageNumber",                TIFFTAG_PAGENUMBER,             TTYPE_SHORT},
  {"Photometric",               TIFFTAG_PHOTOMETRIC,            TTYPE_SHORT},
  {"PlanarConfig",              TIFFTAG_PLANARCONFIG,           TTYPE_SHORT},
  {"Predictor",                 TIFFTAG_PREDICTOR,              TTYPE_SHORT},
  {"PrimaryChromacities",       TIFFTAG_PRIMARYCHROMATICITIES,  TTYPE_VECTORFLOAT},
  {"ReferenceBlackWhite",       TIFFTAG_REFERENCEBLACKWHITE,    TTYPE_VECTORFLOAT},
  {"ResolutionUnit",            TIFFTAG_RESOLUTIONUNIT,         TTYPE_SHORT},
  {"RowsPerStrip",              TIFFTAG_ROWSPERSTRIP,           TTYPE_INT},
  {"SampleFormat",              TIFFTAG_SAMPLEFORMAT,           TTYPE_SHORT},
  {"SamplesPerPixel",           TIFFTAG_SAMPLESPERPIXEL,        TTYPE_SHORT},
  {"SMinSampleValue",           TIFFTAG_SMINSAMPLEVALUE,        TTYPE_DOUBLE},
  {"SMaxSampleValue",           TIFFTAG_SMAXSAMPLEVALUE,        TTYPE_DOUBLE},
  {"Software",                  TIFFTAG_SOFTWARE,               TTYPE_ASCII},
  {"StoNits",                   TIFFTAG_STONITS,                TTYPE_VECTORDOUBLE},
  {"StripByteCounts",           TIFFTAG_STRIPBYTECOUNTS,        TTYPE_VECTORINT},
  {"StripOffsets",              TIFFTAG_STRIPOFFSETS,           TTYPE_VECTORINT},
  {"SubFileType",               TIFFTAG_SUBFILETYPE,            TTYPE_INT},
  //{"SubIFD",                    TIFFTAG_SUBIFD,                 TTYPE_},
  {"TargetPrinter",             TIFFTAG_TARGETPRINTER,          TTYPE_ASCII},
  {"Thresholding",              TIFFTAG_THRESHHOLDING,          TTYPE_SHORT},
  {"TileByteCounts",            TIFFTAG_TILEBYTECOUNTS,         TTYPE_VECTORINT},
  {"TileDepth",                 TIFFTAG_TILEDEPTH,              TTYPE_INT},
  {"TileLength",                TIFFTAG_TILELENGTH,             TTYPE_INT},
  {"TileOffsets",               TIFFTAG_TILEOFFSETS,            TTYPE_VECTORINT},
  {"TileWidth",                 TIFFTAG_TILEWIDTH,              TTYPE_INT},
  //{"TransferFunction",          TIFFTAG_TRANSFERFUNCTION,       TTYPE_},
  {"WhitePoint",                TIFFTAG_WHITEPOINT,             TTYPE_VECTORFLOAT},
  {"XPosition",                 TIFFTAG_XPOSITION,              TTYPE_FLOAT},
  {"XResolution",               TIFFTAG_XRESOLUTION,            TTYPE_FLOAT},
  {"YCbCrCoefficients",         TIFFTAG_YCBCRCOEFFICIENTS,      TTYPE_VECTORFLOAT},
  {"YCbCrPositioning",          TIFFTAG_YCBCRPOSITIONING,       TTYPE_SHORT},
  {"YCbCrSubsampling",          TIFFTAG_YCBCRSUBSAMPLING,       TTYPE_SHORT},
  {"YPosition",                 TIFFTAG_YPOSITION,              TTYPE_FLOAT},
  {"YResolution",               TIFFTAG_YRESOLUTION,            TTYPE_FLOAT},
  //{"ICCProfile",                TIFFTAG_ICCPROFILE,             TTYPE_},

  // GeoTIFF tags -------------------------------------------------------------
  {"ModelTransformation",       34264,                          TTYPE_VECTORDOUBLE},
  {"ModelTiepoint",             33922,                          TTYPE_VECTORDOUBLE},
  {"ModelPixelScale",           33550,                          TTYPE_VECTORDOUBLE},

  {0}
};

static inline TIFFmapping *
findTag (const string & name, TIFFmapping * map)
{
  while (map->name)
  {
	if (name == map->name) return map;
	map++;
  }
  return 0;
}

#ifdef HAVE_GEOTIFF

struct GTIFmapping
{
  char *   name;
  geokey_t key;
};

static GTIFmapping GTIFmap[] =
{
  {"GTModelType",              GTModelTypeGeoKey},
  {"GTRasterType",             GTRasterTypeGeoKey},
  {"GTCitation",               GTCitationGeoKey},
  {"GeographicType",           GeographicTypeGeoKey},
  {"GeogCitation",             GeogCitationGeoKey},
  {"GeogGeodeticDatum",        GeogGeodeticDatumGeoKey},
  {"GeogPrimeMeridian",        GeogPrimeMeridianGeoKey},
  {"GeogLinearUnits",          GeogLinearUnitsGeoKey},
  {"GeogLinearUnitSize",       GeogLinearUnitSizeGeoKey},
  {"GeogAngularUnits",         GeogAngularUnitsGeoKey},
  {"GeogAngularUnitSize",      GeogAngularUnitSizeGeoKey},
  {"GeogEllipsoid",            GeogEllipsoidGeoKey},
  {"GeogSemiMajorAxis",        GeogSemiMajorAxisGeoKey},
  {"GeogSemiMinorAxis",        GeogSemiMinorAxisGeoKey},
  {"GeogInvFlattening",        GeogInvFlatteningGeoKey},
  {"GeogAzimuthUnits",         GeogAzimuthUnitsGeoKey},
  {"GeogPrimeMeridian",        GeogPrimeMeridianLongGeoKey},
  {"ProjectedCSType",          ProjectedCSTypeGeoKey},
  {"PCSCitation",              PCSCitationGeoKey},
  {"Projection",               ProjectionGeoKey},
  {"ProjCoordTrans",           ProjCoordTransGeoKey},
  {"ProjLinearUnits",          ProjLinearUnitsGeoKey},
  {"ProjLinearUnitSize",       ProjLinearUnitSizeGeoKey},
  {"ProjStdParallel1",         ProjStdParallel1GeoKey},
  {"ProjStdParallel",          ProjStdParallelGeoKey},
  {"ProjStdParallel2",         ProjStdParallel2GeoKey},
  {"ProjNatOriginLong",        ProjNatOriginLongGeoKey},
  {"ProjOriginLong",           ProjOriginLongGeoKey},
  {"ProjNatOriginLat",         ProjNatOriginLatGeoKey},
  {"ProjOriginLat",            ProjOriginLatGeoKey},
  {"ProjFalseEasting",         ProjFalseEastingGeoKey},
  {"ProjFalseNorthing",        ProjFalseNorthingGeoKey},
  {"ProjFalseOriginLong",      ProjFalseOriginLongGeoKey},
  {"ProjFalseOriginLat",       ProjFalseOriginLatGeoKey},
  {"ProjFalseOriginEasting",   ProjFalseOriginEastingGeoKey},
  {"ProjFalseOriginNorthing",  ProjFalseOriginNorthingGeoKey},
  {"ProjCenterLong",           ProjCenterLongGeoKey},
  {"ProjCenterLat",            ProjCenterLatGeoKey},
  {"ProjCenterEasting",        ProjCenterEastingGeoKey},
  {"ProjCenterNorthing",       ProjCenterNorthingGeoKey},
  {"ProjScaleAtNatOrigin",     ProjScaleAtNatOriginGeoKey},
  {"ProjScaleAtOrigin",        ProjScaleAtOriginGeoKey},
  {"ProjScaleAtCenter",        ProjScaleAtCenterGeoKey},
  {"ProjAzimuthAngle",         ProjAzimuthAngleGeoKey},
  {"ProjStraightVertPoleLong", ProjStraightVertPoleLongGeoKey},
  {"VerticalCSType",           VerticalCSTypeGeoKey},
  {"VerticalCitation",         VerticalCitationGeoKey},
  {"VerticalDatum",            VerticalDatumGeoKey},
  {"VerticalUnits",            VerticalUnitsGeoKey},
  {0}
};

static inline GTIFmapping *
findTag (const string & name, GTIFmapping * map)
{
  while (map->name)
  {
	if (name == map->name) return map;
	map++;
  }
  return 0;
}

#endif

void
ImageFileDelegateTIFF::get (const string & name, double & value)
{
  TIFFmapping * m = findTag (name, TIFFmap);
  if (m)
  {
	switch (m->type)
	{
	  case TTYPE_INT:
	  {
		unsigned int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	  case TTYPE_SHORT:
	  {
		unsigned short v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	  case TTYPE_SINT:
	  {
		int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	  case TTYPE_ASCII:
	  {
		char * v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = atof (v);
		return;
	  }
	  case TTYPE_FLOAT:
	  {
		float v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	  case TTYPE_DOUBLE:
	  {
		double v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	}
  }

# ifdef HAVE_GEOTIFF
  GTIFmapping * g = findTag (name, GTIFmap);
  if (g)
  {
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, g->key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		unsigned short v;
		if (GTIFKeyGet (gtif, g->key, &v, 0, 1)) value = v;
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, g->key, &v, 0, 1)) value = v;
		return;
	  }
	  case TYPE_ASCII:
	  {
		string v;
		v.resize (length);
		if (GTIFKeyGet (gtif, g->key, (void *) v.c_str (), 0, length))
		{
		  value = atof (v.c_str ());
		}
		return;
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::get (const string & name, string & value)
{
  char buffer[100];

  TIFFmapping * m = findTag (name, TIFFmap);
  if (m)
  {
	switch (m->type)
	{
	  case TTYPE_INT:
	  {
		unsigned int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  sprintf (buffer, "%u", v);
		  value = buffer;
		}
		return;
	  }
	  case TTYPE_SHORT:
	  {
		unsigned short v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  sprintf (buffer, "%hu", v);
		  value = buffer;
		}
		return;
	  }
	  case TTYPE_SINT:
	  {
		int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  sprintf (buffer, "%i", v);
		  value = buffer;
		}
		return;
	  }
	  case TTYPE_ASCII:
	  {
		char * v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v)) value = v;
		return;
	  }
	  case TTYPE_FLOAT:
	  {
		float v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  sprintf (buffer, "%f", v);
		  value = buffer;
		}
		return;
	  }
	  case TTYPE_DOUBLE:
	  {
		double v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  sprintf (buffer, "%lf", v);
		  value = buffer;
		}
		return;
	  }
	}
  }

# ifdef HAVE_GEOTIFF
  GTIFmapping * g = findTag (name, GTIFmap);
  if (g)
  {
	int size;
	tagtype_t type;
	int length = GTIFKeyInfo (gtif, g->key, &size, &type);
	switch (type)
	{
	  case TYPE_SHORT:
	  {
		unsigned short v;
		if (GTIFKeyGet (gtif, g->key, &v, 0, 1))
		{
		  sprintf (buffer, "%hu", v);
		  value = buffer;
		}
		return;
	  }
	  case TYPE_DOUBLE:
	  {
		double v;
		if (GTIFKeyGet (gtif, g->key, &v, 0, 1))
		{
		  sprintf (buffer, "%lf", v);
		  value = buffer;
		}
		return;
	  }
	  case TYPE_ASCII:
	  {
		value.resize (length);
		GTIFKeyGet (gtif, g->key, (void *) value.c_str (), 0, length);
		return;
	  }
	}
  }
# endif
}

void
ImageFileDelegateTIFF::get (const string & name, Matrix<double> & value)
{
  if (name == "ModelTransformation")
  {
	unsigned short count;
	double * v;
	bool found = TIFFGetField (tif, 34264, &count, &v);
	if (! found)
	{
	  found = TIFFGetField (tif, 33920, &count, &v)  &&  count == 16;
	}
	if (found)
	{
	  value = ~Matrix<double> (v, 4, 4);
	  return;
	}

	if (TIFFGetField (tif, 33922, &count, &v)) // ModelTiepoint
	{
	  if (count == 6)  // exactly one tiepoint
	  {
		double * o;
		if (TIFFGetField (tif, 33550, &count, &o)) // ModelPixelScale
		{
		}
	  }
	  else if (count >= 3)
	  {
		// Solve for affine transformation
	  }
	}

	return;
  }

  TIFFmapping * m = findTag (name, TIFFmap);
  if (m)
  {
	switch (m->type)
	{
	  case TTYPE_INT:
	  {
		unsigned int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TTYPE_SHORT:
	  {
		unsigned short v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TTYPE_SINT:
	  {
		int v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TTYPE_ASCII:
	  {
		char * v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = atof (v);
		}
		return;
	  }
	  case TTYPE_FLOAT:
	  {
		float v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TTYPE_DOUBLE:
	  {
		double v;
		if (TIFFGetFieldDefaulted (tif, m->tag, &v))
		{
		  value.resize (1, 1);
		  value(0,0) = v;
		}
		return;
	  }
	  case TTYPE_VECTORDOUBLE:
	  {
		unsigned short count;
		void * data;
		if (TIFFGetFieldDefaulted (tif, m->tag, &count, &data))
		{
		  if (name == "ModelTiepoint")
		  {
			value = Matrix<double> ((double *) data, 6, count / 6);
		  }
		  else
		  {
			value = Matrix<double> ((double *) data, count, 1);
		  }
		}
		return;
	  }
	}
  }
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
