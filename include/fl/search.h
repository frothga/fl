/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef search_h
#define search_h


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
  // General search interface -------------------------------------------------

  template<class T> class SHARED Search;

  /**
	 Encapsulates a vector function.
	 Depending on what search method you choose, it may be a value function
	 or an error function.
  **/
  template<class T>
  class SHARED Searchable
  {
  public:
	virtual ~Searchable () {}

	virtual Search<T> *     search    () = 0;  ///< @return The best search method for the current problem, or at least a reasonable choice.  The caller is responsible for destroying the object.
	virtual MatrixResult<T> start     () = 0;  ///< @return A reasonable starting point for the search.
	virtual void            finish    (const Vector<T> & point) = 0;  ///< Permanently apply the values in the given point to the underlying object, if one exists.
	virtual MatrixResult<T> scales    (const Vector<T> & point) = 0;  ///< Sensitivity along each of the dimensions of the input space.
	/**
	   Determine the number of elements in the result of value(), and
	   configure this object accordingly.  A Search will call this function
	   once at the beginning, before the first call to one of {value(),
	   gradient(), jacobian(), hessian()}.  After that, this object must
	   keep the same size unless it receives another call to dimension(),
	   at which time it may reconfigure itself.
	**/
	virtual int             dimension (const Vector<T> & point) = 0;
	virtual MatrixResult<T> value     (const Vector<T> & point) = 0;  ///< @return The value of the function at a given point.  Must throw an exception if point is out of domain.
	virtual MatrixResult<T> gradient  (const Vector<T> & point, const Vector<T> * currentValue = NULL) = 0;  ///< Treat this as a single-valued function and return the first derivative vector.  Method of converting multi-valued function to single-valued function is arbitrary, but should be differentiable and same as that used by hessian().
	virtual MatrixResult<T> jacobian  (const Vector<T> & point, const Vector<T> * currentValue = NULL) = 0;  ///< Return the gradients for all variables.  currentValue is a hint for estimating gradient by finite differences.
	virtual MatrixResult<T> hessian   (const Vector<T> & point, const Vector<T> * currentValue = NULL) = 0;  ///< Treat this as a single-valued function and return the second derivative matrix.  Method of converting multi-valued function to single-valued function is arbitrary, but should be differentiable and same as that used by gradient().
  };

  /**
	 Computes various derivative functions using finite differences.
	 Regarding the name: "Numeric" as opposed to "Analytic", which would be
	 computed directly.  To make an analytic searchable, derive from this
	 class and override appropriate functions.
	 The programmer must implement at least the start(), dimension() and value()
	 functions to instantiate a subclass.
  **/
  template<class T>
  class SHARED SearchableNumeric : public Searchable<T>
  {
  public:
	SearchableNumeric (T perturbation = -1);

	virtual Search<T> *     search   ();  ///< Return LevenbergMarquardt
	virtual void            finish   (const Vector<T> & point);  ///< Ignores the given point.
	virtual MatrixResult<T> scales   (const Vector<T> & point);  ///< Return a vector of ones
	virtual MatrixResult<T> gradient (const Vector<T> & point, const Vector<T> * currentValue = NULL);  ///< Uses sum of squares to reduce this to a single-valued function.
	virtual MatrixResult<T> jacobian (const Vector<T> & point, const Vector<T> * currentValue = NULL);
	virtual MatrixResult<T> hessian  (const Vector<T> & point, const Vector<T> * currentValue = NULL);  ///< Uses sum of squares to reduce this to a single-valued function.

	T perturbation;  ///< Amount to perturb a variable for finding any of the derivatives by finite differences.
  };

  /**
	 Uses a structurally orthogonal set of calls to the value function to
	 compute the Jacobian, rather than a separate call for every parameter.
	 Assumes that the sparsity structure of the Jacobian is fixed over the
	 entire domain of the function.
  **/
  template<class T>
  class SHARED SearchableSparse : public SearchableNumeric<T>
  {
  public:
	SearchableSparse (T perturbation = -1);

	/**
	   The matrix returned by interaction encodes the sparsity structure of the
	   Jacobian.  It has nonzero entries wherever a parameter (associated with
	   a column of the Jacobian) interracts with a value (associated with a
	   row of the Jacobian).
	**/
	virtual MatrixSparse<bool> interaction () = 0;
	virtual void               cover       ();  ///< Compute a structurally orthogonal cover of the Jacobian based on the interaction matrix.  Called automatically by jacobian() whenever the current cover is stale.

	virtual Search<T> *     search   ();  ///< Return LevenbergMarquardtSparseBK
	virtual MatrixResult<T> gradient (const Vector<T> & point, const Vector<T> * currentValue = NULL);  ///< Compute gradient as 2 * ~jacobian * value.  In a sparse system, this should require fewer calls to value() than the direct method.
	virtual MatrixResult<T> jacobian (const Vector<T> & point, const Vector<T> * currentValue = NULL);  ///< Compute the Jacobian using the cover.

	// These members represent the cover in a way that is easy to execute.
	int coveredDimension;  ///< The size of the result of value() in force when the last call to cover() ocurred.  If -1, then cover() has not yet been called.
	MatrixSparse<int> parameters;  ///< If parameters(i,j) == k, then compute Jacobian(i,k) during the jth call to the function (by perturbing the kth parameter).
	std::vector< std::vector<int> > parms;  ///< The jth entry gives in compact form the parameters that need to be perturbed during the jth call.
  };

  /**
	 Constricts an arbitrary Searchable to a single line.
	 point is single-dimensional.  We only read or write the first element,
	 and ignore all others.
  **/
  template<class T>
  class SearchableConstriction : public SearchableNumeric<T>
  {
  public:
	SearchableConstriction (Searchable<T> & searchable, const Vector<T> & a, const Vector<T> & b) : searchable (searchable), a (a), b (b) {}

	virtual MatrixResult<T> start ()
	{
	  Vector<T> * result = new Vector<T> (1);
	  result->clear ();
	  return result;
	}
	virtual int             dimension (const Vector<T> & point) {return searchable.dimension (point);}
	virtual MatrixResult<T> value     (const Vector<T> & point) {return searchable.value (a + b * point[0]);}

	Searchable<T> & searchable;
	Vector<T> a;
	Vector<T> b;
  };

  /**
	 Interface that allows a Search to opportunistically move to a better
	 position if one is detected during the construction of gradient, jacobian,
	 or hessian.  A Searchable class that inherits this interface need only
	 call update() at the end of its value() function in order to fulfill
	 its promises.  In particular, a class that inherits this interface promises
	 to initialize bestResidual to INFINITY, and to update both bestResidual
	 and bestPoint each time value() is called.  bestPoint need not be valid
	 until bestResidual is less than INFINITY.  The subclass should make no
	 assumptions about how bestPoint is used.  In particular, it should
	 always allocate a fresh block of memory each time it stores an updated
	 value.

	 Exactly what norm to use when comparing the results of value() is
	 currently undefined.
  **/
  template<class T>
  class SearchableGreedy
  {
  public:
	SearchableGreedy () {bestResidual = INFINITY;}

	void update (T residual, const Vector<T> & point)
	{
	  if (residual < bestResidual)
	  {
		bestResidual = residual;
		bestPoint.detach ();
		bestPoint.copyFrom (point);
	  }
	}

	T         bestResidual;
	Vector<T> bestPoint;
  };

  /**
	 Search tries to optimize the choice of "point" in the domain of a function
	 by some criterion.  The exact criterion depends on the specific method.
	 One possibility is to minimize the length of the functions value vector
	 (ie: as in least squares minimization).  Another possibility is to search
	 for the largest value vector.
  **/
  template<class T>
  class SHARED Search
  {
  public:
	virtual ~Search () {}

	virtual void search (Searchable<T> & searchable, Vector<T> & point) = 0;  ///< Finds the point that optimizes the search crierion.  "point" must be initialized to a reasonable starting place.
  };


  // Specific Searches --------------------------------------------------------

  /**
	 Find local minimum of a one parameter function.
	 The parameter of the function must be one-dimensional.  The output
	 of the function can have any dimensionality.  This method will seek
	 a solution that minimizes the Euclidean norm of the output vector
	 (that is, a least square solution).
   **/
  template<class T>
  class SHARED LineSearch : public Search<T>
  {
  public:
	LineSearch (T toleranceF = (T) -1, T toleranceX = (T) -1);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	T toleranceF;
	T toleranceX;
	int maxIterations;
  };

  template<class T>
  class SHARED AnnealingAdaptive : public Search<T>
  {
  public:
	AnnealingAdaptive (bool minimize = true, int levels = 10, int patience = -1);  ///< minimize == true means do least squares; minimize == false means find largest values

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	bool minimize;
	int levels;
	int patience;
  };

  template<class T>
  class SHARED ParticleSwarm : public Search<T>
  {
  public:
	/**
	   @param toleranceF Indicates an acceptable value for the searchable
	   (termination condition), and also indicates whether to minimize or
	   maximize.  If positive, then minimize.  If negative, then maximize.
	   Basically, the negative value indicates to flip the sign of the
	   searchable's value.  Since the optimizer always seeks a number
	   smaller than toleranceF, this effectively changes the problem into
	   a maximization.  A value of zero means minimize, and use a default
	   threshold.
	**/
	ParticleSwarm (int particleCount = -1, T toleranceF = 0, int patience = 10);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	int particleCount;
	T toleranceF;
	int patience;
	int maxIterations;
	T minRandom;  ///< Smallest amount of random motion to introduce in any given cycle.
	T attractionGlobal;  ///< How far to move along the vector towards the global best position.
	T attractionLocal;  ///< How far to move along the vector towards a given particle's best position.
	T constriction;  ///< Maximum size of velocity update, as a portion of distance to the local or global optimum, whichever is farther.
	T inertia;  ///< Portion of previous velocity to include in next velocity.
	T decayRate;  ///< Multiply inertia by this amount during each iteration.

	class SHARED Particle
	{
	public:
	  T         value;
	  Vector<T> position;
	  Vector<T> velocity;
	  T         bestValue;
	  Vector<T> bestPosition;
	};
  };

  template<class T>
  class SHARED GradientDescent : public Search<T>
  {
  public:
	GradientDescent (T toleranceX = -1, T updateRate = -0.01, int patience = 3);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	T toleranceX;
	T updateRate;
	int patience;
  };

  template<class T>
  class SHARED ConjugateGradient : public Search<T>
  {
  public:
	ConjugateGradient (T toleranceX = -1, int restartIterations = 5, int maxIterations = -1);  ///< toleranceX < 0 means use sqrt (machine precision); maxIterations < 1 means use dimension of point

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	T toleranceX;
	T toleranceA;  ///< Stop line search when the movement along the direction vector is less than this proportion of its length.
	int restartIterations;
	int maxIterations;
  };

  template<class T>
  class SHARED NewtonRaphson : public Search<T>
  {
  public:
	NewtonRaphson (int direction = -1, T toleranceX = -1, T updateRate = 1, int maxIterations = 200);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	int direction;
	T toleranceX;
	T updateRate;
	int maxIterations;
  };

  /**
	 LM based on QR decomposition.  Adapted from MINPACK.
  **/
  template<class T>
  class SHARED LevenbergMarquardt : public Search<T>
  {
  public:
	LevenbergMarquardt (T toleranceF = -1, T toleranceX = -1, int maxIterations = 200);  ///< toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	void qrsolv (Matrix<T> & J, const Vector<int> & pivots, const Vector<T> & d,      const Vector<T> & Qy, Vector<T> & x, Vector<T> & jdiag);
	void lmpar  (Matrix<T> & J, const Vector<int> & pivots, const Vector<T> & scales, const Vector<T> & Qy, T delta, T & par, Vector<T> & x);

	T toleranceF;
	T toleranceX;
	int maxIterations;
  };

  /**
	 LM based on Bunch-Kaufman decomposition with sparse implementation.
  **/
  template<class T>
  class SHARED LevenbergMarquardtSparseBK : public Search<T>
  {
  public:
	LevenbergMarquardtSparseBK (T toleranceF = -1, T toleranceX = -1, int maxIterations = 200, int maxPivot = 20);

	virtual void search (Searchable<T> & searchable, Vector<T> & point);

	T toleranceF;
	T toleranceX;
	int maxIterations;
	int maxPivot;  ///< Farthest from diagonal to permit a pivot
  };
}


#endif
