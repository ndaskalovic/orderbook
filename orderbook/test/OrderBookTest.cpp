#include "orderBook.h"
#include <cstdint>
#include <gtest/gtest.h>

static int oid = 0;

OrderPointer makeOrderPointer(OrderType type, Side side, Quantity q, Price price)
{
    Order order(type, oid, side, q, price);
    oid++;
    return std::make_shared<Order>(order);
}

class OrderBookTest : public testing::Test
{
protected:
    OrderBookTest()
    {
        for (int i = 0; i < 10; i++)
        {
            OrderPointer bp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::BUY, 10, 100 - (i + 1));
            filledOb.AddOrder(bp);
            OrderPointer sp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::SELL, 10, 100 + i + 1);
            filledOb.AddOrder(sp);
        }
        }

    ~OrderBookTest() override
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
        // emptyOb.PrintBook();
    }

    OrderBook emptyOb;
    OrderBook filledOb;
};


TEST_F(OrderBookTest, Size)
{
    std::uint64_t norders(10);
    EXPECT_EQ(emptyOb.GetSize(), 0);
    for (std::uint64_t i = 0; i < norders; i++)
    {
        OrderPointer buyp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, 10, 1);
        emptyOb.AddOrder(buyp);
        EXPECT_EQ(emptyOb.GetSize(), i + 1);
    }
    for (std::uint64_t i = 0; i < norders; i++)
    {
        OrderPointer sellp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, 10, 1);
        emptyOb.AddOrder(sellp);
        EXPECT_EQ(emptyOb.GetSize(), norders - (i + 1));
    }

}
TEST_F(OrderBookTest, MatchMarketBuyLimitSell)
{
    OrderPointer marketBuyp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, 10, 1);
    OrderPointer limitSellp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::SELL, 100, 120);
    emptyOb.AddOrder(marketBuyp);
    emptyOb.AddOrder(limitSellp);
    EXPECT_TRUE(marketBuyp->IsFilled());
    EXPECT_EQ(limitSellp->GetFilledQuantity(), 10);
}
TEST_F(OrderBookTest, MatchMarketSellLimitBuy)
{
    OrderPointer marketSellp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, 10, 1);
    OrderPointer limitBuyp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::BUY, 100, 120);
    emptyOb.AddOrder(marketSellp);
    emptyOb.AddOrder(limitBuyp);
    EXPECT_TRUE(marketSellp->IsFilled());
    EXPECT_EQ(limitBuyp->GetFilledQuantity(), 10);
}
TEST_F(OrderBookTest, ClearAllSells)
{
    Quantity quantity = 100;
    OrderPointer clearSellsp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, quantity, 1);
    filledOb.AddOrder(clearSellsp);
    EXPECT_EQ(clearSellsp->GetFilledQuantity(), quantity);
    EXPECT_EQ(filledOb.GetSize(), 10);
}
TEST_F(OrderBookTest, ClearThroughSells)
{
    Quantity quantity = 200;
    OrderPointer clearSellsp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, quantity, 1);
    filledOb.AddOrder(clearSellsp);
    EXPECT_EQ(clearSellsp->GetFilledQuantity(), 100);
    EXPECT_EQ(filledOb.GetSize(), 11);
}
TEST_F(OrderBookTest, ClearThroughBuys)
{
    Quantity quantity = 200;
    OrderPointer clearBuysp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, quantity, 1);
    filledOb.AddOrder(clearBuysp);
    EXPECT_EQ(clearBuysp->GetFilledQuantity(), 100);
    EXPECT_EQ(filledOb.GetSize(), 11);
}
TEST_F(OrderBookTest, ClearAllBuys)
{
    Quantity quantity = 100;
    OrderPointer clearBuysp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, quantity, 1);
    filledOb.AddOrder(clearBuysp);
    EXPECT_EQ(clearBuysp->GetFilledQuantity(), quantity);
    EXPECT_EQ(filledOb.GetSize(), 10);
}
TEST_F(OrderBookTest, FillMarketBuyThroughLevels)
{
    Quantity quantity = 45;
    auto obSize = filledOb.GetSize();
    OrderPointer buyp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, quantity, 1);
    filledOb.AddOrder(buyp);
    EXPECT_TRUE(buyp->IsFilled());
    EXPECT_EQ(filledOb.GetSize(), obSize-4);
}
TEST_F(OrderBookTest, FillMarketSellThroughLevels)
{
    Quantity quantity = 45;
    auto obSize = filledOb.GetSize();
    OrderPointer sellp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, quantity, 1);
    filledOb.AddOrder(sellp);
    EXPECT_TRUE(sellp->IsFilled());
    EXPECT_EQ(filledOb.GetSize(), obSize-4);
}



int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
