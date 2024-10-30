#include "orderBook.h"
#include <cstdint>
#include <gtest/gtest.h>

static int oid = 0;

class OrderBookTest : public testing::Test
{
protected:
    // You can remove any or all of the following functions if their bodies would
    // be empty.

    OrderBookTest()
    {
        // You can do set-up work for each test here.
        
    }

    ~OrderBookTest() override
    {
        // You can do clean-up work that doesn't throw exceptions here.
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right
        // before the destructor).
        // ob.PrintBook();
    }

    // Class members declared here can be used by all tests in the test suite
    // for Foo.
    OrderBook ob;
};

OrderPointer makeOrderPointer(OrderType type, Side side, Quantity q, Price price)
{
    Order order(type, oid, side, q, price);
    oid++;
    return std::make_shared<Order>(order);
}

TEST_F(OrderBookTest, OrderBookSize)
{
    std::uint64_t norders(10);
    EXPECT_EQ(ob.GetSize(), 0);
    for (std::uint64_t i = 0; i < norders; i++)
    {
        OrderPointer buyp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, 10, 1);
        ob.AddOrder(buyp);
        EXPECT_EQ(ob.GetSize(), i+1);
    }
    for (std::uint64_t i = 0; i < norders; i++)
    {
        OrderPointer sellp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, 10, 1);
        ob.AddOrder(sellp);
        EXPECT_EQ(ob.GetSize(), norders-(i+1));
    }

}
TEST_F(OrderBookTest, OrderBookMatchMarketBuyLimitSell)
{
    OrderPointer marketbuyp = makeOrderPointer(OrderType::MARKET_ORDER, Side::BUY, 10, 1);
    OrderPointer limitsellp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::SELL, 100, 120);
    ob.AddOrder(marketbuyp);
    ob.AddOrder(limitsellp);
    EXPECT_EQ(marketbuyp->GetRemainingQuantity(), 0);
    EXPECT_EQ(limitsellp->GetRemainingQuantity(), 90);
}
TEST_F(OrderBookTest, OrderBookMatchMarketSellLimitBuy)
{
    OrderPointer marketsellp = makeOrderPointer(OrderType::MARKET_ORDER, Side::SELL, 10, 1);
    OrderPointer limitbuyp = makeOrderPointer(OrderType::LIMIT_ORDER, Side::BUY, 100, 120);
    ob.AddOrder(marketsellp);
    ob.AddOrder(limitbuyp);
    EXPECT_EQ(marketsellp->GetRemainingQuantity(), 0);
    EXPECT_EQ(limitbuyp->GetRemainingQuantity(), 90);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
