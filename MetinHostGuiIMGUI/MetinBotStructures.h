#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/thread/locks.hpp>

using boost::mutex;
using boost::shared_mutex;
using boost::unique_lock;
using boost::shared_lock;

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

    BUYING_WETKA,
    PUTTING_WETKA,

    TELEPORTING_TO_HANDLARZ,
    TELEPORTING_TO_WAYPOINT,

    CHATBOT_ENABLED,
    CHATBOT_DISABLED,
};

static constexpr std::array<std::string_view, 17> BotStateNames
{
    "DISABLED",
    "AWAITING",
    "MINING",
    "PICKING_ITEM",
    "MOVING",
    "PATH_GENERATION",

    "FISHING",
    "PRESSING_SPACE",
    "PUTTING_ROBAK",

    "BUYING_KILOF",
    "PUTTING_KILOF",

    "BUYING_WEDKA",
    "PUTTING_WEDKA",

    "TELEPORTING_TO_HANDLARZ",
    "TELEPORTING_TO_WAYPOINT",

    "CHATBOT_ENABLED",
    "CHATBOT_DISABLED",
};


struct BotInstance
{
    uint32_t id;
    BotState state;

    std::string nickname; // ExampleBot
    std::string name_with_id; // 0 - ExampleBot
    std::string friendly_name; // 0 - ExampleBot (DESKTOP-2K4J55P)
    std::map<std::string, std::pair<bool, std::vector<std::string>>> private_messages; // bool unReaded

    bool renderBotWindow{ false };
    bool unreadedMessages{ false }; // shortcut for private_messages
    bool colorNextFrame{ false }; // prevent 1 frame red blink if window is focused and private msg arrives
    std::string privateChatSelection;
    char chatBox[100]{ };

    BotInstance(uint32_t id, std::string_view nickname, const std::string& hostname)
    {
        this->id = id;
        this->nickname = nickname;

        state = BotState::DISABLED;
        name_with_id = boost::lexical_cast<std::string>(id) + " - " + this->nickname;
        friendly_name = name_with_id + " (" + hostname + ')';
    }
};

class VMManager
{
public:
    VMManager(std::string_view name, std::unique_ptr<boost::asio::ip::tcp::socket>& socket, std::function<void(VMManager&, const std::vector<uint8_t>&)> callback)
    {
        this->name = name;
        sock = std::move(socket);
        recvCallback = callback;
        read_thread = std::thread(&VMManager::readThread, this);
        write_thread = std::thread(&VMManager::writeThread, this);
    }

    VMManager(const VMManager&) = delete;
    VMManager& operator=(const VMManager&) = delete;
    VMManager(VMManager&&) = delete;
    VMManager& operator=(VMManager&&) = delete;

    ~VMManager()
    {
        read_thread.join();
        write_thread.join();
    };

    std::string name;
    std::unique_ptr<boost::asio::ip::tcp::socket> sock;
    std::queue<std::vector<uint8_t>> sendQueue;
    std::vector<BotInstance> bots;

    mutex sendQueueMutex;

    int threads_exited = 0; // 2 - can be removed, 1 - dont render this object
private:
    std::thread read_thread, write_thread;
    std::function<void(VMManager&, const std::vector<uint8_t>&)> recvCallback;


    void readThread()
    {
        using namespace boost;
        std::vector<uint8_t> buff;

        try
        {
            while (true)
            {
                buff.resize(4);
                asio::read(*sock, asio::buffer(buff), asio::transfer_exactly(4));
                uint32_t packetSize = *(uint32_t*)buff.data();
                buff.resize(packetSize);

                asio::read(*sock, asio::buffer(buff), asio::transfer_exactly(packetSize));
                recvCallback(*this, buff);
            }
        }
        catch (boost::system::system_error& ec)
        {
            sendQueueMutex.lock();
            threads_exited += 1;
            sendQueueMutex.unlock();
            return;
        }
    }

    void writeThread()
    {
        using namespace boost;

        while (true)
        {
            sendQueueMutex.lock();
            try
            {
                while (!sendQueue.empty())
                {
                    asio::write(*sock, asio::buffer(sendQueue.front()), asio::transfer_all());
                    sendQueue.pop();
                }
            }
            catch (boost::system::system_error& ec)
            {
                threads_exited += 1;
                sendQueueMutex.unlock();
                return;
            }

            if (threads_exited > 0)
            {
                threads_exited++;
                sendQueueMutex.unlock();
                return;
            }

            sendQueueMutex.unlock();
            Sleep(50);
        }
    }
};

struct Error
{
    std::string message;
    std::string from;
    std::string type{ "Error" };
};


