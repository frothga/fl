/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4, 1.6, 1.7 Copyright 2005 Sandia Corporation.
Revisions 1.9 thru 1.11 Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.11  2007/03/23 11:38:06  Fred
Correct which revisions are under Sandia copyright.

Revision 1.10  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.9  2006/02/25 23:16:22  Fred
Renamed SmartPointer to PointerStruct.  Created a new PointerPoly class which
manages a pointer to a reference counted object.  PointerPoly is the closest
thing to what most people consider a "smart pointer".  The "poly" refers to the
fact that the object can be of any class that is derived from the one given in
the template parameter.

Revision 1.8  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.7  2005/10/09 03:29:18  Fred
Update revision history and add Sandia copyright notice.

Revision 1.6  2005/04/23 21:04:24  Fred
Add == and != operators to support their namesake operators in Image.  Checks
for equality based on metadata rather than on actual contents of memory block.

Revision 1.5  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.4  2005/01/22 20:47:03  Fred
MSVC compilability fix:  Fix complaint about lvalue.

Revision 1.3  2003/12/30 16:23:05  rothgang
Convert comments to doxygen format.

Revision 1.2  2003/07/09 14:58:01  rothgang
Changed copyFrom() to allow self-duplication.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef pointer_h
#define pointer_h


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ostream>


namespace fl
{
  /*  Preliminary scratchings on atomic operations.
#if defined (__GNUC__)  &&  defined (__i386__)
#elif defined (_MSC_VER)
__declspec(naked) int __fastcall Xadd (volatile int* pNum, int val)
{
    __asm
    {
        lock xadd dword ptr [ECX], EDX
        mov EAX, EDX
        ret
    }
}
#else
  int atomicInc (int * a)
  {
	return ++(*a);
  }

  int atomicDec (int * a)
  {
	return --(*a);
  }
#endif
  */

  /**
	 Keeps track of a block of memory, which can be shared by multiple objects
	 and multiple threads.  The block can either be managed by Pointer, or
	 it can belong to any other part of the system.  Only managed blocks get
	 reference counting, automatic deletion, and reallocation.
  **/
  class Pointer
  {
  public:
	Pointer ()
	{
	  memory = 0;
	  metaData = 0;
	}
	Pointer (const Pointer & that)
	{
	  attach (that);
	}
	Pointer (void * that, int size = 0)
	{
	  // It's a real bad idea to pass size < 0 to this constructor!
	  memory = that;
	  metaData = size;
	}
	Pointer (int size)
	{
	  if (size > 0)
	  {
		allocate (size);
	  }
	  else
	  {
		memory = 0;
		metaData = 0;
	  }
	}
	~Pointer ()
	{
	  detach ();
	}

	Pointer & operator = (const Pointer & that)
	{
	  if (that.memory != memory)
	  {
		detach ();
		attach (that);
	  }
	  return *this;
	}
	Pointer & operator = (void * that)
	{
	  attach (that);
	  return *this;
	}
	void attach (void * that, int size = 0)
	{
	  detach ();
	  memory = that;
	  metaData = size;
	}
	void copyFrom (const Pointer & that)  ///< decouple from memory held by that.  "that" could also be this.
	{
	  if (that.memory)
	  {
		Pointer temp (that);  // force refcount up on memory block
		if (that.memory == memory)  // the enemy is us...
		{
		  detach ();
		}
		int size = temp.size ();
		if (size < 0)
		{
		  throw "Don't know size of block to copy";
		}
		grow (size);
		memcpy (memory, temp.memory, size);
	  }
	  else
	  {
		detach ();
	  }
	}
	void copyFrom (const void * that, int size)
	{
	  if (size > 0)
	  {
		if (that == memory)
		{
		  detach ();
		}
		grow (size);
		memcpy (memory, that, size);
	  }
	  else
	  {
		detach ();
	  }
	}

	void grow (int size)
	{
	  if (metaData < 0)
	  {
		if (((int *) memory)[-2] >= size)
		{
		  return;
		}
		detach ();
	  }
	  else if (metaData >= size)  // note:  metaData >= 0 at this point
	  {
		return;
	  }
	  if (size > 0)
	  {
		allocate (size);
	  }
	}
	void clear ()  // Erase block of memory
	{
	  if (metaData < 0)
	  {
		memset (memory, 0, ((int *) memory)[-2]);
	  }
	  else if (metaData > 0)
	  {
		memset (memory, 0, metaData);
	  }
	  else
	  {
		throw "Don't know size of block to clear";
	  }
	}

	int refcount () const
	{
	  if (metaData < 0)
	  {
		return ((int *) memory)[-1];
	  }
	  return -1;
	}
	int size () const
	{
	  if (metaData < 0)
	  {
		return ((int *) memory)[-2];
	  }
	  else if (metaData > 0)
	  {
		return metaData;
	  }
	  return -1;
	}

	template <class T>
	operator T * () const
	{
	  return (T *) memory;
	}

	bool operator == (const Pointer & that) const
	{
	  return memory == that.memory;
	}
	bool operator != (const Pointer & that) const
	{
	  return memory != that.memory;
	}

	// Functions mainly for internal use

	void detach ()
	{
	  if (metaData < 0)
	  {
		if (--((int *) memory)[-1] == 0)
		{
		  free ((int *) memory - 2);
		}
	  }
	  memory = 0;
	  metaData = 0;
	}

  protected:
	/**
	   This method is protected because it assumes that we aren't responsible
	   for our memory.  We must guarantee that we don't own memory when this
	   method is called.  This is true just after a call to detach, as well
	   as a few other situations.
	**/
	void attach (const Pointer & that)
	{
	  memory = that.memory;
	  metaData = that.metaData;
	  if (metaData < 0)
	  {
		((int *) memory)[-1]++;
	  }
	}
	void allocate (int size)
	{
	  memory = malloc (size + 2 * sizeof (int));
	  if (memory)
	  {
		memory = & ((int *) memory)[2];
		((int *) memory)[-1] = 1;
		((int *) memory)[-2] = size;
		metaData = -1;
	  }
	  else
	  {
		metaData = 0;
		throw "Unable to allocate memory";
	  }
	}

  public:
	void * memory;  ///< Pointer to block in heap.  Must cast as needed.
	/**
	   metaData < 0 indicates memory is a special pointer we constructed.
	   There is meta data associated with the pointer, and all "smart" pointer
	   functions are available.  This is the only time that we can (and must)
	   delete memory.
	   metaData == 0 indicates either memory == 0 or we don't know how big the
	   block is.
	   metaData > 0 indicates the actual size of the block, and that we don't
	   own it.
	**/
	int metaData;
  };

  inline std::ostream &
  operator << (std::ostream & stream, const Pointer & pointer)
  {
	stream << "[" << pointer.memory << " " << pointer.size () << " " << pointer.refcount () << "]";
	//stream << "[" << &pointer << ": " << pointer.memory << " " << pointer.size () << " " << pointer.refcount () << "]";
	return stream;
  }


  /**
	 Like Pointer, except that it works with a known structure,
	 and therefore a fixed amount of memory.  In order to use this template,
	 the wrapped class must have a default constructor (ie: one that takes no
	 arguments).
  **/
  template<class T>
  class PointerStruct
  {
  public:
	PointerStruct ()
	{
	  memory = 0;
	}

	PointerStruct (const PointerStruct & that)
	{
	  attach (that.memory);
	}

	~PointerStruct ()
	{
	  detach ();
	}

	void initialize ()
	{
	  if (! memory)
	  {
		memory = new RefcountBlock;
		memory->refcount = 1;
	  }
	}

	PointerStruct & operator = (const PointerStruct & that)
	{
	  if (that.memory != memory)
	  {
		detach ();
		attach (that.memory);
	  }
	  return *this;
	}

	void copyFrom (const PointerStruct & that)
	{
	  if (that.memory)
	  {
		PointerStruct temp (that);
		detach ();
		initialize ();
		memory->object = temp.memory->object;  // May or may not be a deep copy
	  }
	  else
	  {
		detach ();
	  }
	}

	int refcount () const
	{
	  if (memory)
	  {
		return memory->refcount;
	  }
	  return -1;
	}

	T * operator -> () const
	{
	  return & memory->object;  // Will cause a segment violation if memory == 0
	}

	T & operator * () const
	{
	  return memory->object;  // Ditto
	}

	void detach ()
	{
	  if (memory)
	  {
		if (--(memory->refcount) == 0)
		{
		  delete memory;
		}
		memory = 0;
	  }
	}

	struct RefcountBlock
	{
	  T object;
	  int refcount;
	};
	RefcountBlock * memory;

  protected:
	/**
	   attach assumes that memory == 0, so we must protect it.
	 **/
	void attach (RefcountBlock * that)
	{
	  memory = that;
	  if (memory)
	  {
		memory->refcount++;
	  }
	}
  };


  /**
	 Interface the objects held by PointerPoly must implement.  It is not
	 mandatory that such objects inherit from this class, but if they do
	 then they are guaranteed to satisfy the requirements of PointerPoly.
	 When a ReferenceCounted is first constructed, its count is zero.
	 When PointerPolys attach to or detach from it, they update the count.
	 When the last PointerPoly detaches, it destructs this object.
   **/
  class ReferenceCounted
  {
  public:
	ReferenceCounted () {PointerPolyReferenceCount = 0;}
	mutable int PointerPolyReferenceCount;  ///< The number of PointerPolys that are attached to this instance.
  };
  

  /**
	 Keeps track of an instance of a polymorphic class.  Similar to Pointer
	 and PointerStruct.  Expects that the wrapped object be an instance of
	 ReferenceCounted or at least implement the same interface.

	 <p>Note that this class does not have a copyFrom() function, because it
	 would require adding a duplicate() function to the ReferenceCounted
	 interface.  (We don't know the exact class of "memory", so we don't know
	 how to contruct another instance of it a priori.  A duplicate() function
	 encapsulates this knowledge in the wrapped class itself.)  That may be a
	 reasonable thing to do, but it would also adds to the burden of using
	 this tool.  On the other hand, it is simple enough to duplicate the object
	 in client code.
  **/
  template<class T>
  class PointerPoly
  {
  public:
	PointerPoly ()
	{
	  memory = 0;
	}

	PointerPoly (const PointerPoly & that)
	{
	  memory = 0;
	  attach (that.memory);
	}

	PointerPoly (T * that)
	{
	  memory = 0;
	  attach (that);
	}

	~PointerPoly ()
	{
	  detach ();
	}

	PointerPoly & operator = (const PointerPoly & that)
	{
	  if (that.memory != memory)
	  {
		detach ();
		attach (that.memory);
	  }
	  return *this;
	}

	PointerPoly & operator = (T * that)
	{
	  if (that != memory)
	  {
		detach ();
		attach (that);
	  }
	  return *this;
	}

	int refcount () const
	{
	  if (memory)
	  {
		return memory->PointerPolyReferenceCount;
	  }
	  return -1;
	}

	T * operator -> () const
	{
	  assert (memory);
	  return memory;
	}

	T & operator * () const
	{
	  assert (memory);
	  return *memory;
	}

	template <class T2>
	operator T2 * () const
	{
	  return dynamic_cast<T2 *> (memory);
	}

	bool operator == (const PointerPoly & that) const
	{
	  return memory == that.memory;
	}

	bool operator == (const T * that) const
	{
	  return memory == that;
	}

	bool operator != (const PointerPoly & that) const
	{
	  return memory != that.memory;
	}

	bool operator != (const T * that) const
	{
	  return memory != that;
	}

	/**
	   Binds to the given pointer.  This method should only be called when
	   we are not currently bound to anything.  This class is coded so that
	   this condition is always true when this function is called internally.
	 **/
	void attach (T * that)
	{
	  assert (memory == 0);
	  memory = that;
	  if (memory)
	  {
		memory->PointerPolyReferenceCount++;
	  }
	}

	void detach ()
	{
	  if (memory)
	  {
		assert (memory->PointerPolyReferenceCount > 0);
		if (--(memory->PointerPolyReferenceCount) == 0)
		{
		  delete memory;
		}
		memory = 0;
	  }
	}

	T * memory;
  };
}


#endif
