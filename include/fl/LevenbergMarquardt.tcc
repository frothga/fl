/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Copyright 2005, 2009, 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_levenberg_marquardt_tcc
#define fl_levenberg_marquardt_tcc


#include "fl/search.h"
#include "fl/math.h"
#include "fl/lapackproto.h"
#include "fl/blasproto.h"

#include <float.h>
#include <limits>


namespace fl
{
  // class LevenbergMarquardt -------------------------------------------------

  template<class T>
  LevenbergMarquardt<T>::LevenbergMarquardt (T toleranceF, T toleranceX, int maxIterations)
  {
	this->maxIterations = maxIterations;

	if (toleranceF < (T) 0) toleranceF = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceF = toleranceF;

	if (toleranceX < (T) 0) toleranceX = std::sqrt (std::numeric_limits<T>::epsilon ());
	this->toleranceX = toleranceX;
  }

  /**
	 A loose paraphrase the MINPACK function lmdif.
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::search (Searchable<T> & searchable, Vector<T> & x)
  {
	const T toleranceG = 0;
	const T epsilon = std::numeric_limits<T>::epsilon ();

	// Evaluate the function at the starting x and calculate its norm.
	Vector<T> y;
	searchable.value (x, y);

	int m = y.rows ();
	int n = x.rows ();
	int minmn = std::min (m, n);

	Vector<T> scales (n);
	T par = 0;  // levenberg-marquardt parameter
	T ynorm = y.norm (2);
	T xnorm;
	T delta;

	for (int iteration = 0; iteration < maxIterations; iteration++)
	{
	  Matrix<T> J;
	  searchable.jacobian (x, J, &y);
	  Vector<T> jacobianNorms (n);
	  for (int j = 0; j < n; j++) jacobianNorms[j] = J.column (j).norm (2);

	  // On the first iteration ...
	  if (iteration == 0)
	  {
		// Scale according to the norms of the columns of the initial jacobian.
		for (int j = 0; j < n; j++)
		{
		  scales[j] = jacobianNorms[j];
		  if (scales[j] == 0) scales[j] = 1;
		}

		// Calculate the norm of the scaled x and initialize the step bound delta.
		xnorm = (x & scales).norm (2);
		delta = (xnorm * (T) 1) == (T) 0 ? (T) 1 : xnorm;  // Does the multiplication by 1 here make a difference?  This seems to be a test of floating-point representation.
	  }

	  // QR factorization of J
	  Vector<int> pivots (n);
	  pivots.clear ();
	  Vector<T> tau (minmn);
	  T optimalSize = 0;
	  int lwork = -1;
	  int info = 0;
	  geqp3 (m, n, &J(0,0), J.strideC, &pivots[0], &tau[0], &optimalSize, lwork, info);
	  if (info) throw info;
	  lwork = (int) optimalSize;
	  T * work = (T *) malloc (lwork * sizeof (T));
	  geqp3 (m, n, &J(0,0), J.strideC, &pivots[0], &tau[0], work, lwork, info);
	  free (work);
	  if (info) throw info;

	  // Compute first n elements of ~Q*y
	  Vector<T> Qy;
	  Qy.copyFrom (y);
	  lwork = -1;
	  ormqr ('L', 'T', m, 1, minmn, &J(0,0), J.strideC, &tau[0], &Qy[0], Qy.strideC, &optimalSize, lwork, info);
	  if (info) throw info;
	  lwork = (int) optimalSize;
	  work = (T *) malloc (lwork * sizeof (T));
	  ormqr ('L', 'T', m, 1, minmn, &J(0,0), J.strideC, &tau[0], &Qy[0], Qy.strideC, work, lwork, info);
	  free (work);
	  if (info) throw info;
	  Qy.rows_ = n;

	  // compute the norm of the scaled gradient
	  T gnorm = 0;
	  if (ynorm != 0)
	  {
		for (int j = 0; j < n; j++)
		{
		  T jnorm = jacobianNorms[pivots[j]-1];  // pivots are 1-based
		  if (jnorm != 0)
		  {
			T temp = J.region (0, j, j, j).dot (Qy);  // equivalent to ~J * y using the original J.  (That is, ~R * ~Q * y, where J = QR)
			gnorm = std::max (gnorm, std::fabs (temp / (ynorm * jnorm)));  // infinity norm of g = ~J * y / |y| with some additional scaling
		  }
		}
	  }

	  // test for convergence of the gradient norm
	  if (gnorm <= toleranceG)
	  {
		//info = 4;
		return;
	  }

	  // rescale if necessary
	  for (int j = 0; j < n; j++)
	  {
		scales[j] = std::max (scales[j], jacobianNorms[j]);
	  }

	  // beginning of the inner loop
	  T ratio = 0;
	  while (ratio < 0.0001)
	  {
		// determine the levenberg-marquardt parameter.
		Vector<T> p (n);
		lmpar (J, pivots, scales, Qy, delta, par, p);

		// store the direction p and x + p. calculate the norm of p.
		Vector<T> xp = x - p;  // p is actually negative
		T pnorm = (p & scales).norm (2);

		// on the first iteration, adjust the initial step bound
		if (iteration == 0)
		{
		  delta = std::min (delta, pnorm);
		}

		// evaluate the function at x + p and calculate its norm
		Vector<T> tempY;
		searchable.value (xp, tempY);
		T ynorm1 = tempY.norm (2);

		// compute the scaled actual reduction
		T reductionActual = -1;
		if (ynorm1 / 10 < ynorm)
		{
		  T temp = ynorm1 / ynorm;
		  reductionActual = 1 - temp * temp;
		}

		// compute the scaled predicted reduction and the scaled directional derivative
		Vector<T> Jp (n);
		Jp.clear ();
		for (int j = 0; j < n; j++)
		{
		  Jp += J.region (0, j, j, j) * p[pivots[j]-1];  // equivalent to J * p using the original J, since all scale informtion is in the R part of the QR factorizatoion
		}
		T temp1 = Jp.norm (2) / ynorm;
		T temp2 = std::sqrt (par) * pnorm / ynorm;
		T reductionPredicted = temp1 * temp1 + 2 * temp2 * temp2;
		T dirder = -(temp1 * temp1 + temp2 * temp2);

		// compute the ratio of the actual to the predicted reduction
		ratio = 0;
		if (reductionPredicted != 0)
		{
		  ratio = reductionActual / reductionPredicted;
		}

		// update the step bound
		if (ratio <= 0.25)
		{
		  T update;
		  if (reductionActual >= 0)
		  {
			update = 0.5;
		  }
		  else
		  {
			update = dirder / (2 * dirder + reductionActual);
		  }
		  if (ynorm1 / 10 >= ynorm  ||  update < 0.1)
		  {
			update = (T) 0.1;
		  }
		  delta = update * std::min (delta, pnorm * 10);
		  par /= update;
		}
		else
		{
		  if (par == 0  ||  ratio >= 0.75)
		  {
			delta = pnorm * 2;
			par /= 2;
		  }
		}

		if (ratio >= 0.0001)  // successful iteration.
		{
		  // update x, y, and their norms
		  x     = xp;
		  y     = tempY;
		  xnorm = (x & scales).norm (2);
		  ynorm = ynorm1;
		}

		// tests for convergence
		if (   std::fabs (reductionActual) <= toleranceF
			&& reductionPredicted <= toleranceF
			&& ratio <= 2)
		{
		  // info = 1;
		  return;
		}
		if (delta <= toleranceX * xnorm)
		{
		  // info = 2;
		  return;
		}

		// tests for termination and stringent tolerances
		if (   std::fabs (reductionActual) <= epsilon
			&& reductionPredicted <= epsilon
			&& ratio <= 2)
		{
		  throw (int) 6;
		}
		if (delta <= epsilon * xnorm)
		{
		  throw (int) 7;
		}
		if (gnorm <= epsilon)
		{
		  throw (int) 8;
		}
	  }
	}

	throw (int) 5;  // exceeded maximum iterations
  }

  /**
	 qrsolv algorithm:
	 A generalized least squares problem:  Solve the system
	   Jx=y  and  dx=0,
	 where Jp=QR.  Substitute x=pz to get
	   Rz=~Qy  and  ~pdpz=0.

	 @param J The QR-factored Jacobian.  Since Q has already been applied to
	 y in Qy, we don't need it any more, so the lower-triangular part is free.
	 @param S A matrix such that ~SS = ~p(~JJ+dd)p
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::qrsolv (const Matrix<T> & J, const Vector<int> & pivots, const Vector<T> & d, const Vector<T> & Qy, Vector<T> & x, Matrix<T> & S)
  {
	const int n = J.columns ();

	// Copy J and Qy to preserve input and initialize s.
	// In particular, save the diagonal elements of J in x.
	Vector<T> z;
	z.copyFrom (Qy);
	S.copyFrom (J.region (0, 0, n-1, n-1));  // copies twice as many elements as necessary

	// Eliminate the diagonal matrix d using a givens rotation.
	Vector<T> sdiag (n);
	for (int j = 0; j < n; j++)
	{
	  // Prepare the row of d to be eliminated, locating the
	  // diagonal element using p from the qr factorization.
	  int l = pivots[j] - 1;
	  if (d[l] != 0)
	  {
		sdiag[j] = d[l];
		sdiag.region (j+1).clear ();

		// The transformations to eliminate the row of d modify only a single
		// element of Qy beyond the first n.  This element is is initially zero.
		T extraElement = 0;
		for (int k = j; k < n; k++)
		{
		  // Determine a givens rotation which eliminates the appropriate
		  // element in the current row of d.
		  if (sdiag[k] == 0) continue;
		  T sin;
		  T cos;
		  if (std::fabs (S(k,k)) < std::fabs (sdiag[k]))
		  {
			T cotan = S(k,k) / sdiag[k];
			sin = 0.5 / std::sqrt (0.25 + 0.25 * cotan * cotan);
			cos = sin * cotan;
		  }
		  else
		  {
			T tan = sdiag[k] / S(k,k);
			cos = 0.5 / std::sqrt (0.25 + 0.25 * tan * tan);
			sin = cos * tan;
		  }

		  // Compute the modified diagonal element of S and the modified
		  // extra element of Qy.
		  S(k,k)       =  cos * S(k,k) + sin * sdiag[k];
		  T temp       =  cos * z[k]   + sin * extraElement;
		  extraElement = -sin * z[k]   + cos * extraElement;
		  z[k] = temp;

		  // Accumulate the tranformation in the row of S.
		  for (int i = k + 1; i < n; i++)
		  {
			T temp   =  cos * S(k,i) + sin * sdiag[i];
			sdiag[i] = -sin * S(k,i) + cos * sdiag[i];
			S(k,i)   = temp;
		  }
		}
	  }
	}

	// Solve the triangular system for z.  If the system is singular,
	// then obtain a least squares solution.
	int nsing;
	for (nsing = 0; nsing < n; nsing++) if (sdiag[nsing] == 0) break;
	if (nsing < n) z.region (nsing).clear ();
	trsm ('L', 'U', 'T', 'N', nsing, 1, (T) 1, &S(0,0), S.strideC, &z[0], z.strideC);

	// Permute the components of z back to components of x.
	for (int j = 0; j < n; j++)
	{
	  x[pivots[j]-1] = z[j];
	}
  }

  /**
	 lmpar algorithm:

	 A constrained lls problem:
	   solve (~JJ + pDD)x = ~Jf
	   such that |Dx| is pretty close to delta

	 Start with p = 0 and determine x
	   Solve for x in ~JJx = ~Jf
	   Early out if |Dx| is close to delta
	 Determine min and max values for p
	   J = QR  (so ~JJ = ~RR)
	   solve for b in ~Rb = DDx / |Dx|
	   min = (|Dx| - delta) / (delta * |b|^2)
	   max = |!D~Jf| / delta
	 Initialize p
	   make sure it is in bounds
	   if p is zero, p = |!D~Jf| / |Dx|
	 Iterate
	   solve for x in (~JJ + pDD)x = ~Jf
	   end if |Dx| is close to delta
	     or too many iterations
		 or |Dx| is becoming smaller than delta when min == 0
	   (~JJ + pDD) = QR
	   solve for b in ~Rb = DDx / |Dx|
	   p += (|Dx| - delta) / (delta * |b|^2)
  **/
  template<class T>
  void
  LevenbergMarquardt<T>::lmpar (const Matrix<T> & J, const Vector<int> & pivots, const Vector<T> & scales, const Vector<T> & Qy, T delta, T & par, Vector<T> & x)
  {
	const T minimum = std::numeric_limits<T>::min ();
	const int n = J.columns ();

	Matrix<T> S;
	Vector<T> d (n);

	// Compute and store in x the gauss-newton direction. If the Jacobian is
	// rank-deficient, obtain a least squares solution.
	int nsing;  // index of the first zero diagonal of J.  If no such element exists, then nsing points to one past then end of the diagonal.
	for (nsing = 0; nsing < n; nsing++) if (J(nsing,nsing) == 0) break;
	d.region (0) = Qy.region (0, 0, nsing - 1, 0);
	if (nsing < n) d.region (nsing).clear ();
	//   Solve for x by back-substitution in Rx=~Qy (which comes from QRx=y, where J=QR).
	trsm ('L', 'U', 'N', 'N', nsing, 1, (T) 1, &J(0,0), J.strideC, &d[0], d.strideC);
	for (int j = 0; j < n; j++) x[pivots[j]-1] = d[j];

	// Initialize the iteration counter.
	// Evaluate the function at the origin, and test
	// for acceptance of the gauss-newton direction.
	Vector<T> dx = x & scales;
	T dxnorm = dx.norm (2);
	T fp = dxnorm - delta;
	if (fp <= 0.1 * delta)
	{
	  par = 0;
	  return;
	}

	// If the jacobian is not rank deficient, the newton
	// step provides a lower bound, parl, for the zero of
	// the function. otherwise set this bound to zero.
	T parl = 0;
	if (nsing == n)
	{
	  for (int j = 0; j < n; j++)
	  {
		int l = pivots[j] - 1;
		d[j] = scales[l] * (dx[l] / dxnorm);
	  }

	  // Solve by back-substitution for b in ~Rb=x (where "x" = d*d*x and x is
	  // normalized).
	  trsm ('L', 'U', 'T', 'N', n, 1, (T) 1, &J(0,0), J.strideC, &d[0], d.strideC);

	  T temp = d.norm (2);
	  parl = fp / delta / temp / temp;
	}

	// Calculate an upper bound, paru, for the zero of the function.
	//   d = ~R~Qy, equivalent to ~J * y before factoriztion
	d.copyFrom (Qy);
	trmm ('L', 'U', 'T', 'N', n, 1, (T) 1, &J(0,0), J.strideC, &d[0], d.strideC);
	for (int j = 0; j < n; j++) d[j] /= scales[pivots[j]-1];
	T gnorm = d.norm (2);
	T paru = gnorm / delta;
	if (paru == 0)
	{
	  paru = minimum / std::min (delta, (T) 0.1);
	}

	// If the input par lies outside of the interval (parl,paru),
	// set par to the closer endpoint.
	par = std::max (par, parl);
	par = std::min (par, paru);
	if (par == 0)
	{
	  par = gnorm / dxnorm;
	}

	int iter = 0;
	while (true)
	{
	  iter++;

	  // Evaluate the function at the current value of par.
	  if (par == (T) 0) par = std::max (minimum, (T) 0.001 * paru);
	  d = scales * std::sqrt (par);

	  qrsolv (J, pivots, d, Qy, x, S);

	  dx = x & scales;
	  dxnorm = dx.norm (2);
	  T temp = fp;
	  fp = dxnorm - delta;

	  // If the function is small enough, accept the current value
	  // of par.  Also test for the exceptional cases where parl
	  // is zero or the number of iterations has reached 10.
	  if (   std::fabs (fp) <= 0.1 * delta
		  || (parl == 0  &&  fp <= temp  &&  temp < 0)
		  || iter >= 10)
	  {
		return;
	  }

	  // Compute the Newton correction.
	  for (int j = 0; j < n; j++)
	  {
		int l = pivots[j] - 1;
		d[j] = scales[l] * (dx[l] / dxnorm);
	  }
	  trsm ('L', 'U', 'T', 'N', n, 1, (T) 1, &S(0,0), S.strideC, &d[0], d.strideC);

	  temp = d.norm (2);
	  T parc = fp / delta / temp / temp;

	  // Depending on the sign of the function, update parl or paru.
	  if (fp > 0)
	  {
		parl = std::max (parl, par);
	  }
	  if (fp < 0)
	  {
		paru = std::min (paru, par);
	  }
	  // Compute an improved estimate for par.
	  par = std::max (parl, par + parc);
	}
  }
}


#endif
