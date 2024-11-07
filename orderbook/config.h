#include <string>
#include <cstdint>

namespace configuration
{

    const static std::string DEFAULT_CHANNEL = "aeron:udp?endpoint=localhost:20121";
    const static std::string DEFAULT_PRICE_DATA_CHANNEL = "aeron:udp?endpoint=localhost:20122";
    const static std::int32_t DEFAULT_STREAM_ID = 1001;
    const static std::int32_t DEFAULT_PRICE_DATA_STREAM_ID = 1002;
    const static int DEFAULT_LINGER_TIMEOUT_MS = 10;
    const static int DEFAULT_FRAGMENT_COUNT_LIMIT = 10;
    const static char * DATABASE_PATH = "../orderbook.db";
}
