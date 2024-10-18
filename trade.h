#pragma once

#include "tradeInfo.h"

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

using Trades = std::list<Trade>;