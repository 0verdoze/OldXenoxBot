#include "XenoxBots.h"
#include "ManagerConnection.h"

namespace XenoxBots
{
#define ZWOJ_METALU "|cfff1e6c0|Hitem:468c:4:0:0:0|h[Zw.j Metalu]|h|r"
    static std::array<const char*, 1> chatMessages
    {
        "SELL " ZWOJ_METALU " 1,50 ZL ZA SZTUKE (BLIK/ALLEGRO) PW"
    };
#undef ZWOJ_METALU

    void SpamBotMain()
    {
        VERIFY_MANAGER_CONNECTION;

        maxMinWindow(5, 10, 40);

        if (gCachedState == (uint32_t)BotState::AWAITING)
            setState(BotState::CHATBOT_ENABLED);

        if (gCachedState == (uint32_t)BotState::DISABLED)
            setState(BotState::CHATBOT_DISABLED);

        if (gCachedState != (uint32_t)BotState::CHATBOT_ENABLED)
            return;

        static uint64_t nextMessage{ };
        uint64_t timeNow = get_current_time().QuadPart;

        if (timeNow > nextMessage)
        {
            EvalPyGetRepr("networkModule.net.SendChatPacket('%s', 6)", chatMessages[rand() % chatMessages.size()]);

            nextMessage = timeNow + (add_random_prc(30, 1000) * 10000000ULL);
        }
    }
}