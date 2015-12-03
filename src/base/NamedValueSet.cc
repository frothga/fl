/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/metadata.h"
#include "fl/string.h"

#include <sstream>


using namespace fl;
using namespace std;


// class NamedValueSet --------------------------------------------------------

uint32_t NamedValueSet::serializeVersion = 0;

void
NamedValueSet::clear ()
{
  namedValues.clear ();
}

void
NamedValueSet::serialize (Archive & archive, uint32_t version)
{
  uint32_t count = namedValues.size ();
  archive & count;
  if (archive.in)
  {
	string name;
	string value;
	for (int i = 0; i < count; i++)
	{
	  archive & name;
	  archive & value;
	  namedValues.insert (make_pair (name, value));
	}
  }
  else
  {
	map<string, string>::const_iterator it;
	for (it = namedValues.begin (); it != namedValues.end (); it++)
	{
	  archive & const_cast<string &> (it->first);
	  archive & const_cast<string &> (it->second);
	}
  }
}

void
NamedValueSet::get (const string & name, string & value)
{
  map<string, string>::iterator it = namedValues.find (name);
  if (it != namedValues.end ()) value = it->second;
}

void
NamedValueSet::set (const string & name, const string & value)
{
  map<string, string>::iterator it = namedValues.find (name);
  if (it == namedValues.end ()) namedValues.insert (make_pair (name, value));
  else                          it->second = value;
}

void
NamedValueSet::read (istream & stream)
{
  while (stream.good ())
  {
	string line;
	getline (stream, line);

	// Check for multi-line literal, which is delimited by double-quote marks.
	int first = line.find_first_of ('"');
	int last  = line.find_last_of  ('"');
	if (first != string::npos)
	{
	  line = line.erase (first, 1);
	  if (last != first)
	  {
		line.erase (last - 1, 1);
	  }
	  else
	  {
		line += '\n';
		while (true)
		{
		  char c;
		  stream.get (c);
		  if (! stream.good ()  ||  c == '"') break;
		  line += c;
		}
	  }
	}

	string name;
	string value;
	split (line, "#", name, value);
	trim (name);
	if (name.size () > 0)
	{
	  split (name, "=", name, value);
	  trim (name);
	  trim (value);
	  set (name, value);
	}
  }
}

void
NamedValueSet::read (const string & text)
{
  istringstream iss (text);
  read (iss);
}

void
NamedValueSet::write (ostream & stream) const
{
  map<string, string>::const_iterator it;
  for (it = namedValues.begin (); it != namedValues.end (); it++) stream << it->first << " = " << it->second << endl;
}

void
NamedValueSet::write (string & text) const
{
  ostringstream oss;
  write (oss);
  text = oss.str ();
}

ostream &
fl::operator << (ostream & out, const NamedValueSet & data)
{
  data.write (out);
  return out;
}
