#include <cstdint>
#include <cstdio>
#include <thread>
#include <array>
#include <random>
#include <csignal>
#include <cinttypes>

#include "config.h"
#include "Aeron.h"
#include "messages.h"
#include "orderType.h"
#include "side.h"
#include "aeronUtils.h"
#include "FragmentAssembler.h"
#include "concurrent/SleepingIdleStrategy.h"
#include "sqliteConnection.h"

using namespace aeron::util;
using namespace aeron;

static const std::chrono::duration<long, std::milli> IDLE_SLEEP_MS(1);
static const int FRAGMENTS_LIMIT = 20;

std::atomic<bool> running(true);
std::atomic<Price> currentPrice(-1);

void sigIntHandler(int)
{
    running = false;
}

typedef std::array<std::uint8_t, 16> buffer_t;


fragment_handler_t updateCurrentPrice()
{
    return [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length, const Header &header)
    {
        PriceMessage data = buffer.overlayStruct<PriceMessage>(offset);
        currentPrice.store(data.price);
    };
}


void orderThreadTask(Settings *settings, AeronPublication *publication)
{
    DatabaseConnection DB(configuration::DATABASE_PATH);

    srand(static_cast<unsigned>(std::time(0)));
    AERON_DECL_ALIGNED(buffer_t buffer, 16);
    concurrent::AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    OrderMessage &data = srcBuffer.overlayStruct<OrderMessage>(0);
    long msgLength = sizeof(data);

    std::random_device rd;
    std::mt19937 gen(rd());
    // gamma distribution to skew limit orders closer to current price and taper off
    std::gamma_distribution<> gd(3, 2);
    int buyRatio;

    while (currentPrice == -1)
    {
        std::this_thread::yield();
    }

    while (running)
    {
        buyRatio = DB.GetOrderPressureRatio();
        for (int i = 0; i < 100000; i++)
        {
            data.side = rand() % 1000 < buyRatio ? Side::BUY : Side::SELL;
            // 85% limit orders
            data.type = rand() % 100 < 85 ? OrderType::LIMIT_ORDER : OrderType::MARKET_ORDER;
            if (data.type == OrderType::LIMIT_ORDER)
            {
                int p = static_cast<int>(gd(gen)) + 1;
                data.price = data.side == Side::BUY ? currentPrice + p * -1 : currentPrice + p;
                data.quantity = rand() % 100 + 1;
            } else {
                data.quantity = rand() % 569 + 1;
            }
            int result = publication->publication->offer(srcBuffer, 0, msgLength);
            // not worrying about back pressure since i want max throughput for the demo
        }
        
    }
}

int main(int argc, char **argv)
{
    std::shared_ptr<std::thread> orderThread;
    try
    {
        Settings dataSettings(configuration::DEFAULT_PRICE_DATA_CHANNEL, configuration::DEFAULT_PRICE_DATA_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);
        std::cout << "Publishing to channel " << dataSettings.channel << " on Stream ID " << dataSettings.streamId << std::endl;
        
        Settings orderSettings(configuration::DEFAULT_CHANNEL, configuration::DEFAULT_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);
        std::cout << "Publishing to channel " << orderSettings.channel << " on Stream ID " << orderSettings.streamId << std::endl;

        AeronSubscription dataSubscription = connectToAeronSubscription(dataSettings);
        AeronPublication orderPublication = connectToAeronPublication(orderSettings);
        signal(SIGINT, sigIntHandler);

        FragmentAssembler fragmentAssembler(updateCurrentPrice());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

        orderThread = std::make_shared<std::thread>(orderThreadTask, &orderSettings, &orderPublication);

        aeron::util::OnScopeExit tidy(
            [&]()
            {
                running = false;

                if (nullptr != orderThread && orderThread->joinable())
                {
                    orderThread->join();
                    orderThread = nullptr;
                }
            });

        while (running)
        {
            const int fragmentsRead = dataSubscription.subscription->poll(handler, FRAGMENTS_LIMIT);
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