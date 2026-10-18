# Plan

Roadmap and reusable prompts for building the engine. The rules in `CLAUDE.md` still apply. In particular, Claude never writes code in `engine/`.

## Progress

- [x] 0. Learn the concepts: price-time priority, crossing, partial fills, market vs limit. See `docs/concepts/01-matching-basics.md`.
- [x] 1. Design the OrderBook API, recorded in `docs/design.md`.
- [ ] 2. Write failing tests for a feature, then implement it in `engine/`. Repeat this for each feature.
- [ ] 3. Review each feature's `engine/` changes.
- [ ] 4. Run the ASan tests, then commit and push.
- [ ] 5. Build the benchmark harness and measure.

Steps 2–4 form a loop: tests, then implement, then review, then ASan and commit. Run it once per feature.

## Feature tracker

| # | Feature | Tests | Engine | Reviewed | Committed |
|---|---------|-------|--------|----------|-----------|
| 1 | Non-crossing limit rests, best bid/ask update | ✅ `tests/resting_orders_test.cpp` (13 tests) | ✅ all pass (debug) | ✅ | |
| 1b | Construction: reject empty callback, not copyable | ✅ `tests/construction_test.cpp` (3 tests) | ✅ all pass (debug) | ✅ | |
| 2 | Crossing limit fully fills one resting order | ✅ `tests/crossing_limit_test.cpp` (not in CMake yet) | | | |
| 3 | Partial fills, walking several levels | | | | |
| 4 | FIFO within a level, partial fill keeps place | | | | |
| 5 | Market orders | | | | |
| 6 | Cancel | | | | |
| 7 | Edge cases | | | | |

## Backlog (from reviews)

Items deliberately left for later, and when to deal with them.

| From | Item | Deal with it at |
|------|------|-----------------|
| Review of feature 1 | `addLimit` never checks for a cross, so the book can end up crossed (bid >= ask) | Feature 2 |
| Review of feature 1 | `addMarket` / `cancel` are declared but not defined, so callers get a linker error | Features 5 and 6 |
| Review of feature 1 | `cancel` needs a fast ID -> order location lookup (an extra index), which must be kept up to date when orders fill | Feature 6 |
| Review of feature 1 | `Order` stores `price` and `side`, which are redundant with the map key and the side's map (24 vs 16 bytes). Keep or drop, but be able to justify it | Feature 6 (cancel may need them) |
| Review of feature 1 | Allocation on the hot path: `std::map` node per new level, `std::deque` chunks | Benchmark step (measure first; consider a flat array indexed by tick) |
| Review of feature 1 | `std::function` callback costs an indirect call per trade; a template parameter would allow inlining | Benchmark step |
| Review of feature 1 | `nextId_++` happens before an insert that could throw `bad_alloc`, which would use up an ID with no order | Optional; good interview discussion |
| Review of feature 1 | `[[nodiscard]]` on `addLimit` / `addMarket` / `cancel` (needs tests updated to use or discard the results) | When results are used more widely |
| Design decision 10 | Should `OrderBook` be movable? Deleting copy also removes the implicit move | Any time; decide and record in design.md |

## Workflow prompts

### 1. API design (Claude asks questions, I decide)

> I want to design the OrderBook API. Don't write it. Ask me questions one at a time to help me decide: what types I need (Side, Order, Trade), how orders are identified, what addLimit/addMarket/cancel return, and how trades are reported. At the end, summarise my decisions in docs/design.md.

### 2. Failing tests for the next feature

> Write GoogleTest tests in tests/ for the next feature: [FEATURE, e.g. "a limit order that doesn't cross rests in the book, and best bid/ask update"]. Use the API in docs/design.md. The tests should fail for now. Then tell me what I need to implement, without writing it.

Feature order: see the feature tracker above.

### 3. Review engine changes

> Review my changes in engine/. Look for bugs, edge cases my tests miss, undefined behaviour and anything an interviewer would question. Don't fix anything; list issues by severity and let me fix them.

### 4. ASan, then commit and push

> Run the asan tests. If they pass, commit with a clear message and push.

Uses the `asan` preset in `CMakePresets.json`. Pushes to `origin` (github.com/hasalnut/order-book-one).

### 5. Benchmark harness

> Build a benchmark harness in bench/: generate a realistic synthetic order stream (mostly near the best price, lots of cancels, some market orders), save it to a file, replay it through the engine, and report orders/sec plus p50, p99 and p99.9 latency per order. Use the release preset only.

Prices in the generated stream must be integer ticks.
