/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.5 Copyright 2005 Sandia Corporation.
Revisions 1.7 and 1.8 Copyright 2007 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/03/25 14:06:44  Fred
Fix template instantiation for Factory.

Revision 1.7  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/13 03:41:49  Fred
Add Sandia distribution terms.

Move stream operators from matrix.h to Matrix.tcc

Revision 1.4  2005/09/12 03:27:12  Fred
Fix template instantiation for GCC 3.4.4.

Add Sandia copyright notice.  Must add license information before release.

Revision 1.3  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

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
#include "fl/serialize.h"


using namespace fl;


template class MatrixAbstract<double>;
template class Matrix<double>;
template class MatrixTranspose<double>;
template class MatrixRegion<double>;

template class Factory<MatrixAbstract<double> >;

namespace fl
{
  template std::ostream & operator << (std::ostream & stream, const MatrixAbstract<double> & A);
  template std::istream & operator >> (std::istream & stream, MatrixAbstract<double> & A);
  template MatrixAbstract<double> & operator << (MatrixAbstract<double> & A, const std::string & source);
}
