# Parallel Log Processing Engine

High-performance C++20 log analytics engine optimized for multi-core Linux systems.

## Features

- Multi-threaded parsing with custom thread pool
- Zero-copy key-value log parsing
- Lock-free per-thread aggregation
- Percentile latency calculation (p50, p95, p99)
- Time-window analysis (per-minute buckets)
- Benchmark mode for scalability testing

## Build

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j$(nproc)
```

## Usage

```bash
# Analyze logs
./logengine analyze --input ../data/sample.log --threads 4 --format kv

# Benchmark scalability
./logengine bench --input large.log --threads 1,2,4,8 --repeat 3

# Generate test data
./logengine gen --output test.log --lines 10000000 --endpoints 100 --error_rate 0.02
```

## Profiling

```bash
# Perf stats
perf stat -e cycles,instructions,cache-misses ./logengine analyze --input large.log --threads 8

# Perf record + report
perf record -g ./logengine analyze --input large.log --threads 8
perf report
```

## Testing

```bash
./logengine_tests
```
