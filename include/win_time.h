#ifndef WIN_TIME_COMPAT_H
#define WIN_TIME_COMPAT_H

#ifdef _WIN32
#include <windows.h>
#include <time.h> // This header provides 'struct timespec' on modern MSVC

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

// Polyfill for POSIX clock_gettime on MSVC
#if !defined(HAVE_CLOCK_GETTIME)
static __inline int clock_gettime(int clock_id, struct timespec *spec) {
    (void)clock_id; // Unused on this Windows implementation
    long long wintime;
    GetSystemTimeAsFileTime((FILETIME*)&wintime);
    wintime -= 116444736000000000LL;  // Epoch difference (1601 to 1970)
    spec->tv_sec  = (time_t)(wintime / 10000000LL);
    spec->tv_nsec = (long)((wintime % 10000000LL) * 100);
    return 0;
}
#endif // !defined(HAVE_CLOCK_GETTIME)

#endif // _WIN32

#endif // WIN_TIME_COMPAT_H
