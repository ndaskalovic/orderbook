#include <cstdint>
#include <cstdio>
#include <thread>
#include <array>
#include <csignal>
#include <cinttypes>
#include "config.h"
#include "Aeron.h"
#include "orderMessage.h"
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
        long nOrders;

        while (running)
        {
            std::cout << "\n\nEnter how many order to submit: ";
            std::cin >> nOrders;
            for (std::int64_t i = 0; i < nOrders && running; i++)
            {
                data.price = 100;
                data.quantity = 1;
                data.side = (Side)(i % 2);
                data.type = OrderType::MARKET_ORDER;

                const std::int64_t result = aeronPublication.publication->offer(srcBuffer, 0, msgLength);

                if (result > 0)
                {
                    std::cout << "\rSent " << i + 1 << "/" << nOrders << std::flush;
                }
                else if (BACK_PRESSURED == result)
                {
                    std::cout << "\nOffer failed due to back pressure " << i + 1 << "/" << nOrders << std::endl;
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
                // if (settings.lingerTimeoutMs > 0)
                // {
                //     std::cout << "Lingering for " << settings.lingerTimeoutMs << " milliseconds." << std::endl;
                //     std::this_thread::sleep_for(std::chrono::milliseconds(settings.lingerTimeoutMs));
                // }
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