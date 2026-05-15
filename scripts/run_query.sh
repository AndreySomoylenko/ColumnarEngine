#!/usr/bin/env bash
set -euo pipefail

QUERY="$1"
export COLUMNAR="${COLUMNAR:-$2}"
OUTPUT="$3"
LOGS="$4"

mkdir -p "$(dirname "$OUTPUT")"
mkdir -p "$(dirname "$LOGS")"

WORKDIR="$(mktemp -d)"
trap 'rm -rf "$WORKDIR"' EXIT

pushd "$WORKDIR" >/dev/null

START_NS=$(date +%s%N)

"$OLDPWD/build/sandbox/sandbox_app" run "$QUERY" \
    > stdout.txt \
    2> stderr.txt

END_NS=$(date +%s%N)

ELAPSED_MS=$(((END_NS - START_NS) / 1000000))

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