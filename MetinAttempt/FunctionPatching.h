#pragma once
#include <vector>

#include "types.h"


uint8_t* find_signature(uint8_t* apStart, uint8_t* apEnd, const std::vector<uint8_t>& aSignature) noexcept;
bool patch_function(int addr, void* function_ptr, PATCH_TYPE patch_type);
bool replace_bytes(uint8_t* start, const std::vector<uint8_t>& data);
