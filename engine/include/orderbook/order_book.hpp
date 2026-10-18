#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <map>
#include <deque>

namespace orderbook {

// order ids start at 1, 0 counts as no id
using OrderId = std::uint64_t;
using Price   = std::int64_t;
using Qty     = std::uint32_t;

enum class Side : std::uint8_t {
  Buy,
  Sell
};

enum class AddStatus : std::uint8_t {
  Accepted,
  Rejected
};

enum class CancelResult : std::uint8_t {
  Cancelled,  // The order was resting and has been removed
  NotResting, // The ID was issued but isn't on the book (filled, already cancelled, or a market order)
  UnknownId   // The ID was never issued
};


struct Trade {
  OrderId buyId;
  OrderId sellId;     // the sell order involved
  Price   price;      // always the RESTING (maker) order's price
  Qty     qty;        // quantity that changed hands in this trade
  Side    aggressor;  // side of the INCOMING (taker) order
};

struct AddResult {
  AddStatus status;
  OrderId   id;       // 0 if Rejected
  Qty       filled;   // traded immediately on arrival
  Qty       resting;  // left on the book (always 0 for market orders)
};

class OrderBook {
public:
  using TradeCallback = std::function<void(const Trade&)>;
  explicit OrderBook(TradeCallback onTrade);
  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

  AddResult    addLimit(Side side, Price price, Qty qty);
  AddResult    addMarket(Side side, Qty qty);
  CancelResult cancel(OrderId id);

  std::optional<Price> bestBid() const;
  std::optional<Price> bestAsk() const;

private:
  struct Order {
    OrderId id;
    Price   price;
    Qty     remaining;
    Side    side;
  }; // ~24 bytes with padding
  TradeCallback                                      onTrade_;
  OrderId                                            nextId_ = 1;
  std::map<Price, std::deque<Order>, std::greater<>> bids_;
  std::map<Price, std::deque<Order>>                 asks_;
};

} // namespace orderbook
