#!/usr/bin/env bash
# Performance profiling script using perf
# Usage: ./scripts/profile.sh [executable] [args...]
# Note: Ensure this script is executable before first use:
#   chmod +x scripts/profile.sh

set -e

EXECUTABLE="${1:-./profanity.x64}"
shift || true
# Store arguments in an array to properly handle spaces and prevent command injection
ARGS=("$@")

echo "=========================================="
echo "Performance Profiling Script"
echo "=========================================="
echo "Executable: $EXECUTABLE"
echo "Arguments: ${ARGS[*]}"
echo ""

# Check if executable exists
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: Executable '$EXECUTABLE' not found"
    echo "Please build the project with profiling enabled:"
    echo "  cmake -DENABLE_PROFILING=ON .."
    echo "  make"
    exit 1
fi

# Check if perf is available
if ! command -v perf &> /dev/null; then
    echo "Warning: 'perf' command not found"
    echo "Profiling will use gprof instead"
    echo ""
    
    # Run with gprof
    echo "Running executable with gprof profiling..."
    "$EXECUTABLE" "${ARGS[@]}"
    
    if [ -f gmon.out ]; then
        echo ""
        echo "Generating gprof report..."
        gprof "$EXECUTABLE" gmon.out > profile_report.txt
        echo "Profile report saved to: profile_report.txt"
        echo ""
        echo "Top 20 functions by time:"
        head -n 40 profile_report.txt | tail -n 20
    else
        echo "Warning: gmon.out not generated. Was the executable built with -pg?"
    fi
else
    # Use perf for profiling
    echo "Running with perf profiling..."
    echo ""
    
    # Record performance data
    perf record -g --call-graph dwarf -o perf.data -- "$EXECUTABLE" "${ARGS[@]}"
    
    echo ""
    echo "Generating perf report..."
    perf report -i perf.data --stdio > perf_report.txt
    
    echo "Performance report saved to: perf_report.txt"
    echo ""
    echo "Top hotspots:"
    head -n 30 perf_report.txt
    
    echo ""
    echo "Generating flamegraph (if available)..."
    if command -v flamegraph &> /dev/null; then
        perf script -i perf.data | flamegraph > flamegraph.svg
        echo "Flamegraph saved to: flamegraph.svg"
    else
        echo "Flamegraph tool not found, skipping"
    fi
fi

echo ""
echo "=========================================="
echo "Profiling complete!"
echo "=========================================="
echo ""
echo "Generated files:"
echo "  - perf.data or gmon.out (raw profiling data)"
echo "  - perf_report.txt or profile_report.txt (human-readable report)"
if [ -f flamegraph.svg ]; then
    echo "  - flamegraph.svg (visualization)"
fi
echo ""
echo "For detailed analysis:"
echo "  perf report -i perf.data     # Interactive perf report"
echo "  gprof \"$EXECUTABLE\" gmon.out   # Detailed gprof report"
