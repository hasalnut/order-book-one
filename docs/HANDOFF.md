# Hand-off: resume here

Read this first in a new Claude session. It records where the project is and how we work, so nothing needs re-explaining.

**Last updated:** 2026-10-18

## Read in this order
1. `CLAUDE.md`: the rules (Claude never writes code in `engine/`, prices are integer ticks, run tests before claiming anything works).
2. This file.
3. `docs/PLAN.md`: the roadmap, **feature tracker**, **backlog**, and the reusable workflow prompts.
4. `docs/design.md`: every API decision (1–11), with reasons.
5. `docs/concepts/01-matching-basics.md`: the matching concepts, with a worked example and Q&A.

## Where we are (2026-10-18)

- **Step 2–4 loop, feature 1.** Implemented, reviewed, all feature 1 tests pass on the debug preset.
- **Feature 1b (construction) is done:** the empty callback throws `std::invalid_argument` (the user checks the parameter before moving, rather than using the initialiser list, which is fine), and copying is `= delete`d. **17/17 tests pass on debug.** Style items done.
- **Then step 4:** run the ASan tests (never run yet), commit and push. **Nothing has been committed since the initial skeleton.**
- **Then feature 2:** `tests/crossing_limit_test.cpp` is written (8 tests) but **not yet in `CMakeLists.txt`**. Add it to `orderbook_tests` when starting feature 2.

## Engine state (written by the user)

- `engine/include/orderbook/order_book.hpp`: all public types from `design.md`; `OrderBook` with a private `Order` struct, `onTrade_`, `nextId_ = 1`, `bids_` (`std::map<Price, std::deque<Order>, std::greater<>>`), `asks_` (`std::map<Price, std::deque<Order>>`).
- `engine/src/order_book.cpp`: constructor (moves the callback), `addLimit` (rejects qty 0, rests every order with no matching yet), `bestBid`/`bestAsk` (return `nullopt` when empty).
- `addMarket` and `cancel` are declared but **not defined**.
- `engine/src/placeholder.cpp` and its header still exist because `tests/smoke_test.cpp` and `bench/smoke_bench.cpp` use them. Remove them all together later.

## How we work

The user is a student learning low-latency C++ for interviews, and writes all of `engine/` themselves.

- **Teaching style:** explain concepts, then ask questions one at a time. If the user struggles twice, give the answer with the reasoning. Pseudocode is fine, but never a drop-in implementation for `engine/`.
- **When reviewing:** point out bugs with the file and line and *why*, but don't fix them. Mark severity. Also flag things an interviewer would ask about.
- **The user often makes small slips** (missing `;`, a stray word, copy-paste errors between mirrored functions). Check every line.

### Short commands the user sends

| User says | Claude does |
|---|---|
| **"check again"** | Show the engine files with line numbers, then run the compile-only check: `c++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror -Iengine/include -fsyntax-only engine/src/order_book.cpp`. Report errors and review the changes. |
| **"build"** | Make sure every engine `.cpp` is in `add_library(orderbook ...)`, then run `cmake --preset debug && cmake --build --preset debug && ctest --preset debug`. Report the actual pass or fail. A stale test binary can make `ctest` report old results after a failed build, so always check the build step succeeded. |
| **"step 4"** | `cmake --preset asan && cmake --build --preset asan && ctest --preset asan`. If everything passes, commit with a clear message and push to `origin` (github.com/hasalnut/order-book-one). |
| **"review"** | Run step 3 from `PLAN.md` on the `engine/` changes. |
| **"feature N"** | Write (or add to CMake) the failing tests for feature N from the tracker, then explain what to implement without writing it. |

## Gotchas found so far
- With `-Werror`, `#pragma once` warns "in main file" if you compile a header directly. Check headers by compiling a `.cpp` that includes them.
- Build flags include `-Wconversion -Wshadow -Werror`, so tests use typed literals (`Qty{5}`, `Price{100}`).
- Clang's `-Wunused-private-field` was expected to complain about `onTrade_` (stored but never called), but it didn't. `std::function` isn't a trivial type, so the warning doesn't apply to it.
