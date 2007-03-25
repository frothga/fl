/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6 Copyright 2007 Sandia Corporation.
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


#ifndef fl_factory_h
#define fl_factory_h


#include <string>
#include <map>


namespace fl
{
  typedef void * productFunction (std::istream & stream);
  typedef std::map<std::string, productFunction *> productMap;

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
	 themselves.  An alternative is to encode the requirements in a
	 Product interface that the classes must implement.  The current
	 requirement is only that the classes provide a stream constructor
	 C::C (istream &).  A possible additional requirement is that each
	 class provide its own ID code.  A better arrangement might be to
	 retain the helper class Product and add an Serializable interface
	 that clarifies the requirements on classes without adding
	 implementation burden.
	 <li>There are separate Factories for each class hierarchy rather than
	 a single one shared by all classes.  This enables each hierarchy
	 to have its own ID code namespace, which in turn enables a very terse
	 set of IDs.  Unfortunately, the current setup doesn't use this
	 ability, but instead writes a rather lengthy and globally unique
	 string.
	 </ul>

	 <p>Should schema versioning be combined with Factory?  This would
	 require a record of the current default schema version and a read()
	 function on each class that takes the schema version as a parameter.
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

	static productMap products;
  };
  template <class B> productMap Factory<B>::products;

  template<class B, class C>
  class Product
  {
  public:
	static void * read (std::istream & stream)
	{
	  return new C (stream);
	}
	static void add ()
	{
	  Factory<B>::products.insert (make_pair (typeid (C).name (), &read));
	}
  };
}


#endif
