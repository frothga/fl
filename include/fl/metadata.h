/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_metadata_h
#define fl_metadata_h


#include "fl/archive.h"
#include "fl/matrix.h"

#include <stdint.h>

#include <string>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flBase_EXPORTS
#    define SHARED __declspec(dllexport)
#  else
#    define SHARED __declspec(dllimport)
#  endif
#else
#  define SHARED
#endif


namespace fl
{
 /**
	Inheritable set of convenience functions for accessing and converting
	named values to various other types.

	<p>To make use of the class properly, do the following steps in the
	child class:
	<ul>
	<li>Inherit from Metadata.
	<li>Implement get(string,string) and set(string,string)
	<li>Pull all the other implementations in with "using Metadata::get" and
	"using Metadata::set".  This step is important, because the standard
	behavior of C++ is to hide all the inherited functions when you create
	a get or set function in the child class.  Alternately, you can explicitly
	qualify any call: "object->Metadata::get (...)".
	</ul>
  **/
  class SHARED Metadata
  {
  public:
	virtual ~Metadata ();

	virtual void get (const std::string & name,       std::string & value);
	virtual void set (const std::string & name, const std::string & value);

	// Utility functions.  Should not be virtual, in order to avoid
	// dependency from base to numeric libraries.
	void get (const std::string & name, int32_t        & value);
	void get (const std::string & name, uint32_t       & value);
	void get (const std::string & name, double         & value);
	void get (const std::string & name, Matrix<double> & value);

	void set (const std::string & name, int32_t                value);
	void set (const std::string & name, uint32_t               value);
	void set (const std::string & name, double                 value);
	void set (const std::string & name, const Matrix<double> & value);
  };

  class SHARED NamedValueSet : public Metadata
  {
  public:
	void serialize (fl::Archive & archive, uint32_t version);
	static uint32_t serializeVersion;

	virtual void get (const std::string & name,       std::string & value);
	virtual void set (const std::string & name, const std::string & value);
	using Metadata::get;
	using Metadata::set;

	std::map<std::string, std::string> namedValues;
  };
  SHARED std::ostream & operator << (std::ostream & out, const NamedValueSet & data);
}


#endif
