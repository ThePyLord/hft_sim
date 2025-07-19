#include <cstdlib>
#include <iomanip>

#include "Order.h"

std::string getSideName(Side s) {
    switch (s) {
        case BUY:
            return "BUY";
        case SELL:
            return "SELL";
        default:
            return "UNKNOWN";
    }
}

std::string getTypeName(Type t) {
    switch (t) {
        case MARKET:
            return "MARKET";
        case LIMIT:
            return "LIMIT";
        case STOP:
            return "STOP";
        case STOP_LIMIT:
            return "STOP_LIMIT";
        default:
            return "UNKNOWN";
    }
}

Side Order::getSide() const { return this->side; }

double Order::getPrice() const { return this->price; }
Type Order::getType() const { return this->order_type; }
uint32_t Order::getSize() const { return this->order_size; }

void Order::setSize(uint32_t size) {
    if (size >= 0) {
        this->order_size = size;
    } else {
        printf("Invalid size %d\n", size);
    }
}

Order::Order(Side side, Type orderType, double price, uint32_t order_size) : 
oid(id++), 
side(side), 
order_type(orderType), 
order_size(order_size),
time_of_order(
    std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch())
    .count()
)
 {
    this->price = order_type == MARKET ? -1 : price;  // -1 indicates no price for market orders
    if (side != BUY && side != SELL) {
        std::cerr << "Bad side value: " << static_cast<int>(side) << std::endl;
        throw std::invalid_argument("Invalid order side");
    }
    
    // if (orderType == MARKET) {
    //     std::cerr << "BAD ORDER TYPE: " << getTypeName(orderType) << std::endl;
    //     throw std::invalid_argument("Invalid order type for MARKET order");
    // }
}

/// @brief Creates a limit order
/// @param side The buy/sell side of the order book
/// @param price The price of the limit order
/// @param order_size The order size
/// @return A limit order
Order Order::createLimitOrder(Side side, double price, uint32_t order_size) {
    return Order(side, Type::LIMIT, price, order_size);
}


/// @brief Creates a market order 
/// @param side The buy/sell of the order
/// @param order_size The amount to be traded
/// @return A market order
Order Order::createMarketOrder(Side side, uint32_t order_size) {
    return Order(side, Type::MARKET, -1, order_size);
}

std::ostream& operator<<(std::ostream& os, const Order& obj) {
    os << "[" << getTypeName(obj.order_type) << "|" << getSideName(obj.side) << "] ";
    os << "Order ID: " << obj.oid << ", Price: $"
       << std::fixed << std::setprecision(3) << obj.price
       << ", Size: " << obj.order_size;
    return os;
}
