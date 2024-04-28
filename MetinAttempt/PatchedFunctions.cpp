#include "PatchedFunctions.h"

#include "PosDumper.h"

std::vector<std::function<void(int, const char*)>> chat_append_callbacks;

void __stdcall chat_append(int iType, const char* c_szChat)
{
    SAVE_ECX;
    {
        // std::cout << "Calling all chat functions\n";
        for (const auto& func : chat_append_callbacks)
            func(iType, c_szChat);
        // std::cout << "Called all chat functions\n";
    }
    RESTORE_ECX;
    chat_append_og(iType, c_szChat);
}

std::vector<std::function<void(int, const char*, const char*)>> whisper_append_callbacks;

void __stdcall whisper_append(int iType, const char* c_szName, const char* c_szChat)
{
    SAVE_ECX;
    {
        // std::cout << "Calling all whisper functions\n";
        for (const auto& func : whisper_append_callbacks)
            func(iType, c_szName, c_szChat);
        // std::cout << "Called all whisper functions\n";
    }
    RESTORE_ECX;
    whisper_append_og(iType, c_szName, c_szChat);
}

#if ENABLE_PYTHON_CONSOLE
std::queue<std::string> messages;

void cin_reader()
{
    while (true)
    {
        static std::string s;
        std::getline(std::cin, s);
        messages.push(s);
    }
}
#endif

uint16_t slot_item_count(uint32_t slot_id)
{
    auto& player = *GetObjectPointer<CPythonPlayer>();
    return player.m_playerStatus.aItem[slot_id].count;
}

enum class _BUILDTARGET
{
    SWITCHBOT,
    MINEBOT,
    LOGPOS,
    FISHBOT,
    SPAMBOT,
    NONE
};

void __stdcall hack_main()
{
    static bool called_once = false;
    if (called_once == false)
    {
        called_once = true;
        srand(time(NULL));
    }

#if ENABLE_PYTHON_CONSOLE
    if (messages.size())
    {
        // std::cout << messages.front() << '\n';
        auto output = EvalPyGetRepr(messages.front().c_str());
        std::cout << output << '\n';

        messages.pop();
    }
#endif
#if !ENABLE_PYTHON_CONSOLE

#ifdef __BUILDTARGET
    using namespace XenoxBots;

    if constexpr (__BUILDTARGET == _BUILDTARGET::SWITCHBOT)
    {
        for (int i = 0; i < 45; i++)
            SwitchBotMain(i);
    }
    else if constexpr (__BUILDTARGET == _BUILDTARGET::MINEBOT)
    {
        MineBotMain();
    }
    else if constexpr (__BUILDTARGET == _BUILDTARGET::LOGPOS)
    {
        dump_pos();
    }
    else if constexpr (__BUILDTARGET == _BUILDTARGET::FISHBOT)
    {
        FishBotMain();
    }
    else if constexpr (__BUILDTARGET == _BUILDTARGET::SPAMBOT)
    {
        SpamBotMain();
    }
#else
    static_assert(false, "No build target chosen");
#endif
#endif
}