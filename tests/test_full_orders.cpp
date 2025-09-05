#include <gtest/gtest.h>
#include "OrderBook.h"

TEST(OrderBookTest, MatchFullOrders) {
    OrderBook ob;

    // Simple cross (should match)
    Order buy{Order::createLimitOrder(BUY, 152.0, 100)};
    ob.add_order(buy);
    Order sell{Order::createLimitOrder(SELL, 150.0, 100)};
    ob.add_order(sell);
    ob.match_orders();
    ASSERT_TRUE(ob.getBids().empty());
    ASSERT_TRUE(ob.getAsks().empty());

    // Price priority test
    Order bid{Order::createLimitOrder(BUY, 151.0, 100)};
    Order ask{Order::createLimitOrder(SELL, 151.5, 100)};

    ob.add_order(bid);

    bid = Order::createLimitOrder(BUY, 152.0, 100);
    ob.add_order(bid);  // Should execute first

    ob.add_order(ask);
    ob.match_orders();
    EXPECT_TRUE(ob.getBids().size() == 1);
    EXPECT_TRUE(ob.getBids().begin()->first == 151.0);
}
