#include <cstdint>
#include <thread>
#include <csignal>
#include <iostream>

#include "config.h"
#include "Aeron.h"
#include "FragmentAssembler.h"
#include "concurrent/SleepingIdleStrategy.h"
#include "messages.h"
#include "usings.h"
#include "orderType.h"
#include "side.h"
#include "order.h"
#include "orderBook.h"
#include "sqliteConnection.h"
#include "aeronUtils.h"

using namespace aeron::util;
using namespace aeron;
typedef std::array<std::uint8_t, 16> buffer_t;

std::atomic<bool> running(true);
std::atomic<std::uint64_t> oid = 1;
// 5 most recent orders for logging
std::array<OrderPointer, 5> recentOrders;
std::mutex bufferMutex;
OrderBook book;

void sigIntHandler(int)
{
    running = false;
}

static const std::chrono::duration<long, std::milli> IDLE_SLEEP_MS(1);
static const int FRAGMENTS_LIMIT = 20;

fragment_handler_t addOrderToBook()
{
    return [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length, const Header &header)
    {
        OrderMessage data = buffer.overlayStruct<OrderMessage>(offset);
        Order order(data.type, oid, data.side, data.quantity, data.price);
        OrderPointer orderp = std::make_shared<Order>(order);
        // may incur overhead
        if (book.AddOrder(orderp))
        {
            std::scoped_lock bufLock{bufferMutex};
            recentOrders[oid % 5] = orderp;
        }
        oid++;
    };
}

void priceBroadcastTask(Settings *settings, AeronPublication *publication)
{
    static int idcount = 0;
    DatabaseConnection DB(settings->databasePath);
    AERON_DECL_ALIGNED(buffer_t buffer, 16);
    concurrent::AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    PriceMessage &data = srcBuffer.overlayStruct<PriceMessage>(0);
    long msgLength = sizeof(data);
    while (running)
    {
        data.price = book.GetCurrentPrice();
        const std::int64_t result = publication->publication->offer(srcBuffer, 0, msgLength);
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
}

void dbThreadTask(Settings *settings)
{
    static std::uint64_t idcount = 0;
    DatabaseConnection DB(configuration::DATABASE_PATH);
    Price currentPrice;
    while (running)
    {
        currentPrice = book.GetCurrentPrice();
        const auto now = std::chrono::system_clock::now();
        auto fnow = std::format("{:%FT%TZ}", now);
        DB.InsertPriceVolData(fnow, oid - idcount, currentPrice);
        // at least 5 new orders added since last check
        if ((oid - idcount) > 5)
        {
            std::scoped_lock bufLock{bufferMutex};
            for (int i = 0; i < 5; i++)
            {
                if (recentOrders[i])
                {
                    DB.InsertOrderData(fnow, recentOrders[i]->GetOrderType(), recentOrders[i]->GetOrderSide(), recentOrders[i]->GetOrderPrice(), recentOrders[i]->GetOrderQuantity());
                }
            };
        }
        book.PrintBook();
        std::cout << "Price: " << currentPrice << ", Volume: " << oid - idcount << " Size: " << book.GetSize() << "\n\n";

        idcount = oid;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        }
}

int main(int argc, char **argv)
{
    std::shared_ptr<std::thread> priceBroadcastThread;
    std::shared_ptr<std::thread> dbThread;
    Price startPrice(10000);
    srand(static_cast<unsigned>(std::time(0)));
    // start off the book with some random volume on both sides
    for (int i = 1; i < 1000; i++)
    {
        Order buy = Order(OrderType::LIMIT_ORDER, oid, Side::BUY, rand() % 1000 + 100, startPrice - i);
        OrderPointer buyPointer = std::make_shared<Order>(buy);
        book.AddOrder(buyPointer);
        oid++;
        Order sell = Order(OrderType::LIMIT_ORDER, oid, Side::SELL, rand() % 1000 + 100, startPrice + i);
        OrderPointer sellPointer = std::make_shared<Order>(sell);
        book.AddOrder(sellPointer);
        oid++;
    }

    try
    {
        Settings settings(configuration::DEFAULT_CHANNEL, configuration::DEFAULT_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);
        Settings dataSettings(configuration::DEFAULT_PRICE_DATA_CHANNEL, configuration::DEFAULT_PRICE_DATA_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);

        std::cout << "Subscribing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        AeronPublication dataPublication = connectToAeronPublication(dataSettings);

        AeronSubscription orderSubscription = connectToAeronSubscription(settings);
        signal(SIGINT, sigIntHandler);

        FragmentAssembler fragmentAssembler(addOrderToBook());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

        Subscription &subscriptionRef = *orderSubscription.subscription;

        priceBroadcastThread = std::make_shared<std::thread>(priceBroadcastTask, &dataSettings, &dataPublication);
        dbThread = std::make_shared<std::thread>(dbThreadTask, &dataSettings);

        aeron::util::OnScopeExit tidy(
            [&]()
            {
                running = false;

                if (nullptr != priceBroadcastThread && priceBroadcastThread->joinable())
                {
                    priceBroadcastThread->join();
                    priceBroadcastThread = nullptr;
                }
                if (nullptr != dbThread && dbThread->joinable())
                {
                    dbThread->join();
                    dbThread = nullptr;
                }
            });

        while (running)
        {
            const int fragmentsRead = orderSubscription.subscription->poll(handler, FRAGMENTS_LIMIT);
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