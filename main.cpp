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

using namespace std;

int main() {
    OrderBook book;
    static int id = 0;

    for (int i = 1; i < 9; i++)
    {
        Order order1(OrderType::LIMIT_ORDER, id, Side::BUY, 10*i, 100+(i%4)-5);
        id++;
        OrderPointer op1 = std::make_shared<Order>(order1);
        bool added = book.AddOrder(op1);
        Order order2(OrderType::LIMIT_ORDER, id, Side::SELL, 10*i, 100+(i%5));
        id++;
        OrderPointer op2 = std::make_shared<Order>(order2);
        added = book.AddOrder(op2);
    }
    int type;
    int side;
    int quantity;
    int price;
    book.PrintBook();
    while (true)
    {
        cout << "Place an order:\n---------------\n";
        cout << "Order Type (market: 0, limit: 1):\n";
        cin >> type;
        cout << "Order Side (BUY: 0, SELL: 1):\n";
        cin >> side;
        if (type != 0)
        {
            cout << "Order Price:\n";
            cin >> price;
        }
        cout << "Order Quantity:\n";
        cin >> quantity;
        Order order((OrderType)type, id, (Side)side, (Quantity)quantity, (Price)price);
        cout << (int)(OrderType)type << endl;
        id++;
        OrderPointer op = std::make_shared<Order>(order);
        book.AddOrder(op);
        cout << "\n\n";
        book.PrintBook();
        cout << "\n\n";
    }

    // book.PrintBids();
    // book.PrintAsks();
    return 1;
};
