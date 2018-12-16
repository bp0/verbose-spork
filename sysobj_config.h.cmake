#ifndef __SYSOB_CONFIG_H__
#define __SYSOB_CONFIG_H__

#ifndef SYSOB_SRC_ROOT
#define SYSOB_SRC_ROOT ""
#endif

#if !defined SYSOB_DEBUG_BUILD
#define SYSOB_DEBUG_BUILD 0
#endif

#if (SYSOB_DEBUG_BUILD == 1)
#   define DEBUG(msg,...) fprintf(stderr, "*** %s:%d (%s) *** " msg "\n", \
        __FILE__ + sizeof(SYSOB_SRC_ROOT), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#   define DEBUG(msg,...)
#endif  /* SYSOB_DEBUG_BUILD */

#define DBPING printf("*** %s:%d (%s) PING ***\n", __FILE__, __LINE__, __FUNCTION__);

#endif  /* __SYSOB_CONFIG_H__ */

