/*
Author: Fred Rothganger
Created 10/5/2005 to hold non type-specific LAPACK prototypes.


Revision 1.1          Copyright 2005 Sandia Corporation.
Revision 1.2 thru 1.3 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.3  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.2  2006/02/03 14:26:22  Fred
Make fl::ilaenv() an inline function.  This was the original intention, but
instead it was being laid down in several object files, creating a conflict
when building shared libraries.

Revision 1.1  2005/10/09 03:50:02  Fred
Header file for general LAPACK prototypes that are not specific to any given
numeric type.
-------------------------------------------------------------------------------
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
