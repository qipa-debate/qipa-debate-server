#ifndef __UTIL_H__
#define __UTIL_H__
#include <sys/time.h>

inline int64_t now_ms() {
  timeval now;
  gettimeofday(&now, NULL);
  return now.tv_sec * 1000ll + now.tv_usec / 1000;
}

#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>

#if !defined(LOG)
#include <cstdio>
#define LOG(level, fmt, ...) printf("%" PRId64 ": " fmt "\n", now_ms(), ## __VA_ARGS__)
#endif

#ifdef __GNUC__

#if __GNUC_PREREQ(4, 6)
#include <atomic>
typedef std::atomic<bool> atomic_bool_t;
#else
typedef volatile bool atomic_bool_t;
#endif
#endif


#define TRUE true
#define FALSE false

#define CHECK_POINTER(pValue, rValue, ...) 	if (NULL == pValue) {  \
					                           LOG(ERROR, __VA_ARGS__); \
					                           return (rValue); }

#define CHECK_POINTER_VOID(pValue, ...) 	if (NULL == pValue) {  \
					                           LOG(ERROR, __VA_ARGS__); \
					                           return; }

#endif /* __UTIL_H__ */
