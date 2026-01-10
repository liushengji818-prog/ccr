# Performance Profiling Guide

This guide explains how to profile the performance of the TRON address generator to identify bottlenecks and optimize performance.

## Table of Contents

1. [Building with Profiling Support](#building-with-profiling-support)
2. [Using the Automated Script](#using-the-automated-script)
3. [Manual Profiling with perf](#manual-profiling-with-perf)
4. [Manual Profiling with gprof](#manual-profiling-with-gprof)
5. [Interpreting Results](#interpreting-results)
6. [Common Optimization Strategies](#common-optimization-strategies)

## Building with Profiling Support

### CMake Build

Enable profiling when configuring with CMake:

```bash
mkdir build && cd build
cmake -DENABLE_PROFILING=ON ..
make
```

This adds the `-pg` and `-fno-omit-frame-pointer` flags needed for profiling.

### Makefile Build

For Makefile builds, the profiling flags need to be added manually:

```bash
# Edit Makefile and add -pg to CFLAGS and LDFLAGS
make clean
make
```

## Using the Automated Script

The simplest way to profile is using the provided script:

```bash
# Profile with default options
./scripts/profile.sh ./profanity.x64 --matching addresses.txt

# Profile with custom arguments
./scripts/profile.sh ./profanity.x64 --prefix-count 4 --suffix-count 4
```

The script will:
- Automatically detect available profiling tools (perf or gprof)
- Run the executable with profiling enabled
- Generate human-readable reports
- Create flamegraphs if available

## Manual Profiling with perf

`perf` is a powerful Linux profiling tool that provides detailed performance analysis.

### Basic Usage

```bash
# Record performance data
perf record -g --call-graph dwarf ./profanity.x64 --matching addresses.txt

# View interactive report
perf report

# Generate text report
perf report --stdio > perf_report.txt

# View top functions
perf report --sort cpu --stdio | head -n 50
```

### Advanced perf Commands

```bash
# Record with higher precision
perf record -F 999 -g ./profanity.x64 args...

# Profile specific CPU events
perf record -e cycles,instructions,cache-misses ./profanity.x64 args...

# Get cache statistics
perf stat -e cache-references,cache-misses ./profanity.x64 args...

# Generate flamegraph
perf script | flamegraph > flamegraph.svg
```

### Installing perf

```bash
# Ubuntu/Debian
sudo apt-get install linux-tools-common linux-tools-generic

# Fedora
sudo dnf install perf

# Arch Linux
sudo pacman -S perf
```

## Manual Profiling with gprof

`gprof` is a traditional profiling tool that works well when perf is not available.

### Basic Usage

```bash
# Run the profiling-enabled executable
./profanity.x64 --matching addresses.txt

# Generate report (creates gmon.out)
gprof ./profanity.x64 gmon.out > profile_report.txt

# View top functions
gprof -b ./profanity.x64 gmon.out | head -n 50
```

### Advanced gprof Commands

```bash
# Show call graph
gprof -q ./profanity.x64 gmon.out

# Show only flat profile
gprof -p ./profanity.x64 gmon.out

# Show line-by-line profiling
gprof -l ./profanity.x64 gmon.out
```

## Interpreting Results

### Key Metrics

- **% Time**: Percentage of total execution time spent in a function
- **Cumulative Seconds**: Total time including called functions
- **Self Seconds**: Time spent in the function itself
- **Calls**: Number of times the function was called
- **Self ms/call**: Average time per call

### What to Look For

1. **Hot Functions**: Functions consuming >10% of total time
2. **Unexpected Calls**: Functions called more often than expected
3. **Cache Misses**: High cache-miss rates indicate poor memory locality
4. **Lock Contention**: Time spent waiting for locks in multi-threaded code

### Example Analysis

```
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
 35.00     0.70     0.70   100000     0.00     0.01  Mode::matching
 20.00     1.10     0.40        1   400.00   600.00  Dispatcher::run
 15.00     1.40     0.30   500000     0.00     0.00  base58Encode
```

This shows:
- `Mode::matching` is the hottest function (35% of time)
- `Dispatcher::run` has high cumulative time but calls other functions
- `base58Encode` is called frequently and may benefit from optimization

## Common Optimization Strategies

### 1. Algorithm Optimization

- Replace O(n²) algorithms with O(n log n) or O(n)
- Use better data structures (hash tables instead of linear search)
- Cache computed results

### 2. Memory Access Patterns

```cpp
// Bad: Poor cache locality
for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
        process(matrix[j][i]);

// Good: Sequential access
for (int i = 0; i < rows; i++)
    for (int j = 0; j < cols; j++)
        process(matrix[i][j]);
```

### 3. Reduce Function Call Overhead

```cpp
// Bad: Function call in loop
for (int i = 0; i < n; i++)
    result += expensiveFunction(i);

// Good: Inline or vectorize
inline int expensiveFunction(int x) { ... }
```

### 4. Enable Compiler Optimizations

Build with maximum optimizations:

```bash
cmake -DENABLE_HIGH_PERFORMANCE=ON ..
make
```

This enables `-O3 -march=native -flto` for aggressive optimization. Note that binaries built with `-march=native` are optimized for the build machine's CPU and may not run correctly on machines with different or older CPU features; omit this flag if you need portable binaries.

### 5. Profile-Guided Optimization (PGO)

```bash
# 1. Build with profiling instrumentation
gcc -fprofile-generate -o profanity.x64 *.o

# 2. Run typical workload
./profanity.x64 --matching addresses.txt

# 3. Rebuild with profile data
gcc -fprofile-use -o profanity.x64 *.o
```

## GPU Profiling (OpenCL)

For GPU-specific profiling:

```bash
# AMD GPUs (ROCm)
rocprof ./profanity.x64 args...

# NVIDIA GPUs (CUDA)
nvprof ./profanity.x64 args...

# Intel GPUs
vtune ./profanity.x64 args...
```

## Continuous Profiling

Set up automated performance regression testing:

```bash
# Add to CI pipeline
cmake -DBUILD_BENCHMARKS=ON ..
make benchmark

# Compare against baseline
./bench_core --benchmark_out=results.json
```

## Tips and Best Practices

1. **Profile Representative Workloads**: Use realistic input data and parameters
2. **Profile Multiple Runs**: Results can vary; take averages
3. **Profile Release Builds**: Debug builds have different performance characteristics
4. **Focus on Hot Paths**: Optimize the 20% of code that takes 80% of time
5. **Measure Before and After**: Always verify that optimizations actually help
6. **Consider Trade-offs**: Faster code may use more memory or be less readable

## Troubleshooting

### "perf not found" or Permission Denied

```bash
# Set kernel parameter
sudo sysctl -w kernel.perf_event_paranoid=-1

# Or install perf
sudo apt-get install linux-tools-$(uname -r)
```

### "gmon.out not generated"

- Ensure the executable was built with `-pg`
- Make sure the program runs to completion (generates gmon.out on exit)
- Check that you have write permissions in the current directory

### Profile Data is Empty

- Ensure the program runs long enough (>1 second)
- Check that optimizations didn't eliminate the code
- Verify that `-fno-omit-frame-pointer` is used with `-O2` or `-O3`

## Further Reading

- [perf Wiki](https://perf.wiki.kernel.org/index.php/Tutorial)
- [gprof Manual](https://sourceware.org/binutils/docs/gprof/)
- [Intel VTune Profiler](https://software.intel.com/content/www/us/en/develop/tools/oneapi/components/vtune-profiler.html)
- [Brendan Gregg's Performance Pages](http://www.brendangregg.com/perf.html)
