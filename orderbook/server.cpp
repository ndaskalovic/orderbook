#include <cstdint>
#include <thread>
#include <csignal>
#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <unordered_map>

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
#include "aeronUtils.h"

using namespace aeron::util;
using namespace aeron;

std::atomic<bool> running(true);
std::atomic<int> oid = 0;
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
            std::scoped_lock bufLock{bufferMutex};
            recentOrders[oid % 5] = orderp;
            oid++;
    };
}

void dbThreadTask()
{
    static int idcount = 0;
    DatabaseConnection DB(configuration::DATABASE_PATH);
    while (running)
    {
        const auto now = std::chrono::system_clock::now();
        auto fnow = std::format("{:%FT%TZ}", now);
        DB.InsertPriceVolData(fnow, oid - idcount, book.GetCurrentPrice());
        if ((oid - idcount) > 5)
        {
            std::scoped_lock bufLock{bufferMutex};
            for (int i = 0; i < 5; i++)
            {
                if (recentOrders[i])
                {
                    DB.InsertOrderData(fnow, recentOrders[i]->GetOrderType(), recentOrders[i]->GetOrderSide(), recentOrders[i]->GetOrderPrice(), recentOrders[i]->GetOrderQuantity());
                    // std::cout << "OID: " << oid << ", recent order ID: " << recentOrders[i]->GetOrderId() << "\n";
                }
            };
        }
        std::cout << "Price: " << book.GetCurrentPrice() << ", Volume: " << oid - idcount << "\n\n";

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
        Settings settings(configuration::DEFAULT_CHANNEL, configuration::DEFAULT_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);

        std::cout << "Subscribing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        aeron::Context context;

        AeronSubscription aeronSubscription = connectToAeronSubscription(settings);
        signal(SIGINT, sigIntHandler);

        FragmentAssembler fragmentAssembler(addOrderToBook());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

        Subscription &subscriptionRef = *aeronSubscription.subscription;

        dbThread = std::make_shared<std::thread>(dbThreadTask);

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
            const int fragmentsRead = aeronSubscription.subscription->poll(handler, FRAGMENTS_LIMIT);
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