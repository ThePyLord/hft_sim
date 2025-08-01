#include <gtest/gtest.h>

#include "OrderBook.h"

TEST(CancelOrderTest, CancelValidOrder) {
    OrderBook book;
    // Add test code
    Order order = Order::createLimitOrder(BUY, 100.0, 50);
    Order order2 = Order::createLimitOrder(BUY, 100.0, 25);
    book.add_order(order);
    book.add_order(order2);
    ASSERT_TRUE(book.cancel_order(order)) << "Failed to cancel a valid order";
    // Check that the order is no longer in the order book
    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 1) << "Order book should have 1 bid after cancellation";
}

TEST(CancelOrderTest, CancelValidSellOrder) {
    OrderBook book;
    Order ask1 = Order::createLimitOrder(SELL, 105.0, 30);
    Order ask2 = Order::createLimitOrder(SELL, 106.0, 40);
    book.add_order(ask1);
    book.add_order(ask2);

    ASSERT_TRUE(book.cancel_order(ask1)) << "Failed to cancel a valid sell order";
    auto asks = book.getAsks();
    ASSERT_EQ(asks.size(), 1) << "Order book should have 1 ask after cancellation";
    ASSERT_EQ(asks.begin()->first, 106.0) << "Remaining ask should be at price 106.0";
}

TEST(CancelOrderTest, CancelInvalidOrder) {
    OrderBook book;
    Order order = Order::createLimitOrder(BUY, 100.0, 50);
    Order notInBook = Order::createLimitOrder(BUY, 101.0, 25);

    book.add_order(order);
    ASSERT_FALSE(book.cancel_order(notInBook)) << "Should not be able to cancel order not in book";

    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 1) << "Order book should still have 1 bid";
}

TEST(CancelOrderTest, CancelFromEmptyBook) {
    OrderBook book;
    Order order = Order::createLimitOrder(BUY, 100.0, 50);

    ASSERT_FALSE(book.cancel_order(order)) << "Should not be able to cancel from empty book";
    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 0) << "Order book should remain empty";
}

TEST(CancelOrderTest, CancelLastOrderAtPriceLevel) {
    OrderBook book;
    Order order = Order::createLimitOrder(BUY, 100.0, 50);
    book.add_order(order);

    ASSERT_TRUE(book.cancel_order(order)) << "Failed to cancel last order at price level";
    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 0) << "Price level should be removed when last order cancelled";
}

TEST(CancelOrderTest, CancelFirstOrderInQueue) {
    OrderBook book;
    Order order1 = Order::createLimitOrder(BUY, 100.0, 50);
    Order order2 = Order::createLimitOrder(BUY, 100.0, 25);
    Order order3 = Order::createLimitOrder(BUY, 100.0, 75);

    book.add_order(order1);
    book.add_order(order2);
    book.add_order(order3);

    ASSERT_TRUE(book.cancel_order(order1)) << "Failed to cancel first order in queue";
    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 1) << "Should still have the price level";
    ASSERT_EQ(bids[100.0].size(), 2) << "Should have 2 orders remaining at price level";
    ASSERT_EQ(bids[100.0].front().getSize(), 25) << "Second order should now be first";
}

TEST(CancelOrderTest, CancelMiddleOrderInQueue) {
    OrderBook book;
    Order order1 = Order::createLimitOrder(BUY, 100.0, 50);
    Order order2 = Order::createLimitOrder(BUY, 100.0, 25);
    Order order3 = Order::createLimitOrder(BUY, 100.0, 75);

    book.add_order(order1);
    book.add_order(order2);
    book.add_order(order3);

    ASSERT_TRUE(book.cancel_order(order2)) << "Failed to cancel middle order in queue";
    auto bids = book.getBids();
    ASSERT_EQ(bids[100.0].size(), 2) << "Should have 2 orders remaining";

    // Verify order sequence is maintained
    auto it = bids[100.0].begin();
    ASSERT_EQ(it->getSize(), 50) << "First order should still be order1";
    ++it;
    ASSERT_EQ(it->getSize(), 75) << "Second order should be order3";
}

TEST(CancelOrderTest, CancelMultipleOrders) {
    OrderBook book;
    Order bid1 = Order::createLimitOrder(BUY, 100.0, 50);
    Order bid2 = Order::createLimitOrder(BUY, 99.0, 25);
    Order ask1 = Order::createLimitOrder(SELL, 101.0, 30);
    Order ask2 = Order::createLimitOrder(SELL, 102.0, 40);

    book.add_order(bid1);
    book.add_order(bid2);
    book.add_order(ask1);
    book.add_order(ask2);

    ASSERT_TRUE(book.cancel_order(bid1)) << "Failed to cancel first bid";
    ASSERT_TRUE(book.cancel_order(ask2)) << "Failed to cancel second ask";

    auto bids = book.getBids();
    auto asks = book.getAsks();
    ASSERT_EQ(bids.size(), 1) << "Should have 1 bid remaining";
    ASSERT_EQ(asks.size(), 1) << "Should have 1 ask remaining";
    ASSERT_EQ(bids.begin()->first, 99.0) << "Remaining bid should be at 99.0";
    ASSERT_EQ(asks.begin()->first, 101.0) << "Remaining ask should be at 101.0";
}

TEST(CancelOrderTest, CancelAlreadyCancelledOrder) {
    OrderBook book;
    Order order = Order::createLimitOrder(BUY, 100.0, 50);
    book.add_order(order);

    ASSERT_TRUE(book.cancel_order(order)) << "First cancellation should succeed";
    ASSERT_FALSE(book.cancel_order(order)) << "Second cancellation should fail";

    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 0) << "Order book should be empty";
}

TEST(CancelOrderTest, CancelOrdersAtDifferentPriceLevels) {
    OrderBook book;
    Order bid1 = Order::createLimitOrder(BUY, 100.0, 50);
    Order bid2 = Order::createLimitOrder(BUY, 99.0, 25);
    Order bid3 = Order::createLimitOrder(BUY, 98.0, 75);

    book.add_order(bid1);
    book.add_order(bid2);
    book.add_order(bid3);

    ASSERT_TRUE(book.cancel_order(bid2)) << "Failed to cancel middle price level order";

    auto bids = book.getBids();
    ASSERT_EQ(bids.size(), 2) << "Should have 2 price levels remaining";
    ASSERT_EQ(bids.count(99.0), 0) << "Price level 99.0 should be removed";
    ASSERT_EQ(bids.count(100.0), 1) << "Price level 100.0 should remain";
    ASSERT_EQ(bids.count(98.0), 1) << "Price level 98.0 should remain";
}