#include "XenoxBots.h"


namespace XenoxBots
{
    // hardcoded upgrades combos
    const std::array<std::vector<std::pair<decltype(TPlayerItemAttribute::bType), short>>, 6> combos
    { {
        { {
            {0x48, SHRT_MIN}, // srednie obrazenia
            {0x47, 22},       // obrazenia umiejetnosci
            {0x09, SHRT_MIN}, // szybkosc zaklecia
            {0x11, SHRT_MIN}, // silny na ludzi
            {0x0f, SHRT_MIN}, // szansa na cios krytyczny
        } },
        { {
            {0x48, SHRT_MIN}, // srednie obrazenia
            {0x47, 22},       // obrazenia umiejetnosci
            {0x15, SHRT_MIN}, // silny na nieumarle
            {0x16, SHRT_MIN}, // silny na diably
            {0x09, SHRT_MIN}, // szybkosc zaklecia
        } },
        { {
            {0x48, SHRT_MIN}, // srednie obrazenia
            {0x47, 22},       // obrazenia umiejetnosci
            {0x15, SHRT_MIN}, // silny na nieumarle
            {0x16, SHRT_MIN}, // silny na diably
            {0x0f, SHRT_MIN}, // szansa na cios krytyczny
        } },
        { {
            {0x48, SHRT_MIN}, // srednie obrazenia
            {0x47, 22},       // obrazenia umiejetnosci
            {0x10, SHRT_MIN}, // przeszycie
            {0x11, SHRT_MIN}, // silny na ludzi
            {0x0f, SHRT_MIN}, // szansa na cios krytyczny
        } },
        { {
            {0x48, SHRT_MIN}, // srednie obrazenia
            {0x47, 22},       // obrazenia umiejetnosci
            {0x15, SHRT_MIN}, // silny na nieumarle
            {0x16, SHRT_MIN}, // silny na diably
            {0x10, SHRT_MIN}, // przeszycie
        } },
        { {
            {0x47, 27},       // obrazenia umiejetnosci
            {0x11, SHRT_MIN}, // silny na ludzi
        } }
    } };

    short max_um = SHRT_MIN;

    bool is_upgrade_completed(uint32_t slot_id)
    {
        auto& player = *GetObjectPointer<CPythonPlayer>();
        const auto& itemData = player.m_playerStatus.aItem[slot_id];

        for (const auto& inter_array : combos)
        {
            boost::container::static_vector<bool, 7> completed;
            completed.resize(inter_array.size());

            for (uint32_t i = 0; i < completed.size(); i++) // completed loop
            {
                for (int j = 0; j < 5; j++) // items loop
                {
                    if (itemData.aAttr[j].bType == inter_array[i].first &&
                        itemData.aAttr[j].sValue >= inter_array[i].second)
                        completed[i] = true;

                    if (itemData.aAttr[j].bType == 0x47)
                    {
                        if (itemData.aAttr[j].sValue > max_um)
                        {
                            max_um = itemData.aAttr[j].sValue;
                            std::cout << "Max um: " << max_um << ' ' << itemData.aAttr[0].sValue << '\n';
                        }
                    }
                }
            }

            bool success = true;
            for (uint32_t i = 0; i < completed.size(); i++)
            {
                if (completed[i] == false)
                {
                    success = false;
                    break;
                }
            }

            if (success == true)
            {
                std::cout << "HIT HIT HIT\n";
                return true;
            }
        }

        return false;
    }

    static uint32_t getItemCountInInventory(DWORD id)
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

    struct ItemData
    {
        std::array<uint8_t, 7> buff_type{ 0 };

        friend bool operator== (const ItemData& c1, const ItemData& c2)
        {
            for (int i = 0; i < 7; i++)
            {
                if (c1.buff_type[i] != c2.buff_type[i])
                    return false;
            }

            return true;
        }
    };

    std::array<std::pair<ItemData, LARGE_INTEGER>, 45> last_item_info{ };
    std::array<LARGE_INTEGER, 45> last_zmianki_bought{ };
    LARGE_INTEGER seconds_from_start{ };
    uint64_t upgrade_count = 0;

    void SwitchBotMain(unsigned int ITEM_SLOT_ID)
    {
        constexpr DWORD ULEPSZACZ_ID = 0x000115ac;
        auto& player = *GetObjectPointer<CPythonPlayer>();

        static bool imported_shop = false;
        if (imported_shop == false)
        {
            imported_shop = true;
            EvalPy("globals().update({ \"shop\": __import__(\"shop\") })");
        }

        if (player.m_playerStatus.aItem[ITEM_SLOT_ID].vnum == 0 ||
            player.m_playerStatus.aItem[ITEM_SLOT_ID].vnum == ULEPSZACZ_ID)
            return;

        if (is_upgrade_completed(ITEM_SLOT_ID))
            return;

        ItemData curr_item;
        for (int i = 0; i < 7; i++)
            curr_item.buff_type[i] = player.m_playerStatus.aItem[ITEM_SLOT_ID].aAttr[i].bType;

        auto cTime = get_current_time();

        if ((cTime.QuadPart - seconds_from_start.QuadPart) / 10000 > 1000)
        {
            seconds_from_start = cTime;
            std::cout << upgrade_count << "/s     \r";
            upgrade_count = 0;
        }

        // -30
        if ((curr_item == last_item_info[ITEM_SLOT_ID].first || curr_item.buff_type[4] == 0) &&
            (cTime.QuadPart - last_item_info[ITEM_SLOT_ID].second.QuadPart) / 10000 < 4500 ||
            (cTime.QuadPart - last_item_info[ITEM_SLOT_ID].second.QuadPart) / 10000 < 100)
            return;

        uint32_t upgrade_slot_id = ITEM_SLOT_ID + 45;

        if (player.m_playerStatus.aItem[upgrade_slot_id].vnum == 0x0)
        {
            if (cTime.QuadPart - last_zmianki_bought[ITEM_SLOT_ID].QuadPart < 3000 * 10000)
                return;

            std::string_view result(EvalPyGetRepr("shop.IsOpen()"));
            if (result == std::string_view("1"))
            {
                EvalPy("networkModule.net.SendShopBuyPacket(25)");
                last_zmianki_bought[ITEM_SLOT_ID] = cTime;
            }
            return;
        }

        if (player.m_playerStatus.aItem[upgrade_slot_id].vnum != ULEPSZACZ_ID)
            return;

        last_item_info[ITEM_SLOT_ID].first = curr_item;
        last_item_info[ITEM_SLOT_ID].second = cTime;
        upgrade_count += 1;

        EvalPy("networkModule.net.SendItemUseToItemPacket(%u, %u)", upgrade_slot_id, ITEM_SLOT_ID);
    }
}