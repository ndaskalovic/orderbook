#pragma once

#include <map>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "usings.h"
#include "order.h"

class OrderBook
{
private:
    std::unordered_map<OrderId, OrderPointer> orders_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::uint64_t size_;
    mutable std::mutex ordermutex_;

public:
    OrderBook() : size_(0) {}
    bool AddOrder(OrderPointer order);
    void PrintBids();
    void PrintAsks();
    void PrintBook();
    bool CanFill(OrderPointer order);
    void MatchOrders();
    Price GetCurrentPrice();
    std::uint64_t GetSize() const { return size_; }
};
