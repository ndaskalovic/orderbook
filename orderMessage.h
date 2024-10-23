#pragma once

#include <cstdint>

struct OrderMessage
{
    std::uint16_t type;
    std::uint16_t side;
    std::uint64_t quantity;
    std::uint32_t price;
};