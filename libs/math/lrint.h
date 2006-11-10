#ifndef LRINT_H_
#define LRINT_H_

#if defined (_MSC_VER)

inline int lrint (double flt)
{
  int i;

	_asm
	{
    fld flt
		fistp i
  };
			
	return i;
} 

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
