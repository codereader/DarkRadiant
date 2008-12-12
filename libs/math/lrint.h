#ifndef LRINT_H_
#define LRINT_H_

/**
 * Casting a double to an int the fast way.
 */
#if defined (_MSC_VER)

#if defined (_WIN64)

// Only for SSE2 or x64 

#include <emmintrin.h>  

// greebo: VC++ x64 doesn't support inline assembly, we have to use x64 intrinsics instead
inline int lrint(const double x) {
	return _mm_cvtsd_si32(_mm_load_sd(&x));
}

#else

	// Win32 target
	inline int lrint (double flt) {
		int i;
		
		_asm {
			fld flt
				fistp i
		};
		
		return i;
	} 
#endif

#elif defined(__FreeBSD__)

inline int lrint(double f)
{
  return static_cast<int>(f + 0.5);
}

#elif defined(__GNUC__)

 // lrint is part of ISO C99
#define	_ISOC9X_SOURCE	1
#define _ISOC99_SOURCE	1

#define	__USE_ISOC9X	1
#define	__USE_ISOC99	1

#else
#error "unsupported platform"
#endif

#endif /*LRINT_H_*/
