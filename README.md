# Order Book One

[![CI](https://github.com/hasalnut/order-book-one/actions/workflows/ci.yml/badge.svg)](https://github.com/hasalnut/order-book-one/actions/workflows/ci.yml)

A C++20 limit order book and matching engine, built to learn low-latency C++.

## Layout

| Path | Purpose |
|------|---------|
| `engine/include/orderbook/` | Public engine headers |
| `engine/src/` | Engine implementation |
| `tests/` | GoogleTest unit tests |
| `bench/` | Google Benchmark microbenchmarks |
| `gateway/` | Order entry / market data gateway |
| `bots/` | Simulated trading clients for load and testing |
| `docs/` | Plan, design notes and [benchmark results](docs/results.md) |

## Requirements

- CMake 3.24+
- Ninja
- A C++20 compiler (GCC 13+, Clang 16+ or Apple Clang 15+)

GoogleTest and Google Benchmark are downloaded automatically during configuration.

macOS: `brew install cmake ninja` · Ubuntu: `sudo apt install cmake ninja-build g++`

## Build and test

There are three presets:

| Preset | Use for |
|--------|---------|
| `debug` | Everyday development |
| `asan` | Debug + AddressSanitizer + UBSan, which catches memory and undefined-behaviour bugs |
| `release` | Optimised build. Use this for benchmarks |

```sh
cmake --preset debug           # configure (output in build/debug)
cmake --build --preset debug   # build
ctest --preset debug           # run tests
```

Replace `debug` with `asan` or `release` as needed.

## Benchmarks

```sh
cmake --preset release && cmake --build --preset release
./build/release/orderbook_bench
```

Record results in [docs/results.md](docs/results.md).

## Formatting

```sh
clang-format -i engine/**/*.{hpp,cpp} tests/*.cpp bench/*.cpp
```
