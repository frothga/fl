#ifndef search_h
#define search_h


#include "fl/matrix.h"

#include <math.h>
#include <vector>


namespace fl
{
  // General search interface -------------------------------------------------

  // Encapsulates a vector function.
  // Depending on what search method you choose, it may be a value function
  // or an error function.
  class Searchable
  {
  public:
	virtual int  dimension () = 0;
	virtual void value (const Vector<double> & point, Vector<double> & result) = 0;  // Returns the value of the function at a given point.  Must throw an exception if point is out of domain.
	virtual void gradient (const Vector<double> & point, Vector<double> & result, const int index = 0) = 0;  // Return the first derivative vector for the function value addressed by index.  index is zero-based.  If this function has only one variable, ignore index.
	virtual void jacobian (const Vector<double> & point, Matrix<double> & result, const Vector<double> * currentValue = NULL) = 0;  // Return the gradients for all variables.  currentValue is a hint for estimating gradient by finite differences.
	virtual void jacobian (const Vector<double> & point, MatrixSparse<double> & result, const Vector<double> * currentValue = NULL) = 0;  // Same as above, except omits all zero entries in Jacobian.
	virtual void hessian (const Vector<double> & point, Matrix<double> & result, const int index = 0) = 0;  // Return the second derivative matrix for the function value addressed by index.  If this is a single-valued function, then ignore index.
  };

  // Computes various derivative functions using finite differences.
  // Regarding the name: "Numeric" as opposed to "Analytic", which would be
  // computed directly.
  class SearchableNumeric : public Searchable
  {
  public:
	SearchableNumeric (double perturbation = -1);

	virtual void value (const Vector<double> & point, Vector<double> & result) = 0;  // Returns the value of the function at a given point.  Must throw an exception if point is out of domain.
	virtual void gradient (const Vector<double> & point, Vector<double> & result, const int index = 0);  // Return the first derivative vector for the function value addressed by index.  index is zero-based.  If this function has only one variable, ignore index.
	virtual void jacobian (const Vector<double> & point, Matrix<double> & result, const Vector<double> * currentValue = NULL);  // Return the gradients for all variables.  currentValue is a hint for estimating gradient by finite differences.
	virtual void jacobian (const Vector<double> & point, MatrixSparse<double> & result, const Vector<double> * currentValue = NULL);  // Same as above, except omits all zero entries in Jacobian.
	virtual void hessian (const Vector<double> & point, Matrix<double> & result, const int index = 0);  // Return the second derivative matrix for the function value addressed by index.  If this is a single-valued function, then ignore index.

	double perturbation;  // Amount to perturb a variable for finding any of the derivatives by finite differences.
  };

  // Uses a structurally orthogonal set of calls to the value function to
  // compute the Jacobian, rather than a separate call for every parameter.
  // Assumes that the sparsity structure of the Jacobian is fixed over the
  // entire domain of the function.
  class SearchableSparse : public SearchableNumeric
  {
  public:
	// This constructor does *not* call cover().  However, cover() or its
	// equivalent must be called before the object is ready for use.  The
	// programmer is welcome to write a constructor in the derived class that
	// does call cover().  :)
	SearchableSparse (double perturbation = -1) : SearchableNumeric (perturbation) {}

	// The matrix returned by interaction encodes the sparsity structure of the
	// Jacobian.  It has nonzero entries wherever a parameter (associated with
	// a column of the Jacobian) interracts with a value (associated with a
	// row of the Jacobian).
	virtual MatrixSparse<bool> interaction () = 0;
	virtual void cover ();  // Compute a structurally orthogonal cover of the Jacobian based on the interaction matrix.

	virtual void jacobian (const Vector<double> & point, Matrix<double> & result, const Vector<double> * currentValue = NULL);  // Compute the Jacobian using the cover.
	virtual void jacobian (const Vector<double> & point, MatrixSparse<double> & result, const Vector<double> * currentValue = NULL);  // Ditto, but omit zero entries.

	// These two members represent the cover in a way that is easy to execute.
	MatrixSparse<int> parameters;  // If parameters(i,j) == k, then compute Jacobian(i,k) during the jth call to the function (by perturbing the kth parameter).
	std::vector< std::vector<int> > parms;  // The jth entry gives in compact form the parameters that need to be perturbed during the jth call.
  };

  // Search tries to optimize the choice of "point" in the domain of a function
  // by some criterion.  The exact criterion depends on the specific method.
  // One possibility is to minimize the length of the functions value vector
  // (ie: as in least squares minimization).  Another possibility is to search
  // for the largest value vector.
  class Search
  {
  public:
	virtual void search (Searchable & searchable, Vector<double> & point) = 0;  // Finds the point that optimizes the search crierion.  "point" must be initialized to a reasonable starting point.
  };


  // Specific Searches --------------------------------------------------------

  class AnnealingAdaptive : public Search
  {
  public:
	AnnealingAdaptive (bool minimize = true, int levels = 10, int patience = -1);  // minimize == true means do least squares; minimize == false means find largest values

	virtual void search (Searchable & searchable, Vector<double> & point);

	bool minimize;
	int levels;
	int patience;
  };

  // LM based on QR decomposition.  Translated from MINPACK
  class LevenbergMarquardt : public Search
  {
  public:
	LevenbergMarquardt (double toleranceF = -1, double toleranceX = -1, int maxIterations = 200);  // toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable & searchable, Vector<double> & point);

	double toleranceF;
	double toleranceX;
	int maxIterations;
  };

  /*
  // LM based on normal equations and Bunch-Kaufman decomposition.
  // Modified from MINPACK.
  class LevenbergMarquardtBK : public Search
  {
  public:
	LevenbergMarquardtBK (double toleranceF = -1, double toleranceX = -1, int maxIterations = 200);  // toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable & searchable, Vector<double> & point);

	double toleranceF;
	double toleranceX;
	int maxIterations;
  };
  */

  /*
  // LM that uses MINPACK directly.
  class LevenbergMarquardtMinpack : public Search
  {
  public:
	LevenbergMarquardtMinpack (double toleranceF = -1, double toleranceX = -1, int maxIterations = 200);  // toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable & searchable, Vector<double> & point);

	static void fcn (const int & m, const int & n, double x[], double fvec[], int & info);  // FORTRAN callback

	double toleranceF;
	double toleranceX;
	int maxIterations;
  };
  */

  // Sparsified version of LevenbergMarquardtBK
  class LevenbergMarquardtSparseBK : public Search
  {
  public:
	LevenbergMarquardtSparseBK (double toleranceF = -1, double toleranceX = -1, int maxIterations = 200, int maxPivot = 20);

	virtual void search (Searchable & searchable, Vector<double> & point);

	double toleranceF;
	double toleranceX;
	int maxIterations;
	int maxPivot;  // Farthest from diagonal to permit a pivot
  };

  /*
  // Similar to LevenbergMarquardtSparseBK, but uses sparse Cholesky
  // decomposition instead.  Works OK, but not as numerically stable as BK,
  // and not significantly more efficient either.
  class LevenbergMarquardtSparseCholesky : public Search
  {
  public:
	LevenbergMarquardtSparseCholesky (double toleranceF = -1, double toleranceX = -1, int maxIterations = 200);  // toleranceF/X == -1 means use sqrt (machine precision)

	virtual void search (Searchable & searchable, Vector<double> & point);

	double toleranceF;
	double toleranceX;
	int maxIterations;
  };
  */
}


#endif
