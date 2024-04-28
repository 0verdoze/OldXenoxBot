#pragma once

typedef void* POINTER;

enum class PATCH_TYPE
{
    RAW,
    ASM_JMP_EAX_7BYTE, // this overrides some data (2 bytes) - function must account for this
    E8_PATCH, // E8 is asm call instruction with 4byte relative address as parameter 
};
