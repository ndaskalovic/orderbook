#pragma once

#include <list>
#include <map>
#include <unordered_map>
#include <iostream>
#include "usings.h"
#include "side.h"
#include "orderType.h"

class OrderBook
{
public:
    bool AddOrder(OrderPointer order){
        if (orders_.contains(order->GetOrderId()))
            return false;

        if (order->GetOrderType() == OrderType::FILL_AND_KILL && !CanFill(order))
            return false;
        
        if (order->GetOrderSide() == Side::BUY)
        {
            auto& orders = bids_[order->GetOrderPrice()];
            orders.push_back(order);
        }
        else if (order->GetOrderSide() == Side::SELL)
        {
            auto& orders = asks_[order->GetOrderPrice()];
            orders.push_back(order);

        }
        orders_.insert({order->GetOrderId(), order});
        return true;
    };
    void PrintBids(){
        for (const auto& pair : bids_) {
            std::cout << "Price: " << pair.first << std::endl;
            for (const auto& order : pair.second) {
                std::cout << "    Order ID: " << order->GetOrderId() 
                        << ", Quantity: " << order->GetOrderQuantity() 
                        << std::endl;
            }
        }
    };
    void PrintAsks(){
        for (const auto& pair : asks_) {
            std::cout << "Price: " << pair.first << std::endl;
            for (const auto& order : pair.second) {
                std::cout << "    Order ID: " << order->GetOrderId() 
                        << ", Quantity: " << order->GetOrderQuantity() 
                        << std::endl;
            }
        }
    };
    bool CanFill(OrderPointer order){ return true; }
private:
    std::unordered_map<OrderId, OrderPointer> orders_;
    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
};