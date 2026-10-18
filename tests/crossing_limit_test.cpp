#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "orderbook/order_book.hpp"

// Feature 2: an incoming limit order that crosses fully fills against ONE
// resting order, at the resting order's price. Walking several levels and
// leftover incoming quantity are feature 3. API: docs/design.md.

namespace {

using orderbook::AddResult;
using orderbook::AddStatus;
using orderbook::OrderBook;
using orderbook::Price;
using orderbook::Qty;
using orderbook::Side;
using orderbook::Trade;

class CrossingLimitTest : public ::testing::Test {
 protected:
  std::vector<Trade> trades;
  OrderBook book{[this](const Trade& t) { trades.push_back(t); }};
};

TEST_F(CrossingLimitTest, BuyMatchesRestingSellAtSamePrice) {
  const AddResult sell = book.addLimit(Side::Sell, Price{101}, Qty{5});
  const AddResult buy = book.addLimit(Side::Buy, Price{101}, Qty{5});

  EXPECT_EQ(buy.status, AddStatus::Accepted);
  EXPECT_EQ(buy.filled, Qty{5});
  EXPECT_EQ(buy.resting, Qty{0});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].buyId, buy.id);
  EXPECT_EQ(trades[0].sellId, sell.id);
  EXPECT_EQ(trades[0].price, Price{101});
  EXPECT_EQ(trades[0].qty, Qty{5});
  EXPECT_EQ(trades[0].aggressor, Side::Buy);

  // Both orders are gone, so both sides are empty.
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_EQ(book.bestAsk(), std::nullopt);
}

TEST_F(CrossingLimitTest, SellMatchesRestingBuyAtSamePrice) {
  const AddResult buy = book.addLimit(Side::Buy, Price{100}, Qty{4});
  const AddResult sell = book.addLimit(Side::Sell, Price{100}, Qty{4});

  EXPECT_EQ(sell.filled, Qty{4});
  EXPECT_EQ(sell.resting, Qty{0});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].buyId, buy.id);
  EXPECT_EQ(trades[0].sellId, sell.id);
  EXPECT_EQ(trades[0].price, Price{100});
  EXPECT_EQ(trades[0].qty, Qty{4});
  EXPECT_EQ(trades[0].aggressor, Side::Sell);

  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_EQ(book.bestAsk(), std::nullopt);
}

// The trade happens at the RESTING order's price, not the incoming limit.
TEST_F(CrossingLimitTest, AggressiveBuyTradesAtRestingAskPrice) {
  book.addLimit(Side::Sell, Price{101}, Qty{2});
  book.addLimit(Side::Buy, Price{105}, Qty{2});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, Price{101});
}

TEST_F(CrossingLimitTest, AggressiveSellTradesAtRestingBidPrice) {
  book.addLimit(Side::Buy, Price{100}, Qty{2});
  book.addLimit(Side::Sell, Price{95}, Qty{2});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].price, Price{100});
}

// Incoming is smaller than the resting order: the incoming order is fully
// filled and the resting order keeps the remainder on the book.
TEST_F(CrossingLimitTest, RestingOrderKeepsRemainderAfterSmallerIncoming) {
  const AddResult sell = book.addLimit(Side::Sell, Price{101}, Qty{5});
  const AddResult buy = book.addLimit(Side::Buy, Price{101}, Qty{3});

  EXPECT_EQ(buy.filled, Qty{3});
  EXPECT_EQ(buy.resting, Qty{0});
  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
  EXPECT_EQ(book.bestBid(), std::nullopt);

  // Exactly 2 should be left: a buy for 2 takes it all and empties the side.
  const AddResult buy2 = book.addLimit(Side::Buy, Price{101}, Qty{2});
  EXPECT_EQ(buy2.filled, Qty{2});
  ASSERT_EQ(trades.size(), 2u);
  EXPECT_EQ(trades[1].sellId, sell.id);
  EXPECT_EQ(trades[1].qty, Qty{2});
  EXPECT_EQ(book.bestAsk(), std::nullopt);
}

// A buy one tick below the best ask must not trade.
TEST_F(CrossingLimitTest, OneTickAwayDoesNotCross) {
  book.addLimit(Side::Sell, Price{101}, Qty{5});
  const AddResult buy = book.addLimit(Side::Buy, Price{100}, Qty{5});

  EXPECT_TRUE(trades.empty());
  EXPECT_EQ(buy.filled, Qty{0});
  EXPECT_EQ(buy.resting, Qty{5});
  EXPECT_EQ(book.bestBid(), std::optional<Price>{100});
  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
}

// The incoming order matches the BEST level, not just any crossing level.
TEST_F(CrossingLimitTest, BuyMatchesLowestAskFirst) {
  book.addLimit(Side::Sell, Price{102}, Qty{5});
  const AddResult best = book.addLimit(Side::Sell, Price{101}, Qty{5});
  book.addLimit(Side::Buy, Price{102}, Qty{5});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].sellId, best.id);
  EXPECT_EQ(trades[0].price, Price{101});
  EXPECT_EQ(book.bestAsk(), std::optional<Price>{102});
}

TEST_F(CrossingLimitTest, SellMatchesHighestBidFirst) {
  book.addLimit(Side::Buy, Price{99}, Qty{5});
  const AddResult best = book.addLimit(Side::Buy, Price{100}, Qty{5});
  book.addLimit(Side::Sell, Price{99}, Qty{5});

  ASSERT_EQ(trades.size(), 1u);
  EXPECT_EQ(trades[0].buyId, best.id);
  EXPECT_EQ(trades[0].price, Price{100});
  EXPECT_EQ(book.bestBid(), std::optional<Price>{99});
}

}  // namespace
