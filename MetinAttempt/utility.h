#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

#include <cstdint>
#include <cstdlib>

static inline LARGE_INTEGER get_current_time()
{
    FILETIME now;

    GetSystemTimeAsFileTime(&now);
    LARGE_INTEGER li;

    li.LowPart = now.dwLowDateTime;
    li.HighPart = now.dwHighDateTime;

    return li;
}

inline uint32_t add_random_prc(uint32_t num, uint16_t randomness)
{
    return num + ((num * (rand() % randomness)) / 100);
}

void maxMinWindow(int minutes_min, int seconds_max, int proc);

#define CONCAT_(x,y) x##y
#define CONCAT(x,y) CONCAT_(x,y)

#define CALL_EVERY(time_ms, func, ...) \
    { static LARGE_INTEGER CONCAT(_TIME_, __LINE__){ }; \
    if ((get_current_time().QuadPart - CONCAT(_TIME_, __LINE__).QuadPart) / 10000 > time_ms) \
    { \
        func(__VA_ARGS__); \
        CONCAT(_TIME_, __LINE__) = get_current_time(); \
    }}
