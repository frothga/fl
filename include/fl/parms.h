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
	void getStringList (const std::string & name, std::vector<std::string> & result, const std::vector<std::string> * defaultValue = NULL) const;  ///< parses the value into its comma-separated elements

	std::vector<std::string> names;
	std::vector<std::string> values;
	std::vector<std::string> fileNames;  ///< All strings that didn't have the form name=value.  Not necessarily file names, but we imagine so.
  };

  std::ostream & operator << (std::ostream & out, const Parameters & parms);  ///< Dump a human readable summary of the parameters to a stream.
}


#endif
