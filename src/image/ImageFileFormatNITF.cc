/*
Author: Fred Rothganger
Created 2/26/2006
*/


#include "fl/image.h"
#include "fl/string.h"
#include "fl/endian.h"


using namespace std;
using namespace fl;


// NITF structure -------------------------------------------------------------

/**
   Record in maps used by nitfHeader to guide parsing.
 **/
struct nitfMapping
{
  char * name;  ///< NITF standard field name.  If null, then end of table.
  int    size;  ///< byte count

  /**
	 A terse string that indicates the kind of data in this field.
	 <ul>
	 <li>"c" -- The name field refers to a combination of nitfItem subclass and nitfMapping table.
	 <li>"A" -- ASCII
	 <li>"N" -- Integer
	 <li>"F" -- Float
	 </ul>
  **/
  char * type;
  char * defaultValue;  ///< Blank if standard default for given type.
};

class nitfItem
{
public:
  nitfItem (nitfMapping * map)
  {
	this->map = map;
	data = 0;
  }

  virtual ~nitfItem ()
  {
	if (data) free (data);
  }

  virtual void read (istream & stream)
  {
	data = (char *) malloc (map->size);
	stream.read (data, map->size);

	string bob (data, map->size);
	cerr << "read item: " << map->name << " = " << bob << endl;
  }

  virtual void write (ostream & stream)
  {
	if (! data)
	{
	  data = (char *) malloc (map->size);
	  if (map->defaultValue[1])  // longer than a single character; must be exactly map->size chars long
	  {
		memcpy (data, map->defaultValue, map->size);
	  }
	  else  // one character long; use character to fill field
	  {
		memset (data, map->defaultValue[0], map->size);
	  }
	}
	stream.write ((char *) data, map->size);
  }

  virtual bool contains (const string & name)
  {
	return name == map->name;
  }

  virtual void get (const string & name, string & value)
  {
	value.assign (data, map->size);
  }

  virtual void get (const string & name, double & value)
  {
	string temp (data, map->size);
	value = atof (temp.c_str ());
  }

  virtual void get (const string & name, int & value)
  {
	string temp (data, map->size);
	value = atoi (temp.c_str ());
  }

  virtual void set (const string & name, const string & value)
  {
	int length = value.size ();
	if (length >= map->size)
	{
	  memcpy (data, (char *) value.c_str (), map->size);
	}
	else
	{
	  switch (map->type[0])
	  {
		case 'A':
		case 'F':
		  memcpy (data, (char *) value.c_str (), length);
		  memset (data + length, ' ', map->size - length);
		  break;
		case 'N':
		  int pad = map->size - length;
		  memset (data, '0', pad);
		  memcpy (data + pad, (char *) value.c_str (), length);
	  }
	}
  }

  virtual void set (const string & name, double value)
  {
	char buffer[256];
	if (map->type[0] == 'F')
	{
	  sprintf (buffer, "%lf", value);
	  set (name, buffer);
	}
	else
	{
	  long long v = (long long) rint (value);
	  sprintf (buffer, "%0*lli", map->size, v);
	  memcpy (data, buffer, map->size);
	}
  }

  virtual void set (const string & name, int value)
  {
	char buffer[256];
	if (map->type[0] == 'F')
	{
	  sprintf (buffer, "%i", value);
	  set (name, buffer);
	}
	else
	{
	  sprintf (buffer, "%0*i", map->size, value);
	  memcpy (data, buffer, map->size);
	}
  }

  nitfMapping * map;
  char * data;

  static nitfItem * factory (nitfMapping * m);
};

/**
   Generic reader/writer of a contiguous section of NITF metadata items.
 **/
class nitfItemSet : public nitfItem
{
public:
  nitfItemSet (nitfMapping * map)
  : nitfItem (map)
  {
	nitfMapping * m = map;
	while (m->name)
	{
	  data.push_back (nitfItem::factory (m++));
	}
  }

  virtual ~nitfItemSet ()
  {
	for (int i = 0; i < data.size (); i++)
	{
	  delete data[i];
	}
  }

  virtual void read (istream & stream)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  data[i]->read (stream);
	}
  }

  virtual void write (ostream & stream)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  data[i]->write (stream);
	}
  }

  nitfItem * find (const string & name)
  {
	for (int i = 0; i < data.size (); i++)
	{
	  if (data[i]->contains (name)) return data[i];
	}
	return 0;
  }

  virtual bool contains (const string & name)
  {
	if (find (name)) return true;
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void get (const string & name, double & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void get (const string & name, int & value)
  {
	nitfItem * item = find (name);
	if (item) item->get (name, value);
  }

  virtual void set (const string & name, const string & value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  virtual void set (const string & name, double value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  virtual void set (const string & name, int value)
  {
	nitfItem * item = find (name);
	if (item) item->set (name, value);
  }

  vector<nitfItem *> data;
};

class nitfRepeat : public nitfItem
{
public:
  nitfRepeat (nitfMapping * map)
  : nitfItem (map)
  {
	countItem = nitfItem::factory (map);
  }

  virtual ~nitfRepeat ()
  {
	delete countItem;
	for (int i = 0; i < data.size (); i++)
	{
	  delete data[i];
	}
  }

  virtual void read (istream & stream)
  {
	countItem->read (stream);
	int count;
	countItem->get (map->name, count);

	for (int i = 0; i < count; i++)
	{
	  nitfItemSet * s = new nitfItemSet (&map[1]);
	  s->read (stream);
	  data.push_back (s);
	}
  }

  virtual void write (ostream & stream)
  {
	int count = data.size ();
	countItem->set (map->name, count);
	countItem->write (stream);

	for (int i = 0; i < count; i++)
	{
	  data[i]->write (stream);
	}
  }

  void split (const string & name, string & root, int & index)
  {
	int position = name.find_first_of ("0123456789");
	if (position == string::npos)
	{
	  root = name;
	  index = 0;
	}
	else
	{
	  root = name.substr (0, position);
	  index = atoi (name.substr (position).c_str ());
	}
  }

  virtual bool contains (const string & name)
  {
	string root;
	int index;
	split (name, root, index);

	if (countItem->contains (root)) return true;

	nitfMapping * m = &map[1];
	while (m->name)
	{
	  if (root == m->name) return true;
	  m++;
	}
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void get (const string & name, double & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= (int) data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void get (const string & name, int & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (countItem->contains (root))
	{
	  countItem->get (root, value);
	}
	else if (index > 0  &&  index <= (int) data.size ())
	{
	  data[index-1]->get (root, value);
	}
  }

  virtual void set (const string & name, const string & value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  virtual void set (const string & name, double value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  virtual void set (const string & name, int value)
  {
	string root;
	int index;
	split (name, root, index);
	if (index < 1) return;

	while (data.size () < index)
	{
	  data.push_back (new nitfItemSet (&map[1]));
	}

	data[index-1]->set (root, value);
  }

  nitfItem * countItem;
  vector<nitfItemSet *> data;
};

class nitfGeoloc : public nitfItemSet
{
public:
  nitfGeoloc (nitfMapping * map)
  : nitfItemSet (map)
  {
  }

  virtual void read (istream & stream)
  {
	data[0]->read (stream);
	if (data[0]->data[0] != ' ')
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->read (stream);
	  }
	}
  }

  virtual void write (ostream & stream)
  {
	data[0]->write (stream);
	if (data[1]->data)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->write (stream);
	  }
	}
  }
};

class nitfCompression : public nitfItemSet
{
public:
  nitfCompression (nitfMapping * map)
  : nitfItemSet (map),
	ICcodes ("C1|C3|C4|C5|C8|M1|M3|M4|M5|M8|I1")
  {
  }

  virtual void read (istream & stream)
  {
	data[0]->read (stream);
	string IC (data[0]->data, 2);
	if (ICcodes.find (IC) != string::npos)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->read (stream);
	  }
	}
  }

  virtual void write (ostream & stream)
  {
	data[0]->write (stream);
	string IC (data[0]->data, 2);
	if (ICcodes.find (IC) != string::npos)
	{
	  for (int i = 1; i < data.size (); i++)
	  {
		data[i]->write (stream);
	  }
	}
  }

  string ICcodes;  ///< that require a COMRAT field
};

class nitfExtendableCount : public nitfItem
{
public:
  nitfExtendableCount (nitfMapping * map)
  : nitfItem (map)
  {
  }

  virtual void read (istream & stream)
  {
	char buffer[50];
	stream.read (buffer, map->size);
	buffer[map->size] = 0;
	count = atoi (buffer);
	if (! count)
	{
	  stream.read (buffer, map[1].size);
	  buffer[map[1].size] = 0;
	  count = atoi (buffer);
	}
  }

  virtual void write (ostream & stream)
  {
	char buffer[50];
	if (count < pow (10, map->size))
	{
	  sprintf (buffer, "%i", count);
	}
	else
	{
	  sprintf (buffer, "%0*i", map->size + map[1].size, count);
	}
	stream << buffer;
  }

  virtual bool contains (const string & name)
  {
	nitfMapping * m = map;
	while (m->name)
	{
	  if (name == m->name) return true;
	  m++;
	}
	return false;
  }

  virtual void get (const string & name, string & value)
  {
	char buffer[50];
	sprintf (buffer, "%i", count);
	value = buffer;
  }

  virtual void get (const string & name, double & value)
  {
	value = count;
  }

  virtual void get (const string & name, int & value)
  {
	value = count;
  }

  virtual void set (const string & name, const string & value)
  {
	count = atoi (value.c_str ());
  }

  virtual void set (const string & name, double value)
  {
	count = (int) rint (value);
  }

  virtual void set (const string & name, int value)
  {
	count = value;
  }

  int count;
};

class nitfLUT : public nitfItem
{
public:
  nitfLUT (nitfMapping * map)
  : nitfItem (map)
  {
	lut = 0;
  }

  virtual ~nitfLUT ()
  {
	if (lut) free (lut);
  }

  virtual void read (istream & stream)
  {
	char buffer[10];
	stream.read (buffer, 1);
	buffer[1] = 0;
	NLUTS = atoi (buffer);
	if (NLUTS)
	{
	  stream.read (buffer, 5);
	  buffer[5] = 0;
	  NELUT = atoi (buffer);

	  int count = NLUTS * NELUT;
	  lut = (unsigned char *) malloc (count);
	  stream.read ((char *) lut, count);
	}
  }

  virtual void write (ostream & stream)
  {
	stream << NLUTS;
	if (NLUTS)
	{
	  char buffer[10];
	  sprintf (buffer, "%05i", NELUT);
	  stream.write (buffer, 5);
	  int count = NLUTS * NELUT;
	  stream.write ((char *) lut, count);
	}
  }

  virtual bool contains (const string & name)
  {
	return name == "NLUTS"  ||  name == "NELUT"  ||  name == "LUTD";
  }

  virtual void get (const string & name, string & value)
  {
	char buffer[10];
	if (name == "NLUTS")
	{
	  sprintf (buffer, "%i", NLUTS);
	  value = buffer;
	}
	else if (name == "NELUT")
	{
	  sprintf (buffer, "%05i", NELUT);
	  value = buffer;
	}
	else if (name == "LUTD")
	{
	  value.assign ((char *) lut, NLUTS * NELUT);
	}
  }

  virtual void get (const string & name, double & value)
  {
	if (name == "NLUTS")
	{
	  value = NLUTS;
	}
	else if (name == "NELUT")
	{
	  value = NELUT;
	}
  }

  virtual void get (const string & name, int & value)
  {
	if (name == "NLUTS")
	{
	  value = NLUTS;
	}
	else if (name == "NELUT")
	{
	  value = NELUT;
	}
  }

  virtual void set (const string & name, const string & value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = atoi (value.c_str ());
	}
	else if (name == "NELUT")
	{
	  NELUT = atoi (value.c_str ());
	}
	throw "must resize lut";
  }

  virtual void set (const string & name, double value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = (int) rint (value);
	}
	else if (name == "NELUT")
	{
	  NELUT = (int) rint (value);
	}
	throw "must resize lut";
  }

  virtual void set (const string & name, int value)
  {
	if (name == "NLUTS")
	{
	  NLUTS = value;
	}
	else if (name == "NELUT")
	{
	  NELUT = value;
	}
	throw "must resize lut";
  }

  int NLUTS;
  int NELUT;
  unsigned char * lut;  ///< an NELUT by NLUTS matrix
};

static nitfMapping mapIS[] =
{
  {"NUMI", 3,  "N", "0"},
  {"LISH", 6,  "N", "9"},
  {"LI",   10, "N", "9"},
  {0}
};

static nitfMapping mapGS[] =
{
  {"NUMS", 3, "N", "0"},
  {"LSSH", 4, "N", "9"},
  {"LS",   6, "N", "9"},
  {0}
};

static nitfMapping mapTS[] =
{
  {"NUMT", 3, "N", "0"},
  {"LTSH", 4, "N", "9"},
  {"LT",   5, "N", "9"},
  {0}
};

static nitfMapping mapDES[] =
{
  {"NUMDES", 3, "N", "0"},
  {"LDSH",   4, "N", "9"},
  {"LD",     9, "N", "9"},
  {0}
};

static nitfMapping mapRES[] =
{
  {"NUMRES", 3, "N", "0"},
  {"LRESH",  4, "N", "9"},
  {"LRE",    7, "N", "9"},
  {0}
};

static nitfMapping mapFileHeader[] =
{
  {"FHDR",    4, "A", "NITF"},
  {"FVER",    5, "A", "02.10"},
  {"CLEVEL",  2, "N", "9"},
  {"STYPE",   4, "A", "BF01"},
  {"OSTAID", 10, "A", " "},
  {"FDT",    14, "N", "9"},
  {"FTITLE", 80, "A", " "},
  {"FSCLAS",  1, "A", "U"},
  {"FSCLSY",  2, "A", " "},
  {"FSCODE", 11, "A", " "},
  {"FSCTLH",  2, "A", " "},
  {"FSREL",  20, "A", " "},
  {"FSDCTP",  2, "A", " "},
  {"FSDCDT",  8, "A", " "},
  {"FSDCXM",  4, "A", " "},
  {"FSDG",    1, "A", " "},
  {"FSDGDT",  8, "A", " "},
  {"FSCLTX", 43, "A", " "},
  {"FSCATP",  1, "A", " "},
  {"FSAUT",  40, "A", " "},
  {"FSCRSN",  1, "A", " "},
  {"FSSRDT",  8, "A", " "},
  {"FSCTLN", 15, "A", " "},
  {"FSCOP",   5, "N", "0"},
  {"FSCPYS",  5, "N", "0"},
  {"ENCRYP",  1, "N", "0"},
  {"FBKGC",   3, "A", "\0x00\0x00\0x00"},
  {"ONAME",  24, "A", " "},
  {"OPHONE", 18, "A", " "},
  {"FL",     12, "N", "9"},
  {"HL",      6, "N", "9"},
  {"IS",      0, "c"},
  {"GS",      0, "c"},
  {"NUMX",    3, "N", "0"},
  {"TS",      0, "c"},
  {"DES",     0, "c"},
  {"RES",     0, "c"},
  {0}
};

static nitfMapping mapGeoloc[] =
{
  {"ICORDS",   1, "A", " "},
  {"IGEOLO1", 15, "A", " "},
  {"IGEOLO2", 15, "A", " "},
  {"IGEOLO3", 15, "A", " "},
  {"IGEOLO4", 15, "A", " "},
  {0}
};

static nitfMapping mapIcom[] =
{
  {"NICOM",    1, "N", "0"},
  {"ICOM",    80, "A", " "},
  {0}
};

static nitfMapping mapCompression[] =
{
  {"IC",       2, "A", "NC"},
  {"COMRAT",   4, "A", " "},
  {0}
};

static nitfMapping mapBandCount[] =
{
  {"NBANDS",   1, "N", "0"},
  {"XBANDS",   5, "N", "0"},
  {0}
};

static nitfMapping mapBand[] =
{
  {"bandcount", 0, "c"},
  {"IREPBAND",  2, "A", " "},
  {"ISUBCAT",   6, "A", " "},
  {"IFC",       1, "A", "N"},
  {"IMFLT",     3, "A", " "},
  {"lut",       0, "c"},
  {0}
};

static nitfMapping mapImageHeader[] =
{
  {"IM",      2, "A", "IM"},
  {"IID1",   10, "A", " "},
  {"IDATIM", 14, "N", "9"},
  {"TGTID",  17, "A", " "},
  {"IID2",   80, "A", " "},
  {"ISCLAS",  1, "A", " "},
  {"ISCLSY",  2, "A", " "},
  {"ISCODE", 11, "A", " "},
  {"ISCTLH",  2, "A", " "},
  {"ISREL",  20, "A", " "},
  {"ISDCTP",  2, "A", " "},
  {"ISDCDT",  8, "A", " "},
  {"ISDCXM",  4, "A", " "},
  {"ISDG",    1, "A", " "},
  {"ISDGDT",  8, "A", " "},
  {"ISCLTX", 43, "A", " "},
  {"ISCATP",  1, "A", " "},
  {"ISCAUT", 40, "A", " "},
  {"ISCRSN",  1, "A", " "},
  {"ISSRDT",  8, "A", " "},
  {"ISCTLN", 15, "A", " "},
  {"ENCRYP",  1, "N", "0"},
  {"ISORCE", 42, "A", " "},
  {"NROWS",   8, "N", "9"},
  {"NCOLS",   8, "N", "9"},
  {"PVTYPE",  3, "A", "INT"},
  {"IREP",    8, "A", "MONO    "},
  {"ICAT",    8, "A", "VIS     "},
  {"ABPP",    2, "N", "9"},
  {"PJUST",   1, "A", "R"},
  {"geoloc",      0, "c"},
  {"icom",        0, "c"},
  {"compression", 0, "c"},
  {"band",        0, "c"},
  {"ISYNC",   1, "N", "0"},
  {"IMODE",   1, "A", "P"},
  {"NBPR",    4, "N", "1"},
  {"NBPC",    4, "N", "1"},
  {"NPPBH",   4, "N", "0"},
  {"NPPBV",   4, "N", "0"},
  {"NBPP",    2, "N", "9"},
  {"IDLVL",   3, "N", "001"},
  {"IALVL",   3, "N", "0"},
  {"ILOC",   10, "N", "0"},
  {"IMAG",    4, "F", "1.0 "},
  // user and extension headers go here
  {0}
};

enum nitfItemID
{
  idItem,
  idRepeat,
  idItemSet,
  idGeoloc,
  idCompression,
  idExtendableCount,
  idLUT
};

struct nitfTypeMapping
{
  char *        name;
  nitfItemID    id;
  nitfMapping * map;  ///< alternate map
};

nitfTypeMapping typeMap[] =
{
  {"IS",          idRepeat,          mapIS},
  {"GS",          idRepeat,          mapGS},
  {"TS",          idRepeat,          mapTS},
  {"DES",         idRepeat,          mapDES},
  {"RES",         idRepeat,          mapRES},
  {"geoloc",      idGeoloc,          mapGeoloc},
  {"icom",        idRepeat,          mapIcom},
  {"compression", idCompression,     mapCompression},
  {"band",        idRepeat,          mapBand},
  {"bandcount",   idExtendableCount, mapBandCount},
  {"lut",         idLUT,             0},
  {0}
};

nitfItem *
nitfItem::factory (nitfMapping * m)
{
  if (m->type[0] == 'A'  ||  m->type[0] == 'N'  ||  m->type[0] == 'F')
  {
	return new nitfItem (m);
  }

  if (m->type[0] != 'c') throw "Expected class type";  // error in map tables

  nitfTypeMapping * t = typeMap;
  while (t->name)
  {
	if (strcmp (t->name, m->name) == 0)
	{
	  nitfMapping * mm = t->map ? t->map : m;
	  switch (t->id)
	  {
		case idItem:            return new nitfItem            (mm);
		case idRepeat:          return new nitfRepeat          (mm);
		case idItemSet:         return new nitfItemSet         (mm);
		case idGeoloc:          return new nitfGeoloc          (mm);
		case idCompression:     return new nitfCompression     (mm);
		case idExtendableCount: return new nitfExtendableCount (mm);
		case idLUT:             return new nitfLUT             (mm);
	  }
	}
	t++;
  }

  throw "Can't find typeMap entry!";  // error in map tables
}

class nitfImageSection
{
public:
  nitfImageSection ()
  : header (mapImageHeader)
  {
	BMRBND = 0;
	format = 0;
	ownFormat = false;
  }

  ~nitfImageSection ()
  {
	if (BMRBND) free (BMRBND);
	if (ownFormat  &&  format) delete format;
  }

  void get (const string & name, string & value)
  {
	header.get (name, value);
  }

  void get (const string & name, int & value)
  {
	header.get (name, value);
  }

  void read (istream & stream, Image & image, int x, int y, int width, int height)
  {
	if (IC[0] == 'N')  // no compression
	{
	  PixelBufferPacked * buffer = (PixelBufferPacked *) image.buffer;
	  if (! buffer) image.buffer = buffer = new PixelBufferPacked;
	  image.format = format;

	  if (! width ) width  = NCOLS - x;
	  if (! height) height = NROWS - y;
	  if (x < 0)
	  {
		width += x;
		x = 0;
	  }
	  if (y < 0)
	  {
		height += y;
		y = 0;
	  }
	  width  = min (width,  (int) NCOLS - x);
	  height = min (height, (int) NROWS - y);
	  width  = max (width,  0);
	  height = max (height, 0);
	  image.resize (width, height);
	  if (! width  ||  ! height) return;

	  char * imageMemory = (char *) buffer->memory;
	  int stride = format->depth * width;

	  // If the requested image is anything other than exactly the union of a
	  // set of blocks in the file, then must use temporary storage to read in
	  // blocks.
	  Image block (*image.format);
	  if (x % NPPBH  ||  y % NPPBV  ||  width != NPPBH  ||  height % NPPBV) block.resize (NPPBH, NPPBV);
	  int blockSize = NPPBH * NPPBV * format->depth;
	  char * blockBuffer = (char *) ((PixelBufferPacked *) block.buffer)->memory;

	  for (int oy = 0; oy < height;)  // output y: position in output image
	  {
		int ry = oy + y;  // working y in raster
		int by = ry / NPPBV;  // vertical block number
		int iy = ry % NPPBV;  // input y: offset from top of block
		int h = min ((int) NPPBV - iy, height - oy);  // height of usable portion of block

		for (int ox = 0; ox < width;)
		{
		  int rx = ox + x;
		  int bx = rx / NPPBH;
		  int ix = rx % NPPBH;
		  int w = min ((int) NPPBH - ix, width - ox);

		  int blockIndex = by * NBPR + bx;
		  // Should also deal with multiple bands here, in the case of planar formats

		  unsigned int blockAddress;
		  if (BMRBND)
		  {
			blockAddress = BMRBND[blockIndex];
			if (blockAddress == 0xFFFFFFFF)
			{
			  blockAddress = 0;
			}
			else
			{
			  blockAddress += offset;
			}
		  }
		  else
		  {
			blockAddress = offset + blockIndex * blockSize;
		  }

		  if (w == width  &&  h == NPPBV)
		  {
			if (blockAddress)
			{
			  stream.seekg (blockAddress);
			  stream.read (imageMemory + oy * stride, blockSize);
			}
			else
			{
			  memset (imageMemory + oy * stride, 0, blockSize);
			}
		  }
		  else
		  {
			if (blockAddress)
			{
			  stream.seekg (blockAddress);
			  stream.read (blockBuffer, blockSize);
			}
			else
			{
			  memset (blockBuffer, 0, blockSize);
			}
			image.bitblt (block, ox, oy, ix, iy, w, h);
		  }

		  ox += w;
		}

		oy += h;
	  }

#     if BYTE_ORDER == LITTLE_ENDIAN
	  // Must do endian conversion for gray formats
	  if (*format == GrayShort)
	  {
		bswap ((unsigned short *) imageMemory, width * height);
	  }
	  else if (*format == GrayFloat)
	  {
		bswap ((unsigned int *) imageMemory, width * height);
	  }
	  else if (*format == GrayDouble)
	  {
		bswap ((unsigned long long *) imageMemory, width * height);
	  }
#     endif
	}
  }

  void read (istream & stream)
  {
	offset = stream.tellg ();
	offset += LISH;

	header.read (stream);

	header.get ("IC",     IC);
	header.get ("IMODE",  IMODE);
	header.get ("NBANDS", NBANDS);
	header.get ("NBPR",   NBPR);
	header.get ("NBPC",   NBPC);
	header.get ("NROWS",  NROWS);
	header.get ("NCOLS",  NCOLS);
	header.get ("NPPBH",  NPPBH);
	header.get ("NPPBV",  NPPBV);


	// Determine PixelFormat
	string IREP;
	get ("IREP", IREP);
	int NBPP;
	get ("NBPP", NBPP);
	string PVTYPE;
	get ("PVTYPE", PVTYPE);

	if (IREP == "MONO    ")
	{
	  if (PVTYPE == "INT"  ||  PVTYPE == "SI ")  // should distinguish signed from unsigned, but currently don't have signed PixelFormats
	  {
		switch (NBPP)
		{
		  case 8:
			format = &GrayChar;
			break;
		  case 16:
			format = &GrayShort;
			break;
		}
	  }
	  else if (PVTYPE == "R  ")
	  {
		switch (NBPP)
		{
		  case 32:
			format = &GrayFloat;
			break;
		  case 64:
			format = &GrayDouble;
			break;
		}
	  }
	}
	else if (IREP == "RGB     ")
	{
	  // This is too simplistic.  Should take into account the band layout as well.

	  if (PVTYPE == "INT"  ||  PVTYPE == "SI ")  // should distinguish signed from unsigned, but currently don't have signed PixelFormats
	  {
		if (NBPP == 8) format = &RGBChar;
	  }
	}

	if (! format) throw "Can't match format";
	cerr << "format = " << typeid (*format).name () << endl;


	// Parse block mask if it exists
	if (IC[0] == 'M'  ||  IC[1] == 'M')
	{
	  stream.seekg (offset);
	  IMDATOFF = 0;
	  stream.read ((char *) &IMDATOFF, 4);
	  if (! IMDATOFF) throw "failed to read IMDATOFF";
#     if BYTE_ORDER == LITTLE_ENDIAN
	  bswap (&IMDATOFF);
#     endif
	  offset += IMDATOFF;
	  cerr << "IMDATOFF = " << IMDATOFF << endl;

	  BMRLNTH = 0;
	  TMRLNTH = 0;
	  TPXCDLNTH = 0;
#     if BYTE_ORDER == LITTLE_ENDIAN
	  stream.read (((char *) &BMRLNTH) + 1, 1);
	  stream.read (((char *) &BMRLNTH) + 0, 1);
	  stream.read (((char *) &TMRLNTH) + 1, 1);
	  stream.read (((char *) &TMRLNTH) + 0, 1);
	  stream.read (((char *) &TPXCDLNTH) + 1, 1);
	  stream.read (((char *) &TPXCDLNTH) + 0, 1);
#     else
	  stream.read ((char *) &BMRLNTH, 2);
	  stream.read ((char *) &TMRLNTH, 2);
	  stream.read ((char *) &TPXCDLNTH, 2);
#     endif
	  cerr << "BMRLNTH = " << BMRLNTH << endl;
	  cerr << "TMRLNTH = " << TMRLNTH << endl;
	  cerr << "TPXCDLNTH = " << TPXCDLNTH << endl;

	  stream.seekg ((istream::off_type) ceil (TPXCDLNTH / 8.0), ios_base::cur);  // skip the TPXCD if it exists

	  if (BMRLNTH)
	  {
		int size = NBPR * NBPC;
		if (IMODE == "S") size *= NBANDS;
		BMRBND = (unsigned int *) malloc (size * sizeof (unsigned int));
		stream.read ((char *) BMRBND, size * sizeof (unsigned int));

#       if BYTE_ORDER == LITTLE_ENDIAN
		cerr << "BMRBND before = " << BMRBND[0] << endl;
		bswap (BMRBND, size);
		cerr << "BMRBND after = " << BMRBND[0] << endl;
#       endif
	  }
	}
  }

  int LISH;
  int LI;
  unsigned int offset;

  string IC;
  string IMODE;
  int NBPR;
  int NBPC;
  int NBANDS;
  int NROWS;
  int NCOLS;
  int NPPBH;
  int NPPBV;

  unsigned int IMDATOFF;
  unsigned short BMRLNTH;
  unsigned short TMRLNTH;
  unsigned short TPXCDLNTH;
  unsigned int * BMRBND;

  PixelFormat * format;
  bool ownFormat;  ///< If true, we should manage the lifespan of format.

  nitfItemSet header;
};

class nitfFileHeader
{
public:
  nitfFileHeader ()
  : header (mapFileHeader)
  {
  }

  ~nitfFileHeader ()
  {
	for (int i = 0; i < images.size (); i++)
	{
	  delete images[i];
	}
  }

  void get (const string & name, string & value)
  {
	header.get (name, value);
  }

  void get (const string & name, int & value)
  {
	header.get (name, value);
  }

  void read (istream & stream)
  {
	header.read (stream);

	int offset;
	get ("HL", offset);

	int NUMI;
	get ("NUMI", NUMI);
	for (int i = 0; i < NUMI; i++)
	{
	  nitfImageSection * h = new nitfImageSection;
	  images.push_back (h);

	  char buffer[10];
	  sprintf (buffer, "LISH%03i", i+1);
	  get (buffer, h->LISH);
	  sprintf (buffer, "LI%03i", i+1);
	  get (buffer, h->LI);

	  stream.seekg (offset);
	  h->read (stream);

	  offset += h->LISH + h->LI;
	}
  }

  void write (ostream & stream)
  {
  }

  nitfItemSet header;
  vector<nitfImageSection *> images;
};


// class ImageFileDelegateNITF ------------------------------------------------

class ImageFileDelegateNITF : public ImageFileDelegate
{
public:
  ImageFileDelegateNITF (istream * in, ostream * out, bool ownStream = false)
  {
	this->in = in;
	this->out = out;
	this->ownStream = ownStream;

	header = new nitfFileHeader;
	if (in)
	{
	  header->read (*in);
	}
  }
  ~ImageFileDelegateNITF ();

  virtual void read (Image & image, int x = 0, int y = 0, int width = 0, int height = 0);
  virtual void write (const Image & image, int x = 0, int y = 0);

  virtual void get (const string & name, string & value);

  istream * in;
  ostream * out;
  bool ownStream;
  nitfFileHeader * header;
};

ImageFileDelegateNITF::~ImageFileDelegateNITF ()
{
  if (ownStream)
  {
	if (in) delete in;
	if (out) delete out;
  }
  if (header) delete header;
}

void
ImageFileDelegateNITF::read (Image & image, int x, int y, int width, int height)
{
  if (! in) throw "ImageFileDelegateNITF not open for reading";
  if (! header->images.size ()) throw "No image to read";

  header->images[0]->read (*in, image, x, y, width, height);
}

void
ImageFileDelegateNITF::write (const Image & image, int x, int y)
{
  if (! out) throw "ImageFileDelegateNITF not open for writing";
}

void
ImageFileDelegateNITF::get (const string & name, string & value)
{
  if (! header) return;
  header->get (name, value);
}


// class ImageFileFormatNITF --------------------------------------------------

ImageFileDelegate *
ImageFileFormatNITF::open (std::istream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (&stream, 0, ownStream);
}

ImageFileDelegate *
ImageFileFormatNITF::open (std::ostream & stream, bool ownStream) const
{
  return new ImageFileDelegateNITF (0, &stream, ownStream);
}

float
ImageFileFormatNITF::isIn (std::istream & stream) const
{
  string magic = "         ";  // 9 spaces
  getMagic (stream, magic);
  if (magic == "NITF02.10")  // that's all we handle for now
  {
	return 1;
  }
  return 0;
}

float
ImageFileFormatNITF::handles (const std::string & formatName) const
{
  if (strcasecmp (formatName.c_str (), "nitf") == 0)
  {
	return 1;
  }
  if (strcasecmp (formatName.c_str (), "ntf") == 0)
  {
	return 0.9;
  }
  return 0;
}
