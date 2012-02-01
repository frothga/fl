/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/metadata.h"

using namespace fl;
using namespace std;


// class NamedValueSet --------------------------------------------------------

uint32_t NamedValueSet::serializeVersion = 0;

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

ostream &
fl::operator << (ostream & out, const NamedValueSet & data)
{
  map<string, string>::const_iterator it;
  for (it = data.namedValues.begin (); it != data.namedValues.end (); it++) out << it->first << " = " << it->second << endl;
}
