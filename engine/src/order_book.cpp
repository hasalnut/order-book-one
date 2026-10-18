#include "orderbook/order_book.hpp"
#include <utility>
#include <stdexcept>

namespace orderbook {

OrderBook::OrderBook(TradeCallback onTrade) {
  if (onTrade == nullptr) {
    throw std::invalid_argument("OrderBook(onTrade): onTrade is not assigned");
  }
  onTrade_ = std::move(onTrade);
}

AddResult OrderBook::addLimit(Side side, Price price, Qty qty) {
  if (qty == 0) {
    return {AddStatus::Rejected, 0, 0, 0};
  }
  const OrderId id = nextId_++;
  if (side == Side::Buy) {
    bids_[price].emplace_back(id, price, qty, side);
  } else {
    asks_[price].emplace_back(id, price, qty, side);
  }
  return {AddStatus::Accepted, id, 0, qty};
}

std::optional<Price> OrderBook::bestBid() const {
  if (bids_.empty()) {
    return std::nullopt;
  }
  return bids_.begin()->first;
}

std::optional<Price> OrderBook::bestAsk() const {
  if (asks_.empty()) {
    return std::nullopt;
  }
  return asks_.begin()->first;
}

} // namespace orderbook