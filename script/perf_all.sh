#!/usr/bin/env bash
set -euo pipefail

# ── config ────────────────────────────────────────────────────────────────────
BUILD_DIR="build"
OUTPUT_DIR="perf_data"
CALL_GRAPH="${CALL_GRAPH:-dwarf,16384}"
MIN_FREQ=100                        # give up below this
BENCH_ARGS="${BENCH_ARGS:-}"
# ──────────────────────────────────────────────────────────────────────────────

mkdir -p "$OUTPUT_DIR"

BINARIES=( "$BUILD_DIR"/bench_* )
if [[ ! -e "${BINARIES[0]}" ]]; then
    echo "No bench_* binaries found in $BUILD_DIR. Did you build with -DBUILD_BENCHMARKS=ON?"
    exit 1
fi

echo "Configuring kernel perf permissions..."
echo -1 | sudo tee /proc/sys/kernel/perf_event_paranoid > /dev/null
echo 0  | sudo tee /proc/sys/kernel/kptr_restrict       > /dev/null

MAX_FREQ=$(cat /proc/sys/kernel/perf_event_max_sample_rate)
echo "Kernel max sample rate: $MAX_FREQ Hz"

# Run perf record at a given frequency, return 0 if clean (no lost chunks)
run_perf() {
    local bin="$1"
    local out="$2"
    local freq="$3"
    
    # Capture stderr where perf prints lost-chunk warnings
    local perf_stderr
    perf_stderr=$(
        perf record \
        -g \
        --call-graph "$CALL_GRAPH" \
        -F "$freq" \
        -m 256 \
        -o "$out" \
        -- "$bin" $BENCH_ARGS 2>&1
    )
    
    echo "$perf_stderr"
    
    # Treat any lost-chunk warning as failure
    if echo "$perf_stderr" | grep -q "lost [0-9]* chunks"; then
        return 1
    fi
    return 0
}

for BIN in "${BINARIES[@]}"; do
    NAME=$(basename "$BIN")
    PERF_OUT="$OUTPUT_DIR/${NAME}.perf.data"
    
    echo ""
    echo "━━━ $NAME ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "  binary : $BIN"
    echo "  output : $PERF_OUT"
    
    freq=$MAX_FREQ
    success=0
    
    while [[ $freq -ge $MIN_FREQ ]]; do
        echo "  trying frequency: $freq Hz"
        
        if run_perf "$BIN" "$PERF_OUT" "$freq"; then
            echo "  ✓ clean recording at $freq Hz"
            success=1
            break
        else
            echo "  ✗ lost chunks at $freq Hz — backing off"
            freq=$(( freq / 2 ))
        fi
    done
    
    if [[ $success -eq 0 ]]; then
        echo "  ✗ could not get a clean recording down to $MIN_FREQ Hz"
        echo "    last output kept at $PERF_OUT — may have partial data"
    fi
    
    echo "  done → open with: hotspot $PERF_OUT"
done

echo ""
echo "All perf data written to $OUTPUT_DIR/"
echo "Open individual files with hotspot:"
for BIN in "${BINARIES[@]}"; do
    NAME=$(basename "$BIN")
    echo "  hotspot $OUTPUT_DIR/${NAME}.perf.data"
done