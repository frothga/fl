/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.10, 1.11, 1.13 thru 1.17 Copyright 2005 Sandia Corporation.
Revisions 1.19 thru 1.21             Copyright 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.21  2007/03/23 11:38:05  Fred
Correct which revisions are under Sandia copyright.

Revision 1.20  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.19  2006/02/25 23:11:48  Fred
Renamed SmartPointer to PointerStruct as part of a rework of pointer.h.

Revision 1.18  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.17  2005/10/09 03:26:43  Fred
Update revision history and add Sandia copyright notice.

Move stream operators and Matrix2x2::geev() into appropriate .tcc files.

Add operator * to MatrixTranspose.

Revision 1.16  2005/09/12 03:57:21  Fred
Add comment to revision history.

Revision 1.15  2005/09/11 23:03:28  Fred
Make operator ! a member of Matrix and add the operator to MatrixAbstract and
Matrix3x3 as well.

Revision 1.14  2005/09/10 17:27:19  Fred
Add detail to revision history.  Add Sandia copyright.  This will need to be
updated with license info before release.

Revision 1.13  2005/08/07 03:10:08  Fred
GCC 3.4 compilability fix: explicitly specify "this" for inherited member
variables in a template.

Revision 1.12  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.11  2005/01/22 20:45:59  Fred
MSVC compilability fix:  Change interface of frob() to be float rather than
templated type.  Make Matrix2x2::geev(complex) a nested template rather than an
explicit template specialization in order to avoid mutliple definitions.

Revision 1.10  2005/01/12 04:59:18  rothgang
Use math.h to add functions (sqrt, pow) to the std namespace to ensure that all
common numeric types are covered.

Revision 1.9  2004/08/30 00:04:56  rothgang
Add sumSquares() and elementMultiply().

Revision 1.8  2004/06/22 21:26:41  rothgang
Remove unecessary negation in Matrix2x2::geev().

Revision 1.7  2004/05/10 20:12:57  rothgang
Switch from using C99 style complex numbers to STL complex template.

Revision 1.6  2004/04/19 16:34:46  rothgang
Get rid of factory method.  This is now handled by factory template.

Revision 1.5  2003/12/30 16:30:30  rothgang
Change default value of withName to false.  Convert comments to doxygen format.
Add MatrixSparse::frob().  Add Matrix::detach().  Still have an issue with
outputting complex numbers to a human readable stream.

Revision 1.4  2003/09/07 22:19:56  rothgang
Add real read and write methods to MatrixPacked.

Revision 1.3  2003/08/11 14:17:56  rothgang
Changed interface for clear() so that it can take an arbitrary scalar rather
than just setting everything to zero.  Added geev from Matrix2x2<complex
double>.

Revision 1.2  2003/07/09 14:53:53  rothgang
Changed which methods have inline implementations in the header file.  Mainly,
the element accessors () and [] should be inlineable.  Also added a template
constructor to each class that allows type as well as structural conversion.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_matrix_h
#define fl_matrix_h


#include "fl/pointer.h"
#include "fl/math.h"

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <complex>


// The linear algebra package has the following goals:
// * Be simple and straightforward for a programmer to use.  It should be
//   easy to express most common linear algebra calculations using
//   overloaded operators.
// * Work seamlessly with LAPACK.  To this end, storage is always column
//   major.
// * Be lightweight to compile.  We avoid putting much implementation in
//   the headers, even though we are using templates.
// * Be lightweight at run-time.  Eg: shallow copy semantics, and only a
//   couple of variables that need to be copied.  Should limit total copying
//   to 16 bytes or less.

// Other notes:
// * In general, the implementation does not protect you from shooting yourself
//   in the foot.  Specifically, there is no range checking or verification
//   that memory addresses are valid.  All these do is make a bug easier to
//   find (rather than eliminate it), and they cost at runtime.  In cases
//   where there is some legitimate interpretation of bizarre parameter values,
//   we assume the programmer meant that interpretation and plow on.


namespace fl
{
  // Forward declarations
  template<class T> class Matrix;
  template<class T> class MatrixTranspose;
  template<class T> class MatrixRegion;


  // Matrix general interface -------------------------------------------------

  /**
	 We reserve the name "Matrix" for a dense matrix, rather than for the
	 abstract type.  This makes coding a little prettier, since dense
	 matrices are the most common case.
  **/
  template<class T>
  class MatrixAbstract
  {
  public:
	virtual ~MatrixAbstract ();

	// Assignment should be shallow copy when possible, but may be deep copy
	// if convenient to the class.  Tip: never rely on shallow copy semantics.
	// Ie: never expect a change in one matrix to be reflected in another.
	// Also never rely on deep copy semantics unless it is guaranteed by a
	// function.
	// No assignment operator is defined at this level because it would have
	// bad interactions with automatically generated assignment operators
	// in derived classes.

	// Structural functions
	// These are the core functions in terms of which most other functions can
	// be implemented.  To some degree, they abstract away the actual storage
	// structure of the matrix.
	virtual T & operator () (const int row, const int column) const = 0;  ///< Element accesss
    virtual T & operator [] (const int row) const;  ///< Element access, treating us as a vector.
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract * duplicate () const = 0; ///< Make a new instance of self on the heap, with shallow copy semantics.  Used for views.  Since this is class sensitive, it must be overridden.
	virtual void clear (const T scalar = (T) 0);  ///< Set all elements to given value.
	virtual void resize (const int rows, const int columns = 1) = 0;  ///< Change number of rows and columns.  Does not preserve data.

	// Higher level functions
	virtual T frob (float n) const;  ///< Generalized Frobenius norm: (sum_elements (element^n))^(1/n).  Effectively: INFINITY is max, 1 is sum, 2 is standard Frobenius norm
	virtual T sumSquares () const;  ///< Similar to frob(2), but without taking the square root.
	virtual void normalize (const T scalar = 1.0);  ///< View matrix as vector and adjust so frob (2) == scalar.
	virtual T dot (const MatrixAbstract & B) const;  ///< View both matrices as vectors and return dot product.  Ie: returns the sum of the products of corresponding elements.
	virtual Matrix<T> cross (const MatrixAbstract & B) const;  ///< View both matrices as vectors and return cross product.  (Is there a better definition that covers 2D matrices?)
	virtual void identity (const T scalar = 1.0);  ///< Set main diagonal to scalar and everything else to zero.
	virtual MatrixRegion<T> row (const int r) const;  ///< Returns a view row r.  The matrix is oriented "horizontal".
	virtual MatrixRegion<T> column (const int c) const;  ///< Returns a view of column c.
	virtual MatrixRegion<T> region (const int firstRow = 0, const int firstColumn = 0, int lastRow = -1, int lastColumn = -1) const;  ///< Same as call to MatrixRegion<T> (*this, firstRow, firstColumn, lastRow, lastColumn)

	// Basic operations

	// The reason to have operators at this abstract level is to allow some
	// manipulation of matrices even if we know nothing of their storage
	// method.  The other possible motivation (laziness thru inheritance)
	// doesn't really pan out, since C++ tries to resolve an overloaded
	// function name in the current class rather than looking up the hierarchy.
	// However, polymorphism is still a valid motivation.

	// Ideally, if a subclass reimplements one of these operations, the return
	// type should be as specific as possible.  Unfortunately, some C++
	// compilers (namely MS VC++) won't allow a subclass to be declared as
	// the return type of an overidden function.  Therefore, once we declare
	// these operations, the return type becomes "nailed down" for all
	// subclasses.  If a given return type is not universally useful, the
	// function can't be declared (or used) at this abstract level.
	// * Some operations that involve MatrixAbstract must return a dense
	// matrix.  For these, we know the return type must be Matrix.
	// * The notation "M<T>" in commented out interfaces means that it is
	// better to return the actual type of the subclass.
	// * We can be less picky about return type for *= /= += -= because they
	// usually won't be in the middle of an expression.

	// For basic arithmetic each class may have up to 24 operators:
	// 3 overloads for each of 8 operators: * / + - *= /= += -=
	// The overloads are for: 1) MatrixAbstract, 2) the current class,
	// and 3) a scalar.  A class may have more overloads if it wishes to
	// directly interract with other matrix classes.

	virtual bool operator == (const MatrixAbstract & B) const;  ///< Two matrices are equal if they have the same shape and the same elements.
	bool operator != (const MatrixAbstract & B) const
	{
	  return ! ((*this) == B);
	}

	Matrix<T> operator ! () const;  ///< Invert matrix if square, otherwise create pseudo-inverse.  Can't be virtual, because this forces a link dependency on LAPACK for any matrix class that uses it to implement inversion.
	//virtual M<T> operator ~ () const;  ///< Transpose matrix.  Not implemented here since small matrices could directly transpose their contents.

	virtual Matrix<T> operator * (const MatrixAbstract & B) const;  ///< Multiply matrices: this * B
	//virtual M<T> operator * (const T scalar) const;  ///< Multiply each element by scalar
	virtual Matrix<T> elementMultiply (const MatrixAbstract & B) const;  ///< Elementwise multiplication.  This isn't a C operator, but certainly a basic operation.
	//virtual Matrix<T> operator / (const MatrixAbstract & B) const;  ///< Elementwise division.  Could mean this * !B, but such expressions are done other ways in linear algebra.
	//virtual M<T> operator / (const T scalar) const;  ///< Divide each element by scalar
	virtual Matrix<T> operator + (const MatrixAbstract & B) const;  ///< Elementwise sum.
	//virtual M<T> operator + (const T scalar) const;  ///< Add scalar to each element
	virtual Matrix<T> operator - (const MatrixAbstract & B) const;  ///< Elementwise difference.
	//virtual M<T> operator - (const T scalar) const;  ///< Subtract scalar from each element

	virtual MatrixAbstract & operator *= (const MatrixAbstract & B);  ///< this = this * B
	virtual MatrixAbstract & operator *= (const T scalar);  ///< this = this * scalar
	//virtual MatrixAbstract & operator /= (const MatrixAbstract & B);  ///< Elementwise divide and store back to this
	virtual MatrixAbstract & operator /= (const T scalar);  ///< this = this / scalar
	virtual MatrixAbstract & operator += (const MatrixAbstract & B);  ///< Elementwise sum, stored back to this
	//virtual MatrixAbstract & operator += (const T scalar);  ///< Increase each element by scalar
	virtual MatrixAbstract & operator -= (const MatrixAbstract & B);  ///< Elementwise difference, stored back to this
	virtual MatrixAbstract & operator -= (const T scalar);  ///< Decrease each element by scalar

	// Serialization
	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	// Global Data
	static int displayWidth;  ///< Number of character positions per cell to use when printing out matrix.
	static int displayPrecision;  ///< Number of significant digits to output.
  };

  /// Dump human readable matrix.  Intended for printable output only.
  template<class T>
  std::ostream &
  operator << (std::ostream & stream, const MatrixAbstract<T> & A);

  /**
	 Load matrix from human-readable stream.  Not idempotent with operator <<
	 because this function expects to encounter number of rows
	 and columns as part of stream.  (However, one can easily output them
	 directly before using operator <<.)
  **/
  template<class T>
  std::istream &
  operator >> (std::istream & stream, MatrixAbstract<T> & A);

  /**
	 Load matrix from human-readable string.  Matrix must already be sized
	 correctly.  Elements are read on row-major order.
  **/
  template<class T>
  MatrixAbstract<T> &
  operator << (MatrixAbstract<T> & A, const std::string & source);


  // Concrete matrices  -------------------------------------------------------

  template<class T>
  class Matrix : public MatrixAbstract<T>
  {
  public:
	Matrix ();
	Matrix (const int rows, const int columns = 1);
	Matrix (const MatrixAbstract<T> & that);
	template<class T2>
	Matrix (const MatrixAbstract<T2> & that)
	{
	  int h = that.rows ();
	  int w = that.columns ();
	  resize (h, w);
	  T * i = (T *) data;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  *i++ = (T) that (r, c);
		}
	  }
	}
	Matrix (std::istream & stream);
	Matrix (T * that, const int rows, const int columns = 1);  ///< Attach to memory block pointed to by that
	Matrix (Pointer & that, const int rows = -1, const int columns = 1);  ///< Share memory block with that.  rows == -1 or columns == -1 means infer number from size of memory.  At least one of {rows, columns} must be positive.
	virtual ~Matrix ();
	void detach ();  ///< Set the state of this matrix as if it has no data.  Releases (but only frees if appropriate) any memory.

	virtual T & operator () (const int row, const int column) const
	{
	  return ((T *) data)[column * rows_ + row];
	}
    virtual T & operator [] (const int row) const
	{
	  return ((T *) data)[row];
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns = 1);
	virtual void copyFrom (const Matrix<T> & that);

	virtual Matrix reshape (const int rows, const int columns = 1) const;

	virtual T frob (float n) const;
	virtual T sumSquares () const;
	virtual T dot (const Matrix & B) const;
	virtual Matrix transposeSquare () const;  ///< Computes the upper triangular part of the symmetric matrix (~this * this).

	Matrix operator ! () const;
	virtual MatrixTranspose<T> operator ~ () const;
	virtual Matrix operator * (const MatrixAbstract<T> & B) const;
	virtual Matrix operator * (const Matrix & B) const;
	virtual Matrix operator * (const T scalar) const;
	virtual Matrix operator / (const T scalar) const;
	virtual Matrix operator + (const Matrix & B) const;
	virtual Matrix operator - (const Matrix & B) const;

	virtual Matrix & operator *= (const Matrix & B);
	virtual MatrixAbstract<T> & operator *= (const T scalar);
	virtual MatrixAbstract<T> & operator /= (const T scalar);
	virtual Matrix & operator += (const Matrix & B);
	virtual Matrix & operator -= (const Matrix & B);
	virtual MatrixAbstract<T> & operator -= (const T scalar);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	// Data
	Pointer data;
	int rows_;
	int columns_;
  };

  /**
	 Vector is not a special class in this package.  All operations and
	 functions are on matrices.  Period.  This Vector class is
	 a specialization of Matrix that makes it more convenient to access
	 elements without referring to column 0 all the time.
  **/
  template<class T>
  class Vector : public Matrix<T>
  {
  public:
	Vector ();
	Vector (const int rows);
	Vector (const MatrixAbstract<T> & that);
	template<class T2>
	Vector (const MatrixAbstract<T2> & that)
	{
	  int h = that.rows ();
	  int w = that.columns ();
	  resize (h, w);
	  T * i = (T *) this->data;
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  *i++ = (T) that (r, c);
		}
	  }
	}
	Vector (const Matrix<T> & that);
	Vector (std::istream & stream);
	Vector (T * that, const int rows);  ///< Attach to memory block pointed to by that
	Vector (Pointer & that, const int rows = -1);  ///< Share memory block with that.  rows == -1 means infer number from size of memory

	Vector & operator = (const MatrixAbstract<T> & that);
	Vector & operator = (const Matrix<T> & that);

	virtual MatrixAbstract<T> * duplicate () const;
	virtual void resize (const int rows, const int columns = 1);
  };

  /**
	 This Matrix is presumed to be Symmetric.  It could also be Hermitian
	 or Triangular, but these require more specialization.  The whole point
	 of having this class is to take advantage of symmetry to cut down on
	 memory accesses.

	 For purpose of calls to LAPACK, this matrix stores the upper triangular
	 portion.
  **/
  template<class T>
  class MatrixPacked : public MatrixAbstract<T>
  {
  public:
	MatrixPacked ();
	MatrixPacked (const int rows);  ///< columns = rows
	MatrixPacked (const MatrixAbstract<T> & that);
	MatrixPacked (std::istream & stream);

	virtual T & operator () (const int row, const int column) const;
    virtual T & operator [] (const int row) const;
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns = -1);
	virtual void copyFrom (const MatrixAbstract<T> & that);
	virtual void copyFrom (const MatrixPacked & that);

	virtual MatrixPacked operator ~ () const;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	// Data
	Pointer data;
	int rows_;  ///< columns = rows
  };

  /**
	 Stores only nonzero elements.  Assumes that every column has at least
	 one non-zero entry, so stores a structure for every column.  This is
	 a trade-off between time and space (as always).  If the matrix is
	 extremely sparse (not all columns used), then a sparse structure for
	 holding the column structures would be better.
  **/
  template<class T>
  class MatrixSparse : public MatrixAbstract<T>
  {
  public:
	MatrixSparse ();
	MatrixSparse (const int rows, const int columns);
	MatrixSparse (const MatrixAbstract<T> & that);
	MatrixSparse (std::istream & stream);
	~MatrixSparse ();

	void set (const int row, const int column, const T value);  ///< If value is non-zero, creates element if not already there; if value is zero, removes element if it exists.
	virtual T & operator () (const int row, const int column) const;
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);  ///< Completely ignore the value of scalar, and simply delete all data.
	virtual void resize (const int rows, const int columns = 1);  ///< Changing number of rows has no effect at all.  Changing number of columns resizes column list.
	virtual void copyFrom (const MatrixSparse & that);

	virtual T frob (float n) const;

	virtual MatrixSparse operator - (const MatrixSparse & B) const;

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	int rows_;
	fl::PointerStruct< std::vector< std::map<int, T> > > data;
  };

  /**
	 A square matrix that always returns the same value for a diagonal
	 element and zero for any other element.
  **/
  template<class T>
  class MatrixIdentity : public MatrixAbstract<T>
  {
  public:
	MatrixIdentity ();
	MatrixIdentity (int size, T value = 1);

	virtual T & operator () (const int row, const int column) const;
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns = -1);

	int size;
	T value;
  };

  /**
	 A square matrix that only stores values for the diagonal entries
	 and returns zero for any other element.
  **/
  template<class T>
  class MatrixDiagonal : public MatrixAbstract<T>
  {
  public:
	MatrixDiagonal ();
	MatrixDiagonal (const int rows, const int columns = -1);
	MatrixDiagonal (const Vector<T> & that, const int rows = -1, const int columns = -1);

	virtual T & operator () (const int row, const int column) const;
    virtual T & operator [] (const int row) const;
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns = -1);

	int rows_;
	int columns_;
	Pointer data;
  };


  // Views --------------------------------------------------------------------

  // These matrices wrap other matrices and provide a different interpretation
  // of the row and column coordinates.  Views are always "dense" in the sense
  // that every element maps to an element in the wrapped Matrix.  However,
  // they have no storage of their own.

  // Views do two jobs:
  // 1) Make it convenient to access a Matrix in manners other than dense.
  //    E.g.: You could do strided access without having to write out the
  //    index offsets and multipliers.
  // 2) Act as a proxy for some rearrangement of the Matrix in the next
  //    stage of calculation in order to avoid copying elements.
  //    E.g.: A Transpose view allows one to multiply a transposed Matrix
  //    without first moving all the elements to their tranposed positions.

  /// this(i,j) maps to that(j,i)
  template<class T>
  class MatrixTranspose : public MatrixAbstract<T>
  {
  public:
	MatrixTranspose (const MatrixAbstract<T> & that);
	virtual ~MatrixTranspose ();

	virtual T & operator () (const int row, const int column) const
	{
	  return (*wrapped) (column, row);
	}
    virtual T & operator [] (const int row) const
	{
	  return (*wrapped)[row];
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns);

	virtual Matrix<T> operator * (const MatrixAbstract<T> & B) const;
	virtual Matrix<T> operator * (const T scalar) const;

	// It is the job of the matrix being transposed to make another instance
	// of itself.  It is our responsibility to delete this object when we
	// are destroyed.
	MatrixAbstract<T> * wrapped;
  };

  template<class T>
  class MatrixRegion : public MatrixAbstract<T>
  {
  public:
	MatrixRegion (const MatrixRegion & that);
	MatrixRegion (const MatrixAbstract<T> & that, const int firstRow = 0, const int firstColumn = 0, int lastRow = -1, int lastColumn = -1);
	virtual ~MatrixRegion ();

	template<class T2>
	MatrixRegion & operator = (const MatrixAbstract<T2> & that)
	{
	  int h = that.rows ();
	  int w = that.columns ();
	  resize (h, w);
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  (*this)(r,c) = (T) that(r,c);
		}
	  }
	  return *this;
	}
	MatrixRegion & operator = (const MatrixRegion & that);

	virtual T & operator () (const int row, const int column) const
	{
	  return (*wrapped)(row + firstRow, column + firstColumn);
	}
    virtual T & operator [] (const int row) const
	{
	  return (*wrapped)(row % rows_ + firstRow, row / rows_ + firstColumn);
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void clear (const T scalar = (T) 0);
	virtual void resize (const int rows, const int columns = 1);

	virtual MatrixTranspose<T> operator ~ () const;
	virtual Matrix<T> operator * (const MatrixAbstract<T> & B) const;
	virtual Matrix<T> operator * (const T scalar) const;

	MatrixAbstract<T> * wrapped;
	int firstRow;
	int firstColumn;
	int rows_;
	int columns_;
  };


  // Small Matrix classes -----------------------------------------------------

  // There are two reasons for making small Matrix classes.
  // 1) Avoid overhead of managing memory.
  // 2) Certain numerical operations (such as computing eigenvalues) have
  //    direct implementations in small matrix sizes (particularly 2x2).

  template<class T, int R, int C>
  class MatrixFixed : public MatrixAbstract<T>
  {
  public:
	MatrixFixed ();
	template<class T2>
	MatrixFixed (const MatrixAbstract<T2> & that)
	{
	  const int h = std::min (that.rows (),    R);
	  const int w = std::min (that.columns (), C);
	  for (int c = 0; c < w; c++)
	  {
		for (int r = 0; r < h; r++)
		{
		  data[c][r] = (T) that(r,c);
		}
	  }
	}
	MatrixFixed (std::istream & stream);

	virtual T & operator () (const int row, const int column) const
	{
	  return (T &) data[column][row];
	}
    virtual T & operator [] (const int row) const
	{
	  return ((T *) data)[row];
	}
	virtual int rows () const;
	virtual int columns () const;
	virtual MatrixAbstract<T> * duplicate () const;
	virtual void resize (const int rows = R, const int columns = C);

	virtual MatrixFixed<T,C,R>   operator ~  () const;
	virtual Matrix<T>            operator *  (const MatrixAbstract<T> & B) const;
	virtual MatrixFixed<T,R,C>   operator *  (const MatrixFixed<T,R,C> & B) const;
	virtual MatrixFixed<T,R,C>   operator *  (const T scalar) const;
	virtual MatrixFixed<T,R,C>   operator /  (const T scalar) const;
	virtual MatrixFixed<T,R,C> & operator *= (const MatrixFixed<T,R,C> & B);
	virtual MatrixAbstract<T> &  operator *= (const T scalar);

	virtual void read (std::istream & stream);
	virtual void write (std::ostream & stream, bool withName = false) const;

	// Data
	T data[C][R];
  };

  template<class T, int R, int C>
  Matrix<T> operator ! (const MatrixFixed<T,R,C> & A);

  template<class T>
  MatrixFixed<T,2,2> operator ! (const MatrixFixed<T,2,2> & A);

  template<class T>
  void geev (const MatrixFixed<T,2,2> & A, Matrix<T> & eigenvalues);

  template<class T>
  void geev (const MatrixFixed<T,2,2> & A, Matrix<std::complex<T> > & eigenvalues);
}


#endif
