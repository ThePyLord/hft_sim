#ifndef ORDER_H
#define ORDER_H

#include <chrono>
#include <cstdint>
#include <iostream>

static int id = 0;

enum Side {
    BUY,
    SELL,
    // Add other order types as needed
};

enum Type {
    MARKET,
    LIMIT,
    STOP,
    STOP_LIMIT,
};

std::string getSideName(Side s);
std::string getTypeName(Type t);

class Order {
   private:
    Side side;
    Type order_type;
    double price;
    uint32_t order_size;
    int oid;
    uint64_t time_of_order;

   public:
    Order() = delete;
    Order(Side side, Type type, double price, uint32_t order_size);
    ~Order() = default;

    static Order createLimitOrder(Side side, double price, uint32_t order_size);
    static Order createMarketOrder(Side side, uint32_t order_size);

    Side getSide() const;
    double getPrice() const;
    Type getType() const;
    uint32_t getSize() const;

    void setSize(uint32_t size);

    friend std::ostream& operator<<(std::ostream& os, const Order& obj);
};

#endif