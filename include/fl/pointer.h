#ifndef pointer_h
#define pointer_h


#include <stdlib.h>
#include <string.h>
#include <ostream>


namespace fl
{
  // Keeps track of a block of memory, which can be shared by multiple objects
  // and multiple threads.  The block can either be managed by Pointer, or
  // it can belong to any other part of the system.  Only managed blocks get
  // reference counting, automatic deletion, and reallocation.
  // This class is intended to be thread-safe, but in its current state it is
  // not.  Need to implement a semaphore
  // of some sort for the reference count.  IE: use an atomic operation such
  // as XADD that includes an exchange.  Want to avoid using pthreads so we
  // don't have to link another library.
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
	void copyFrom (const Pointer & that)
	{
	  if (that.memory != memory)
	  {
		if (! that.memory)
		{
		  detach ();
		}
		else
		{
		  int size = that.size ();
		  if (size < 0)
		  {
			throw "Don't know size of block to copy";
		  }
		  grow (size);
		  memcpy (memory, that.memory, size);
		}
	  }
	}
	void copyFrom (const void * that, int size)
	{
	  if (size <= 0)
	  {
		detach ();
	  }
	  else if (that != memory)
	  {
		grow (size);
		memcpy (memory, that, size);
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
	void attach (const Pointer & that)
	{
	  // This method is protected because it assumes that we aren't responsible
	  // for our memory.  We must guarantee that we don't own memory when this
	  // method is called.  This is true just after a call to detach, as well
	  // as a few other situations.
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
		(int *) memory += 2;
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
	void * memory;  // Pointer to block in heap.  Must cast as needed.
	// metaData < 0 indicates memory is a special pointer we constructed.
	// There is meta data associated with the pointer, and all "smart" pointer
	// functions are available.  This is the only time that we can (and must)
	// delete memory.
	// metaData == 0 indicates either memory == 0 or we don't know how big the
	// block is.
	// metaData > 0 indicates the actual size of the block, and that we don't
	// own it.
	int metaData;
  };

  inline std::ostream &
  operator << (std::ostream & stream, const Pointer & pointer)
  {
	stream << "[" << pointer.memory << " " << pointer.size () << " " << pointer.refcount () << "]";
	//stream << "[" << &pointer << ": " << pointer.memory << " " << pointer.size () << " " << pointer.refcount () << "]";
	return stream;
  }


  // SmartPointer is like Pointer, except that it works with a known structure,
  // and therefore a fixed amount of memory.  In order to use this template,
  // the wrapped class must have a default constructor (ie: one that takes no
  // arguments).
  template<class T>
  class SmartPointer
  {
  public:
	SmartPointer ()
	{
	  memory = 0;
	}

	SmartPointer (const SmartPointer & that)
	{
	  attach (that.memory);
	}

	~SmartPointer ()
	{
	  detach ();
	}

	void initialize ()
	{
	  if (! memory)
	  {
		memory = new SmartBlock;
		memory->refcount = 1;
	  }
	}

	SmartPointer & operator = (const SmartPointer & that)
	{
	  if (that.memory != memory)
	  {
		detach ();
		attach (that.memory);
	  }
	  return *this;
	}

	void copyFrom (const SmartPointer & that)
	{
	  if (that.memory != memory)
	  {
		if (! that.memory)
		{
		  detach ();
		}
		else
		{
		  initialize ();
		  memory->object = that.memory->object;  // May or may not be a deep copy
		}
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

	struct SmartBlock
	{
	  T object;
	  int refcount;
	};
	SmartBlock * memory;

  protected:
	// attach assumes that memory == 0, so we must protect it.
	void attach (SmartBlock * that)
	{
	  memory = that;
	  if (memory)
	  {
		memory->refcount++;
	  }
	}
  };
}


#endif
