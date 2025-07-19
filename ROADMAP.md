# HFT Simulator Roadmap (C++)

## Phase 1: Core Order Book Implementation
### Step 1 - Basic Limit Order Book (LOB)
- Implement a **price-time priority** matching engine.
- Use `std::map` or custom heap for bid/ask queues.
- Support order types: `LIMIT`, `MARKET`, `CANCEL`.

### Step 2 - Order Matching Logic
- Handle **partial fills** and **total fills**.
- Generate trade events (e.g., `Trade: 100 AAPL @ $150`).

### Step 3 - Data Structures Optimization
- Replace `std::map` with a **flat memory layout** (arrays for price levels).
- Use **lock-free queues** if multithreading.

---

## Phase 2: Performance Optimization
### Step 4 - Latency Measurement
- Add nanosecond timestamps (`<chrono>` in C++).
- Log order-to-trade latency.

### Step 5 - Multithreading
- Simulate market data feed vs. strategy thread.
- Use **atomic variables** or **SPSC queues**.

### Step 6 - Memory Pooling
- Pre-allocate orders to avoid `new/delete` overhead.

---

## Phase 3: Trading Strategy Simulation
### Step 7 - Basic Market Making
- Implement a **spread-based strategy**:
  - Quote bid/ask around mid-price.
  - Adjust for inventory risk.

### Step 8 - Latency Arbitrage (Optional)
- Simulate **tick-to-trade delays**.
- Exploit cross-exchange price discrepancies.

---

## Phase 4: Visualization & Analysis
### Step 9 - Python Integration (Optional)
- Export LOB snapshots via **Pybind11**.
- Visualize with `matplotlib`:
  ```python
  plt.plot(order_book["bids"], label="Bids")