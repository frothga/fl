/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
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
