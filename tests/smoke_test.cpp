#include <gtest/gtest.h>

#include "orderbook/placeholder.hpp"

// Proves the build, link and test pipeline works. Replace with real tests.
TEST(Smoke, EngineLibraryLinks) {
  EXPECT_EQ(orderbook::placeholder(), 42);
}
