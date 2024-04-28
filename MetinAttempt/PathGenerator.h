#pragma once
#include "SharedHeader.h"
#include <deque>

std::vector<D3DXVECTOR2> genPath(D3DXVECTOR3 origin, D3DXVECTOR3 target, bool* fail);
