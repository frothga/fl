/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/04/23 19:38:11  Fred
Add UIUC copyright notice.

Revision 1.5  2004/04/19 21:23:37  rothgang
Create template versions of Search classes.

Revision 1.4  2004/04/19 17:01:43  rothgang
Remove unused variants of LM.  This is in preparation for converting the entire
Search architecture into a set of templates so the programmer can choose
between single and double precision.

Revision 1.3  2004/02/15 17:57:41  rothgang
Remove SearchableNumeric::value(), since it was already an abstract function in
the superclass.

Revision 1.2  2003/12/30 16:22:28  rothgang
Convert comments to doxygen format.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef search_h
#define search_h


#include "fl/matrix.h"

#include <vector>


namespace fl
{
  // General search interface -------------------------------------------------

  /**
	 Encapsulates a vector function.
	 Depending on what search method you choose, it may be a value function
	 or an error function.
  **/
  template<class T>
  class Searchable
  {
  public:
	virtual int  dimension () = 0;
	virtual void value (const Vector<T> & point, Vector<T> & result) = 0;  ///< Returns the value of the function at a given point.  Must throw an exception if point is out of domain.
	virtual void gradient (const Vector<T> & point, Vector<T> & result, const int index = 0) = 0;  ///< Return the first derivative vector for the function value addressed by index.  index is zero-based.  If this function has only one variable, ignore index.
	virtual void jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue = NULL) = 0;  ///< Return the gradients for all variables.  currentValue is a hint for estimating gradient by finite differences.
	virtual void jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue = NULL) = 0;  ///< Same as above, except omits all zero entries in Jacobian.
	virtual void hessian (const Vector<T> & point, Matrix<T> & result, const int index = 0) = 0;  ///< Return the second derivative matrix for the function value addressed by index.  If this is a single-valued function, then ignore index.
  };

  /**
	 Computes various derivative functions using finite differences.
	 Regarding the name: "Numeric" as opposed to "Analytic", which would be
	 computed directly.  To make an analytic searchable, derive from this
	 class and override appropriate functions.
	 The programmer must implement at least the dimension() and value()
	 functions to instantiate a subclass.
  **/
  template<class T>
  class SearchableNumeric : public Searchable<T>
  {
  public:
	SearchableNumeric (T perturbation = -1);

	virtual void gradient (const Vector<T> & point, Vector<T> & result, const int index = 0);
	virtual void jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue = NULL);
	virtual void jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue = NULL);
	virtual void hessian (const Vector<T> & point, Matrix<T> & result, const int index = 0);

	T perturbation;  ///< Amount to perturb a variable for finding any of the derivatives by finite differences.
  };

  /**
	 Uses a structurally orthogonal set of calls to the value function to
	 compute the Jacobian, rather than a separate call for every parameter.
	 Assumes that the sparsity structure of the Jacobian is fixed over the
	 entire domain of the function.
  **/
  template<class T>
  class SearchableSparse : public SearchableNumeric<T>
  {
  public:
	/**
	   This constructor does *not* call cover().  However, cover() or its
	   equivalent must be called before the object is ready for use.  The
	   programmer is welcome to write a constructor in the derived class that
	   does call cover().  :)
	**/
	SearchableSparse (T perturbation = -1) : SearchableNumeric<T> (perturbation) {}

	/**
	   The matrix returned by interaction encodes the sparsity structure of the
	   Jacobian.  It has nonzero entries wherever a parameter (associated with
	   a column of the Jacobian) interracts with a value (associated with a
	   row of the Jacobian).
	**/
	virtual MatrixSparse<bool> interaction () = 0;
	virtual void cover ();  ///< Compute a structurally orthogonal cover of the Jacobian based on the interaction matrix.

	virtual void jacobian (const Vector<T> & point, Matrix<T> & result, const Vector<T> * currentValue = NULL);  ///< Compute the Jacobian using the cover.
	virtual void jacobian (const Vector<T> & point, MatrixSparse<T> & result, const Vector<T> * currentValue = NULL);  ///< Ditto, but omit zero entries.

	// These two members represent the cover in a way that is easy to execute.
	MatrixSparse<int> parameters;  ///< If parameters(i,j) == k, then compute Jacobian(i,k) during the jth call to the function (by perturbing the kth parameter).
	std::vector< std::vector<int> > parms;  ///< The jth entry gives in compact form the parameters that need to be perturbed during the jth call.
  };

  /**
	 Search tries to optimize the choice of "point" in the domain of a function
	 by some criterion.  The exact criterion depends on the specific method.
	 One possibility is to minimize the length of the functions value vector
	 (ie: as in least squares minimization).  Another possibility is to search
	 for the largest value vector.
  **/
  template<class T>
  class Search
  {
  public:
	virtual void search (Searchable<T> & searchable, Vector<T> & point) = 0;  ///< Finds the point that optimizes the search crierion.  "point" must be initialized to a reasonable starting point.
  };


  // Specific Searches --------------------------------------------------------

  template<class T>
  class AnnealingAdaptive : public Search<T>
  {
  public:
	AnnealingAdaptive (bool minimize = true, int levels = 10, int patience = -1);  ///< minimize == true means do least squares; minimize == false means find largest values

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	bool minimize;
	int levels;
	int patience;
  };

  /**
	 LM based on QR decomposition.  Adapted from MINPACK.
  **/
  template<class T>
  class LevenbergMarquardt : public Search<T>
  {
  public:
	LevenbergMarquardt (T toleranceF = -1, T toleranceX = -1, int maxIterations = 200);  ///< toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	void qrfac (Matrix<T> & a, Vector<int> & ipvt, Vector<T> & rdiag, Vector<T> & acnorm);
	void qrsolv (Matrix<T> & r, const Vector<int> & ipvt, const Vector<T> & diag, const Vector<T> & qtb, Vector<T> & x, Vector<T> & sdiag);
	void lmpar (Matrix<T> & r, const Vector<int> & ipvt, const Vector<T> & diag, const Vector<T> & qtb, T delta, T & par, Vector<T> & x);

	static const T epsilon;  ///< FLT_EPSILON or DBL_EPSILON
	static const T minimum;  ///< FLT_MIN or DBL_MIN
	T toleranceF;
	T toleranceX;
	int maxIterations;
  };

  /**
	 LM based on Bunch-Kaufman decomposition with sparse implementation.
  **/
  template<class T>
  class LevenbergMarquardtSparseBK : public Search<T>
  {
  public:
	LevenbergMarquardtSparseBK (T toleranceF = -1, T toleranceX = -1, int maxIterations = 200, int maxPivot = 20);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	static const T epsilon;  ///< FLT_EPSILON or DBL_EPSILON
	static const T minimum;  ///< FLT_MIN or DBL_MIN
	T toleranceF;
	T toleranceX;
	int maxIterations;
	int maxPivot;  ///< Farthest from diagonal to permit a pivot
  };
}


#endif
