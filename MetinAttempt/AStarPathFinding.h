#pragma once
#include "SharedHeader.h"
#include <vector>

#include "XENOX_objects.h"

std::vector<D3DXVECTOR2> genPath2(D3DXVECTOR3 origin, D3DXVECTOR3 target, std::vector<D3DXVECTOR3>* entityList);
