#ifndef WIN_TIME_COMPAT_H
#define WIN_TIME_COMPAT_H

#ifdef _WIN32
#include <windows.h>
#include <time.h>

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

// Polyfill for POSIX clock_gettime on MSVC
#if !defined(HAVE_CLOCK_GETTIME)
static __inline int clock_gettime(int clock_id, struct timespec *spec) {
    (void)clock_id;
    __int64 wintime; 
    
    // GetSystemTimePreciseAsFileTime is preferred for its higher resolution.
    // It is available on Windows 8 / Server 2012 and later.
    // We can fall back to GetSystemTimeAsFileTime for older systems if needed,
    // but for a modern security tool, assuming a recent OS is reasonable.
    GetSystemTimePreciseAsFileTime((FILETIME*)&wintime);

    // Convert from Windows epoch (1601-01-01) to Unix epoch (1970-01-01)
    wintime -= 116444736000000000LL;
    
    spec->tv_sec  = (time_t)(wintime / 10000000LL);
    spec->tv_nsec = (long)((wintime % 10000000LL) * 100);
    return 0;
}
#endif // !defined(HAVE_CLOCK_GETTIME)

#endif // _WIN32

#endif // WIN_TIME_COMPAT_H
