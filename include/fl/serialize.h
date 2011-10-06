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


#ifndef fl_serialize_h
#define fl_serialize_h

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

  class Archive;
  typedef void serializeFunction (void * me, Archive & archive, uint32_t version);

  class ClassDescription
  {
  public:
	productCreate *     create;
	serializeFunction * serialize;
	std::string         name;     ///< name to write to stream
	uint32_t            index;    ///< serial # of class in archive; 0xFFFFFFFF means not assigned yet
	uint32_t            version;
  };

  /**
	 Manages all bookkeeping needed to read and write object structures
	 on a stream.
	 This is not the most sophisticated serialization scheme.  It can't do
	 everything.  If you want to do everything, use Boost.
	 The rules for this method are:
	 * Everything is either a primitive type or a subclass of Serializable.
	 * Serializable is always polymorphic, with at least one virtual function:
	   serialize(Archive).
	 * Select STL classes get special treatment, and are effectively primitive:
	   string and vector.
	 * Serializables are numbered sequentially in the archive, starting at zero.
	 * Pointers are written as the index of the referenced Serializable.
	 * Just before a Serializable class is used for the first time, a record
	   describing it appears in the archive.  This record contains only
	   information that might be unknown at that point.  In particular,
	   a reference to Serializable will only cause a version number to be
	   written, while a pointer to Serializable will cause a class name to
	   be written, followed by a version number.
	 * Classes are numbered sequentially in the archive, starting at zero.
	 * If a pointer appears and its referenced Serializable has not yet
	   appeared, then the referenced Serializable appears immediately after.
	 * When a Serializable is written out to fulfill a pointer, the record
	   begins with a class index.
	 * No reference class members are allowed.  Therefore, a Serializable is
	   either already instantiated, or is instantiated based on its class
	   index (using a static factory function).
	 * To implement all this, every class'es serialize() function must begin
	   with a call that registers both the static factory function and the
	   version number of the class.
   **/
  class Archive
  {
  public:
	SHARED Archive (std::istream & in,  bool ownStream = false);
	SHARED Archive (std::ostream & out, bool ownStream = false);
	SHARED Archive (const std::string & fileName, const std::string & mode);
	SHARED ~Archive ();

	SHARED void open (std::istream & in, bool ownStream = false);
	SHARED void open (std::ostream & out, bool ownStream = false);
	SHARED void open (const std::string & fileName, const std::string & mode);
	SHARED void close ();

	/**
	   Create a class description record in memory.
	   Only use for classes that can be instantiated (have a default
	   constructor and are not abstract).  Note that all classes that are
	   serialized polymorphically must be intantiable.
	   @param name Specifies the string actually stored in the stream that
	   identifies this class.  Can be called several times with different
	   values.  When reading a stream, all values act as aliases for the same
	   class.  When writing a stream, only the value given in the last call
	   is used.  If no value is given, defaults to the RTTI name of the class.
	   This isn't necessarily the best thing, because RTTI names can vary
	   between compilers, and even between versions of the same compiler.
	 **/
	template<class T>
	void registerClass (const std::string & name = "")
	{
	  // Inner-class used to lay down a function that can construct an object
	  // of type T.
	  struct Product
	  {
		static void * create ()
		{
		  return new T;  // default constructor
		}
		static void serialize (void * me, Archive & archive, uint32_t version)
		{
		  ((T *) me)->serialize (archive, version);
		}
	  };

	  // Create or locate basic registration record
	  std::string typeidName = typeid (T).name ();
	  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
	  if (c == classesOut.end ())
	  {
		ClassDescription * info = new ClassDescription;
		info->index   = 0xFFFFFFFF;
		info->version = T::serializeVersion;
		c = classesOut.insert (make_pair (typeidName, info)).first;
	  }
	  c->second->create    = Product::create;
	  c->second->serialize = Product::serialize;
	  c->second->name      = (name.size () == 0) ? typeidName : name;

	  // Add or reassign alias for initial lookup when reading from stream
	  alias.insert (make_pair (c->second->name, c->second));
	}

	template<class T>
	Archive & operator & (T & data)
	{
	  // Auto-register if necessary
	  std::string typeidName = typeid (T).name ();  // Because reference members are not allowed, we assume that T is exactly the class we are working with, rather than a parent thereof.
	  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
	  if (c == classesOut.end ())
	  {
		ClassDescription * info = new ClassDescription;
		info->create    = 0;
		info->serialize = 0;
		info->name      = typeidName;
		info->index     = 0xFFFFFFFF;
		info->version   = T::serializeVersion;
		c = classesOut.insert (make_pair (typeidName, info)).first;
		// No need for alias, unless class is also treated polymorphically.
	  }

	  // Assign class index if necessary
	  if (c->second->index == 0xFFFFFFFF)
	  {
		c->second->index = classesIn.size ();
		classesIn.push_back (c->second);
		// Serialize the class description.  Only need version #.  Don't need
		// class name because it is implicit.
		(*this) & c->second->version;
	  }

	  // Record the pointer
	  if (in)
	  {
		pointersIn.push_back (&data);
	  }
	  else
	  {
		if (pointersOut.find (&data) == pointersOut.end ())
		{
		  pointersOut.insert (make_pair (&data, pointersOut.size ()));
		}
	  }

	  data.serialize (*this, c->second->version);
	  return *this;
	}

	template<class T>
	Archive & operator & (T * & data)
	{
	  if (in)
	  {
		uint32_t pointer;
		(*this) & pointer;
		if (pointer > pointersIn.size ()) throw "pointer index out of range in archive";
		if (pointer < pointersIn.size ()) data = (T *) pointersIn[pointer];
		else  // new pointer, so serialize associated object
		{
		  uint32_t classIndex;
		  (*this) & classIndex;
		  if (classIndex >  classesIn.size ()) throw "class index out of range in archive";
		  if (classIndex == classesIn.size ())  // new class, so deserialize class info
		  {
			std::string name;
			(*this) & name;

			std::map<std::string, ClassDescription *>::iterator a = alias.find (name);
			if (a == alias.end ()) throw "Class needs to be registered before first occurrence in archive.";
			a->second->index = classIndex;
			classesIn.push_back (a->second);

			(*this) & a->second->version;
		  }

		  ClassDescription * d = classesIn[classIndex];
		  if (d->create    == 0) throw "create() has not been registered";
		  if (d->serialize == 0) throw "serialize() has not been registered";
		  data = (T *) d->create ();
		  pointersIn.push_back (data);
		  d->serialize (data, *this, d->version);
		}
	  }
	  else
	  {
		std::map<void *, uint32_t>::iterator p = pointersOut.find (data);
		if (p != pointersOut.end ()) (*this) & p->second;
		else
		{
		  p = pointersOut.insert (std::make_pair (data, pointersOut.size ())).first;
		  (*this) & p->second;

		  std::string typeidName = typeid (*data).name ();
		  std::map<std::string, ClassDescription *>::iterator c = classesOut.find (typeidName);
		  if (c == classesOut.end ()) throw "Attempt to write unregistered polymorphic class";
		  if (c->second->index < 0xFFFFFFFF) (*this) & c->second->index;
		  else
		  {
			c->second->index = classesIn.size ();
			classesIn.push_back (c->second);
			(*this) & c->second->index;
			(*this) & c->second->name;
			(*this) & c->second->version;
		  }

		  c->second->serialize (data, *this, c->second->version);
		}
	  }
	  return *this;
	}

	template<class T>
	Archive & operator & (std::vector<T> & data)
	{
	  uint32_t count = data.size ();
	  (*this) & count;
	  if (in)
	  {
		if (in->bad ()) throw "stream bad";
		data.reserve (data.size () + count);
		for (int i = 0; i < count; i++)
		{
		  T temp;
		  (*this) & temp;
		  data.push_back (temp);
		}
	  }
	  else
	  {
		for (int i = 0; i < count; i++) (*this) & data[i];
	  }
	  return (*this);
	}

	std::istream * in;
	std::ostream * out;
	bool ownStream;

	std::vector<void *>        pointersIn;   ///< mapping from serial # to pointer
	std::map<void *, uint32_t> pointersOut;  ///< mapping from pointer to serial #; @todo change this to unordered_map when broadly available

	std::vector<ClassDescription *>           classesIn;   ///< mapping from serial # to class description; ClassDescription objects are held by classesOut
	std::map<std::string, ClassDescription *> classesOut;  ///< mapping from RTTI name to class description; one-to-one
	std::map<std::string, ClassDescription *> alias;       ///< mapping from user-assigned name to class description; can be many-to-one
  };

  // These are necessary, because any type that does not have an explicit
  // operator&() function must have a serialize() function.  The programmer
  // may define others locally as needed.
  template<> SHARED Archive & Archive::operator & (std::string    & data);
  template<> SHARED Archive & Archive::operator & (uint8_t        & data);
  template<> SHARED Archive & Archive::operator & (uint16_t       & data);
  template<> SHARED Archive & Archive::operator & (uint32_t       & data);
  template<> SHARED Archive & Archive::operator & (uint64_t       & data);
  template<> SHARED Archive & Archive::operator & (int8_t         & data);
  template<> SHARED Archive & Archive::operator & (int16_t        & data);
  template<> SHARED Archive & Archive::operator & (int32_t        & data);
  template<> SHARED Archive & Archive::operator & (int64_t        & data);
  template<> SHARED Archive & Archive::operator & (float          & data);
  template<> SHARED Archive & Archive::operator & (double         & data);
  template<> SHARED Archive & Archive::operator & (bool           & data);
}


#endif
