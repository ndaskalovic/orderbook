#include <cstdint>
#include <thread>
#include <csignal>
#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <unordered_map>
#include <sqlite3.h>

#include "config.h"
#include "Aeron.h"
#include "FragmentAssembler.h"
#include "concurrent/SleepingIdleStrategy.h"
#include "orderMessage.h"
#include "usings.h"
#include "trade.h"
#include "tradeInfo.h"
#include "orderType.h"
#include "side.h"
#include "order.h"
#include "orderBook.h"
#include "sqliteConnection.h"

using namespace aeron::util;
using namespace aeron;

std::atomic<bool> running(true);
std::atomic<int> oid = 0;
OrderBook book;

void sigIntHandler(int)
{
    running = false;
}

static const std::chrono::duration<long, std::milli> IDLE_SLEEP_MS(1);
static const int FRAGMENTS_LIMIT = 20;

struct Settings
{
    std::string channel = configuration::DEFAULT_CHANNEL;
    std::int32_t streamId = configuration::DEFAULT_STREAM_ID;
    const char *databasePath = configuration::DATABASE_PATH;
};

fragment_handler_t addOrderToBook()
{
    return [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length, const Header &header)
    {
        OrderMessage data = buffer.overlayStruct<OrderMessage>(offset);
        Order order(data.type, oid, data.side, data.quantity, data.price);
        OrderPointer orderp = std::make_shared<Order>(order);
        if (book.AddOrder(orderp))
            oid++;
    };
}

void dbThreadTask(char *dbPath)
{
    static int idcount = 0;
    DatabaseConnection DB(dbPath);
    while (running)
    {
        std::cout << "Price: " << book.GetCurrentPrice() << ", Volume: " << oid - idcount << "\n\n";
        const auto now = std::chrono::system_clock::now();
        DB.InsertPriceVolData(std::format("{:%FT%TZ}", now).c_str(), oid - idcount, book.GetCurrentPrice());
        idcount = oid;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main(int argc, char **argv)
{
    std::shared_ptr<std::thread> dbThread;
    Price startPrice(100);
    for (int i = 1; i < 20; i++)
    {
        Order buy = Order(OrderType::LIMIT_ORDER, oid, Side::BUY, rand() % 20000 + 100000, startPrice - i);
        OrderPointer buyPointer = std::make_shared<Order>(buy);
        book.AddOrder(buyPointer);
        oid++;
        Order sell = Order(OrderType::LIMIT_ORDER, oid, Side::SELL, rand() % 20000 + 100000, startPrice + i);
        OrderPointer sellPointer = std::make_shared<Order>(sell);
        book.AddOrder(sellPointer);
        oid++;
    }

    try
    {
        Settings settings;

        std::cout << "Subscribing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        aeron::Context context;

        context.newSubscriptionHandler(
            [](const std::string &channel, std::int32_t streamId, std::int64_t correlationId)
            {
                std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
            });

        context.availableImageHandler(
            [](Image &image)
            {
                std::cout << "Available image correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
                std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
            });

        context.unavailableImageHandler(
            [](Image &image)
            {
                std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
                std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
            });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);
        signal(SIGINT, sigIntHandler);
        // add the subscription to start the process
        std::int64_t id = aeron->addSubscription(settings.channel, settings.streamId);

        std::shared_ptr<Subscription> subscription = aeron->findSubscription(id);
        // wait for the subscription to be valid
        while (!subscription)
        {
            std::this_thread::yield();
            subscription = aeron->findSubscription(id);
        }

        const std::int64_t channelStatus = subscription->channelStatus();

        std::cout
            << "Subscription channel status (id=" << subscription->channelStatusId() << ") "
            << (channelStatus == ChannelEndpointStatus::CHANNEL_ENDPOINT_ACTIVE ? "ACTIVE" : std::to_string(channelStatus))
            << std::endl;

        FragmentAssembler fragmentAssembler(addOrderToBook());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

        Subscription &subscriptionRef = *subscription;

        dbThread = std::make_shared<std::thread>(dbThreadTask, settings.databasePath);

        aeron::util::OnScopeExit tidy(
            [&]()
            {
                running = false;

                if (nullptr != dbThread && dbThread->joinable())
                {
                    dbThread->join();
                    dbThread = nullptr;
                }
            });

        while (running)
        {
            const int fragmentsRead = subscription->poll(handler, FRAGMENTS_LIMIT);
            idleStrategy.idle(fragmentsRead);
        }
    }
    catch (const SourcedException &e)
    {
        std::cerr << "FAILED: " << e.what() << " : " << e.where() << std::endl;
        return -1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "FAILED: " << e.what() << " : " << std::endl;
        return -1;
    }

    return 0;
}