#pragma once

#include <list>
#include <memory>
#include <format>
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
    Quantity GetRemainingQuantity() const { return quantity_ - filledQuantity_; }
    Price GetOrderPrice() const { return price_; }
    bool IsFilled() const { return (quantity_ - filledQuantity_) == 0; }
    void Fill(Quantity quantity)
    {
        if (quantity > GetRemainingQuantity())
        {
            throw std::logic_error(std::format("Order ({}) cannot be filled more than its remaining quantity", GetOrderId()));
        }
        filledQuantity_ += quantity;
    };
    void ToGoodTillCancel(Price price)
    {
        if (GetOrderType() != OrderType::MARKET_ORDER)
        {
            throw std::logic_error(std::format("Order ({}, type: {}) cannot have its price adjusted, only market orders can", GetOrderId(), (int)GetOrderType()));
        }
        price_ = price;
    };

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