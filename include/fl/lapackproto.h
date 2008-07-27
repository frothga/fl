/*
Author: Fred Rothganger
Created 10/5/2005 to hold non type-specific LAPACK prototypes.


Copyright 2005, 2008 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#ifndef fl_lapackproto_h
#define fl_lapackproto_h


extern "C"
{
  // This approach to calling ilaenv assumes LAPACK is compiled by g77
  int ilaenv_ (const int &  ispec,
			   const char * name,
			   const char * opts,
			   const int &  n1,
			   const int &  n2,
			   const int &  n3,
			   const int &  n4,
			   const int    length_name,
			   const int    length_opts);
}

namespace fl
{
  inline int
  ilaenv (const int    ispec,
		  const char * name,
		  const char * opts,
		  const int    n1 = -1,
		  const int    n2 = -1,
		  const int    n3 = -1,
		  const int    n4 = -1)
  {
	return ilaenv_ (ispec, name, opts, n1, n2, n3, n4, strlen (name), strlen (opts));
  }
}


#endif
