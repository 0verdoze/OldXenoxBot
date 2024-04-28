#include "XenoxBots.h"
#include "PatchedFunctions.h"
#include "ManagerConnection.h"

#include <windows.h>


namespace XenoxBots
{
    constexpr const char* FISHING_PRESS_SPACE_MSG = "|cff00c6ff<Informacja> Nacisnij %d razy spacje.";

    static std::string_view FISHING_END_MSG("|cff00c6ff<Informacja> Uzyj ponownie, aby zakonczyc lowienie.");
    static std::string_view FISHING_NO_ROBAK_MSG("Nie masz przyn�ty na haczyku.");
    static std::string_view FISHING_FAILED_MSG("Straci�e� przyn�t�.");

    constexpr static std::array<ItemVNUM, 22> FISH_ID_ARRAY
    { {
        27803, // Kara�
        27804, // Ryba Mandaryna
        27805, // Du�y Kara�
        27806, // Karp
        27807, // �oso�
        27808, // Amur
        27809, // Pstr�g
        27810, // W�gorz
        27811, // T�czowy Pstr�g
        27812, // Rzeczny Pstr�g
        27813, // Krasnopi�rka
        27814, // Oko�
        27815, // Tenchi
        27816, // Sum
        27817, // Piskorz
        27818, // Ryba Lotosu
        27819, // S�odka Ryba
        27820, // Gromadnik
        27821, // Shiri
        27822, // Lustrzany Karp
        27823, // Z�oty Kara�
        0x00'01'35'd7, // trofeum zlotego karasia
    } };

    static std::vector<std::string> unreaded_messages;

    void on_chat_append(int iType, const char* c_szMsg)
    {
        // std::cout << iType << ' ' << c_szMsg << '\n';
        if (iType == 1)
            unreaded_messages.push_back(std::string(c_szMsg));
    }

    uint32_t spaces_to_be_pressed = 0;

    void fishing();
    void putRobak();
    void pressSpace();
    void buyFishingRod();
    void putFishingRod();
    static void teleportToHandlarz();
    static void teleportToWayPoint(uint32_t);

    void FishBotMain()
    {
        VERIFY_MANAGER_CONNECTION

            static bool calledOnce = false;
        if (!calledOnce)
        {
            calledOnce = true;
            chat_append_callbacks.push_back(on_chat_append);
        }

        maxMinWindow(5, 10, 40);

        switch ((BotState)gCachedState)
        {
        case BotState::FISHING:
            if (GetObjectPointer<CPythonPlayer>()->m_playerStatus.aDSItem[4].vnum != FISHING_ROD_ID)
                setState(BotState::PUTTING_WEDKA);
            else
                fishing();
            break;
        case BotState::AWAITING:
            setState(BotState::PUTTING_ROBAK);
            break;
        case BotState::PUTTING_ROBAK:
            putRobak();
            break;
        case BotState::PRESSING_SPACE:
            pressSpace();
            break;
        case BotState::BUYING_WEDKA:
            buyFishingRod();
            break;
        case BotState::PUTTING_WEDKA:
            // try to find fishing rod in inventory if that fails go to TELEPORTING_TO_HANDLARZ
            putFishingRod();
            break;
        case BotState::TELEPORTING_TO_HANDLARZ:
            teleportToHandlarz();
            break;
        case BotState::TELEPORTING_TO_WAYPOINT:
            teleportToWayPoint(0);
            break;
        case BotState::DISABLED:
            unreaded_messages.clear();
            break;
        }
    }

    void putRobak()
    {
        static LARGE_INTEGER last_buff_applied{ };
        uint32_t ROBAK_SLOT = -1;
        uint32_t BUFF_SLOT = -1;

        auto player = GetObjectPointer<CPythonPlayer>();

        for (int32_t i = 0; i < sizeof(player->m_playerStatus.aItem) / sizeof(player->m_playerStatus.aItem[0]); i++)
        {
            if (player->m_playerStatus.aItem[i].vnum == 0x0)
                continue;

            if (player->m_playerStatus.aItem[i].vnum == ROBAK_ID)
                ROBAK_SLOT = i;

            if (player->m_playerStatus.aItem[i].vnum == ENCHANT_ITEM_ID)
                BUFF_SLOT = i;

            //if (ROBAK_SLOT != -1 && BUFF_SLOT != -1)
            //    break;

            if (std::find(
                std::cbegin(FISH_ID_ARRAY),
                std::cend(FISH_ID_ARRAY),
                player->m_playerStatus.aItem[i].vnum) != std::cend(FISH_ID_ARRAY))
            {
                // open ryba
                EvalPy("networkModule.net.SendItemUsePacket(%u)", i);
            }
        }

        if ((get_current_time().QuadPart - last_buff_applied.QuadPart) / 10000 > 1000 * 60 * 30)
        {
            if (BUFF_SLOT == -1)
            {
                std::cout << "No BUFF in eq\n";
            }
            else
            {
                last_buff_applied = get_current_time();
                EvalPy("networkModule.net.SendItemUsePacket(%u)", BUFF_SLOT);
            }
        }

        if (ROBAK_SLOT == -1)
        {
            std::cout << "No ROBAK in eq\n";
            return;
        }

        EvalPy("networkModule.net.SendItemUsePacket(%u)", ROBAK_SLOT);

        setState(BotState::FISHING);
    }

    void fishing()
    {
        CurrentAction CurrentAction = GetCurrentAction();
        static LARGE_INTEGER LAST_RZUCENIE{ };
        static bool robakNext = true;
        // might brake
        if (CurrentAction == FISHING_ACTION)
        {
            while (!unreaded_messages.empty())
            {
                auto& last_msg = unreaded_messages.back();
                if (last_msg == FISHING_END_MSG)
                {
                    spaces_to_be_pressed = 1;
                    setState(BotState::PRESSING_SPACE);
                }
                else
                {
                    int params = sscanf_s(last_msg.c_str(), FISHING_PRESS_SPACE_MSG, &spaces_to_be_pressed);
                    if (params == 1)
                    {
                        robakNext = true;
                        setState(BotState::PRESSING_SPACE);
                    }
                }

                unreaded_messages.pop_back();
            }
        }
        else if (CurrentAction == FISHING_IDLE)
        {
            while (!unreaded_messages.empty())
            {
                auto& last_msg = unreaded_messages.back();

                if (last_msg == FISHING_NO_ROBAK_MSG)
                {
                    setState(BotState::PUTTING_ROBAK);
                }
                else if (last_msg == FISHING_FAILED_MSG)
                {
                    setState(BotState::PUTTING_ROBAK);
                    unreaded_messages.clear();
                    break;
                }

                unreaded_messages.pop_back();
            }

            if (robakNext)
            {
                robakNext = false;
                setState(BotState::PUTTING_ROBAK);
            }
            else if ((get_current_time().QuadPart - LAST_RZUCENIE.QuadPart) / 10000 > 2000) // re rzut
            {
                LAST_RZUCENIE = get_current_time();
                spaces_to_be_pressed = 1;
                setState(BotState::PRESSING_SPACE);
            }
        }
        else
            ;//std::cout << "Unknown action\n";
    }

    void pressSpace()
    {
        constexpr uint32_t randomness = 30; // %
        constexpr uint32_t pressDelay = 200 * 10000; // ms
        constexpr uint32_t pressTime = 300 * 10000; // ms
        static LARGE_INTEGER pressEndAt{ };
        static LARGE_INTEGER pressAvalAt{ };
        static LARGE_INTEGER lastCall{ };
        static bool isPressing = false;

        auto setKey = [](int state)
        {
            EvalPy("networkModule.player.SetAttackKeyState(%d)", state);
        };

        if ((get_current_time().QuadPart - lastCall.QuadPart) > 1000 * 10000)
        {
            lastCall = get_current_time();
            pressAvalAt.QuadPart = get_current_time().QuadPart + add_random_prc(1000 * 10000, randomness);
            return;
        }

        lastCall = get_current_time();

        if (isPressing)
        {
            if (get_current_time().QuadPart > pressEndAt.QuadPart)
            {
                --spaces_to_be_pressed;
                isPressing = false;
                setKey(false);
                pressAvalAt.QuadPart = get_current_time().QuadPart + add_random_prc(pressDelay, randomness);

            }
        }
        else
        {
            if (spaces_to_be_pressed == 0)
            {
                setState(BotState::FISHING);
            }
            else if (get_current_time().QuadPart > pressAvalAt.QuadPart)
            {
                isPressing = true;
                setKey(true);
                pressEndAt.QuadPart = get_current_time().QuadPart + add_random_prc(pressTime, randomness);
            }
        }

        //auto player = (CPythonPlayer*)GetObjectPointer("CPythonPlayer");

        //if (isPressing)
        //    player->m_isAtkKey = true;
        //else
        //    player->m_isAtkKey = false;
    }

    void buyFishingRod()
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
                { return obj.second->m_stName == std::string_view("Rybak"); });

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

    void putFishingRod()
    {
        if (GetObjectPointer<CPythonPlayer>()->m_playerStatus.aDSItem[4].vnum == FISHING_ROD_ID)
        {
            setState(BotState::AWAITING);
            return;
        }

        // try to find pixaxe in inventory
        auto& inv = GetObjectPointer<CPythonPlayer>()->m_playerStatus.aItem;

        for (size_t i = 0; i < std::size(inv); i++)
        {
            if (inv[i].vnum == FISHING_ROD_ID)
            {
                EvalPy("networkModule.net.SendItemUsePacket(%u)", i);
                setState(BotState::AWAITING);
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
            { return obj.second->m_stName == std::string_view("Rybak"); });

        if (result != cend(chrMgr.m_kAliveInstMap) &&
            getDistanceSqrAuto(localPlayer->second->m_GraphicThingInstance.m_currentPos, result->second->m_GraphicThingInstance.m_currentPos) < SQUARE(400.0))
            setState(BotState::BUYING_WEDKA);
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