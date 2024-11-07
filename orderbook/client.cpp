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

using namespace aeron::util;
using namespace aeron;

std::atomic<bool> running(true);

void sigIntHandler(int)
{
    running = false;
}

typedef std::array<std::uint8_t, 16> buffer_t;

int main(int argc, char **argv)
{
    int bratio;
    try
    {
        Settings settings(configuration::DEFAULT_CHANNEL, configuration::DEFAULT_STREAM_ID, configuration::DATABASE_PATH, configuration::DEFAULT_LINGER_TIMEOUT_MS);
        std::cout << "Publishing to channel " << settings.channel << " on Stream ID " << settings.streamId << std::endl;

        AeronPublication aeronPublication = connectToAeronPublication(settings);
        signal(SIGINT, sigIntHandler);

        const std::int64_t channelStatus = aeronPublication.publication->channelStatus();

        AERON_DECL_ALIGNED(buffer_t buffer, 16);
        concurrent::AtomicBuffer srcBuffer(&buffer[0], buffer.size());
        OrderMessage &data = srcBuffer.overlayStruct<OrderMessage>(0);
        long msgLength = sizeof(data);
        int side;
        long q;

        while (running)
        {
            std::cout << "\n\nEnter the side: ";
            std::cin >> side;
            std::cout << "\n\nEnter the quantity: ";
            std::cin >> q;

            data.quantity = q;
            data.side = (Side)side;
            data.type = OrderType::MARKET_ORDER;

            const std::int64_t result = aeronPublication.publication->offer(srcBuffer, 0, msgLength);

            if (result > 0)
            {
                std::cout << "Sent\n";
            }
            else if (BACK_PRESSURED == result)
            {
                std::cout << "\nOffer failed due to back pressure\n";
            }
            else if (NOT_CONNECTED == result)
            {
                std::cout << "\nOffer failed because publisher is not connected to a subscriber" << std::endl;
            }
            else if (ADMIN_ACTION == result)
            {
                std::cout << "\nOffer failed because of an administration action in the system" << std::endl;
            }
            else if (PUBLICATION_CLOSED == result)
            {
                std::cout << "\nOffer failed because publication is closed" << std::endl;
            }
            else
            {
                std::cout << "\nOffer failed due to unknown reason " << result << std::endl;
            }

            if (!aeronPublication.publication->isConnected())
            {
                std::cout << "No active subscribers detected" << std::endl;
            }
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