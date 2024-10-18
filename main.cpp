#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <unordered_map>
#include "usings.h"
#include "trade.h"
#include "tradeInfo.h"
#include "orderType.h"
#include "side.h"
#include "order.h"
#include "orderBook.h"


int main() {
    Order order1(OrderType::MARKET_ORDER, 1, Side::BUY, 100, 200);
    Order order2(OrderType::MARKET_ORDER, 2, Side::BUY, 50, 201);
    OrderPointer op1 = std::make_shared<Order>(order1);
    OrderPointer op2 = std::make_shared<Order>(order2);

    OrderBook orderBook;

    bool added = orderBook.AddOrder(op1);
    added = orderBook.AddOrder(op2);
    orderBook.PrintBids();
    return 1;
};
