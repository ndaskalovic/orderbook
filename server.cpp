#include <cstdint>
#include <thread>
#include <csignal>

#include "config.h"
#include "util/CommandOptionParser.h"
#include "Aeron.h"
#include "FragmentAssembler.h"
#include "concurrent/SleepingIdleStrategy.h"
#include "orderMessage.h"

using namespace aeron::util;
using namespace aeron;

std::atomic<bool> running(true);

void sigIntHandler(int)
{
    running = false;
}

static const std::chrono::duration<long, std::milli> IDLE_SLEEP_MS(1);
static const int FRAGMENTS_LIMIT = 20;


struct Settings
{
    // std::string dirPrefix;
    std::string channel = configuration::DEFAULT_CHANNEL;
    std::int32_t streamId = configuration::DEFAULT_STREAM_ID;
};


fragment_handler_t printStringMessage()
{
    return [&](const AtomicBuffer &buffer, util::index_t offset, util::index_t length, const Header &header)
    {
        OrderMessage data = buffer.overlayStruct<OrderMessage>(offset);
        std::cout
            << "-->"
            // << "--> Message to stream " << header.streamId() << " from session " << header.sessionId()
            // << "(" << length << "@" << offset << ") <<" << " "
            << " Price: " << data.price
            << " Quantity: " << data.quantity
            << " Side: " << data.side
            << " Type: " << data.type
            // << ">>"
            << std::endl;
    };
}

int main(int argc, char **argv)
{
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

        FragmentAssembler fragmentAssembler(printStringMessage());
        fragment_handler_t handler = fragmentAssembler.handler();
        SleepingIdleStrategy idleStrategy(IDLE_SLEEP_MS);

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