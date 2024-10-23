#include <string>
#include <cstdint>

namespace configuration
{

    const static std::string DEFAULT_CHANNEL = "aeron:udp?endpoint=localhost:20121";
    const static std::string DEFAULT_PING_CHANNEL = "aeron:udp?endpoint=localhost:20123";
    const static std::string DEFAULT_PONG_CHANNEL = "aeron:udp?endpoint=localhost:20124";
    const static std::int32_t DEFAULT_STREAM_ID = 1001;
    const static int DEFAULT_LINGER_TIMEOUT_MS = 0;
    const static int DEFAULT_FRAGMENT_COUNT_LIMIT = 10;
}
