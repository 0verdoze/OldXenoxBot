#include "MetinGui.h"
#include "MetinBotStructures.h"

#include "imgui.h"

#include <algorithm>
#include <future>
#include <iostream>
#include <iomanip>
#include <queue>
#include <sstream>
#include <thread>

#include <boost/locale.hpp>

#include <Mmsystem.h>

#include "resource.h"
#include "MetinHostGuest.pb.h"

using namespace ImGui;

#define BeginListBox ListBoxHeader
#define EndListBox ListBoxFooter

static shared_mutex gVM_LIST_MUTEX;
static std::vector<std::unique_ptr<VMManager>> gVM_LIST;

static uint8_t* MessageSound = nullptr;

constexpr const char* NO_INSTANCE = "No instance selected";

void RenderBotWindow(size_t VM_INDEX, size_t BOT_INDEX);

void SendPacket(size_t VM_INDEX, const std::vector<uint8_t>& data, uint8_t packetType);
void SetBotStatus(size_t VM_INDEX, size_t BOT_INDEX, BotState botStatus);
void CrashBot(size_t VM_INDEX, size_t BOT_INDEX);
void PhaseSelectBot(size_t VM_INDEX, size_t BOT_INDEX);
void SendChatMessage(size_t VM_INDEX, size_t BOT_INDEX, std::string_view msg);
void SendPrivateMessage(size_t VM_INDEX, size_t BOT_INDEX, std::string_view nickname, std::string_view msg);

void TcpAcceptThread();
void TcpWorkThread();

std::string CurrentTime();

// VM selection combo vars
static std::string ComboPreview(NO_INSTANCE);
static int ComboSelection = -1;

static std::thread TcpAccept, TcpWork;
static std::queue<Error> ErrorQueue;
static HWND windowHandle = NULL;

bool gShutdownApp = false;

void GuiRun(void* hwnd)
{
    shared_lock lock(gVM_LIST_MUTEX);

    static bool runOnce = false;
    if (!runOnce)
    {
        GOOGLE_PROTOBUF_VERIFY_VERSION;

        runOnce = true;
        windowHandle = (HWND)hwnd;

        TcpAccept = std::thread(TcpAcceptThread);
        TcpWork = std::thread(TcpWorkThread);

        auto hModule = GetModuleHandle(NULL);

        auto hResInfo = FindResourceEx(hModule, TEXT("WAVE"), MAKEINTRESOURCE(IDR_WAVE1), MAKELANGID(LANG_POLISH, SUBLANG_DEFAULT));

        if (hResInfo == nullptr)
            return;

        auto hRes = LoadResource(hModule, hResInfo);
        if (hRes == nullptr)
            return;

        MessageSound = (uint8_t*)LockResource(hRes);
    }

    ImGui::SetNextWindowSize(ImVec2(420, 300), ImGuiCond_FirstUseEver);
    ImGui::Begin("Remote hosts");
    if (BeginCombo("Host selection", ComboPreview.c_str()))
    {
        for (size_t i = 0; i < gVM_LIST.size(); i++)
        {
            const auto& host = *gVM_LIST[i];
            bool redText = std::any_of(std::cbegin(host.bots), std::cend(host.bots),
                [](const auto& bot)
                {
                    return bot.unreadedMessages;
                });

            if (redText)
                PushStyleColor(ImGuiCol_Text, ImVec4(.7f, .0f, .0f, 1.f));

            if (Selectable(host.name.c_str()))
            {
                ComboSelection = i;
                ComboPreview = host.name;
            }

            if (redText)
                PopStyleColor();
        }
        EndCombo();
    }

    if (ComboSelection != -1)
    {
        ComboPreview = gVM_LIST[ComboSelection]->name;

        // if gVM_LIST.size() <= ComboSelection we consider it as error and want application crash
        if (Button("Disable All"))      SetBotStatus(ComboSelection, -1, BotState::DISABLED);
        SameLine();
        if (Button("Enable All"))       SetBotStatus(ComboSelection, -1, BotState::AWAITING);
        SameLine();
        if (Button("Phase Select All")) PhaseSelectBot(ComboSelection, -1);
        //SameLine();
        //if (Button("Crash All"))        CrashBot(ComboSelection, -1);

        if (BeginChild("BotListChild"))
        {
            if (BeginListBox("BotList", ImVec2(-1, -1)))
            {
                for (size_t i = 0; i < gVM_LIST[ComboSelection]->bots.size(); i++)
                {
                    auto& bot = gVM_LIST[ComboSelection]->bots[i];

                    if (bot.unreadedMessages)
                        PushStyleColor(ImGuiCol_Text, ImVec4(.7f, .0f, .0f, 1.f));

                    if (Selectable((bot.name_with_id + " - " + std::string(BotStateNames[(size_t)bot.state])).c_str()))
                        bot.renderBotWindow = true;

                    if (bot.unreadedMessages)
                        PopStyleColor();
                }

                EndListBox();
            }
        }
        EndChild();
    }
    ImGui::End();

    for (size_t i = 0; i < gVM_LIST.size(); i++)
    {
        for (size_t j = 0; j < gVM_LIST[i]->bots.size(); j++)
            RenderBotWindow(i, j);
    }

    if (!ErrorQueue.empty())
    {
        static bool oldError = false;

        if (!oldError)
        {
            oldError = true;

#ifndef PUBLIC_RELEASE
            sndPlaySoundA((LPSTR)MessageSound, SND_MEMORY | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);
#endif

            std::thread([](auto windowHandle)
                {
                    ShowWindow(windowHandle, SW_RESTORE);

                    SetWindowPos(windowHandle, HWND_TOPMOST,   0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);  // it will bring window at the most front but makes it Always On Top.
                    SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);  // just after above call, disable Always on Top.
                }, windowHandle).detach();
        }

        ImGui::SetNextWindowSize(ImVec2(300, 220), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(ErrorQueue.front().type.c_str()))
        {
            Text("Got message from function: %s", ErrorQueue.front().from.c_str());
            TextWrapped("Message says: %s", ErrorQueue.front().message.c_str());

            if (Button("OK"))
            {
                ErrorQueue.pop();
                oldError = false;
            }
        }
        ImGui::End();
    }
}

void RenderBotWindow(size_t VM_INDEX, size_t BOT_INDEX)
{
    BotInstance& bot = gVM_LIST[VM_INDEX]->bots[BOT_INDEX];
    int ColorCount = 0;

    if (!bot.renderBotWindow)
        return;

    if (bot.colorNextFrame)
    {
        PushStyleColor(ImGuiCol_Border, ImVec4(.7f, .0f, .0f, 1.f));
        PushStyleColor(ImGuiCol_TitleBg, ImVec4(.7f, .0f, .0f, 1.f));
        PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(.7f, .0f, .0f, 1.f));
        PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(.6f, .1f, .1f, 1.f));
        ColorCount += 4;
    }

    bot.colorNextFrame = bot.unreadedMessages;

    ImGui::SetNextWindowSize(ImVec2(420, 500), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(bot.friendly_name.c_str(), &bot.renderBotWindow))
        goto EPILOGUE;

    PopStyleColor(ColorCount);
    ColorCount = 0;

    if (IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        // disable red text if we are already selected
        if (bot.unreadedMessages == true && !bot.privateChatSelection.empty())
        {
            bot.private_messages[bot.privateChatSelection].first = false;

            bot.unreadedMessages =
                std::any_of(
                    bot.private_messages.cbegin(),
                    bot.private_messages.cend(),
                    [](const auto& pair) { return pair.second.first == true; }
            );

            bot.colorNextFrame = bot.unreadedMessages;
        }
    }

    Text("Bot status - %s", BotStateNames[(size_t)bot.state]);

    if (Button("Disable Bot"))      SetBotStatus    (VM_INDEX, BOT_INDEX, BotState::DISABLED);
    SameLine();
    if (Button("Enable Bot"))       SetBotStatus    (VM_INDEX, BOT_INDEX, BotState::AWAITING);
    SameLine();
    if (Button("Phase Select"))     PhaseSelectBot  (VM_INDEX, BOT_INDEX);
    //SameLine();
    //if (Button("Crash Bot"))    CrashBot(VM_INDEX, BOT_INDEX);

    {
        const char* PreviewValue = "MSG Selection";

        if (!bot.privateChatSelection.empty())
        {
            PreviewValue = bot.privateChatSelection.c_str();
        }

        if (BeginCombo("Private Messages", PreviewValue))
        {
            auto iter = bot.private_messages.begin();
            for (size_t i = 0; i < bot.private_messages.size(); i++, iter++)
            {
                if (iter->second.first == true)
                    PushStyleColor(ImGuiCol_Text, ImVec4(.7f, .0f, .0f, 1.f));

                if (Selectable(iter->first.c_str()))
                {
                    bot.privateChatSelection = iter->first;
                    if (iter->second.first == true)
                    {
                        iter->second.first = false;

                        bot.unreadedMessages =
                            std::any_of(
                                bot.private_messages.cbegin(),
                                bot.private_messages.cend(),
                                [](const auto& pair) { return pair.second.first == true; }
                        );

                        bot.colorNextFrame = bot.unreadedMessages;
                        PopStyleColor();
                    }
                }

                if (iter->second.first == true)
                    PopStyleColor();
            }
            EndCombo();
        }

        if (!bot.privateChatSelection.empty())
        {
            if (BeginChild("Messages List", ImVec2(0,-25), true))
            {
                for (const auto& msg : bot.private_messages[bot.privateChatSelection].second)
                    TextWrapped(msg.c_str());

                SetScrollHereY();
            }
            EndChild();

            auto& messageBuff = bot.chatBox;
            if (InputText("Chat", messageBuff, sizeof(messageBuff) - 1, ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (messageBuff[0] != '\0')
                {
                    SendPrivateMessage(VM_INDEX, BOT_INDEX, bot.privateChatSelection, messageBuff);
                    bot.private_messages[bot.privateChatSelection].second.push_back(CurrentTime() + bot.nickname + ": " + std::string(messageBuff));
                    messageBuff[0] = '\0';
                }

                SetKeyboardFocusHere();
            }
        }
    }
EPILOGUE:
    PopStyleColor(ColorCount);
    ImGui::End();
}

void registerBotInstances(VMManager& vmMgr, const MetinGuestHost::RegisterBotInstanceMessage& register_bots)
{
    for (int i = 0; i < register_bots.bot_size(); i++)
    {
        const auto& registerData = register_bots.bot(i);
        vmMgr.bots.push_back({ registerData.botid(), registerData.botname(), vmMgr.name });
    }
}

void HandlePacketIncPacket(VMManager &vmMgr, const std::vector<uint8_t>& buff)
{
#define GET_VAR(origin, offset, type) (*(type*)(origin.data() + offset))
    boost::lock_guard lock(gVM_LIST_MUTEX);

    switch (buff[0])
    {
    // Compability layer
    case 0xF1: // set vmMgr name
    {
        std::string_view vmName(reinterpret_cast<const char*>(buff.data()) + 1, buff.size() - 2);
        vmMgr.name = vmName;
        break;
    }
    case 0xC0: // register bot instance
    {
        uint32_t botId = GET_VAR(buff, 1, uint32_t);
        std::string_view botName(reinterpret_cast<const char*>(buff.data()) + 5, buff.size() - 5);
        vmMgr.bots.push_back({botId, botName, vmMgr.name});
        break;
    }
    case 0xC1: // unregister bot instance
    {
        uint32_t botId = GET_VAR(buff, 1, uint32_t);
        
        auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
            [botId](const auto& obj)
            {
                return botId == obj.id;
            });

        if (result == std::end(vmMgr.bots))
            ErrorQueue.push({ "Got unregister bot instance packet but there is no bot with specified id", "HandleIncPacket", "Warning" });
        else
            vmMgr.bots.erase(result);
        break;
    }
    case 0xB0: // recv private message
    {
        uint32_t botId = GET_VAR(buff, 1, uint32_t);
        std::string_view nickname(reinterpret_cast<const char*>(buff.data()) + 5); // null terminated
        
        size_t message_start = 5 + nickname.size() + 1;
        std::string_view message(reinterpret_cast<const char*>(buff.data()) + message_start, buff.size() - message_start);

        std::string message_utf8 = boost::locale::conv::to_utf<char>(CurrentTime() + std::string(message), "Windows-1250");
        // std::string message_utf8(message);

        auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
            [botId](const auto& obj)
            {
                return botId == obj.id;
            });

        if (result == std::end(vmMgr.bots))
            ErrorQueue.push({ "Got private message packet but there is no bot with specified id, here is msg: " + message_utf8, "HandleIncPacket", "Warning" });
        else
        {
            result->unreadedMessages = true;
            result->private_messages[std::string(nickname)].first = true;
            result->private_messages[std::string(nickname)].second.push_back(message_utf8);
        }

        sndPlaySoundA((LPSTR)MessageSound, SND_MEMORY | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);

        std::thread([](auto windowHandle)
            {
                ShowWindow(windowHandle, SW_RESTORE);

                SetWindowPos(windowHandle, HWND_TOPMOST,   0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);   // it will bring window at the most front but makes it Always On Top.
                SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE); // just after above call, disable Always on Top.
            }, windowHandle).detach();

        break;
    }
    case 0xA0: // bot status change
    {
        uint32_t botId = GET_VAR(buff, 1, uint32_t);
        BotState botState = GET_VAR(buff, 5, BotState);

        auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
            [botId](const auto& obj)
            {
                return botId == obj.id;
            });

        if (result == std::end(vmMgr.bots))
            ErrorQueue.push({ "Got bot state packet but there is no bot with specified id", "HandleIncPacket", "Info" });
        else
            result->state = botState;

        break;
    }
    
    // New Protocol
    case 0xF2: // register manager
    {
        MetinGuestHost::RegisterVMInstanceMessage register_instance;
        register_instance.ParseFromArray(buff.data() + 1, buff.size() - 1);

        vmMgr.name = register_instance.vmname();
        registerBotInstances(vmMgr, register_instance.botinstance());
        break;
    }
    case 0xF3: // register bot
    {
        MetinGuestHost::RegisterBotInstanceMessage register_bots;
        register_bots.ParseFromArray(buff.data() + 1, buff.size() - 1);

        registerBotInstances(vmMgr, register_bots);
        break;
    }
    case 0xF4: // unregister bot
    {
        MetinGuestHost::UnregisterBotInstanceMessage unregister_bots;
        unregister_bots.ParseFromArray(buff.data() + 1, buff.size() - 1);

        for (int i = 0; i < unregister_bots.botid_size(); i++)
        {
            uint32_t botId = unregister_bots.botid(i);

            auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
                [botId](const auto& obj)
                {
                    return botId == obj.id;
                });

            if (result == std::end(vmMgr.bots))
                ErrorQueue.push({ "Got unregister bot instance packet but there is no bot with specified id", "HandleIncPacketv2", "Warning" });
            else
                vmMgr.bots.erase(result);
        }

        break;
    }
    case 0xF5: // update status
    {
        MetinGuestHost::BotStatusMessage botStatusMessage;
        botStatusMessage.ParseFromArray(buff.data() + 1, buff.size() - 1);

        for (int i = 0; i < botStatusMessage.status_size(); i++)
        {
            const auto& botStatus = botStatusMessage.status(i);
            uint32_t botId = botStatus.botid();
            BotState botState = (BotState)botStatus.botstatus();

            auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
                [botId](const auto& obj)
                {
                    return botId == obj.id;
                });

            if (result == std::end(vmMgr.bots)) // FIXME:
                ;// ErrorQueue.push({ "Got bot state packet but there is no bot with specified id", "HandleIncPacketv2" });
            else
                result->state = botState;
        }

        break;
    }
    case 0xF6: // got private message
    {
        MetinGuestHost::PrivateMessage priv_msg;
        priv_msg.ParseFromArray(buff.data() + 1, buff.size() - 1);

        std::string message_utf8 = boost::locale::conv::to_utf<char>(CurrentTime() + priv_msg.textmessage(), "Windows-1250");
        const std::string& nickname = priv_msg.targetnickname();
        // std::string message_utf8(message);

        auto result = std::find_if(std::begin(vmMgr.bots), std::end(vmMgr.bots),
            [botId=priv_msg.botid()](const auto& obj)
            {
                return botId == obj.id;
            });

        if (result == std::end(vmMgr.bots))
            ErrorQueue.push({ "Got private message packet but there is no bot with specified id, here is msg: " + message_utf8, "HandleIncPacketv2", "Warning" });
        else
        {
            result->unreadedMessages = true;
            result->private_messages[nickname].first = true;
            result->private_messages[nickname].second.push_back(message_utf8);
        }

        sndPlaySoundA((LPSTR)MessageSound, SND_MEMORY | SND_ASYNC | SND_NODEFAULT | SND_NOSTOP);

        std::thread([](auto windowHandle)
            {
                ShowWindow(windowHandle, SW_RESTORE);

                SetWindowPos(windowHandle, HWND_TOPMOST,   0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);  // it will bring window at the most front but makes it Always On Top.
                SetWindowPos(windowHandle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE);  // just after above call, disable Always on Top.
            }, windowHandle).detach();

         break;
    }
    }

#undef GET_VAR
}

void TcpAcceptThread()
{
    using namespace boost::asio;
    static io_context io_context;

    ip::tcp::acceptor acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), 7987));

    while (!gShutdownApp)
    {
        auto sock = std::make_unique<ip::tcp::socket>(io_context);
        acceptor.accept(*sock);

        std::unique_ptr mgr = std::make_unique<VMManager>("NotSet", sock, HandlePacketIncPacket);

        unique_lock lock(gVM_LIST_MUTEX);
        gVM_LIST.push_back(std::move(mgr));

        if (ComboSelection == -1)
            ComboSelection = 0;
    };
}

void TcpWorkThread()
{
    while (true)
    {
        for (int i = 0; i < gVM_LIST.size(); i++)
        {
            if (gVM_LIST[i]->threads_exited == 2)
            {
                boost::lock_guard lock(gVM_LIST_MUTEX);
                ErrorQueue.push({ gVM_LIST[i]->name + " has disconnected", "TcpDisconnectHandler", "Info" });

                gVM_LIST.erase(gVM_LIST.begin() + i);

                if (ComboSelection == i)
                {
                    ComboSelection = -1;
                    ComboPreview = NO_INSTANCE;
                }
                else if (ComboSelection > i)
                    ComboSelection -= 1;

                i--;
            }
        }

        Sleep(100);
    }
}

void SendPacket(size_t VM_INDEX, const std::vector<uint8_t>& data, uint8_t packetType)
{
    auto& vmMgr = *gVM_LIST[VM_INDEX];

    std::vector<uint8_t> readyPacket;
    readyPacket.resize(data.size() + 5); // uint32_t packet size + uint8_t packet type

    uint32_t packetSize = data.size() + 1;
    memcpy(readyPacket.data(), &packetSize, sizeof(packetSize));

    readyPacket[4] = packetType;
    std::copy(data.cbegin(), data.cend(), readyPacket.begin() + 5);

    boost::lock_guard lock(vmMgr.sendQueueMutex);
    vmMgr.sendQueue.push(std::move(readyPacket));
}

void SetBotStatus(size_t VM_INDEX, size_t BOT_INDEX, BotState botStatus)
{
    std::vector<uint8_t> packet;
    MetinGuestHost::BotStatus_ statusMessage;

    if (BOT_INDEX == -1)
        statusMessage.set_botid(BOT_INDEX);
    else
        statusMessage.set_botid(gVM_LIST[VM_INDEX]->bots[BOT_INDEX].id);

    statusMessage.set_botstatus((uint32_t)botStatus);

    packet.resize(statusMessage.ByteSizeLong());
    statusMessage.SerializeToArray(packet.data(), packet.size());

    SendPacket(VM_INDEX, packet, 0xF7);
}

void CrashBot(size_t VM_INDEX, size_t BOT_INDEX)
{
    constexpr std::string_view KILLER_USER = "~~NED";
    SendPrivateMessage(VM_INDEX, BOT_INDEX, KILLER_USER, "");
}

void PhaseSelectBot(size_t VM_INDEX, size_t BOT_INDEX)
{
    SendChatMessage(VM_INDEX, BOT_INDEX, "/phase_select");
}

void SendChatMessage(size_t VM_INDEX, size_t BOT_INDEX, std::string_view msg)
{
    constexpr std::string_view CHAT_USER = "~~CHT";
    SendPrivateMessage(VM_INDEX, BOT_INDEX, CHAT_USER, msg);
}

void SendPrivateMessage(size_t VM_INDEX, size_t BOT_INDEX, std::string_view nickname, std::string_view msg_ascii)
{
    std::vector<uint8_t> packet;

    std::string msg_utf8 = boost::locale::conv::from_utf<char>(std::string(msg_ascii), "Windows-1250");

    MetinGuestHost::PrivateMessage priv_msg;
    if (BOT_INDEX == -1)
        priv_msg.set_botid(BOT_INDEX);
    else
        priv_msg.set_botid(gVM_LIST[VM_INDEX]->bots[BOT_INDEX].id);

    priv_msg.set_targetnickname(nickname.data(), nickname.size());
    priv_msg.set_textmessage(msg_utf8);

    packet.resize(priv_msg.ByteSizeLong());
    priv_msg.SerializeToArray(packet.data(), packet.size());

    SendPacket(VM_INDEX, packet, 0xF8);
}

std::string CurrentTime()
{
    std::stringstream oss;

#pragma warning(push)
#pragma warning(disable : 4996)
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
#pragma warning(pop)

    oss << std::setw(2) << std::setfill('0') << std::put_time(&tm, "%H:%M:%S ");
    return oss.str();
}
