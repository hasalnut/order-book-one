# OrderBook API design

Decisions made during the API design session (step 1 in `docs/PLAN.md`). This file describes the **interface and its behaviour**. The implementation in `engine/` is written by hand.

Concepts this builds on are in `docs/concepts/01-matching-basics.md`.

**Files:** everything public lives in `engine/include/orderbook/order_book.hpp` (included by tests as `"orderbook/order_book.hpp"`), inside `namespace orderbook`. The implementation goes in `engine/src/order_book.cpp`.

## Big picture

```
Traders (tests, bots/, gateway/)        OrderBook                         Outputs
  addLimit / addMarket / cancel  --->   match against resting Orders ---> Trade callback (per trade)
                                        rest leftover limit qty      ---> AddResult / CancelResult (to caller)
                                        bestBid / bestAsk queries
```

- **Inputs** (public): `addLimit`, `addMarket`, `cancel`, and the queries.
- **Outputs** (public): `AddResult` and `CancelResult` go to the caller. `Trade` events go to the callback.
- **Internal** (engine only): `Order`, the resting state of the book. Callers never hold one, only an `OrderId`.

## Basic types

| Name      | Definition                          | Why |
|-----------|-------------------------------------|-----|
| `Side`    | `enum class Side : std::uint8_t { Buy, Sell }` | Call sites read as `Side::Buy`, not `true`. Scoped and has no implicit int conversion. 1 byte. |
| `OrderId` | `using OrderId = std::uint64_t;`    | Never negative. 64-bit so it never wraps in practice. |
| `Price`   | `using Price = std::int64_t;`       | **Integer ticks**, never float/double. Signed so `ask - bid` is safe and negative prices are representable. |
| `Qty`     | `using Qty = std::uint32_t;`        | Plenty per order. **Watch out:** unsigned `remaining - fill` wraps if `fill > remaining`. |

## Decisions

### 1. Order identification: engine-assigned
- The engine assigns IDs from an increasing counter, **starting at 1**.
- **`0` means "no ID"**. It's used for rejected orders and is never issued.
- Uniqueness is guaranteed for free, so there's no duplicate-ID error path.
- Limit and market orders draw from the **same counter**. Market orders need an ID to label their trades, even though they can never be cancelled.
- Trade-off: real exchanges accept client-chosen IDs at the edge and map them to internal IDs. Tests must store the returned ID rather than hardcode one.

### 2. `Trade`: an event between two orders
```
Trade {
  OrderId buyId;      // the buy order involved
  OrderId sellId;     // the sell order involved
  Price   price;      // always the RESTING (maker) order's price
  Qty     qty;        // quantity that changed hands in this trade
  Side    aggressor;  // side of the INCOMING (taker) order
}
```
- A Trade is an immutable event. It's not stored by the engine.
- One incoming order can produce **several** trades: one per resting order it matches.
- Example: "A×E 5 @ 101" is `{ buyId=E, sellId=A, price=101, qty=5, aggressor=Buy }`.

### 3. `Order`: internal resting state (engine only)
```
Order { OrderId id; Price price; Qty remaining; Side side; }   // ~24 bytes with padding
```
- **No `filled` field.** It's already reported through `AddResult` and `Trade`. Storing it again would duplicate data.
- **No `type` field.** Market orders never rest, so every Order on the book is a limit order and a type field would always hold the same value.
- Smaller orders mean more of them fit in cache.

### 4. Trade reporting: callback
- The callback is passed to the `OrderBook` constructor and called **once per trade, in match order**, during `addLimit`/`addMarket`.
- Type: `std::function<void(const Trade&)>` for now.
- **Revisit at the benchmark step:** `std::function` adds an indirect call per trade. The alternative is to make the callback a template parameter, so the call can be inlined. Measure before switching.
- Tests: the lambda pushes into a `std::vector<Trade>` owned by the test.

### 5. `addLimit` / `addMarket` return `AddResult`
```
enum class AddStatus : std::uint8_t { Accepted, Rejected };

AddResult {
  AddStatus status;
  OrderId   id;       // 0 if Rejected
  Qty       filled;   // traded immediately on arrival
  Qty       resting;  // left on the book (always 0 for market orders)
}
```
- Cancelled quantity isn't stored. The caller can work it out as `requested - filled - resting`. For a limit order this is always 0. For a market order it's the part that found no liquidity.
- Both functions return the same struct, so callers handle them the same way.

### 6. `cancel` returns `CancelResult`
```
enum class CancelResult : std::uint8_t { Cancelled, NotResting, UnknownId };
```
| Value        | Meaning | How it's detected |
|--------------|---------|-------------------|
| `Cancelled`  | The order was resting and has been removed | Found in the book |
| `NotResting` | The ID was issued but isn't on the book (filled, already cancelled, or a market order) | `0 < id < nextId` and not in the book |
| `UnknownId`  | The ID was never issued | `id == 0` or `id >= nextId` |

- Failing to cancel is **normal**, because of races (a cancel arrives just after a fill). So it's a return value, **not an exception**. Exceptions are slow and unpredictable when thrown, and are for situations that shouldn't happen.
- `NotResting` doesn't distinguish filled from cancelled. Telling them apart would mean keeping a history of every order forever.

### 7. Queries
```
std::optional<Price> bestBid() const;   // highest resting buy price, or empty
std::optional<Price> bestAsk() const;   // lowest resting sell price, or empty
```
- `std::optional` rather than a sentinel, because with signed prices `0` or `-1` could be real prices.

### 8. Input validation
- `qty == 0` is **rejected**: `{ Rejected, id = 0, filled = 0, resting = 0 }`. No ID is consumed and the book is unchanged.
- Prices aren't range-checked, since negative and zero prices are allowed.

### 9. Construction
- **An empty callback is rejected in the constructor**, by throwing `std::invalid_argument`. A constructor can't return a status, and construction is a one-off cold path, so throwing is fine here. The hot path never needs to check the callback, because it's guaranteed to be non-empty.

### 10. Copying
- **`OrderBook` is not copyable** (copy constructor and copy assignment are `= delete`).
- Why: mainly **correctness**. An accidental copy duplicates every resting order, so two books would disagree while sharing one callback. Second, cost: a copy is an expensive cold-path operation that nobody intended.
- **Moving: not decided yet.** Note that deleting the copy operations also stops the compiler generating move operations. If moves are wanted, they must be explicitly `= default`.

### 11. `[[nodiscard]]`: deferred
- To be revisited when callers start using `AddResult` / `CancelResult` values. Several tests currently ignore `addLimit`'s result, so adding it would need test updates (with `-Werror`).

## API sketch (signatures only)

```
class OrderBook {
public:
  using TradeCallback = std::function<void(const Trade&)>;
  explicit OrderBook(TradeCallback onTrade);   // throws std::invalid_argument if empty
  OrderBook(const OrderBook&) = delete;
  OrderBook& operator=(const OrderBook&) = delete;

  AddResult    addLimit(Side side, Price price, Qty qty);
  AddResult    addMarket(Side side, Qty qty);
  CancelResult cancel(OrderId id);

  std::optional<Price> bestBid() const;
  std::optional<Price> bestAsk() const;
};
```

## Behaviour rules (what the tests will check)

1. **Price-time priority.** Better price first. At the same price, FIFO by arrival.
2. **Crossing.** A buy crosses if `price >= bestAsk`. A sell crosses if `price <= bestBid`.
3. **Trade price** = the resting order's price.
4. A limit order's **leftover rests** at the back of its price level's queue.
5. A market order's **leftover is cancelled**. It never rests.
6. A **partially filled resting order keeps its queue position**.
7. A fully filled resting order is **removed**. An empty price level is removed.
8. **Callback order** matches the matching order: best price first, then FIFO.

## Threading

`OrderBook` is **not thread-safe**. It's meant to be owned and called by a single thread. This is the usual model for matching engines: one thread per book (or per group of instruments), with no locks on the hot path. Concurrency belongs at the edges, e.g. the gateway feeding orders through a queue.

## Possible later additions (not decided yet)

- `modify` / replace. Per the concepts lesson, increasing quantity or changing price = cancel + new order (back of queue), and decreasing quantity keeps queue position.
- More queries, e.g. quantity at a price level, or order count.
- Switching from `std::function` to a template callback, after benchmarking.
