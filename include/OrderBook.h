#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <list>
#include <map>
#include <vector>

#include "Order.h"

extern int matches;
int getMatches();

class OrderBook {
   private:
    std::map<double, std::list<Order>, std::greater<double>> bids;  // Price -> Orders (highest first)
    std::map<double, std::list<Order>, std::less<double>> asks;     // Price -> Orders (lowest first)
    std::vector<Order> orderHistory;                                // To keep track of all orders
    void executeTrade(Order& bid, Order& ask, uint32_t fill_qty);
    void match_market_order(Order& order);

   public:
    OrderBook() = default;

    void add_order(Order& order);
    void cancel_order(Order& order);
    void match_orders();

    const std::map<double, std::list<Order>, std::greater<double>>& getBids() const { return bids; }
    const std::map<double, std::list<Order>, std::less<double>>& getAsks() const { return asks; }
    const std::vector<Order>& getOrderHistory() const { return orderHistory; }
};

#endif