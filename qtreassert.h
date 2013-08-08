#ifndef INCLUDE_QTREASSERT_H_
#define INCLUDE_QTREASSERT_H_

/*
   qtreassert.h
   Revision 27-jan-2006
*/

#include "config_debug.h"


#ifndef NDEBUG

#ifdef __cplusplus
extern "C"
#endif
void qtre_assert_failed (const char * a, const char * file, size_t line);

#define ASSERT(a) if (a) ; else \
	qtre_assert_failed (#a, __FILE__, __LINE__)

#else

#define ASSERT(a)

#endif

#endif

/* End of qtreassert.h */
