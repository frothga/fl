#ifndef fl_minpackproto_h
#define fl_minpackproto_h


// C/C++ interfaces to MINPACK library


typedef void (* lmdifFunction) (const int & m,
								const int & n,
								double x[],
								double fvec[],
								int & iflag);

typedef void (* lmstrFunction) (const int & m,
								const int & n,
								double x[],
								double fvec[],  // length m
								double fjrow[], // length n
								int & iflag);

extern "C"
{
  void lmdif1_ (lmdifFunction fcn,
				const int & m,
				const int & n,
				double x[],
				double fvec[],
				const double & tol,
				int & info,
				int iwa[],  // Must have at least n elements
				double wa[],  // Must have at least m * n + 5 * n + m.  Routine uses m *n + 2 * m + 6 * n.  Go figure.
				const int & lwa);

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

  void lmstr_ (lmstrFunction fcn,
			   const int & m,
			   const int & n,
			   double x[],     // length n
			   double fvec[],  // length m
			   double fjac[],  // size n * n
			   const int & ldfjac,
			   const double & ftol,
			   const double & xtol,
			   const double & gtol,
			   const int & maxfev,
			   double diag[],  // length n
			   const int & mode,
			   const double & factor,
			   const int & nprint,
			   int & info,
			   int & nfev,
			   int & njev,
			   int ipvt[],     // length n
			   double qtf[],   // length n
			   double wa1[],   // length n
			   double wa2[],   // length n
			   double wa3[],   // length n
			   double wa4[]);  // length m

  double dpmpar_ (const int & i);
}


#endif
