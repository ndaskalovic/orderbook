#include <iostream>
#include <memory>
#include <map>
#include <list>
#include <unordered_map>

using OrderId = std::uint64_t;
using Quantity = std::uint32_t;
using Price = std::int32_t;

enum class Side
{
    BUY,
    SELL
};

struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity;
};

class Trade
{
public:
    Trade(const TradeInfo& bid, const TradeInfo& ask):
        bidTrade_{ bid },
        askTrade_{ ask }
    {  }
    const TradeInfo &GetBid() const { return bidTrade_; }
    const TradeInfo &GetAsk() const { return askTrade_; }
private:
    TradeInfo bidTrade_;
    TradeInfo askTrade_;
};

enum class OrderType {
    MARKET_ORDER,
    LIMIT_ORDER,
    FILL_AND_KILL
};


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
