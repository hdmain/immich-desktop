#include "core/ProcessMemory.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <malloc.h>
#elif defined(Q_OS_MACOS)
#include <malloc/malloc.h>
#endif

namespace Aurora {

void trimProcessMemory()
{
#if defined(Q_OS_WIN)
    if (HANDLE process = GetCurrentProcess())
        EmptyWorkingSet(process);
#elif defined(Q_OS_LINUX) && defined(__GLIBC__)
    // Return free heap arenas to the OS so RSS drops after cache clears.
    malloc_trim(0);
#elif defined(Q_OS_MACOS)
    // Hint libmalloc that large transient allocations can be reclaimed.
    malloc_zone_pressure_relief(nullptr, 0);
#endif
}

} // namespace Aurora
