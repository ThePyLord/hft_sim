#include "OrderBook.h"

#include <algorithm>
#include <cassert>

int matches = 0;

int getMatches() {
    return matches;
}

void OrderBook::executeTrade(Order& bid, Order& ask, uint32_t fill_qty) {
    bid.setSize(bid.getSize() - fill_qty);
    ask.setSize(ask.getSize() - fill_qty);
}

void OrderBook::match_market_order(Order& order) {
    // auto& opposite_book = (order.getSide() == BUY ? asks : bids);
    if (order.getSide() == BUY) {
        std::string asksEmpty = asks.empty() ? "true" : "false";
        if(asksEmpty == "true")
            return;
            
        // Iterator for asks (loop through price levels with this)
        auto askIter = asks.begin();
        while (order.getSize() > 0 && !askIter->second.empty()) {
            // Loop through orders at given price orders using this iterator
            auto& asksAtPrice = askIter->second;
            auto& ask = asksAtPrice.front();
            printf("Matching order! [LIMIT ORDER] %d x %d [MARKET]\n", ask.getSize(), order.getSize());
            uint32_t fill_qty = std::min(ask.getSize(), order.getSize());

            uint32_t rem_asks = ask.getSize() - fill_qty;
            executeTrade(order, ask, fill_qty);
            matches++;
            if (rem_asks == 0) {
                asksAtPrice.pop_front();
            }
            
            if(asksAtPrice.empty()) {
                // Removes the iterator for the asks at this given price
                asks.erase(askIter);
            }
        }
    } else {
        std::string emptyBids = bids.empty() ? "true" : "false";
        
        if (emptyBids == "true")
            return;
        auto bidIter = bids.begin();
        while (order.getSize() > 0 && !bidIter->second.empty()) {
            // Loop through orders at given price orders using this iterator
            auto& bidsAtPrice = bidIter->second;
            auto& bid = bidsAtPrice.front();
            printf("Matching order! [LIMIT ORDER] %d x %d [MARKET]\n", bid.getSize(), order.getSize());
            uint32_t fill_qty = std::min(bid.getSize(), order.getSize());

            uint32_t rem_bids = bid.getSize() - fill_qty;
            executeTrade(order, bid, fill_qty);
            matches++;
            if (rem_bids == 0) {
                bidsAtPrice.pop_front();
            }

            if(bidsAtPrice.empty()) {
                // Removes the iterator for the bids at this given price
                bids.erase(bidIter);
            }
        }
    }
}

void OrderBook::add_order(Order& order) {
    if (order.getType() == Type::MARKET) {
        match_market_order(order);
        return;
    }

    if (order.getSide() == BUY) {
        this->bids[order.getPrice()].push_back(order);
    } else if (order.getSide() == SELL) {
        this->asks[order.getPrice()].push_back(order);
    }
    this->orderHistory.push_back(order);
}

void OrderBook::cancel_order(Order& order) {
    // TODO: implement this method
    // auto orders = order.getSide() == BUY ? bids : asks;

    // for (auto it = orders.begin(); it != orders.end(); ++it) {
    // }
}

void OrderBook::match_orders() {
    // int matches = 0;
    printf("Preparing to match: %ld bids and %ld asks\n", bids.size(), asks.size());
    while (!bids.empty() && !asks.empty()) {
        auto bidIter = bids.begin();
        auto askIter = asks.begin();
        if (bidIter->second.empty()) {
            bids.erase(bidIter);
            continue;
        }
        if (askIter->second.empty()) {
            asks.erase(askIter);
            continue;
        }

        auto bidPrice = bidIter->first;
        auto askPrice = askIter->first;

        if (bidPrice >= askPrice) {
            // Handled "partial" fills
            if (bidIter->second.size() > 0 && askIter->second.size() > 0) {
                Order& bidOrder = bidIter->second.front();
                Order& askOrder = askIter->second.front();
                uint32_t trade_quantity = std::min(bidOrder.getSize(), askOrder.getSize());

                uint32_t rem_bids = bidOrder.getSize() - trade_quantity;
                uint32_t rem_asks = askOrder.getSize() - trade_quantity;

                executeTrade(bidOrder, askOrder, trade_quantity);

                if (rem_bids == 0)
                    bidIter->second.pop_front();
                if (rem_asks == 0)
                    askIter->second.pop_front();

                if (bidIter->second.empty())
                    bids.erase(bidIter);
                if (askIter->second.empty())
                    asks.erase(askIter);

                matches++;
            }
        } else {
            break;
        }
    }
}
