/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_factory_h
#define fl_factory_h

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <map>
#include <typeinfo>
#include <iostream>
#include <fstream>

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
	 <li>Factory acts as a kind of "mix-in", so that factory behavior can
	 be added to arbitrary class hierarchies while imposing minimal
	 requirements on the classes themselves.
	 <li>There are separate Factories for each class hierarchy rather than
	 a single one shared by all classes.  This enables each hierarchy
	 to have its own ID code namespace, which in turn enables a very terse
	 set of IDs.
	 </ul>
   **/
  template<class B>
  class Factory
  {
  public:
	/**
	   Instantiates a named subclass.  This function serves two roles.
	   One is to act as a subroutine of read().  The other is to allow
	   users to instantiate named subclasses directly without necessarily
	   having a stream in hand.  The result only contains data set by
	   the default constructor.
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

	static const char * classID (const B & data)
	{
	  std::string name = typeid (data).name ();
	  productMappingOut::iterator entry = registry.out.find (name);
	  if (entry == registry.out.end ())
	  {
		std::string error = "Attempt to use unregistered class: ";
		error += name;
		throw error.c_str ();
	  }
	  return entry->second.c_str ();
	}

	static void write (std::ostream & stream, const B & data)
	{
	  stream << classID (data) << std::endl;
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

	  // Remove any old mapping
	  productMappingOut::iterator outEntry = Factory<B>::registry.out.find (typeidName);
	  if (outEntry != Factory<B>::registry.out.end ())
	  {
		productMappingIn::iterator inEntry = Factory<B>::registry.in.find (outEntry->second);
		if (inEntry != Factory<B>::registry.in.end ()) Factory<B>::registry.in.erase (inEntry);
		Factory<B>::registry.out.erase (outEntry);
	  }

	  std::string uniqueName;
	  if (name.size ())
	  {
		uniqueName = name;
	  }
	  else
	  {
		// Search for a unique name.  This implementation is exceedingly
		// inefficient, but given that the number classes registered is
		// generally much less than 100, and that this is a one-time
		// process, the cost doesn't matter too much.
		char temp[32];
		for (int i = 0; ; i++)
		{
		  sprintf (temp, "%i", i);
		  if (Factory<B>::registry.in.find (temp) == Factory<B>::registry.in.end ()) break;
		}
		uniqueName = temp;
	  }

	  Factory<B>::registry.in.insert  (std::make_pair (uniqueName, &create));
	  Factory<B>::registry.out.insert (std::make_pair (typeidName, uniqueName));
	}
  };
}


#endif
