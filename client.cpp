#include <cstdint>
#include <cstdio>
#include <thread>
#include <array>
#include <csignal>
#include <cinttypes>
#include "config.h"
#include "Aeron.h"
#include "orderMessage.h"

using namespace aeron::util;
using namespace aeron;

std::atomic<bool> running(true);

void sigIntHandler(int)
{
    running = false;
}

struct Settings
{
    std::string channel = configuration::DEFAULT_CHANNEL;
    std::int32_t streamId = configuration::DEFAULT_STREAM_ID;
    int lingerTimeoutMs = configuration::DEFAULT_LINGER_TIMEOUT_MS;
};

typedef std::array<std::uint8_t, 256> buffer_t;

int main(int argc, char **argv)
{
    long nMessages;
    if (argc == 3 && !std::strcmp("-n", argv[1]))
    {
        nMessages = atoi(argv[2]);
    }
    else
    {
        nMessages = 10;
    }
    try
    {
        Settings settings;
        std::cout << "Publishing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        aeron::Context context;

        context.newPublicationHandler(
            [](const std::string &channel, std::int32_t streamId, std::int32_t sessionId, std::int64_t correlationId)
            {
                std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":" << sessionId << std::endl;
            });

        std::shared_ptr<Aeron> aeron = Aeron::connect(context);
        signal(SIGINT, sigIntHandler);
        // add the publication to start the process
        std::int64_t id = aeron->addPublication(settings.channel, settings.streamId);

        std::shared_ptr<Publication> publication = aeron->findPublication(id);
        // wait for the publication to be valid
        while (!publication)
        {
            std::this_thread::yield();
            publication = aeron->findPublication(id);
        }

        const std::int64_t channelStatus = publication->channelStatus();

        std::cout << "Publication channel status (id=" << publication->channelStatusId() << ") "
                  << (channelStatus == ChannelEndpointStatus::CHANNEL_ENDPOINT_ACTIVE ? "ACTIVE" : std::to_string(channelStatus))
                  << std::endl;

        AERON_DECL_ALIGNED(buffer_t buffer, 32);
        concurrent::AtomicBuffer srcBuffer(&buffer[0], buffer.size());
        OrderMessage &data = srcBuffer.overlayStruct<OrderMessage>(0);
        long msgLength = sizeof(data);

        std::this_thread::sleep_for(std::chrono::seconds(2));
        for (std::int64_t i = 0; i < nMessages && running; i++)
        // while (running)
        {
            // std::cout << "Place an order:\n---------------\n";
            // std::cout << "Order Type (market: 0, limit: 1):\n";
            // std::cin >> data.type;
            // std::cout << "Order Side (BUY: 0, SELL: 1):\n";
            // std::cin >> data.side;
            // if (data.type != 0)
            // {
            //     std::cout << "Order Price:\n";
            //     std::cin >> data.price;
            // }
            // std::cout << "Order Quantity:\n";
            // std::cin >> data.quantity;
            data.price = 100;
            data.quantity = 10 + i;
            data.side = i % 2;
            data.type = (i + 1) % 2;

            const std::int64_t result = publication->offer(srcBuffer, 0, msgLength);

            if (result > 0)
            {
                std::cout << "Message sent" << std::endl;
            }
            else if (BACK_PRESSURED == result)
            {
                std::cout << "Offer failed due to back pressure" << std::endl;
            }
            else if (NOT_CONNECTED == result)
            {
                std::cout << "Offer failed because publisher is not connected to a subscriber" << std::endl;
            }
            else if (ADMIN_ACTION == result)
            {
                std::cout << "Offer failed because of an administration action in the system" << std::endl;
            }
            else if (PUBLICATION_CLOSED == result)
            {
                std::cout << "Offer failed because publication is closed" << std::endl;
            }
            else
            {
                std::cout << "Offer failed due to unknown reason " << result << std::endl;
            }

            if (!publication->isConnected())
            {
                std::cout << "No active subscribers detected" << std::endl;
            }

            std::cout << std::endl;
        }

        std::cout << "Done sending." << std::endl;

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