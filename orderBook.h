#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include "usings.h"
// #include "side.h"
// #include "orderType.h"
#include "order.h"

class OrderBook
{
private:
    std::unordered_map<OrderId, OrderPointer> orders_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
public:
    bool AddOrder(OrderPointer order);
    void PrintBids();
    void PrintAsks();
    void PrintBook();
    bool CanFill(OrderPointer order);
    void MatchOrders();
    Price GetCurrentPrice();
};
