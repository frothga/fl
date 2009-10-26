/*
Author: Fred Rothganger
Created 10/19/2009 to support thread-safe Pointer implementation on generic
systems.


Copyright 2010 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.
*/


#include "fl/pointer.h"


#ifdef needFLmutexPointer
pthread_mutex_t fl::mutexPointer = PTHREAD_MUTEX_INITIALIZER;
#endif
