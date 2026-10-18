#include <gtest/gtest.h>

#include <stdexcept>
#include <type_traits>

#include "orderbook/order_book.hpp"

// Construction and copy rules: docs/design.md, decisions 9 and 10.

namespace {

using orderbook::OrderBook;
using orderbook::Trade;

TEST(Construction, EmptyCallbackIsRejected) {
  EXPECT_THROW(OrderBook{OrderBook::TradeCallback{}}, std::invalid_argument);
}

TEST(Construction, NonEmptyCallbackIsAccepted) {
  EXPECT_NO_THROW(OrderBook{[](const Trade&) {}});
}

// Checked at runtime (not static_assert) so a failure shows up as a failing
// test instead of breaking the whole test build.
TEST(Construction, IsNotCopyable) {
  EXPECT_FALSE(std::is_copy_constructible_v<OrderBook>);
  EXPECT_FALSE(std::is_copy_assignable_v<OrderBook>);
}

}  // namespace
