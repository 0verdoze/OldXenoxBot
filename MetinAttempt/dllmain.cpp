// dllmain.cpp : Definiuje punkt wejścia dla aplikacji DLL.
// #include "pch.h"

#include "framework.h"
#include <iostream>
#include <thread>

#include "FunctionPatching.h"
#include "PatchedFunctions.h"

#define MAIN_HOOK 1
#define KILL_SWITCH 0

#if KILL_SWITCH
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <IcmpAPI.h>
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Ws2_32.lib")
#endif


bool is_attached = false;

std::thread t1;
std::thread killSwitch;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    FILE* fDummy;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        if (is_attached)
            break;

#if MAIN_HOOK
        AllocConsole();

#if ENABLE_PYTHON_CONSOLE
        freopen_s(&fDummy, "CONIN$", "r", stdin);
#endif
        freopen_s(&fDummy, "CONOUT$", "w", stdout);

        for (const auto& pair : patch_map)
        {
            std::cout << "Patching " << pair.first << '\n';

            for (auto addr : pair.second.to_patch)
                patch_function((int)addr.first, (void*)pair.second.new_function, addr.second);
        }

        std::cout << "Patching main loop\n";
        int addr = 0x004C7E5F;

        DWORD oldProtect = 0;
        VirtualProtect((void*)addr, 2 + 5, PAGE_EXECUTE_WRITECOPY, &oldProtect);
        *(uint8_t*)addr = (uint8_t)0xE8;
        patch_function(addr, hack_main, PATCH_TYPE::E8_PATCH);

        ((uint8_t*)addr)[5] = (uint8_t)0xEB;
        ((uint8_t*)addr)[6] = (uint8_t)0xD2 - 5;

        VirtualProtect((void*)addr, 2 + 6, oldProtect, NULL);

#if ENABLE_PYTHON_CONSOLE
        t1 = std::thread(cin_reader);
#endif
#endif

#if KILL_SWITCH
        killSwitch = std::thread([]()
            {
                HANDLE icmp_handle = IcmpCreateFile();
                if (icmp_handle == INVALID_HANDLE_VALUE)
                    throw;

                IN_ADDR dest_ip{};

                if (1 != InetPtonA(AF_INET, "8.8.8.8", &dest_ip))
                    throw;

                constexpr WORD payload_size = 1;
                unsigned char payload[payload_size]{ 42 };

                constexpr DWORD reply_buf_size = sizeof(ICMP_ECHO_REPLY) + payload_size + 8;
                unsigned char reply_buf[reply_buf_size]{};

                while (true)
                {
                    DWORD reply_count = IcmpSendEcho(icmp_handle, dest_ip.S_un.S_addr,
                        payload, payload_size, NULL, reply_buf, reply_buf_size, 3);

                    if (reply_count == 0)
                        throw;

                    Sleep(500);
                }
            });
#endif

        is_attached = true;
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }

    return TRUE;
}

