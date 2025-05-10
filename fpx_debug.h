#ifndef FPX_DEBUG_H
#define FPX_DEBUG_H

#include <stdio.h>
#include <time.h>

#ifdef FPX_DEBUG_ENABLE

#define FPX_DEBUG(s) printf("\e[0;32mFPXLIBC DEBUG:\e[0m %s\e[0m\n", s)

#else

#define FPX_DEBUG(s)

#endif // FPX_DEBG_ENABLE


#endif // FPX_DEBUG_H
