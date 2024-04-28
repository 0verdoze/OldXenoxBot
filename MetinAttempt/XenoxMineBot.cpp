#include "XenoxBots.h"
#include "ManagerConnection.h"

namespace XenoxBots
{
    constexpr static std::array<std::pair<std::string_view, ItemVNUM>, 10> OreNames{ {
        { "¯y³a Ametystu",            AMETYST_ID },
        { "¯y³a Bia³ego Z³ota",        WHITE_GOLD_ID },
        { "¯y³a Ebonitu",            EBONIT_ID },
        { "¯y³a Jadeitu",            JADEIT_ID },
        { "¯y³a Kryszta³u",            KRYSZTAL_ID },
        { "¯y³a Miedzi",            MIEDZ_ID },
        { "¯y³a Niebiañskich £ez",    NIEB_LZY_ID },
        { "¯y³a Srebra",            SILVER_ID },
        { "¯y³a Z³ota",                GOLD_ID },
        { "Sterta Muszli",            PERLA_ID }
    } };

#undef ORE

    struct
    {
        DWORD oreVID = 0;
        std::vector<D3DXVECTOR2> wayPoints;
    } current_path;

    uint32_t target_count = 0;

    decltype(OreNames)::const_iterator OreIterator{ OreNames.cbegin() };

    void mine();
    void findNewOre();
    void finishPickingItem();
    void finishLongMove();
    void finishPathGeneration();
    void buyPixaxe();
    void putPixaxe();
    static void teleportToHandlarz();
    static void teleportToWayPoint(uint32_t);

    uint32_t getInitTargetCount();

    inline void debugPrint(const char* _Str)
    {
        std::cout << _Str << '\n';
    }

    inline void debugPrint2(const char* _Str)
    {
        return;
        std::cout << _Str << '\n';
        Sleep(1000);
    }

    // #define InfoPrint(x) x;
#define InfoPrint(x);

    LARGE_INTEGER last_path_creation_time{ 0, 0 };

    void MineBotMain()
    {
        VERIFY_MANAGER_CONNECTION

            static LARGE_INTEGER last_exec{ };
        LARGE_INTEGER li = get_current_time();

        auto elapsed = (li.QuadPart - last_exec.QuadPart) / 10000;

        if ((BotState)gCachedState != BotState::DISABLED)
            maxMinWindow(60, 10, 10);

        switch ((BotState)gCachedState)
        {
        case BotState::MINING:
            if (elapsed > 1500)
            {
                mine();
                last_exec = li;
            }
            break;
        case BotState::AWAITING:
            if (GetObjectPointer<CPythonPlayer>()->m_playerStatus.aDSItem[4].vnum != PIXAXE_ID)
                setState(BotState::PUTTING_KILOF);
            else
                findNewOre();
            break;
        case BotState::PICKING_ITEM:
            if (elapsed > 1500)
            {
                finishPickingItem();
                last_exec = li;
            }
            break;
        case BotState::MOVING:
            if ((get_current_time().QuadPart - last_path_creation_time.QuadPart) / 10000 > 15000) // sec
                setState(BotState::AWAITING);
            else
                finishLongMove();
            break;
        case BotState::PATH_GENERATION:
            finishPathGeneration();
            break;

        case BotState::BUYING_KILOF:
            buyPixaxe();
            break;

        case BotState::PUTTING_KILOF:
            // try to find pixaxe in inventory if that fails go to TELEPORTING_TO_HANDLARZ
            putPixaxe();
            break;

        case BotState::TELEPORTING_TO_HANDLARZ:
            teleportToHandlarz();
            break;

        case BotState::TELEPORTING_TO_WAYPOINT:
            teleportToWayPoint(0);
            break;

        case BotState::DISABLED:
            break;
        }
    }

    uint32_t getPlayerCount(D3DXVECTOR3 pos, double radius = 200.0 * 200.0)
    {
        uint32_t ply_count = 0;
        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
        auto& player = *GetObjectPointer<CPythonPlayer>();

        for (const auto& ply : chrMgr.m_kAliveInstMap)
        {
            if (getDistanceSqr(ply.second->m_GraphicThingInstance.m_currentPos, pos) < radius)
            {
                if (ply.first != player.m_dwMainCharacterIndex)
                    ply_count += 1;
            }
        }

        return ply_count;
    }

    uint32_t getItemCountInInventory(DWORD id)
    {
        auto& player = *GetObjectPointer<CPythonPlayer>();

        uint32_t count = 0;

        for (const auto& item : player.m_playerStatus.aItem)
        {
            if (item.vnum == id)
                count += item.count;
        }

        return count;
    }

    uint32_t getInitTargetCount()
    {
        uint32_t tCount = (std::numeric_limits<uint32_t>::max)();

        decltype(OreIterator) lIter{ OreNames.cbegin() };

        for (const auto& ore : OreNames)
        {
            uint32_t itemCount = getItemCountInInventory(ore.second);

            if (itemCount < tCount)
                tCount = itemCount;
        }

        InfoPrint(std::cout << "tCount: " << tCount << '\n');
        tCount = ((tCount / 200) * 200) + 200;

        for (; lIter != OreNames.cend(); lIter++)
        {
            if (getItemCountInInventory(lIter->second) < tCount)
                break;
        }

        OreIterator = lIter;
        return tCount;
    }

    void mine()
    {
        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();

        auto result = chrMgr.m_kAliveInstMap.find(current_path.oreVID);

        if (result == chrMgr.m_kAliveInstMap.end())
        {
            setState(BotState::PICKING_ITEM);
            InfoPrint(std::cout << "Picking item via Killer is dead\n");
            return;
        }

        auto& player = *GetObjectPointer<CPythonPlayer>();

        if (player.m_eReservedMode == CPythonPlayer::MODE_NONE)
        {
            CurrentAction CurrentAction = GetCurrentAction();

            if (CurrentAction == IDLE_ACTION)
            {
                setState(BotState::PICKING_ITEM);
                InfoPrint(std::cout << "Picking item via Jak to zdechl\n");
            }
            else if (CurrentAction != MINING_ACTION)
                InfoPrint(std::cout << "Unknown action type\n");

            //player.m_eReservedMode = CPythonPlayer::MODE_CLICK_ACTOR;
            //player.m_dwVIDReserved = oreVID;
        }
    }

    std::deque<std::tuple<DWORD, D3DXVECTOR3, double, uint32_t>> validOres;

    void findNewOre()
    {
        target_count = getInitTargetCount();

        std::cout << "target count " << target_count << '\n';

        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
        auto& player = *GetObjectPointer<CPythonPlayer>();

        auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
        if (localPlayer == chrMgr.m_kAliveInstMap.cend())
        {
            std::cout << "Error: unable to find <SELF>, skipping\n";
            return;
        }

        validOres.clear();

        for (const auto& entity : chrMgr.m_kAliveInstMap)
        {
            if (entity.second->m_stName == OreIterator->first)
            {
                validOres.push_back({
                    entity.first,
                    entity.second->m_GraphicThingInstance.m_currentPos,
                    getDistanceSqr(localPlayer->second->m_GraphicThingInstance.m_currentPos, entity.second->m_GraphicThingInstance.m_currentPos),
                    getPlayerCount(entity.second->m_GraphicThingInstance.m_currentPos)
                    });
            }
        }

        std::sort(validOres.begin(), validOres.end(), [](auto& a, auto& b)
            {
                if (std::get<uint32_t>(a) < std::get<uint32_t>(b))
                    return true;

                return std::get<double>(a) < std::get<double>(b);
            });

        setState(BotState::PATH_GENERATION);
    }

    void finishPickingItem()
    {
        constexpr double MAX_PICKUP_DISTANCE = 350.0 * 350.0;

        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
        auto& player = *GetObjectPointer<CPythonPlayer>();
        auto& items = *GetObjectPointer<CPythonItem>();

        auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
        if (localPlayer == chrMgr.m_kAliveInstMap.cend())
        {
            std::cout << "Error: unable to find <SELF>, skipping\n";
            return;
        }

        auto result = items.m_GroundItemInstanceMap.find(player.m_dwIIDReserved);
        if (result == items.m_GroundItemInstanceMap.cend())
        {
            std::pair<DWORD, double> closest_item{ 0, INFINITY };
            // find item to pickup
            for (const auto& item : items.m_GroundItemInstanceMap)
            {
                decltype(closest_item) current_item{ item.first,
                    getDistanceSqr(localPlayer->second->m_GraphicThingInstance.m_currentPos, item.second->v3EndPosition) };

                std::cout
                    << item.second->v3EndPosition.x << ' '
                    << item.second->v3EndPosition.y << ' '
                    << item.second->v3EndPosition.z << '\n'
                    << localPlayer->second->m_GraphicThingInstance.m_currentPos.x << ' '
                    << localPlayer->second->m_GraphicThingInstance.m_currentPos.y << ' '
                    << localPlayer->second->m_GraphicThingInstance.m_currentPos.z << "\n\n";

                if (current_item.second < closest_item.second &&
                    (item.second->stOwnership == player.m_stName)) // || item.second->stOwnership.size() == 0))
                {
                    closest_item = current_item;
                    InfoPrint(std::cout << "Replacing Item\n");
                }

                std::cout << current_item.second << '\n';
            }

            if (closest_item.second < MAX_PICKUP_DISTANCE)
            {
                InfoPrint(std::cout << "Found item, distance " << sqrt(closest_item.second) << '\n');
                EvalPy("networkModule.net.SendItemPickUpPacket(%u)", closest_item.first);
                //player.m_eReservedMode = CPythonPlayer::MODE_CLICK_ITEM;
                //player.m_dwIIDReserved = closest_item.first;
            }
            else
            {
                InfoPrint(std::cout << "Picked all items\n");
                setState(BotState::AWAITING);
            }
        }
        else if (player.m_eReservedMode != CPythonPlayer::MODE_CLICK_ITEM)
        {
            std::cout << "This shouldn't occour\n";
            // this shouldnt occour
            player.m_eReservedMode = CPythonPlayer::MODE_CLICK_ITEM;
        }
        else
            std::cout << "Awaiting for item to be picked up\n";
    }

    std::future<std::vector<D3DXVECTOR2>> AStarPromise;
    bool isGenerating = false;

    void finishPathGeneration()
    {
        if (isGenerating)
        {
            if (AStarPromise._Is_ready() == false)
                return;

            // std::cout << "ready\n";

            isGenerating = false;

            current_path.wayPoints = AStarPromise.get();
            if (current_path.wayPoints.size() > 0 && current_path.wayPoints.size() < 150) // max path len
            {
                current_path.oreVID = std::get<DWORD>(validOres.front());
                last_path_creation_time = get_current_time();
                setState(BotState::MOVING);
            }
            else
                validOres.pop_front();
        }
        else
        {
            if (validOres.size() == 0)
            {
                setState(BotState::AWAITING);  // unable to find path
                return;
            }

            auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
            auto& player = *GetObjectPointer<CPythonPlayer>();
            auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
            auto targetEntity = chrMgr.m_kAliveInstMap.find(std::get<DWORD>(validOres.front()));

            if (localPlayer == chrMgr.m_kAliveInstMap.end())
            {
                std::cout << "Unable to find <SELF> in finishPathGeneration\n";
                return;
            }
            if (targetEntity == chrMgr.m_kAliveInstMap.end())
            {
                validOres.pop_front();
                return;
            }

            auto entityList = new std::vector<D3DXVECTOR3>();

            for (const auto& entity : chrMgr.m_kAliveInstMap)
            {
                if (entity.first != localPlayer->first &&
                    entity.first != targetEntity->first)
                {
                    if (
                        std::find_if(std::cbegin(OreNames), std::cend(OreNames), [&entity](auto& a)
                            {
                                return a.first == entity.second->m_stName;
                            }) == std::cend(OreNames)
                                )
                    {
                        entityList->push_back(entity.second->m_GraphicThingInstance.m_currentPos);
                    }
                }
            }

            AStarPromise = std::async(
                &genPath2,
                localPlayer->second->m_GraphicThingInstance.m_currentPos,
                targetEntity->second->m_GraphicThingInstance.m_currentPos,
                entityList
            );

            isGenerating = true;
        }
    }

    void finishLongMove()
    {
        constexpr double changeTargetDistance = SQUARE(100.0);

        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();
        auto& player = *GetObjectPointer<CPythonPlayer>();

        auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
        if (localPlayer == chrMgr.m_kAliveInstMap.cend())
        {
            std::cout << "Error: unable to find <SELF> at finishLongMove, skipping\n";
            return;
        }

        if (current_path.wayPoints.size() > 3)
        {
            if (getDistanceSqr(localPlayer->second->m_GraphicThingInstance.m_currentPos, current_path.wayPoints.back()) < changeTargetDistance)
            {
                current_path.wayPoints.pop_back();
                if (current_path.wayPoints.size() > 3)
                {
                    player.m_eReservedMode = CPythonPlayer::MODE_CLICK_POSITION;
                    player.m_kPPosReserved.x = +current_path.wayPoints.back().x;
                    player.m_kPPosReserved.y = -current_path.wayPoints.back().y;
                }
            }
            else if (player.m_eReservedMode == CPythonPlayer::MODE_NONE)
            {
                player.m_eReservedMode = CPythonPlayer::MODE_CLICK_POSITION;
                player.m_kPPosReserved.x = +current_path.wayPoints.back().x;
                player.m_kPPosReserved.y = -current_path.wayPoints.back().y;
            }
        }

        if (current_path.wayPoints.size() <= 3)
        {
            player.m_dwVIDReserved = current_path.oreVID;
            player.m_eReservedMode = CPythonPlayer::MODE_CLICK_ACTOR;
            setState(BotState::MINING);
        }
    }

    void buyPixaxe()
    {
        static int state = 0;
        static LARGE_INTEGER last_action{ };
        auto& player = *GetObjectPointer<CPythonPlayer>();
        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();

        if ((get_current_time().QuadPart - last_action.QuadPart) / 10000 < 1000)
            return;

        last_action = get_current_time();

        if (state == 0)
        {
            auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
            if (localPlayer == chrMgr.m_kAliveInstMap.cend())
            {
                std::cout << "Error: unable to find <SELF>, skipping\n";
                return;
            }

            auto result = std::find_if(cbegin(chrMgr.m_kAliveInstMap), cend(chrMgr.m_kAliveInstMap), [](const auto& obj)
                { return obj.second->m_stName == std::string_view("Deokbae"); });

            if (result == cend(chrMgr.m_kAliveInstMap) ||
                getDistanceSqrAuto(localPlayer->second->m_GraphicThingInstance.m_currentPos, result->second->m_GraphicThingInstance.m_currentPos) > SQUARE(400.0))
            {
                setState(BotState::TELEPORTING_TO_HANDLARZ);
                return;
            }

            EvalPy("networkModule.net.SendOnClickPacket(%u)", result->first);
            ++state;
        }
        else if (state == 1)
        {
            EvalPy("__import__('event').SelectAnswer(1, 0)");
            ++state;
        }
        else if (state == 2)
        {
            if (EvalPyGetRepr("__import__('shop').IsOpen()") != std::string_view("1"))
                state = 0;
            else
            {
                EvalPy("networkModule.net.SendShopBuyPacket(0)");
                ++state;
            }
        }
        else if (state == 3)
        {
            EvalPy("__import__('uiShop').ShopDialog().Close()");
            state = 0;
            setState(BotState::TELEPORTING_TO_WAYPOINT);
        }
    }

    void putPixaxe()
    {
        if (GetObjectPointer<CPythonPlayer>()->m_playerStatus.aDSItem[4].vnum == PIXAXE_ID)
        {
            setState(BotState::AWAITING);
            return;
        }

        // try to find pixaxe in inventory
        auto& inv = GetObjectPointer<CPythonPlayer>()->m_playerStatus.aItem;

        for (size_t i = 0; i < std::size(inv); i++)
        {
            if (inv[i].vnum == PIXAXE_ID)
            {
                EvalPy("networkModule.net.SendItemUsePacket(%u)", i);
                setState(BotState::AWAITING);
                findNewOre();
                return;
            }
        }

        setState(BotState::TELEPORTING_TO_HANDLARZ);
        return;
    }

    static void teleportToHandlarz()
    {
        using std::cbegin;
        using std::cend;

        // try to find handlarz in range
        auto& player = *GetObjectPointer<CPythonPlayer>();
        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();

        auto localPlayer = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
        if (localPlayer == chrMgr.m_kAliveInstMap.cend())
        {
            std::cout << "Error: unable to find <SELF>, skipping\n";
            return;
        }

        auto result = std::find_if(cbegin(chrMgr.m_kAliveInstMap), cend(chrMgr.m_kAliveInstMap), [](const auto& obj)
            { return obj.second->m_stName == std::string_view("Deokbae"); });

        if (result != cend(chrMgr.m_kAliveInstMap) &&
            getDistanceSqrAuto(localPlayer->second->m_GraphicThingInstance.m_currentPos, result->second->m_GraphicThingInstance.m_currentPos) < SQUARE(400.0))
            setState(BotState::BUYING_KILOF);
        else
            CALL_EVERY(30000, teleportToWayPoint, 1);
    }

    static void teleportToWayPoint(uint32_t id)
    {
        if (gCachedState != (uint32_t)BotState::TELEPORTING_TO_WAYPOINT)
        {
            EvalPy("networkModule.net.SendChatPacket('/warp_to_location %u')", id);
            return;  // let caller handle this teleportation
        }

        CALL_EVERY(30000, EvalPy, "networkModule.net.SendChatPacket('/warp_to_location %u')", id);

        auto& player = *GetObjectPointer<CPythonPlayer>();
        auto& chrMgr = *GetObjectPointer<CPythonCharacterManager>();

        auto result = chrMgr.m_kAliveInstMap.find(player.m_dwMainCharacterIndex);
        static bool lastState = false;
        bool isLoadingScreen = result == std::end(chrMgr.m_kAliveInstMap); // or dead

        if (isLoadingScreen == false && isLoadingScreen != lastState)
        {
            // tp complete
            setState(BotState::AWAITING);
        }

        lastState = isLoadingScreen;
    };
}
