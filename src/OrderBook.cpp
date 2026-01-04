#include "OrderBook.h"

#include <algorithm>
#include <format>
#include <cassert>
// #include "Logger.h"

int matches = 0;

int getMatches() {
    return matches;
}

/**
 * Adds an order to the order book.
 * Market orders are matched immediately.
 * @param order Order to be added to the order book
 */
void OrderBook::add_order(Order& order) {
    if (order.getType() == MARKET) {
      // Handle market orders immediately
      
        logger.debug(std::format("Adding market order {} to order book", getSideName(order.getSide())));
        logger.debug(std::format("Order price: {}", order.getPrice().value()));
        match_market_order(order);
        return;
    }
    // assert that order has a price (limit orders need a value)
    // assert(order.getPrice().has_value() && "Limit orders must have a price");
    if (order.getSide() == BUY) {
        // Store the location of the order in the bids map
        bids[order.getPrice().value()].push_back(order);
        // bidsDeque[order.getPrice().value()].push_back(order);
        if (!bids[order.getPrice().value()].empty()) {
            _order_locations[order.getId()] = std::prev(bids[order.getPrice().value()].end());
            // std::string string = std::format("Added BUY order ID {} at price {:.2f} size {}", order.getId(), order.getPrice().value(), order.getSize());
            // logger.debug(string);
        }
    } else if (order.getSide() == SELL) {
        asks[order.getPrice().value()].push_back(order);
        // asksDeque[order.getPrice().value()].push_back(order);
        if (!asks[order.getPrice().value()].empty()) {
            _order_locations[order.getId()] = std::prev(asks[order.getPrice().value()].end());
            // std::string string = std::format("Added SELL order ID {} at price {:.2f} size {}", order.getId(), order.getPrice().value(), order.getSize());
            // logger.debug(string);
        }
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

void OrderBook::executeTrade(Order& bid, Order& ask, uint32_t fill_qty) {
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - bid.getTimestamp()).count();
    // Log the trade execution
    if (ask.getPrice().has_value()) {
        std::string trade_info = std::format(" (ASK) Trade executed: {} units at price {:.2f} (Latency: {}µs)",
                                     fill_qty, ask.getPrice().value(), latency);
        logger.info(trade_info);
    } else if (bid.getPrice().has_value()) {
      std::string trade_info = std::format(" (BID) Trade executed: {} units at price {:.2f} (Latency: {}µs)",
                                     fill_qty, bid.getPrice().value(), latency);
        logger.info(trade_info);
    }

    bid.setSize(bid.getSize() - fill_qty);
    ask.setSize(ask.getSize() - fill_qty);
}


void OrderBook::match_market_order(Order& order) {
    
    if (bids.empty() || asks.empty()) {
        logger.debug("Other side of book is empty, cannot match market order");
        return;
    }
    if (order.getSide() == BUY) {
        // Iterator for asks (loop through price levels with this)
        auto askIter = asks.begin();
        while (order.getSize() > 0 && askIter != asks.end()) {
            // Loop through orders at given price orders using this iterator
            auto& asksAtPrice = askIter->second;
            if (asksAtPrice.empty()) {
                // Move to the next price level if no orders at this price
                logger.debug("No asks at this price level for market buy order");
                ++askIter;
                continue;
            }
            auto& ask = asksAtPrice.front();
            
            uint32_t fill_qty = std::min(ask.getSize(), order.getSize());

            uint32_t rem_asks = ask.getSize() - fill_qty;
            executeTrade(order, ask, fill_qty);
            matches++;
            logger.debug("Matched orders in order book");
            logger.debug("Checking for empty orders after match");
            if (rem_asks == 0) {
                asksAtPrice.pop_front();
            }
            
            if(asksAtPrice.empty()) {
                // Removes the iterator for the asks at this given price
                askIter = asks.erase(askIter);
            }
        }
    } else {
        auto bidIter = bids.rbegin();
        while (order.getSize() > 0 && bidIter != bids.rend()) {
            // Loop through orders at given price orders using this iterator
            auto& bidsAtPrice = bidIter->second;
            if (bidsAtPrice.empty()) {
              // Move to the next price level if no orders at this price
                logger.debug("No bids at this price level for market sell order");
                ++bidIter;
                continue;
            }
            auto& bid = bidsAtPrice.front();
            uint32_t fill_qty = std::min(bid.getSize(), order.getSize());

            uint32_t rem_bids = bid.getSize() - fill_qty;
            executeTrade(bid, order, fill_qty);
            matches++;
            logger.debug("Matched orders in order book");
            logger.debug("Checking for empty orders after match");
            // Size exhausted for this bid order (fully filled)
            if (rem_bids == 0) {
                logger.debug("Removing bid order after market sell");
                bidsAtPrice.pop_front();
            }

            if(bidsAtPrice.empty()) {
                // Removes the iterator for the bids at this given price
                // Use the base() method to convert reverse_iterator to regular iterator
                // This allows us to safely erase the element
                logger.debug("Removing bid price level after market sell");
                bidIter = std::reverse_iterator(bids.erase(std::next(bidIter).base()));
            }
        }
    }
}

void OrderBook::match_orders() {
    
    auto bidIter = bids.begin();
    auto askIter = asks.begin();
    logger.debug("Matching orders in order book...");
    while (!bids.empty() && !asks.empty()) {
        // If the bid or ask is empty, remove it from the book
        // Safely handle iterator invalidation
        if (bidIter != bids.end()) {
            bidIter = bids.erase(bidIter);
            logger.debug("Removing empty bid price level in matching orders");
            continue;
        }
        if (askIter != asks.end()) {
            logger.debug("Removing empty ask price level in matching orders");
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
                logger.debug("Matched orders in order book");
                logger.debug("Checking for empty orders after match");
                if (rem_bids == 0) {
                    bidIter->second.pop_front();
                    // Remove the order from the order locations map
                    logger.debug("Removing bid order after match");
                    _order_locations.erase(bidOrder.getId());
                }
                if (rem_asks == 0) {
                    askIter->second.pop_front();
                    // Remove the order from the order locations map
                    logger.debug("Removing ask order after match");
                    _order_locations.erase(askOrder.getId());
                }

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
