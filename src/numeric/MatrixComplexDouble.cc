/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.5 and 1.6 Copyright 2005 Sandia Corporation.
Revisions 1.8 and 1.9 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.9  2007/03/25 14:06:44  Fred
Fix template instantiation for Factory.

Revision 1.8  2007/03/23 10:57:28  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/13 03:41:00  Fred
Add Sandia distribution terms.

Move stream operators from matrix.h to Matrix.tcc

Revision 1.5  2005/09/12 03:26:42  Fred
Fix template instantiation for GCC 3.4.4.

Add Sandia copyright notice.  Must add license information before release.

Revision 1.4  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.3  2004/05/10 20:12:28  rothgang
Switch from using C99 style complex numbers to STL complex template.

Revision 1.2  2004/04/19 17:24:16  rothgang
Add productMap for MatrixAbstract.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#include "fl/Matrix.tcc"
#include "fl/MatrixComplex.tcc"
#include "fl/serialize.h"


using namespace fl;
using namespace std;


template class MatrixAbstract<complex<double> >;
template class Matrix<complex<double> >;
template class MatrixTranspose<complex<double> >;
template class MatrixRegion<complex<double> >;

template class Factory<MatrixAbstract<complex<double> > >;

namespace fl
{
  template std::ostream & operator << (std::ostream & stream, const MatrixAbstract<complex<double> > & A);
  template std::istream & operator >> (std::istream & stream, MatrixAbstract<complex<double> > & A);
  template MatrixAbstract<complex<double> > & operator << (MatrixAbstract<complex<double> > & A, const std::string & source);
}
