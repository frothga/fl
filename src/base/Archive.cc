/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/archive.h"


using namespace fl;
using namespace std;


// class Archive --------------------------------------------------------------

Archive::Archive (istream & in, bool ownStream)
: in (&in),
  out (0),
  ownStream (ownStream)
{
}

Archive::Archive (ostream & out, bool ownStream)
: in (0),
  out (&out),
  ownStream (ownStream)
{
}

Archive::Archive (const string & fileName, const string & mode)
{
  in        = 0;
  out       = 0;
  ownStream = false;
  open (fileName, mode);
}

Archive::~Archive ()
{
  close ();
}

void
Archive::open (istream & in, bool ownStream)
{
  close ();
  this->in        = &in;
  this->ownStream = ownStream;
}

void
Archive::open (ostream & out, bool ownStream)
{
  close ();
  this->out       = &out;
  this->ownStream = ownStream;
}

void
Archive::open (const string & fileName, const string & mode)
{
  close ();
  if (mode == "r")
  {
	in        = new ifstream (fileName.c_str (), ios::binary);
	ownStream = true;
  }
  else if (mode == "w")
  {
	out       = new ofstream (fileName.c_str (), ios::binary);
	ownStream = true;
  }
  else throw "Unknown file mode";
}

void
Archive::close ()
{
  pointersIn .clear ();
  pointersOut.clear ();
  classesIn  .clear ();
  map<string, ClassDescription *>::iterator it;
  for (it = classesOut.begin (); it != classesOut.end (); it++) delete it->second;
  classesOut .clear ();

  if (ownStream)
  {
	if (in)  delete in;
	if (out) delete out;
  }
  in  = 0;
  out = 0;
}

template<>
Archive &
Archive::operator & (string & data)
{
  uint32_t count = data.size ();
  (*this) & count;
  if (in)
  {
	if (in->bad ()) throw "stream bad";
	data.resize (count);
	in->read ((char *) data.c_str (), count);
  }
  else
  {
	out->write ((char *) data.c_str (), count);
  }
  return *this;
}

template<>
Archive &
Archive::operator & (uint8_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (uint16_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (uint32_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (uint64_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (int8_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (int16_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (int32_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (int64_t & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (float & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (double & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}

template<>
Archive &
Archive::operator & (bool & data)
{
  if (in) in ->read  ((char *) &data, sizeof (data));
  else    out->write ((char *) &data, sizeof (data));
  return *this;
}
