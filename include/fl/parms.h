/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.4  2004/07/22 15:06:02  rothgang
Change "list" type functions to take a string as default rather than a vector
of strings.  Makes specifying the default easier in the client program.

Revision 1.3  2004/01/13 19:40:48  rothgang
Add getIntList().

Revision 1.2  2003/12/30 16:24:51  rothgang
Add function to output parms in human readable form.  Convert comments to
doxygen format.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_parms_h
#define fl_parms_h


#include <map>
#include <string>
#include <vector>


namespace fl
{
  /**
	 Stores a collection of named values.  Names are caseless, and no name
	 may be a prefix of another.  Stores values as human readable strings.
	 Several convenience functions parse the value into different formats.
  **/
  class Parameters
  {
  public:
	Parameters ();
	Parameters (int argc, char * argv[]);

	void parse (const std::string & line);
	void parse (int argc, char * argv[]);
	void read (const std::string & parmFileName);
	void read (std::istream & stream);
	void write (const std::string & parmFileName) const;
	void write (std::ostream & stream) const;

	int find (const std::string & name) const;  ///< finds index of given name in names vector.  returns -1 if not found.
	void update (const std::string & name, const std::string & value);  ///< causes a single entry called "name" to exist and have "value"

	// For simple types, return the value directly.  For complex structures,
	// return the value in a reference parameter.
	char * getChar (const std::string & name, const char * defaultValue = "") const;
	int getInt (const std::string & name, int defaultValue = 0) const;
	float getFloat (const std::string & name, float defaultValue = 0) const;
	void getStringList (const std::string & name, std::vector<std::string> & result, const char * defaultValue = "") const;  ///< parses the value into its comma-separated elements
	void getIntList (const std::string & name, std::vector<int> & result, const char * defaultValue = "") const;  ///< Separates the value into its comma-delimited components and interprets them as integers.

	std::vector<std::string> names;
	std::vector<std::string> values;
	std::vector<std::string> fileNames;  ///< All strings that didn't have the form name=value.  Not necessarily file names, but we imagine so.
  };

  std::ostream & operator << (std::ostream & out, const Parameters & parms);  ///< Dump a human readable summary of the parameters to a stream.
}


#endif
