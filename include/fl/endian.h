/*
Author: Fred Rothganger
Created 3/11/2006 to provide machine endian related defines and functions


Revisions 1.1 thru 1.3 Copyright 2006 Sandia Corporation.
Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
the U.S. Government retains certain rights in this software.
Distributed under the GNU Lesser General Public License.  See the file LICENSE
for details.


-------------------------------------------------------------------------------
$Log$
Revision 1.4  2007/02/18 14:50:37  Fred
Use CVS Log to generate revision history.

Revision 1.3  2006/03/12 03:20:08  Fred
Convert to local labels in gcc code to avoid errors when bswap is inlined
multiple times in same program.  Haven't yet gotten local labels to work in
MSVC.  The MASM macro @@ is rejected by the inline assembler.  Labels in MSVC
inline assembly only have function scope, so as long as there is not more than
one instance of bswap per function it will be ok.

Revision 1.2  2006/03/12 00:27:52  Fred
Rearrange order of routines according to # of bits processed.

Add generic routines if not x86.

Write entire loop in x86 assembly rather than mixing C code.

Stop using double underscores on endian defines.

Revision 1.1  2006/03/11 19:56:42  Fred
Header for managing byte order issues.
-------------------------------------------------------------------------------
*/


#ifndef fl_endian_h
#define fl_endian_h


#ifdef _MSC_VER


// MSVC generally compiles to i86.  Deal with other cases (such as alpha)
// as they come up.
#define LITTLE_ENDIAN 1234
#define BYTE_ORDER    LITTLE_ENDIAN

static inline unsigned int
bswap (unsigned int x)
{
  __asm
  {
	mov   eax, x
	bswap eax
	mov   x, eax
  }
  return x;
}

static inline void
bswap (unsigned short * x, unsigned int count = 1)
{
  __asm
  {
	mov  ebx, x
	mov  ecx, count
	bswap16_top:
	ror  WORD PTR [ebx], 8
	add  ebx, 2
	loop bswap16_top
  }
}

static inline void
bswap (unsigned int * x, unsigned int count = 1)
{
  __asm
  {
	mov   ebx, x
	mov   ecx, count
	bswap32_top:
	mov   eax, [ebx]
	bswap eax
	mov   [ebx], eax
	add   ebx, 4
	loop  bswap32_top
  }
}

static inline void
bswap (unsigned long long * x, unsigned int count = 1)
{
  __asm
  {
	mov   ebx, x
	mov   ecx, count
	bswap32_top:
	mov   eax, [ebx]
	mov   edx, [ebx+4]
	bswap eax
	bswap edx
	mov   [ebx], edx
	mov   [ebx+4], eax
	add   ebx, 8
	loop  bswap32_top
  }
}


#else  // GCC


#include <sys/param.h>

#ifdef i386

static inline unsigned int
bswap (unsigned int x)
{
  __asm ("bswap %0" : "=r" (x) : "0" (x));
  return x;
}

static inline void
bswap (unsigned short * x, unsigned int count = 1)
{
  __asm ("1:"
		 "rorw   $8, (%0);"
		 "addl   $2, %0;"
		 "loopl  1b;"
		 :
		 : "r" (x), "c" (count));
}

static inline void
bswap (unsigned int * x, unsigned int count = 1)
{
  __asm ("1:"
		 "movl   (%0), %%eax;"
		 "bswap  %%eax;"
		 "movl   %%eax, (%0);"
		 "addl   $4, %0;"
		 "loopl  1b;"
		 :
		 : "r" (x), "c" (count)
		 : "eax");
}

#ifdef ARCH_X86_64

/// \todo Test this code!
static inline void
bswap (unsigned long long * x, unsigned int count = 1)
{
  __asm ("1:"
		 "mov    (%0), %%rax;"
		 "bswap  %%rax;"
		 "mov    %%rax, (%0);"
		 "add    $8, %0;"
		 "loop   1b;"
		 :
		 : "r" (x), "c" (count)
		 : "rax");
}

#else  // 32-bit x86

static inline void
bswap (unsigned long long * x, unsigned int count = 1)
{
  __asm ("1:"
		 "movl   (%0), %%eax;"
		 "movl   4(%0), %%edx;"
		 "bswap  %%eax;"
		 "bswap  %%edx;"
		 "movl   %%edx, (%0);"
		 "movl   %%eax, 4(%0);"
		 "addl   $8, %0;"
		 "loopl  1b;"
		 :
		 : "r" (x), "c" (count)
		 : "eax", "edx");
}

#endif  // select x64 version of bswap(long long)

#else   // not an x86 CPU, so use generic routines

#include <byteswap.h>

static inline unsigned int
bswap (unsigned int x)
{
  return bswap_32 (x);
}

static inline void
bswap (unsigned short * x, unsigned int count = 1)
{
  unsigned short * end = x + count;
  while (x < end)
  {
	*x = bswap_16 (*x);
	x++;
  }
}

static inline void
bswap (unsigned int * x, unsigned int count = 1)
{
  unsigned int * end = x + count;
  while (x < end)
  {
	*x = bswap_32 (*x);
	x++;
  }
}

static inline void
bswap (unsigned long long * x, unsigned int count = 1)
{
  unsigned long long * end = x + count;
  while (x < end)
  {
	*x = bswap_64 (*x);
	x++;
  }
}

#endif  // select CPU architecture

#endif  // select compiler


#endif
