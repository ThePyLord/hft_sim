#include <cassert>

#include "OrderBook.h"

void test_matching() {
    OrderBook book;

    // Simple cross (should match)
    Order buy{Order::createLimitOrder(BUY, 152.0, 100)};
    book.add_order(buy);
    Order sell{Order::createLimitOrder(SELL, 150.0, 100)};
    book.add_order(sell);
    book.match_orders();
    assert(book.getBids().empty() && book.getAsks().empty());

    // Price priority test
    Order bid{Order::createLimitOrder(BUY, 151.0, 100)};
    Order ask{Order::createLimitOrder(SELL, 151.5, 100)};
    
    book.add_order(bid);

    bid = Order::createLimitOrder(BUY, 152.0, 100);
    book.add_order(bid);  // Should execute first

    book.add_order(ask);
    book.match_orders();
    assert(book.getBids().size() == 1);
    assert(book.getBids().begin()->first == 151.0);
}

int main(int argc, char const *argv[]) {
    test_matching();
    return 0;
}
