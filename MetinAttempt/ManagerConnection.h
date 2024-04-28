#pragma once
#include <cstdint>

#define BOOST_DATE_TIME_NO_LIB
#define BOOST_OVERRIDE
#include <boost/interprocess/managed_windows_shared_memory.hpp>

enum class BotState : uint32_t
{
    DISABLED,
    AWAITING,
    MINING,
    PICKING_ITEM,
    MOVING,
    PATH_GENERATION,

    FISHING,
    PRESSING_SPACE,
    PUTTING_ROBAK,

    BUYING_KILOF,
    PUTTING_KILOF,

    BUYING_WEDKA,
    PUTTING_WEDKA,

    TELEPORTING_TO_HANDLARZ,
    TELEPORTING_TO_WAYPOINT,

    CHATBOT_ENABLED,
    CHATBOT_DISABLED,
};

struct SharedMemory
{
    uint32_t botId;
    uint32_t heartbeat;
    BotState state;
    char nickname[32];
};

struct PrivateMessage
{
    char nickname[32]{ 0 };
    char message[512]{ 0 };
};

using namespace boost::interprocess;

typedef managed_windows_shared_memory::segment_manager segment_manager_t;
typedef allocator<void, segment_manager_t> void_allocator;

typedef allocator<SharedMemory, segment_manager_t>  ShmemAllocator;
typedef std::vector<SharedMemory, ShmemAllocator> SharedVector;

typedef allocator<PrivateMessage, segment_manager_t> PrivateMessageAllocator;
typedef std::deque<PrivateMessage, PrivateMessageAllocator> MessageQueue;

constexpr char SegmentName[] = "Metin2";
constexpr char ObjectName[] = "InstanceDataVector";

constexpr char IncomingMsg[] = "Minebot<-VMMgr";
constexpr char OutgoingMsg[] = "Minebot->VMMgr";
constexpr char MsgQName[] = "MsgQ";

// Shared memory stuff
extern managed_windows_shared_memory sharedVectorSegment, IncomingMsgSegment, OutgoingMsgSegment;
extern SharedVector* sharedVector;
extern MessageQueue* IncMsgQ, * OutMsgQ;

extern uint32_t gBotId;
extern uint32_t gCachedState;
extern bool gIsSetup;
// /Shared memory stuff

void setState(BotState st);

void on_whisper_append(int iType, const char* c_szName, const char* c_szMsg);

bool setupSharedMemory();

void heartBeat();

#define VERIFY_MANAGER_CONNECTION if (!setupSharedMemory()) \
        return; \
    heartBeat(); \
    if (gCachedState == -1) { \
        std::cout << "This bot probably timed out\n"; \
        gIsSetup = false; \
        return; \
    }
