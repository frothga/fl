/*
Author: Fred Rothganger
Copyright (c) 2001-2004 Dept. of Computer Science and Beckman Institute,
                        Univ. of Illinois.  All rights reserved.
Distributed under the UIUC/NCSA Open Source License.  See the file LICENSE
for details.


Revisions 1.2 thru 1.7 Copyright 2005 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.8  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.7  2005/10/09 03:57:53  Fred
Place UIUC license in the file LICENSE rather than LICENSE-UIUC.

Revision 1.6  2005/10/09 03:41:49  Fred
Move the file name LICENSE up to previous line, for better symmetry with UIUC
notice.

Revision 1.5  2005/10/08 19:22:45  Fred
Update revision history and add Sandia copyright notice.

Add BLUEGRAY and company.

Revision 1.4  2005/05/02 00:25:20  Fred
Change all symbolic constants to upper case, in keeping with C programming
conventions.

Moved alpha channel to LSB in order to be compatible with new convention for
get/setRGBA().

Revision 1.3  2005/04/23 19:35:05  Fred
Add UIUC copyright notice.  Note files that I revised after leaving UIUC on
11/21.

Revision 1.2  2005/01/12 04:55:55  rothgang
Warning about conflict with other headers.

Revision 1.1  2003/07/08 23:19:47  rothgang
branches:  1.1.1;
Initial revision

Revision 1.1.1.1  2003/07/08 23:19:47  rothgang
Imported sources
-------------------------------------------------------------------------------
*/


#ifndef fl_color_h
#define fl_color_h


#define WHITE     0xFFFFFFFF
#define GRAY100   0xFFFFFFFF
#define GRAY50    0x808080FF
#define GRAY0     0x000000FF
#define BLACK     0x000000FF

#define RED       0xFF0000FF
#define GREEN     0x00FF00FF

#define BLUE100    0x0000FFFF
#define BLUEGRAY50 0x8080FFFF
#define BLUE       BLUE100

#define PURPLE    0xFF00FFFF

#define CYAN100   0x00FFFFFF
#define CYAN50    0x008080FF
#define CYAN      CYAN100

#define YELLOW100 0xFFFF00FF
#define YELLOW90  0xE6E600FF
#define YELLOW75  0xC0C000FF
#define YELLOW50  0x808000FF
#define YELLOW    YELLOW90


#endif
