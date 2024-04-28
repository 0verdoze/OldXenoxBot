#include "utility.h"

HWND windowHandle = NULL;
BOOL CALLBACK EnumWindowsProcMy(HWND hwnd, LPARAM lParam)
{
    DWORD lpdwProcessId;
    char windowName[512]{ 0 };

    GetWindowThreadProcessId(hwnd, &lpdwProcessId);
    if (lpdwProcessId == lParam)
    {
        GetWindowTextA(hwnd, windowName, sizeof(windowName));
        if (strcmp(windowName, "XenoxMT2 - Nowe oblicze legendy!") == 0)
        {
            windowHandle = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

void maxMinWindow(int minutes_min, int seconds_max, int proc)
{
    static LARGE_INTEGER nextMinMax{ };
    static LARGE_INTEGER minMaxEnd{ };

    // std::cout << windowHandle << '\n';

    if (windowHandle == NULL)
    {
        EnumWindows(EnumWindowsProcMy, GetCurrentProcessId());
        if (windowHandle == NULL)
            return;
    }

    if (get_current_time().QuadPart > nextMinMax.QuadPart)
    {
        // max
        nextMinMax.QuadPart = get_current_time().QuadPart + (add_random_prc(minutes_min * 60 * 1000, proc) * 10000ULL);
        minMaxEnd.QuadPart = get_current_time().QuadPart + seconds_max * 1000ULL * 10000ULL;
        ShowWindow(windowHandle, SW_RESTORE);
    }
    else if (get_current_time().QuadPart > minMaxEnd.QuadPart)
    {
        // min
        ShowWindow(windowHandle, SW_MINIMIZE);
        minMaxEnd.QuadPart = LLONG_MAX;
    }
}