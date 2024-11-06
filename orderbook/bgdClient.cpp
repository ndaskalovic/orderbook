#include <cstdint>
#include <cstdio>
#include <thread>
#include <array>
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

using namespace aeron::util;
using namespace aeron;

static const std::chrono::duration<long, std::milli> IDLE_SLEEP_MS(1);
static const int FRAGMENTS_LIMIT = 20;

std::atomic<bool> running(true);
std::atomic<Price> currentPrice(0);

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
    srand(time(NULL));
    AERON_DECL_ALIGNED(buffer_t buffer, 16);
    concurrent::AtomicBuffer srcBuffer(&buffer[0], buffer.size());
    OrderMessage &data = srcBuffer.overlayStruct<OrderMessage>(0);
    long msgLength = sizeof(data);
    while (running)
    {

        data.side = rand() % 2  == 1 ? Side::BUY : Side::SELL;
        data.type = rand() % 100 < 52 ? OrderType::LIMIT_ORDER : OrderType::MARKET_ORDER;
        if (data.type == OrderType::LIMIT_ORDER)
        {
            int p = rand() % 5 + 1;
            data.price = data.side == Side::BUY ? currentPrice + p * -1 : currentPrice + p;
            data.quantity = rand() % 30 + 1;
        } else {
            data.price = 0;
            data.quantity = rand() % 35 + 1;
        }
        publication->publication->offer(srcBuffer, 0, msgLength);
        // about 7k orders per second
        std::this_thread::sleep_for(std::chrono::microseconds(rand() % 2 + 1));
        
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