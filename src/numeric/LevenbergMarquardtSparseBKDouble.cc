/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.4 and 1.5 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.7  2007/03/23 10:57:27  Fred
Use CVS Log to generate revision history.

Revision 1.6  2005/10/13 04:14:25  Fred
Put UIUC license info in the file LICENSE rather than LICENSE-UIUC.

Revision 1.5  2005/10/13 03:33:53  Fred
Add Sandia distribution terms.

Revision 1.4  2005/09/12 03:25:46  Fred
Fix template instantiation for GCC 3.4.4.

Add Sandia copyright notice.  Must add license information before release.

Revision 1.3  2005/04/23 19:40:05  Fred
Add UIUC copyright notice.

Revision 1.2  2004/05/10 17:48:05  rothgang
Move definitions of epsilon and minimum from include file into source file
because they were creating multiple definition errors at link time.

Revision 1.1  2004/04/19 21:21:41  rothgang
Instantiate template versions of Search classes.
-------------------------------------------------------------------------------
*/


#include "fl/LevenbergMarquardtSparseBK.tcc"


using namespace fl;


template <> const double LevenbergMarquardtSparseBK<double>::epsilon = DBL_EPSILON;
template <> const double LevenbergMarquardtSparseBK<double>::minimum = DBL_MIN;
template class LevenbergMarquardtSparseBK<double>;
