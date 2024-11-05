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

void sigIntHandler(int)
{
    running = false;
}

typedef std::array<std::uint8_t, 16> buffer_t;


fragment_handler_t printPrice()
{
    return [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length, const Header &header)
    {
        PriceMessage data = buffer.overlayStruct<PriceMessage>(offset);
        std::cout << "Price= " << data.price << "\n";
    };
}

int main(int argc, char **argv)
{
    try
    {
        Settings settings(configuration::DEFAULT_PRICE_DATA_CHANNEL, configuration::DEFAULT_PRICE_DATA_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);
        std::cout << "Publishing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        AeronSubscription dataSubscription = connectToAeronSubscription(settings);
        signal(SIGINT, sigIntHandler);

        FragmentAssembler fragmentAssembler(printPrice());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

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