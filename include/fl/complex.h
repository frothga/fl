#ifndef fl_complex_h
#define fl_complex_h


// Support for complex types.
// Two options exist:
// 1) Use std::complex<T>
// 2) Use C99 standard support for complex types.
// Option 2 is more elegant, though less portable.  It depends on the
// compiler's willingness to support the C99 standard in C++ code.  This seems
// like a reasonable assumption.  Option 2 also allows the use of GNU
// extensions which make working with complex numbers even more elegant (at
// the complete sacrifice of portability).
// What we lose from not using option 1 is immediate support for stream
// insertion and extraction.  This is easy enough to code, so we choose to use
// C99 standard support, GNU extensions, and code some stream operators...

#include </usr/include/complex.h>
#include <iostream>

#ifndef complex
#define complex _Complex
#endif


// Can't template on just the base type of the complex variable, so we must
// explicitly code each one.

inline std::ostream &
operator << (std::ostream & stream, const complex float & c)
{
  stream << __real__ c;
  if (__imag__ c >= 0)
  {
	stream << "+";
  }
  stream << __imag__ c << "i";

  return stream;
}

inline std::ostream &
operator << (std::ostream & stream, const complex double & c)
{
  stream << __real__ c;
  if (__imag__ c >= 0)
  {
	stream << "+";
  }
  stream << __imag__ c << "i";

  return stream;
}



#endif
