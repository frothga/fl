/*
Author: Fred Rothganger

Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef neighbor_h
#define neighbor_h


#include "fl/matrix.h"

#include <vector>

#undef SHARED
#ifdef _MSC_VER
#  ifdef flNumeric_EXPORTS
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
	 Retrieves items in space near a given query.
   **/
  class SHARED Neighbor
  {
  public:
	virtual ~Neighbor ();

	static uint32_t serializeVersion;
	void serialize (Archive & archive, uint32_t version);

	/**
	   Prepare internal structures for fast retrieval of neighbors.
	   @param data We do not take ownership of the collection, but expect the
	   individual items to live as long as this object does.  Even though they
	   are MatrixAbstract, we expect the values for a vector to be contiguous
	   in memory, such that we can retrieve only the zeroeth element and
	   iterate over the others with a pointer.
	**/
	virtual void set  (const std::vector<MatrixAbstract<float> *> & data) = 0;
	virtual void find (const MatrixAbstract<float> & query, std::vector<MatrixAbstract<float> *> & result) const = 0;

	/**
	   Helper class for storing an arbitrary object along with the vector.
	 **/
	class SHARED Entry : public MatrixAbstract<float>
	{
	public:
	  Entry (MatrixAbstract<float> * point, void * item);

	  virtual MatrixAbstract<float> * clone (bool deep = false) const;
	  virtual float & operator () (const int row, const int column) const
	  {
		return (*point)(row,column);
	  }
	  virtual float & operator [] (const int row) const
	  {
		return (*point)[row];
	  }
	  virtual int rows () const;
	  virtual int columns () const;
	  virtual void resize (const int rows, const int columns = 1);

	  MatrixAbstract<float> * point;
	  void * item;
	};
  };


  // KD Tree ------------------------------------------------------------------

  /**
	 An implementation based loosely on the paper "Algorithms for Fast Vector
	 Quantization" by Sunil Arya and David Mount.
   **/
  class SHARED KDTree
  {
  public:
	KDTree ();
	virtual ~KDTree ();
	void clear ();

	void serialize (Archive & archive, uint32_t version);

	virtual void set  (const std::vector<MatrixAbstract<float> *> & data);
	virtual void find (const MatrixAbstract<float> & query, std::vector<MatrixAbstract<float> *> & result) const;

	class Query
	{
	public:
	  int k;
	  float oneEpsilon;  ///< (1+epsilon)^2
	  const MatrixAbstract<float> * point;
	  std::multimap<float, MatrixAbstract<float> *> sorted;
	};

	class Node
	{
	public:
	  virtual ~Node ();
	  virtual void search (float distance, Query & q) const = 0;
	};

	class Branch : public Node
	{
	public:
	  virtual ~Branch ();
	  virtual void search (float distance, Query & q) const;

	  int dimension;
	  float lo;  ///< Lowest value along the dimension
	  float hi;  ///< Highest value along the dimension
	  float mid;  ///< The cut point along the dimension
	  Node * lowNode;  ///< bleow mid
	  Node * highNode;  ///< above mid
	};

	class Leaf : public Node
	{
	public:
	  virtual void search (float distance, Query & q) const;

	  std::vector<MatrixAbstract<float> *> points;
	};

	Node * construct (std::vector<MatrixAbstract<float> *> & points);  ///< Recursively construct a tree that handles the given volume of points.
	void   sort      (std::vector<MatrixAbstract<float> *> & points, int dimension);  ///< rearrange points so they are in ascending order along the given dimension

	Node * root;
	Vector<float> lo;
	Vector<float> hi;

	int bucketSize;
	int k;
	float epsilon;  ///< We prune the search when the nearest rectangle is farther than (1+epsilon).
  };
}


#endif
