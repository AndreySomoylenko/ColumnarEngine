#!/usr/bin/env bash
set -euo pipefail

QUERY="$1"
export COLUMNAR="${COLUMNAR:-$2}"
OUTPUT="$3"
LOGS="$4"
OUTPUT_DIR="$(dirname "$OUTPUT")"
LOGS_DIR="$(dirname "$LOGS")"
RESULTS_CSV="$OUTPUT_DIR/result.csv"
RESULTS_LOCK="$OUTPUT_DIR/.result_csv.lock"

mkdir -p "$OUTPUT_DIR"
mkdir -p "$LOGS_DIR"

append_result() {
    while ! mkdir "$RESULTS_LOCK" 2>/dev/null; do
        sleep 0.05
    done

    if [[ ! -f "$RESULTS_CSV" ]]; then
        echo "query,elapsed_ms" > "$RESULTS_CSV"
    fi
    echo "$QUERY,$ELAPSED_MS" >> "$RESULTS_CSV"

    rmdir "$RESULTS_LOCK"
}

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

pushd "$WORKDIR" >/dev/null

START_NS=$(date +%s%N)

"$OLDPWD/build/sandbox/sandbox_app" run "$QUERY" \
    > stdout.txt \
    2> stderr.txt

END_NS=$(date +%s%N)

ELAPSED_MS=$(((END_NS - START_NS) / 1000000))
append_result

QUERY_FILE="$(printf 'query%02d.csv' "$((QUERY + 1))")"

if [[ ! -f "$QUERY_FILE" ]]; then
    cat stderr.txt >> "$LOGS"
    echo "query result file not found: $QUERY_FILE" >> "$LOGS"
    exit 1
fi

cp "$QUERY_FILE" "$OUTPUT"

{
    echo "query=$QUERY"
    echo "elapsed_ms=$ELAPSED_MS"
    echo "--- stdout ---"
    cat stdout.txt
    echo "--- stderr ---"
    cat stderr.txt
} > "$LOGS"

popd >/dev/null
