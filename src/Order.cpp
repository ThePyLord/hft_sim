#include <cstdlib>
#include <iomanip>

#include "Order.h"

const uint32_t Order::getId() const {
    return oid;
}

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

std::optional<double> Order::getPrice() const { return this->price; }
Type Order::getType() const { return this->order_type; }
uint32_t Order::getSize() const { return this->order_size; }
std::chrono::steady_clock::time_point Order::getTimestamp() const {
    return this->timestamp;
}

void Order::setSize(uint32_t size) {
    if (size >= 0) {
        this->order_size = size;
    }
}

/// @brief Creates an order with the given parameters
/// @param side The buy/sell side of the order book
/// @param orderType The type of the order (MARKET, LIMIT, etc.)
/// @param order_size The size of the order
/// @param price The price of the order (optional)
/// @return An order object
Order::Order(Side side, Type orderType, double price, uint32_t order_size) : 
oid(id++), 
side(side),
order_type(orderType),
price(price),
order_size(order_size),
timestamp(std::chrono::steady_clock::now())
{
    if (side != BUY && side != SELL) {
        std::cerr << "Bad side value: " << static_cast<int>(side) << std::endl;
        throw std::invalid_argument("Invalid order side");
    }
    
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
    return Order(side, Type::MARKET, {}, order_size);
}

std::ostream& operator<<(std::ostream& os, const Order& obj) {
    os << "[" << getTypeName(obj.order_type) << "|" << getSideName(obj.side) << "] ";
    os << "Order ID: " << obj.oid;
    if (obj.price.has_value()) {
        os <<  ", Price: $" << std::fixed << std::setprecision(3) << obj.price.value();
    }
    else
       os << ", Size: " << obj.order_size;
    return os;
}
