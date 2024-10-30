#pragma once

#include <cstdint>
#include "usings.h"
#include "side.h"
#include "orderType.h"

struct OrderMessage
{
    OrderType type;
    Side side;
    Quantity quantity;
    Price price;
};