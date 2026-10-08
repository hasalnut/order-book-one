# Benchmark Results

A log of benchmark runs over time. Always use the `release` preset, note the machine, and add a new row instead of overwriting old ones, so the history shows what each change did.

Run with:

```sh
cmake --preset release && cmake --build --preset release
./build/release/orderbook_bench --benchmark_repetitions=5 --benchmark_report_aggregates_only=true
```

| Date | Commit | Benchmark | Machine / CPU | Compiler | Time/op (median) | Throughput | Notes |
|------|--------|-----------|---------------|----------|------------------|------------|-------|
| 2026-10-08 | initial | BM_Placeholder | Apple Silicon (10 cores) | Apple clang 21 | 0.68 ns | — | Pipeline smoke test only, not meaningful |
