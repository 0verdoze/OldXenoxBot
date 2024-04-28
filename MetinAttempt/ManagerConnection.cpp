#include "ManagerConnection.h"
#include "SharedHeader.h"
#include "PatchedFunctions.h"

#include <iostream>

// Global Shared memory stuff
managed_windows_shared_memory sharedVectorSegment, IncomingMsgSegment, OutgoingMsgSegment;
SharedVector* sharedVector;
MessageQueue* IncMsgQ, * OutMsgQ;

uint32_t gBotId;
uint32_t gCachedState = -1;
bool gIsSetup = false;
// /Global Shared memory stuff

void setState(BotState st)
{
    for (auto& botData : *sharedVector)
    {
        if (botData.botId == gBotId)
        {
            botData.state = st;
            break;
        }
    }
}

void on_whisper_append(int iType, const char* c_szName, const char* c_szMsg)
{
    if (OutMsgQ != nullptr)
    {
        PrivateMessage msg{ };
        int nickSize = strlen(c_szName);
        int messageSize = strlen(c_szMsg);

        if (nickSize >= sizeof(PrivateMessage::nickname))
        {
            std::cout << "Received private message from nickname longer than PrivateMessage::nickname buffer\n";
            std::cout << c_szName << ':' << c_szMsg << '\n';
            return;
        }

        if (messageSize >= sizeof(PrivateMessage::message))
        {
            std::cout << "Received private message longer than PrivateMessage::message buffer\n";
            std::cout << c_szName << ':' << c_szMsg << '\n';
            return;
        }

        memcpy(&(msg.nickname[0]), c_szName, nickSize);
        memcpy(&(msg.message[0]), c_szMsg, messageSize);

        OutMsgQ->push_back(msg);
    }
}

bool setupSharedMemory()
{
    static bool called_once = false;
    if (!called_once)
    {
        whisper_append_callbacks.push_back(on_whisper_append);
        called_once = true;
    }

    if (gIsSetup == true)
        return true;

    try
    {
        sharedVectorSegment = managed_windows_shared_memory(open_only, SegmentName);
        sharedVector = sharedVectorSegment.find<SharedVector>(ObjectName).first;

        if (sharedVector == nullptr)
        {
            std::cout << "sharedVector failed\n";
            return false;
        }

        if (sharedVector->size() == 0)
        {
            gBotId = 0;
        }
        else
        {
            // debugPrint(("size non zero " + boost::lexical_cast<std::string>(sharedVector->size())).c_str());
            gBotId = sharedVector->back().botId + 1;
        }

        IncomingMsgSegment = managed_windows_shared_memory(open_or_create, (IncomingMsg + boost::lexical_cast<std::string>(gBotId)).c_str(), sizeof(PrivateMessage) * 100);
        OutgoingMsgSegment = managed_windows_shared_memory(open_or_create, (OutgoingMsg + boost::lexical_cast<std::string>(gBotId)).c_str(), sizeof(PrivateMessage) * 2000);

        const PrivateMessageAllocator incAlloc(IncomingMsgSegment.get_segment_manager());
        const PrivateMessageAllocator outAlloc(OutgoingMsgSegment.get_segment_manager());

        IncMsgQ = IncomingMsgSegment.find_or_construct<MessageQueue>(MsgQName)(incAlloc);
        OutMsgQ = OutgoingMsgSegment.find_or_construct<MessageQueue>(MsgQName)(outAlloc);

        IncMsgQ->clear();
        OutMsgQ->clear();

        SharedMemory botData{
            gBotId, 0, BotState::DISABLED
        };

        auto& player = *GetObjectPointer<CPythonPlayer>();
        if (player.m_stName.empty())
        {
            return false;
        }

        memcpy(&(botData.nickname[0]), player.m_stName.c_str(), player.m_stName.size());
        botData.nickname[player.m_stName.size()] = 0;

        sharedVector->push_back(botData);
    }
    catch (...) // const interprocess_exception&
    {
        std::cout << "Unable to gain access to shared memory\n";
        return false;
    }

    gIsSetup = true;
    return true;
}


void heartBeat()
{
    gCachedState = -1;
    for (auto& botData : *sharedVector)
    {
        if (botData.botId == gBotId)
        {
            botData.heartbeat += 1;
            gCachedState = (uint32_t)botData.state;
            break;
        }
    }

    if (gCachedState != -1)  // MSG Client<-Manager
    {
        while (IncMsgQ->size())
        {
            // IncMsgQ->front().
            char messageBuff[sizeof(PrivateMessage::message) * 2]{ 0 };
            char nicknameBuff[sizeof(PrivateMessage::nickname) * 2]{ 0 };
            int buf_index = 0;

            const auto& message = IncMsgQ->front().message;
            const auto& nickname = IncMsgQ->front().nickname;

            if (nickname == std::string_view("~~NED"))
            {
                throw;  // terminate process
            }
            else if (nickname == std::string_view("~~CHT"))
            {
                EvalPy("networkModule.net.SendChatPacket('%s')", message);
            }
            else
            {
                for (int i = 0; i < sizeof(PrivateMessage::message); i++)
                {
                    if (message[i] == '\\' || message[i] == '"')
                    {
                        messageBuff[buf_index] = '\\';
                        buf_index += 1;
                    }

                    messageBuff[buf_index] = message[i];
                    buf_index += 1;
                }

                buf_index = 0;

                for (int i = 0; i < sizeof(PrivateMessage::nickname); i++)
                {
                    if (nickname[i] == '\\' || nickname[i] == '"')
                    {
                        nicknameBuff[buf_index] = '\\';
                        buf_index += 1;
                    }

                    nicknameBuff[buf_index] = nickname[i];
                    buf_index += 1;
                }

                EvalPy("networkModule.net.SendWhisperPacket(\"%s\", \"%s\")", nickname, message);
            }
            IncMsgQ->pop_front();
        }
    }
}