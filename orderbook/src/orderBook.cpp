#include <mutex>
#include <iostream>
#include <cmath>

#include "usings.h"
#include "orderType.h"
#include "order.h"
#include "side.h"
#include "orderBook.h"
#include "trade.h"
#include "tradeInfo.h"

void OrderBook::MatchOrders()
{
    Trades trades;
    while (true)
    {
        if (asks_.empty() || bids_.empty())
        {
            break;
        }

        auto &[bidPrice, bids] = *bids_.begin();
        auto &[askPrice, asks] = *asks_.begin();

        if (bidPrice < askPrice)
        {
            break;
        }
        while (!bids.empty() && !asks.empty())
        {
            auto bid = bids.front();
            auto ask = asks.front();
            Quantity quantity = std::min(bid->GetRemainingQuantity(), ask->GetRemainingQuantity());

            bid->Fill(quantity);
            ask->Fill(quantity);

            if (bid->IsFilled())
            {
                bids.pop_front();
                orders_.erase(bid->GetOrderId());
                size_ -= 1;
            }
            if (ask->IsFilled())
            {
                asks.pop_front();
                orders_.erase(ask->GetOrderId());
                size_ -= 1;
            }

            trades.push_back(Trade{
                TradeInfo{bid->GetOrderId(), bid->GetOrderPrice(), quantity},
                TradeInfo{ask->GetOrderId(), ask->GetOrderPrice(), quantity}});
        }

        if (bids.empty())
        {
            bids_.erase(bidPrice);
        }
        if (asks.empty())
        {
            asks_.erase(askPrice);
        }
        if (!asks_.empty() && !bids_.empty())
        {
            auto &[bidPrice, bids] = *bids_.begin();
            auto &[askPrice, asks] = *asks_.begin();
            auto bid = bids.front();
            auto ask = asks.front();
            // move market order to next price level if not filled completely
            if (bid->GetOrderType() == OrderType::MARKET_ORDER)
            {
                bids.pop_front();
                if (bids.empty())
                {
                    bids_.erase(bid->GetOrderPrice());
                }
                bid->UpdatePrice(askPrice);
                bids_[askPrice].push_front(bid);
            }
            else if (ask->GetOrderType() == OrderType::MARKET_ORDER)
            {
                asks.pop_front();
                if (asks.empty())
                {
                    asks_.erase(ask->GetOrderPrice());
                }
                ask->UpdatePrice(bidPrice);
                asks_[bidPrice].push_front(ask);
            }
        }
    }
}


bool OrderBook::AddOrder(OrderPointer order)
{
    std::scoped_lock bookLock{ordermutex_};
    if (orders_.contains(order->GetOrderId()))
        return false;

    if (order->GetOrderType() == OrderType::FILL_OR_KILL && !CanFill(order))
        return false;

    if (order->GetOrderType() == OrderType::MARKET_ORDER)
    {
        if (order->GetOrderSide() == Side::BUY && !asks_.empty())
        {
            const auto &[worstAsk, _] = *asks_.begin();
            order->UpdatePrice(worstAsk);
        }
        else if (order->GetOrderSide() == Side::BUY && asks_.empty())
        {
            // const auto &[topBid, _] = *bids_.begin();
            // order->UpdatePrice(topBid);
            // ignore when opposite side of book empty
            return false;
        }
        if (order->GetOrderSide() == Side::SELL && !bids_.empty())
        {
            const auto &[worstBid, _] = *bids_.begin();
            order->UpdatePrice(worstBid);
        }
        else if (order->GetOrderSide() == Side::SELL && bids_.empty())
        {
            // const auto &[topAsk, _] = *asks_.begin();
            // order->UpdatePrice(topAsk);
            // ignore when opposite side of book empty
            return false;
        }
    }

    if (order->GetOrderType() == OrderType::LIMIT_ORDER)
    {
        if (order->GetOrderSide() == Side::BUY && order->GetOrderPrice() >= asks_.begin()->first)
        {
            return false;
        }
        else if (order->GetOrderSide() == Side::SELL && order->GetOrderPrice() <= bids_.begin()->first)
        {
            return false;
        }
        }

    if (order->GetOrderSide() == Side::BUY)
    {
        auto &orders = bids_[order->GetOrderPrice()];
        orders.push_back(order);
    }
    else if (order->GetOrderSide() == Side::SELL)
    {
        auto &orders = asks_[order->GetOrderPrice()];
        orders.push_back(order);
    }
    orders_.insert({order->GetOrderId(), order});
    size_ += 1;
    MatchOrders();
    return true;
}


void OrderBook::PrintBids()
{
    for (const auto &pair : bids_)
    {
        std::cout << "Price: " << pair.first << std::endl;
        for (const auto &order : pair.second)
        {
            std::cout << "    Order ID: " << order->GetOrderId()
                      << ", Quantity: " << order->GetOrderQuantity()
                      << std::endl;
        }
    }
};
void OrderBook::PrintAsks()
{
    for (const auto &pair : asks_)
    {
        std::cout << "Price: " << pair.first << std::endl;
        for (const auto &order : pair.second)
        {
            std::cout << "    Order ID: " << order->GetOrderId()
                      << ", Quantity: " << order->GetOrderQuantity()
                      << std::endl;
        }
    }
};

void OrderBook::PrintBook()
{
    std::scoped_lock bookLock{ordermutex_};
    int c = 0;
    for (const auto &pair : asks_)
    {
        int q = 0;
        for (const auto &order : pair.second)
        {
            q += order->GetOrderQuantity() - order->GetFilledQuantity();
        }
        c++;
        if (c == 6)
            break;
        std::cout << "\033[1;31m" << pair.first << " | " << q << "\033[0m" << std::endl;
    }
    c = 0;
    std::cout << std::endl;
    for (const auto &pair : bids_)
    {
        int q = 0;
        for (const auto &order : pair.second)
        {
            q += order->GetOrderQuantity() - order->GetFilledQuantity();
        }
        c++;
        if (c == 6)
            break;
        std::cout << "\033[1;32m" << pair.first << " | " << q << "\033[0m" << std::endl;
    }
}

bool OrderBook::CanFill(OrderPointer order)
{
    if (order->GetOrderSide() == Side::BUY)
    {
        // TODO: add levelinfo to quickly compute vol at each level and evaluate this for fill or kill
        if (asks_.empty())
            return false;
        return true;
    }
    else if (order->GetOrderSide() == Side::SELL)
    {
        if (bids_.empty())
            return false;
        return true;
    }
    else
    {
        return false;
    }
}

Price OrderBook::GetCurrentPrice()
{
    std::scoped_lock bookLock{ordermutex_};
    static bool roundUp = false;
    if (!bids_.empty() && !asks_.empty())
    {
        auto &[bidPrice, b] = *bids_.begin();
        auto &[askPrice, a] = *asks_.begin();
        // int prices so need to randomly round up or down to balance mid price
        roundUp = !roundUp;
        float midPrice = ((float)askPrice + (float)bidPrice) / 2;
        return roundUp ? Price(std::ceil(midPrice)) : Price(std::floor(midPrice));
    }
    else if (!bids_.empty() && asks_.empty())
    {
        auto &[bidPrice, _] = *bids_.begin();
        return bidPrice;
    }
    else if (bids_.empty() && !asks_.empty())
    {
        auto &[askPrice, _] = *asks_.begin();
        return askPrice;
    } else
    {
        return Price(1);
    }
}