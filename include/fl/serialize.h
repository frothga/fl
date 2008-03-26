/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6 Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.6  2007/03/25 14:57:58  Fred
Fix template instantiation issues:  Add a template for the static member
Factory::products.  Make productFunction and productMap types non-templated
(by returning a void * rather than a specific type) in order to simplify
template structure the some compilers were choking on.

Revision 1.5  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.4  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.3  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.2  2004/08/30 00:06:08  rothgang
Include class name in exception message.

Revision 1.1  2004/04/19 17:03:08  rothgang
Create template for extracting polymorphic objects from a stream.
-------------------------------------------------------------------------------
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
	virtual void read  (std::istream stream) = 0;
	virtual void write (std::ostream stream) const = 0;
  };

  typedef void * productFunction (std::istream & stream);
  typedef std::map<std::string, productFunction *> productMap;
  typedef std::map<std::string, std::string> productIndex;

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
	 <li>Factory and its helper class Product were meant to act as a kind
	 of "mix-in", so that factory behavior could be added to arbitrary
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
	static B * read (std::istream & stream)
	{
	  std::string name;
	  getline (stream, name);
	  typename productMap::iterator entry = products.find (name);
	  if (entry == products.end ())
	  {
		std::string error = "Unknown class name in stream: ";
		error += name;
		throw error.c_str ();
	  }
	  return (B *) (*entry->second) (stream);
	}

	static void write (std::ostream & stream, const B & data)
	{
	  productIndex::iterator it = index.find (typeid (B).name ());
	  if (it == index.end ()) throw "Class not found in Factory::index";
	  stream << it->second << std::endl;
	  data.write (stream);
	}

	static productMap products;
	static productIndex index;
  };
  template <class B> productMap   Factory<B>::products;
  template <class B> productIndex Factory<B>::index;

  template<class B, class C>
  class Product
  {
  public:
	static void * read (std::istream & stream)
	{
	  C * result = new C;  // default constructor
	  result->read (stream);
	  return result;
	}

	static void add (const std::string & name = "")
	{
	  if (name.size ())
	  {
		Factory<B>::products.insert (make_pair (name, &read));
		Factory<B>::index.insert (make_pair (typeid (B).name (), name));
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
		  if (Factory<B>::products.find (uniqueName) == Factory<B>::products.end ()) break;
		}

		Factory<B>::products.insert (make_pair (uniqueName, &read));
		Factory<B>::index.insert (make_pair (typeid (B).name (), name));
	  }
	}
  };
}


#endif
