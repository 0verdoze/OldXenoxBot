#include "FunctionPatching.h"

#include <iostream>
#include <iomanip>

#include <Windows.h>

// check if data at `address` matches provided signature
bool match_signature(uint8_t* address, const std::vector<uint8_t>& signature)
{
    for (auto i : signature)
    {
        auto current = *address;
        address++;

        if (i == 0xCC)
            continue;

        if (current != i)
            return false;
    }

    return true;
}

uint8_t* find_signature(uint8_t* start_ptr, uint8_t* end_ptr, const std::vector<uint8_t >& signature) noexcept
{
    const uint8_t first = signature[0];
    const uint8_t* end = end_ptr - signature.size();

    for (; start_ptr < end; ++start_ptr)
    {
        if (*start_ptr != first)
        {
            continue;
        }
        if (match_signature(start_ptr, signature))
        {
            return start_ptr;
        }
    }

    return nullptr;
}

// patch instructions at `addr` and replace it with a call to `function_ptr`
bool patch_function(int addr, void* function_ptr, PATCH_TYPE patch_type)
{
    DWORD oldProtect = 0;

    // disable protection at target address
    // we disable it for 32 bytes as its more then we would need
    if (!VirtualProtect((void*)addr, 32, PAGE_EXECUTE_WRITECOPY, &oldProtect))
        return false;

    switch (patch_type)
    {
    case (PATCH_TYPE::RAW):
    {
        // raw patch
        *(int*)addr = (int)function_ptr;
        break;
    }
    case (PATCH_TYPE::E8_PATCH):
    {
        // E8 patch, we are replacing target address of `call` instruction

        // ensure that `addr` points to E8 instruction
        if (((uint8_t*)addr)[0] != 0xE8)
        {
            std::cout << "E8_PATCH failed, instruction is " << ((uint8_t*)addr)[0];
            return false;
        }

        // `call` instruction works on relative addresses, calculate it
        // we need to subtract 5 from the result, as E8 is 5 byte instruction
        // and cpu will jump to relative address only after executing it (ie. EIP will increase by 5)
        int rel_jump = (int)function_ptr - addr - 5;
        // and set it
        *(int*)(addr + 1) = rel_jump;

        break;
    }
    default:
        std::cout << "Unable to patch " << addr << " unknown patch type\n";
        return false;
    }

    // restore memory protection
    VirtualProtect((void*)addr, 32, oldProtect, NULL);

    return true;
}

