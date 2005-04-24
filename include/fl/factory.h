/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See LICENSE-UIUC
for details.
*/


#ifndef fl_factory_h
#define fl_factory_h


#include <string>
#include <map>


namespace fl
{
  template<class B>
  class Factory
  {
  public:
	typedef B * productFunction (std::istream & stream);
	typedef std::map<std::string, productFunction *> productMap;

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
	  return (*entry->second) (stream);
	}

	static productMap products;
  };

  template<class B, class C>
  class Product
  {
  public:
	static B * read (std::istream & stream)
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
