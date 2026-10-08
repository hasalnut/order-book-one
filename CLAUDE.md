# Order Book One

A C++20 limit order book and matching engine, built to learn low-latency C++ for placement interviews. See `docs/PLAN.md` for the roadmap.

## Rules

### `engine/` — learning zone: do not write code
- Never write or edit code in `engine/`. This includes small fixes, refactors and "just this once" changes.
- Instead: explain concepts, review my code, point out bugs (say where and why, but let me fix them), and ask me questions that lead me to the answer.
- Short pseudocode to illustrate an idea is fine. A full drop-in implementation is not.

### Everywhere else: write code freely
Build files (CMake etc.), `tests/`, `bench/`, CI, `docs/` and `bots/` are fair game. Write and edit them directly.

### Prices are integer ticks
- Prices are always integer ticks. Never use `float` or `double` for prices, in any directory, including tests, benchmarks and bots.
- Convert to or from decimal only at the edges (input parsing, display), and never inside matching logic.

### Verify before claiming
- Run the tests before saying anything works. Report the actual result: pass or fail, with the output when something fails.
- If the tests can't be run (no build yet, missing tests), say that plainly rather than implying the code works.
