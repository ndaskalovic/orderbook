#include <cstdint>
#include <iostream>
#include "Aeron.h"

using namespace aeron::util;
using namespace aeron;

void subscriptionHandler(const std::string &channel, std::int32_t streamId, std::int64_t correlationId)
{
    std::cout << "Subscription: " << channel << " " << correlationId << ":" << streamId << std::endl;
}

void publicationHandler(const std::string &channel, std::int32_t streamId, std::int32_t sessionId, std::int64_t correlationId)
{
    std::cout << "Publication: " << channel << " " << correlationId << ":" << streamId << ":" << sessionId << std::endl;
}

void availableImageHandler(Image &image)
{
    std::cout << "Available image correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
    std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
}

void unavailableImageHandler(Image &image)
{
    std::cout << "Unavailable image on correlationId=" << image.correlationId() << " sessionId=" << image.sessionId();
    std::cout << " at position=" << image.position() << " from " << image.sourceIdentity() << std::endl;
}

struct Settings
{
    std::string channel;
    std::int32_t streamId;
    const char *databasePath;
    std::int32_t lingerTimeoutMs;
};

struct AeronSubscription
{
    std::shared_ptr<Aeron> aeron;
    std::int64_t id;
    std::shared_ptr<Subscription> subscription;
};

struct AeronPublication
{
    std::shared_ptr<Aeron> aeron;
    std::int64_t id;
    std::shared_ptr<Publication> publication;
};

AeronSubscription connectToAeronSubscription(Settings settings)
{
    aeron::Context context;
    context.newSubscriptionHandler(subscriptionHandler);
    context.availableImageHandler(availableImageHandler);
    context.unavailableImageHandler(unavailableImageHandler);
    std::shared_ptr<Aeron> aeron = Aeron::connect(context);

    std::int64_t id = aeron->addSubscription(settings.channel, settings.streamId);

    std::shared_ptr<Subscription> subscription = aeron->findSubscription(id);
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
    AeronSubscription aeronConn(aeron, id, subscription);
    return aeronConn;
}

AeronPublication connectToAeronPublication(Settings settings)
{
    aeron::Context context;
    context.newPublicationHandler(publicationHandler);
    std::shared_ptr<Aeron> aeron = Aeron::connect(context);

    std::int64_t id = aeron->addPublication(settings.channel, settings.streamId);

    std::shared_ptr<Publication> publication = aeron->findPublication(id);
    while (!publication)
    {
        std::this_thread::yield();
        publication = aeron->findPublication(id);
    }
    const std::int64_t channelStatus = publication->channelStatus();

    std::cout << "Publication channel status (id=" << publication->channelStatusId() << ") "
              << (channelStatus == ChannelEndpointStatus::CHANNEL_ENDPOINT_ACTIVE ? "ACTIVE" : std::to_string(channelStatus))
              << std::endl;
    AeronPublication aeronConn(aeron, id, publication);
    return aeronConn;
}