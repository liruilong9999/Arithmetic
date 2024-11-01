#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

#include <stdlib.h>
#include "tools_gloabal.h"

#ifdef __cplusplus
extern "C"
{
#endif
TOOLS_EXPORT void * dbg_calloc(size_t nmemb, size_t size, const char *, int);
TOOLS_EXPORT void * dbg_malloc(size_t size, const char *, int);
TOOLS_EXPORT void * dbg_realloc(void * ptr, size_t size, const char *, int);
TOOLS_EXPORT char * dbg_strdup(const char * s, const char *, int);
TOOLS_EXPORT void   dbg_free(void * ptr, const char *, int);
TOOLS_EXPORT void   assureNoLeaks(const char * file);
TOOLS_EXPORT void   printAllocStats();
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
TOOLS_EXPORT void * operator new(size_t size, const char *, int, bool);
TOOLS_EXPORT void * operator new[](size_t size, const char *, int, bool);
 void   operator delete(void * ptr);
 void   operator delete[](void * ptr);
TOOLS_EXPORT void   removeEntry(void * ptr, const char *, int, int);
#endif

#if !defined(NDEBUG) && !defined(arm)
#define USING_DEBUG_ALLOC
#endif

#ifdef USING_DEBUG_ALLOC

#define CALLOC(nmemb, sz) dbg_calloc((nmemb), (sz), __FILE__, __LINE__)
#define MALLOC(sz)        dbg_malloc((sz), __FILE__, __LINE__)
#define REALLOC(ptr, sz)  dbg_realloc((ptr), (sz), __FILE__, __LINE__)
#define STRDUP(str)       dbg_strdup((str), __FILE__, __LINE__)
#define FREE(ptr)         dbg_free((ptr), __FILE__, __LINE__)

#ifdef __cplusplus
#define NEW        new (__FILE__, __LINE__, false)
#define NEW_OBJARR new (__FILE__, __LINE__, true)
#define DELETE(ptr)                                \
    do                                             \
    {                                              \
        removeEntry((ptr), __FILE__, __LINE__, 0); \
        delete (ptr);                              \
    } while (0)
#define DELETE_ARR(ptr)                            \
    do                                             \
    {                                              \
        removeEntry((ptr), __FILE__, __LINE__, 1); \
        delete[] (ptr);                            \
    } while (0)
#define DELETE_OBJARR(ptr)                         \
    do                                             \
    {                                              \
        removeEntry((ptr), __FILE__, __LINE__, 2); \
        delete[] (ptr);                            \
    } while (0)
#endif

#define ASSURE_NO_LEAKS()   assureNoLeaks(__FILE__)
#define PRINT_ALLOC_STATS() printAllocStats()

#else /* USING_DEBUG_ALLOC */

#define CALLOC(nmemb, sz) calloc((nmemb), (sz))
#define MALLOC(sz)        malloc(sz)
#define REALLOC(ptr, sz)  realloc((ptr), (sz))
#define STRDUP(str)       strdup(str)
#define FREE(ptr)         free(ptr)

#ifdef __cplusplus
#define NEW                new
#define NEW_OBJARR         new
#define DELETE(ptr)        delete (ptr)
#define DELETE_ARR(ptr)    delete[] (ptr)
#define DELETE_OBJARR(ptr) delete[] (ptr)
#endif

#define ASSURE_NO_LEAKS()
#define PRINT_ALLOC_STATS()

#endif /* USING_DEBUG_ALLOC */

#endif /* DEBUG_ALLOC_H */
