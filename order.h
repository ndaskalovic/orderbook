#pragma once

#include <list>
#include <memory>
#include "usings.h"
#include "side.h"
#include "orderType.h"

class Order
{
public:
    Order(OrderType orderType, OrderId orderId, Side side, Quantity quantity, Price price):
        orderType_{orderType},
        orderId_{orderId},
        side_{side},
        quantity_{quantity},
        price_{price},
        filledQuantity_{0}
    { }
    OrderType GetOrderType() const { return orderType_; }
    OrderId GetOrderId() const { return orderId_; }
    Side GetOrderSide() const { return side_; }
    Quantity GetOrderQuantity() const { return quantity_; }
    Quantity GetFilledQuantity() const { return filledQuantity_; }
    Price GetOrderPrice() const { return price_; }
private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity quantity_;
    Quantity filledQuantity_;
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;