#include "OrderBook.h"

#include <algorithm>
#include <format>
#include <cassert>
#include "Logger.h"

int matches = 0;

int getMatches() {
    return matches;
}

void OrderBook::executeTrade(Order& bid, Order& ask, uint32_t fill_qty) {
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - bid.getTimestamp()).count();
    Logger& logger = Logger::getInstance();
    // Log the trade execution
    if (ask.getPrice().has_value()) {
        std::string trade_info = std::format("Trade executed: {} units at price {:.2f} {:} (Latency: {}µs)",
                                     fill_qty, ask.getPrice().value(), 4, latency);
        logger.info(trade_info);
    }

    bid.setSize(bid.getSize() - fill_qty);
    ask.setSize(ask.getSize() - fill_qty);
}

void OrderBook::match_market_order(Order& order) {
    
    if (order.getSide() == BUY) {
        bool asksEmpty = asks.empty() ? true : false;
        if(asksEmpty)
            return;
            
        // Iterator for asks (loop through price levels with this)
        auto askIter = asks.begin();
        while (order.getSize() > 0 && !askIter->second.empty()) {
            // Loop through orders at given price orders using this iterator
            auto& asksAtPrice = askIter->second;
            auto& ask = asksAtPrice.front();
            // printf("Matching order! [LIMIT ORDER] %d x %d [MARKET]\n", ask.getSize(), order.getSize());
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
        // bool emptyBids = bids.empty() ? true : false;
        
        if (bids.empty())
            return;
        auto bidIter = bids.rbegin();
        while (order.getSize() > 0 && !bidIter->second.empty()) {
            // Loop through orders at given price orders using this iterator
            auto& bidsAtPrice = bidIter->second;
            auto& bid = bidsAtPrice.front();
            // printf("Matching order! [LIMIT ORDER] %d x %d [MARKET]\n", bid.getSize(), order.getSize());
            uint32_t fill_qty = std::min(bid.getSize(), order.getSize());

            uint32_t rem_bids = bid.getSize() - fill_qty;
            executeTrade(order, bid, fill_qty);
            matches++;
            if (rem_bids == 0) {
                bidsAtPrice.pop_front();
            }

            if(bidsAtPrice.empty()) {
                // Removes the iterator for the bids at this given price
                // Use the base() method to convert reverse_iterator to regular iterator
                // This allows us to safely erase the element
                bids.erase(std::next(bidIter).base());
            }
        }
    }
}

/**
 * Adds an order to the order book.
 * Market orders are matched immediately.
 * @param order Order to be added to the order book
 */
void OrderBook::add_order(Order& order) {
    if (order.getType() == MARKET) {
        // Handle market orders immediately
        match_market_order(order);
        return;
    }
    // assert that order has a price (limit orders need a value)
    if (order.getSide() == BUY) {
        // Store the location of the order in the bids map
        bids[order.getPrice().value()].push_back(order);
        // bidsDeque[order.getPrice().value()].push_back(order);
        _order_locations[order.getId()] = std::prev(bids[order.getPrice().value()].end());
            
    } else if (order.getSide() == SELL) {
        asks[order.getPrice().value()].push_back(order);
        // asksDeque[order.getPrice().value()].push_back(order);
        _order_locations[order.getId()] = std::prev(asks[order.getPrice().value()].end());
    }
}

bool OrderBook::cancel_order(Order& order) {
    // TODO: Complete this method (orders need to be removed from the order book)
    auto it = _order_locations.find(order.getId());
    if (it == _order_locations.end()) {
        // Order not found
        // std::cout << "Order with ID " << order.getId() << " not found in order book." << std::endl;
        return false;
    }
    // std::cout << it->first << " => "  << getSideName(it->second->getSide()) << " " << it->second->getPrice().value() << std::endl;

    auto side = it->second->getSide();
    auto price = it->second->getPrice().value();

     if (side == BUY) {
        bids[price].erase(it->second); // Remove the order from the list at this price
        if (bids[price].empty()) {
            bids.erase(price); // Remove the price level if no orders left
        }
    }
    else if (side == SELL) {
        asks[price].erase(it->second); // Remove the order from the list at this price;
        if (asks[price].empty()) {
            asks.erase(price);
        }
    }
    _order_locations.erase(it);
    // std::cout << "Order with ID " << order.getId() << " cancelled successfully." << std::endl;
    return true;
}

void OrderBook::match_orders() {
    
    while (!bids.empty() && !asks.empty()) {
        auto bidIter = bids.begin();
        auto askIter = asks.begin();
        
        // If the bid or ask is empty, remove it from the book
        // Safely handle iterator invalidation
        if (bidIter->second.empty()) {
            bidIter = bids.erase(bidIter);
            continue;
        }
        if (askIter->second.empty()) {
            askIter = asks.erase(askIter);
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

                if (rem_bids == 0) {
                    bidIter->second.pop_front();
                    // Remove the order from the order locations map
                    _order_locations.erase(bidOrder.getId());
                }
                if (rem_asks == 0)
                    askIter->second.pop_front();
                    // Remove the order from the order locations map
                    _order_locations.erase(askOrder.getId());

                // Safely handle iterator invalidation
                if (bidIter->second.empty())
                    bidIter = bids.erase(bidIter);
                if (askIter->second.empty())
                    askIter = asks.erase(askIter);

                matches++;
            }
        } else {
            break;
        }
    }
}
