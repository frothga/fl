/*
Author: Fred Rothganger
Created 3/11/2006 to machine endian related defines and functions
*/


#ifndef fl_endian_h
#define fl_endian_h


#ifdef _MSC_VER


// MSVC generally compiles to i86.  Deal with other cases (such as alpha)
// as they come up.
#define __LITTLE_ENDIAN 1234
#define __BYTE_ORDER __LITTLE_ENDIAN

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
bswap (unsigned int * x, unsigned int count = 1)
{
  __asm
  {
	mov   ebx, x
	mov   ecx, count
	top:
	mov   eax, [ebx]
	bswap eax
	mov   [ebx], eax
	add   ebx, 4
	loop  top
  }
}

static inline void
bswap (unsigned short * x, unsigned int count = 1)
{
  __asm
  {
	mov  ebx, x
	mov  ecx, count
	top:
	ror  WORD PTR [ebx], 8
	add  ebx, 2
	loop top
  }
}


#else  // GCC


#include <sys/param.h>

static inline unsigned int
bswap (unsigned int x)
{
  __asm ("bswap %0" : "=r" (x) : "0" (x));
  return x;
}

static inline void
bswap (unsigned int * x, unsigned int count = 1)
{
  unsigned int * end = x + count;
  while (x < end)
  {
	__asm ("bswap %0" : "=r" (*x) : "0" (*x));
	x++;
  }
}

static inline void
bswap (unsigned short * x, unsigned int count = 1)
{
  unsigned short * end = x + count;
  while (x < end)
  {
	__asm ("rorw $8, %0" : "=q" (*x) : "0" (*x));
	x++;
  }
}


#endif  // select compiler


static inline void
bswap (unsigned long long * x, unsigned int count = 1)
{
  throw "Need to write 64-bit bswap";
}


#endif
