// LM using direct calls to MINPACK.
// This is useful mainly as a reference point to verify that the other LM
// implementations are functionally equivalent.  IE: it is good for debugging.
// The other implementations are a better choice for general use because they
// don't require linking to external libraries.


#include "fl/search.h"


// For debugging
#include <iostream>
using namespace std;


using namespace fl;


// Minpack declarations -------------------------------------------------------

typedef void (* lmdifFunction) (const int & m,
								const int & n,
								double x[],
								double fvec[],
								int & iflag);

extern "C"
{
  void lmdif_ (lmdifFunction fcn,
			   const int & m,
			   const int & n,
			   double x[],     // length n
			   double fvec[],  // length m
			   const double & ftol,
			   const double & xtol,
			   const double & gtol,
			   const int & maxfev,
			   const double & epsfcn,
			   double diag[],  // length n
			   const int & mode,
			   const double & factor,
			   const int & nprint,
			   int & info,
			   int & nfev,
			   double fjac[],  // size m * n
			   const int & ldfjac,
               int ipvt[],     // length n
			   double qtf[],   // length n
			   double wa1[],   // length n
			   double wa2[],   // length n
			   double wa3[],   // length n
			   double wa4[]);  // length m

  double dpmpar_ (const int & i);
}



// class LevenbergMarquardtMinpack --------------------------------------------

LevenbergMarquardtMinpack::LevenbergMarquardtMinpack (double toleranceF, double toleranceX, int maxIterations)
{
  this->maxIterations = maxIterations;

  if (toleranceF < 0)
  {
	toleranceF = sqrt (dpmpar_ (1));
  }
  this->toleranceF = toleranceF;

  if (toleranceX < 0)
  {
	toleranceX = sqrt (dpmpar_ (1));
  }
  this->toleranceX = toleranceX;
}

union PointerHack
{
  double value;
  Searchable * pointer;
};

void
LevenbergMarquardtMinpack::search (Searchable & searchable, Vector<double> & point)
{
  int m = searchable.dimension ();
  int n = point.rows ();

  double gtol = 0;
  int maxfev = maxIterations * (n + 1);
  int mode = 1;
  double factor = 1.0;

  double perturbation = dpmpar_ (1);
  if (SearchableNumeric * sn = dynamic_cast<SearchableNumeric *> (&searchable))
  {
	perturbation = sn->perturbation;
	perturbation *= perturbation;  // because fdjac2 does a sqrt on this value
  }

  double * fvec = new double[m + 1];  // One extra space to hold pointer to searchable
  double * diag = new double[n];
  double * fjac = new double[m * n];
  int *    ipvt = new int[n];
  double * qtf  = new double[n];
  double * wa   = new double[3 * n];
  double * wa4  = new double[m + 1];  // Also has a holder for pointer to searchable

  int nfev;
  int info;

  PointerHack p;
  p.pointer = &searchable;
  fvec[0] = p.value;
  wa4[0] = p.value;

  lmdif_ (fcn,
		  m,
		  n,
		  & point[0],
		  & fvec[1],
		  toleranceF,
		  toleranceX,
		  gtol,
		  maxfev,
		  perturbation,
		  diag,
		  mode,
		  factor,
		  0,
		  info,
		  nfev,
		  fjac,
		  m,
		  ipvt,
		  qtf,
		  & wa[0],
		  & wa[n],
		  & wa[2 * n],
		  & wa4[1]);

  delete fvec;
  delete diag;
  delete fjac;
  delete ipvt;
  delete qtf;
  delete wa;
  delete wa4;

  if (info < 1  ||  info > 4)
  {
	throw info;
  }
}

void
LevenbergMarquardtMinpack::fcn (const int & m, const int & n, double x[], double fvec[], int & info)
{
  PointerHack p;
  p.value = fvec[-1];
  Vector<double> value (fvec, m);
  p.pointer->value (Vector<double> (x, n), value);
  if (value.data.memory != fvec)
  {
	memcpy (fvec, (double *) value.data, m * sizeof (double));
  }
}
