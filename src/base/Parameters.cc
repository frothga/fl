#include "fl/parms.h"
#include "fl/string.h"

#include <fstream>
#include <algorithm>


using namespace fl;
using namespace std;


// class Parameters -----------------------------------------------------------

Parameters::Parameters ()
{
}

Parameters::Parameters (int argc, char * argv[])
{
  parse (argc, argv);
}

void
Parameters::parse (const std::string & line)
{
  string name;
  string value;

  split (line, "#", name, value);
  trim (name);
  if (name.size () == 0)
  {
	return;
  }

  bool isFileName = name.find_first_of ('=') == string::npos;
  split (name, "=", name, value);
  trim (name);

  if (isFileName)
  {
	fileNames.push_back (name);
  }
  else
  {
	lowercase (name);
	trim (value);
	if (name == "include")  // include is a reserved word that means load a file
	{
	  read (value);
	}
	else
	{
	  update (name, value);
	}
  }
}

void
Parameters::parse (int argc, char * argv[])
{
  for (int i = 1; i < argc; i++)
  {
	parse (argv[i]);
  }
}

void
Parameters::read (const std::string & parmFileName)
{
  ifstream ifs (parmFileName.c_str ());
  read (ifs);
}

void
Parameters::read (std::istream & stream)
{
  while (stream.good ())
  {
	string line;
	getline (stream, line);

	trim (line);
	if (line == "endOfParms")
	{
	  break;
	}

	parse (line);
  }
}

void
Parameters::write (const std::string & parmFileName) const
{
  ofstream ofs (parmFileName.c_str ());
  write (ofs);
}

void
Parameters::write (std::ostream & stream) const
{
  for (int i = 0; i < names.size (); i++)
  {
	stream << names[i] << "=" << values[i] << endl;
  }
  for (int i = 0; i < fileNames.size (); i++)
  {
	stream << fileNames[i] << endl;
  }
  stream << "endOfParms" << endl;
}

int
Parameters::find (const std::string & name) const
{
  string temp = name;
  lowercase (temp);
  int tempSize = temp.size ();
  int longestPrefix = 0;
  int bestIndex = -1;
  for (int i = 0; i < names.size (); i++)
  {
	int length = names[i].size ();
	if (length == tempSize)
	{
	  if (names[i] == temp)
	  {
		return i;
	  }
	}
	else if (length < tempSize)
	{
	  if (names[i] == temp.substr (0, length))
	  {
		if (length >= longestPrefix)  // ">=" allows latter entries to override earlier ones
		{
		  longestPrefix = length;
		  bestIndex = i;
		}
	  }
	}
	else  // length > tempSize
	{
	  if (names[i].substr (0, tempSize) == temp)
	  {
		// There's no good way to choose among several names with prefixes
		// equal to the search string.  Therefore, don't even try.
		longestPrefix = tempSize;
		bestIndex = i;
	  }
	}
  }
  return bestIndex;
}

void
Parameters::update (const std::string & name, const std::string & value)
{
  bool plus = false;
  string tempName = name;
  if (*name.rbegin () == '+')
  {
	plus = true;
	tempName.resize (name.size () - 1);
	trim (tempName);  // In case there was space before the '+'
  }

  int index = find (tempName);
  if (index < 0)
  {
	names.push_back (tempName);
	values.push_back (value);
  }
  else
  {
	names[index] = name;
	if (plus)
	{
	  values[index] += "," + value;
	}
	else
	{
	  values[index] = value;
	}
  }
}

char *
Parameters::getChar (const std::string & name, const char * defaultValue) const
{
  int i = find (name);
  if (i >= 0)
  {
	return const_cast<char *> (values[i].c_str ());
  }
  else
  {
	return const_cast<char *> (defaultValue);
  }
}

int
Parameters::getInt (const std::string & name, int defaultValue) const
{
  char * value = getChar (name);
  if (*value)
  {
	if (value[0] == '0')
	{
	  return strtoul (value, 0, 0);
	}
	else
	{
	  return strtol (value, 0, 0);
	}
  }
  else
  {
	return defaultValue;
  }
}

float
Parameters::getFloat (const std::string & name, float defaultValue) const
{
  char * value = getChar (name);
  if (*value)
  {
	return atof (value);
  }
  else
  {
	return defaultValue;
  }
}

void
Parameters::getStringList (const string & name, vector<string> & result, const vector<string> * defaultValue) const
{
  string value = getChar (name);

  result.clear ();
  if (value.size () == 0)
  {
	if (defaultValue)
	{
	  result = *defaultValue;
	}
	return;
  }

  while (value.size ())
  {
	string piece;
	split (value, ",", piece, value);
	result.push_back (piece);
  }
}

void
Parameters::getIntList (const string & name, vector<int> & result, const vector<int> * defaultValue) const
{
  string value = getChar (name);

  result.clear ();
  if (value.size () == 0)
  {
	if (defaultValue)
	{
	  result = *defaultValue;
	}
	return;
  }

  while (value.size ())
  {
	string piece;
	split (value, ",", piece, value);
	result.push_back (atoi (piece.c_str ()));
  }
}

ostream &
fl::operator << (ostream & out, const Parameters & parms)
{
  out << "name-value pairs:" << endl;
  for (int i = 0; i < parms.names.size (); i++)
  {
	out << "  " << parms.names[i] << " = " << parms.values[i] << endl;
  }

  out << "fileNames:" << endl;
  for (int i = 0; i < parms.fileNames.size (); i++)
  {
	out << "  " << parms.fileNames[i] << endl;
  }

  return out;
}
