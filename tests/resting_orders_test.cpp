#include <gtest/gtest.h>

#include <optional>
#include <vector>

#include "orderbook/order_book.hpp"

// Feature 1: a limit order that doesn't cross rests in the book, and
// bestBid/bestAsk update. No test here sends an order that can cross.
// API: docs/design.md.

namespace {

using orderbook::AddResult;
using orderbook::AddStatus;
using orderbook::OrderBook;
using orderbook::OrderId;
using orderbook::Price;
using orderbook::Qty;
using orderbook::Side;
using orderbook::Trade;

class RestingOrdersTest : public ::testing::Test {
 protected:
  // Declared before `book` so it is constructed first.
  std::vector<Trade> trades;
  OrderBook book{[this](const Trade& t) { trades.push_back(t); }};
};

TEST_F(RestingOrdersTest, EmptyBookHasNoBestBidOrAsk) {
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_EQ(book.bestAsk(), std::nullopt);
}

TEST_F(RestingOrdersTest, BuyRestsAndBecomesBestBid) {
  const AddResult r = book.addLimit(Side::Buy, Price{100}, Qty{5});

  EXPECT_EQ(r.status, AddStatus::Accepted);
  EXPECT_NE(r.id, OrderId{0});
  EXPECT_EQ(r.filled, Qty{0});
  EXPECT_EQ(r.resting, Qty{5});

  EXPECT_EQ(book.bestBid(), std::optional<Price>{100});
  EXPECT_EQ(book.bestAsk(), std::nullopt);
  EXPECT_TRUE(trades.empty());
}

TEST_F(RestingOrdersTest, SellRestsAndBecomesBestAsk) {
  const AddResult r = book.addLimit(Side::Sell, Price{101}, Qty{3});

  EXPECT_EQ(r.status, AddStatus::Accepted);
  EXPECT_NE(r.id, OrderId{0});
  EXPECT_EQ(r.filled, Qty{0});
  EXPECT_EQ(r.resting, Qty{3});

  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_TRUE(trades.empty());
}

TEST_F(RestingOrdersTest, IdsStartAtOneAndIncrease) {
  const AddResult a = book.addLimit(Side::Buy, Price{99}, Qty{1});
  const AddResult b = book.addLimit(Side::Sell, Price{101}, Qty{1});
  const AddResult c = book.addLimit(Side::Buy, Price{98}, Qty{1});

  EXPECT_EQ(a.id, OrderId{1});
  EXPECT_EQ(b.id, OrderId{2});
  EXPECT_EQ(c.id, OrderId{3});
}

TEST_F(RestingOrdersTest, HigherBuyImprovesBestBid) {
  book.addLimit(Side::Buy, Price{99}, Qty{5});
  book.addLimit(Side::Buy, Price{100}, Qty{5});

  EXPECT_EQ(book.bestBid(), std::optional<Price>{100});
}

TEST_F(RestingOrdersTest, LowerBuyDoesNotChangeBestBid) {
  book.addLimit(Side::Buy, Price{100}, Qty{5});
  book.addLimit(Side::Buy, Price{99}, Qty{5});

  EXPECT_EQ(book.bestBid(), std::optional<Price>{100});
}

TEST_F(RestingOrdersTest, LowerSellImprovesBestAsk) {
  book.addLimit(Side::Sell, Price{102}, Qty{5});
  book.addLimit(Side::Sell, Price{101}, Qty{5});

  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
}

TEST_F(RestingOrdersTest, HigherSellDoesNotChangeBestAsk) {
  book.addLimit(Side::Sell, Price{101}, Qty{5});
  book.addLimit(Side::Sell, Price{102}, Qty{5});

  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
}

TEST_F(RestingOrdersTest, SamePriceOrdersBothRest) {
  const AddResult a = book.addLimit(Side::Buy, Price{100}, Qty{5});
  const AddResult b = book.addLimit(Side::Buy, Price{100}, Qty{3});

  EXPECT_EQ(a.resting, Qty{5});
  EXPECT_EQ(b.resting, Qty{3});
  EXPECT_NE(a.id, b.id);
  EXPECT_EQ(book.bestBid(), std::optional<Price>{100});
}

// The book from t=1..4 of docs/concepts/01-matching-basics.md.
TEST_F(RestingOrdersTest, WorkedExampleBookBeforeAnyCross) {
  book.addLimit(Side::Sell, Price{101}, Qty{5});  // A
  book.addLimit(Side::Sell, Price{101}, Qty{3});  // B
  book.addLimit(Side::Sell, Price{102}, Qty{4});  // C
  book.addLimit(Side::Buy, Price{99}, Qty{6});    // D

  EXPECT_EQ(book.bestBid(), std::optional<Price>{99});
  EXPECT_EQ(book.bestAsk(), std::optional<Price>{101});
  EXPECT_TRUE(trades.empty());
}

TEST_F(RestingOrdersTest, ZeroQuantityIsRejectedAndBookUnchanged) {
  const AddResult r = book.addLimit(Side::Buy, Price{100}, Qty{0});

  EXPECT_EQ(r.status, AddStatus::Rejected);
  EXPECT_EQ(r.id, OrderId{0});
  EXPECT_EQ(r.filled, Qty{0});
  EXPECT_EQ(r.resting, Qty{0});
  EXPECT_EQ(book.bestBid(), std::nullopt);
  EXPECT_TRUE(trades.empty());
}

TEST_F(RestingOrdersTest, RejectedOrderDoesNotConsumeAnId) {
  book.addLimit(Side::Sell, Price{101}, Qty{0});
  const AddResult r = book.addLimit(Side::Sell, Price{101}, Qty{1});

  EXPECT_EQ(r.status, AddStatus::Accepted);
  EXPECT_EQ(r.id, OrderId{1});
}

// Prices are signed ticks (design.md), so zero and negative prices are real
// prices, not "empty" sentinels.
TEST_F(RestingOrdersTest, ZeroAndNegativePricesRest) {
  book.addLimit(Side::Buy, Price{-5}, Qty{1});
  EXPECT_EQ(book.bestBid(), std::optional<Price>{-5});

  book.addLimit(Side::Buy, Price{0}, Qty{1});
  EXPECT_EQ(book.bestBid(), std::optional<Price>{0});
}

}  // namespace
