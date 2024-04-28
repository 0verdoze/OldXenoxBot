#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <queue>
#include <iomanip>
#include <map>
#include <functional>

#include <boost/lexical_cast.hpp>

#include <Windows.h>

#include "types.h"
#include "FunctionPatching.h"

#include "SharedHeader.h"

#define SAVE_ECX void* __OBJ_POINTER; __asm mov __OBJ_POINTER, ECX
#define RESTORE_ECX __asm mov ECX, __OBJ_POINTER

#define ENABLE_PYTHON_CONSOLE 0

void __stdcall chat_append(int iType, const char* c_szChat);
void __stdcall whisper_append(int iType, const char* c_szName, const char* c_szChat);

#if ENABLE_PYTHON_CONSOLE
void cin_reader();
#endif

void __stdcall hack_main();

extern std::vector<std::function<void(int, const char*)>> chat_append_callbacks;
extern std::vector<std::function<void(int, const char*, const char*)>> whisper_append_callbacks;

struct PatchData
{
    POINTER new_function;
    POINTER og_function_address;
    std::vector<std::pair<POINTER, PATCH_TYPE>> to_patch;
};

static std::unordered_map<std::string, PatchData> patch_map
{
    {"chat_append", { (POINTER)chat_append, (POINTER)chat_append_og, { { (POINTER)0x00808068, PATCH_TYPE::RAW } } }},
    {"whisper_append", { (POINTER)whisper_append, (POINTER)whisper_append_og, { { (POINTER)0x004ad4b1, PATCH_TYPE::E8_PATCH } } }},
};
