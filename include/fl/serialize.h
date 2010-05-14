/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_serialize_h
#define fl_serialize_h


#include <stdio.h>
#include <string>
#include <map>
//#include <ext/hash_map>  // hash_map has different include paths on different systems.  The simplest solution is probably just to add it to the include path in Config
#include <typeinfo>


namespace fl
{
  /*  Initial work on archive objects.
  class ArchiveIn
  {
  public:
	ArchiveIn (std::istream & stream) : stream (stream) {};

	template<class T>
	void raw (T & data)
	{
	  stream.read ((char *) &T, sizeof (T));
	}

	std::istream & stream;
	std::vector<void *> pointers;
  };

  class ArchiveOut
  {
  public:
	ArchiveOut (std::ostream & stream) : stream (stream) {};

	template<class T>
	void raw (T & data)
	{
	  stream.write ((char *) &T, sizeof (T));
	}

	std::ostream & stream;
	std::hash_map<void *, int> pointers;
  };
  */

  /**
	 Defines the interface required of any class that expects to be stored
	 on a stream.  It is optional whether a serializable class actually
	 inherits from this class or not, because presently nothing in the
	 serialization machinery explicitly asks for an object of this class.
   **/
  class Serializable
  {
	Serializable ();  ///< A default constructor is mandatory.
	virtual void read  (std::istream & stream) = 0;
	virtual void write (std::ostream & stream) const = 0;
  };

  typedef void * productCreate ();
  typedef std::map<std::string, productCreate *> productMappingIn;
  typedef std::map<std::string, std::string>     productMappingOut;
  /**
	 Bundling both mappings in a single object makes it easier to instantiate
	 the registry in those case where it is necessary.
  **/
  struct productRegistry
  {
	productMappingIn  in;
	productMappingOut out;
  };

  static inline std::ostream & operator << (std::ostream & out, const productRegistry & data)
  {
	out << "in:";
	for (productMappingIn::const_iterator it = data.in.begin (); it != data.in.end (); it++)
	{
	  out << std::endl << "  " << it->first << " --> " << (void *) it->second;
	}
	out << std::endl << "out:";
	for (productMappingOut::const_iterator it = data.out.begin (); it != data.out.end (); it++)
	{
	  out << std::endl << "  " << it->first << " --> " << it->second;
	}
	return out;
  }

  /**
	 Manages the extraction of a polymorphic type from a stream.
	 This involves reading a special ID code that indicates which derived
	 class is actually stored, and then constructing an object of that
	 class.

	 <p>Design reasoning:
	 <ul>
	 <li>If a Factory constructs an object of a particular class, it
	 must inevitably refer to code of that class, which forces linkage.
	 Since one of the goals of this library is to minimize linkage of
	 unused modules, we require the client program to set up the Factory
	 by registering the desired classes.  The alternative is to pre-register
	 everything or hard-code a factory function.  These would force the
	 linkage of the entire class hierarchy.
	 <li>Factory and its helper class Product are meant to act as a kind
	 of "mix-in", so that factory behavior can be added to arbitrary
	 class hierarchies while imposing minimal requirements on the classes
	 themselves.
	 <li>There are separate Factories for each class hierarchy rather than
	 a single one shared by all classes.  This enables each hierarchy
	 to have its own ID code namespace, which in turn enables a very terse
	 set of IDs.
	 </ul>

	 <p>Should schema versioning be combined with Factory?  This would
	 require a record of the current default schema version and a read()
	 function on each class that takes the schema version as a parameter.
	 Also, the mapping between classes and IDs would need to be a function
	 of version.
   **/
  template<class B>
  class Factory
  {
  public:
	/**
	   Instantiates a named subclass.  This function serves two roles.
	   One is to act as a subroutine of read().  The other is to allow
	   users to instantiate named subclasses directly without necessarily
	   having a stream in hand.  This function does not fill-in any data
	   in the resulting object.  It is just the result of the default
	   constructor.
	 **/
	static B * create (const std::string & name)
	{
	  productMappingIn::iterator entry = registry.in.find (name);
	  if (entry == registry.in.end ())
	  {
		std::string error = "Unknown class name: ";
		error += name;
		throw error.c_str ();
	  }
	  return (B *) (*entry->second) ();
	}

	static B * read (std::istream & stream)
	{
	  std::string name;
	  getline (stream, name);
	  B * result = (B *) create (name);
	  result->read (stream);  // Since read() is virtual, this will call the appropriate stream extractor for the derived class.
	  return result;
	}

	static void write (std::ostream & stream, const B & data)
	{
	  std::string name = typeid (data).name ();
	  productMappingOut::iterator entry = registry.out.find (name);
	  if (entry == registry.out.end ())
	  {
		std::string error = "Attempt to write unregistered class: ";
		error += name;
		throw error.c_str ();
	  }
	  stream << entry->second << std::endl;
	  data.write (stream);  // Since "data" is a reference, this should call write() on the derived class rather than the base class.
	}

	static productRegistry registry;
  };
  template <class B> productRegistry Factory<B>::registry;

  template<class B, class D>  // "B" for base class, and "D" for derived class
  class Product
  {
  public:
	static void * create ()
	{
	  return new D;  // default constructor
	}

	static void add (const std::string & name = "")
	{
	  std::string typeidName = typeid (D).name ();

	  if (name.size ())
	  {
		Factory<B>::registry.in.insert  (make_pair (name,       &create));
		Factory<B>::registry.out.insert (make_pair (typeidName, name));
	  }
	  else
	  {
		// Search for a unique name.  This implementation is exceedingly
		// inefficient, but given that the number classes registered is
		// generally much less than 100, and that this is a one-time
		// process, the cost doesn't matter too much.
		char uniqueName[32];
		for (int i = 0; ; i++)
		{
		  sprintf (uniqueName, "%i", i);
		  if (Factory<B>::registry.in.find (uniqueName) == Factory<B>::registry.in.end ()) break;
		}

		Factory<B>::registry.in.insert  (make_pair (uniqueName, &create));
		Factory<B>::registry.out.insert (make_pair (typeidName, uniqueName));
	  }
	}
  };
}


#endif
