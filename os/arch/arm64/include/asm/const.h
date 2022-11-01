#ifndef __ASM_CONST_H
#define __ASM_CONST_H

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

#define _UL(x)		(_AC(x, UL))
#define _ULL(x)		(_AC(x, ULL))
  
#define UL(x)		(_UL(x))
#define ULL(x)		(_ULL(x))
  
#define BIT(nr)		(UL(1) << (nr))
  
#define GENMASK(h, l) \
        (((~UL(0)) - (UL(1) << (l)) + 1) & \
         (~UL(0) >> (BITS_PER_LONG - 1 - (h))))
#endif
