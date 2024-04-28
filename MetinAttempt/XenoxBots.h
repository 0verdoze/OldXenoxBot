#pragma once

#include "AStarPathFinding.h"
#include "XENOX_objects.h"
#include "XENOX_functions.h"
#include "PlayerActions.h"
#include "ItemVNUM.h"

#include <array>
#include <iostream>
#include <deque>
#include <future>
#include <string_view>

#include <boost/lexical_cast.hpp>
#include <boost/container/static_vector.hpp>

#include "utility.h"


namespace XenoxBots
{
    void FishBotMain();
    void SwitchBotMain(unsigned int ITEM_SLOT_ID);
    void SpamBotMain();
    void MineBotMain();
}

