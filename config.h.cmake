#ifndef __CONFIG_H__
#define __CONFIG_H__

#if !defined DEBUG_BUILD
#define DEBUG_BUILD 0
#endif

#if (DEBUG_BUILD == 1)
#   define DEBUG(msg,...) fprintf(stderr, "*** %s:%d (%s) *** " msg "\n", \
        __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#   define DEBUG(msg,...)
#endif  /* DEBUG_BUILD */

#endif  /* __CONFIG_H__ */

